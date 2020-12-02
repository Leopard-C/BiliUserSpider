#include "task_manager.h"
#include <fstream>
#include "../log/logger.h"
#include "../json/json.h"
#include "../status/status_code.h"
#include "../global.h"


TaskManager::TaskManager()
    : data_file_(g_dataDir + "/task.json") {}

TaskManager::~TaskManager() {
    writeDataToFile();
    g_stop();
}

/* 申请任务 (连续的用户ID)
 * 返回值:
 *   SERVER_STOPPED
 *   NO_ERROR
 */
int TaskManager::getTask(int clientId,  /* 客户端ID */
                        int count,     /* 累计爬取用户数量 */
                        int* taskId,   /* 任务ID */
                        int* midStart, /* 起始用户ID（包含） */
                        int* midEnd)   /* 结束用户ID（不包含） */
{
    std::lock_guard<std::mutex> lck(mutex_);
    time_t now = time(NULL);

    /* 服务器停止运行 */
    if (!g_isRunning) {
        return SERVER_STOPPED;
    }

    /* 如果有任务被回收了，就优先分配这些任务 */
    if (!tasks_recycled_.empty()) {
        LInfo("Assign recycled task");
        auto& front = tasks_recycled_.front();
        *midStart = front.first;
        *midEnd = front.second;
        tasks_recycled_.pop_front();
    }
    else {
        /* 所有任务已经完成，返回-1 */
        if (mid_current_ >= mid_end_) {
            return NO_TASK_AVAIL;
        }

        *midStart = mid_current_;
        if (mid_current_ + count > mid_end_) {
            *midEnd = mid_end_ - mid_current_;
            mid_current_ = mid_end_;
        }
        else {
            *midEnd = mid_current_ + count;
            mid_current_ += count;
        }
    }

    /* 记录该任务 */
    time_t ttl = now + g_svrConfig.main_task_timeout;
    tasks_.emplace(task_id_current_, BiliTask(clientId, *midStart, *midEnd, ttl));
    LInfo("Get Task: {} {} - {},  {:.3f}", task_id_current_, *midStart, *midEnd,
         (mid_current_ - mid_start_) / double(mid_end_ - mid_start_));
    printTasks();

    /* 每3s写入一次文件 */
    if (now - last_time_write_to_file_ > 3) {
        last_time_write_to_file_ = now;
        writeDataToFile();
    }

    *taskId = task_id_current_++;
    return NO_ERROR;
}


/* 提交任务 */
void TaskManager::commit(int clientId, int taskId) {
    std::lock_guard<std::mutex> lck(mutex_);
    time_t now = time(NULL);
    LInfo("Commit Task: {} {}", clientId, taskId);

    /* 清除任务 */
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        if (it->second.client_id == clientId) {
            LInfo("Task {} completed", taskId);
            tasks_.erase(it);
        }
        else {
            /* won't get here */
            LCritical("Client id not match");
        }
    }
    else {
        LError("task {} not found", taskId);
    }

    /* 回收过期的任务 */
    for (auto it = tasks_.begin(); it != tasks_.end(); ) {
        if (it->second.ttl < now) {
            LWarn("Recycle task: {}", it->first);
            tasks_recycled_.emplace_back(it->second.mid_start, it->second.mid_end);
            it = tasks_.erase(it);
        }
        else {
            ++it;
        }
    }

    /* 最少间隔3s写入一次文件 */
    if (now - last_time_write_to_file_ > 3) {
        last_time_write_to_file_ = now;
        writeDataToFile();
    }

    printTasks();
    printTasksRecycled();
}


/* 打印任务列表到控制台(不写入日志文件) */
void TaskManager::printTasks() {
    LogF("Running Tasks: %d", tasks_.size());
    //for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    //    LogF("  %d: %d - %d  %ld", it->first, it->second.mid_start,
    //         it->second.mid_end, it->second.ttl);
    //}
    LogF("-------------------------------");
}


/* 打印回收的任务列表到控制台(不写入日志文件) */
void TaskManager::printTasksRecycled() {
    LogF("Recycled Tasks: %d", tasks_recycled_.size());
    //size_t size = tasks_recycled_.size();
    //for (size_t i = 0; i < size; ++i) {
    //    LogF("  %lu: %d - %d", i, tasks_recycled_[i].first, tasks_recycled_[i].second);
    //}
    LogF("-------------------------------");
}


/* 回收指定客户端的所有任务（客户端已经退出） */
void TaskManager::recycle(int clientId) {
    std::lock_guard<std::mutex> lck(mutex_);
    time_t now = time(NULL);

    int count = 0;
    for (auto it = tasks_.begin(); it != tasks_.end(); ) {
        if (it->second.client_id == clientId) {
            ++count;
            tasks_recycled_.emplace_back(it->second.mid_start, it->second.mid_end);
            it = tasks_.erase(it);
        }
        else {
            ++it;
        }
    }

    /* 每3s写入一次文件 */
    if (now - last_time_write_to_file_ > 3) {
        last_time_write_to_file_ = now;
        writeDataToFile();
    }

    LWarn("Recycle {} tasks of clientId:{} OK", count, clientId);
    printTasksRecycled();
}


/* 获取任务进度 */
void TaskManager::getProgress(int* midStart, int* midEnd, int* midCurrent) {
    *midStart = mid_start_;
    *midEnd = mid_end_;
    *midCurrent = mid_current_;
}

/* 设置任务范围 */
void TaskManager::setRange(int midStart, int midEnd, int midCurrent) {
    /* 如果正在运行，只能修改终止mid */
    /* 否则可以修改所有项 */
    mid_end_ = midEnd;
    if (!g_isRunning) {
        mid_start_ = midStart;
        mid_current_ = midCurrent;
    }
}


/* 服务器启动时，从文件读取存档 */
bool TaskManager::readData() {
    std::ifstream ifs(data_file_);
    if (!ifs) {
        LError("Read data file failed: {}", data_file_);
        return false;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root, false)) {
        LError("Parse data file failed: {}", data_file_);
        return false;
    }

    /* 总体任务情况 */
    if (checkNull(root, "mid_current", "mid_end", "task_id_current")) {
        LError("Missing key:mid_current/mid_end/task_id_current");
        return false;
    }
    mid_start_ = mid_current_ = getIntVal(root, "mid_current");
    mid_end_ = getIntVal(root, "mid_end");
    task_id_current_ = getIntVal(root, "task_id_current");

    /* 未完成(回收)的任务 */
    if (!checkNull(root, "tasks")) {
        Json::Value& tasks = root["tasks"];
        int size = tasks.size();
        for (int i = 0; i < size; ++i) {
            Json::Value& task = tasks[i];
            if (checkNull(task, "start", "end")) {
                LError("Missing key:tasks[{}][start/end]", i);
                return false;
            }
            int start = getIntVal(task, "start");
            int end = getIntVal(task, "end");
            tasks_recycled_.emplace_back(start, end);
        }
        LInfo("Read {} tasks from file", size);
    }

    printTasks();
    printTasksRecycled();

    return true;
}


/* 写入文件存档 */
bool TaskManager::writeDataToFile() {
    std::ofstream ofs(data_file_);
    if (!ofs) {
        LError("Open data file failed: {}", data_file_);
        return false;
    }

    Json::Value root, tasks;
    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        Json::Value taskNode;
        taskNode["start"] = it->second.mid_start;
        taskNode["end"] = it->second.mid_end;
        tasks.append(taskNode);
    }
    for (auto it = tasks_recycled_.begin(); it != tasks_recycled_.end(); ++it) {
        Json::Value taskNode;
        taskNode["start"] = it->first;
        taskNode["end"] = it->second;
        tasks.append(taskNode);
    }

    root["task_id_current"] = task_id_current_;
    root["mid_current"] = mid_current_;
    root["mid_end"] = mid_end_;
    root["tasks"] = tasks;

    ofs << root.toStyledString();
    return true;
}

