#include "task_manager.h"
#include "../log/logger.h"
#include "../json/json.h"
#include "../api/api_task.h"
#include "../status/status_code.h"
#include "../global.h"
#include <fstream>
#include <thread>


TaskManager::TaskManager() : current_main_task_(-1, 0, 0) {
    LogF("Construct TaskManager");
}

TaskManager::~TaskManager() {
    LogF("Destruct TaskManager");
}

void TaskManager::start(MysqlDbPool* mysqlDbPool) {
    mysql_db_pool_ = mysqlDbPool;
}

/*
 *  爬虫线程向本服务器申请任务
 *   返回 sub_task_id
***/
int TaskManager::getSubTask(std::vector<int>& mids) {
    std::lock_guard<std::mutex> lck(mutex_);
    mids.reserve(g_clientConfig.sub_task_step);

    /* 1. 检查是否有发生过错误的mid, 有的话就先分配这些id */
    if (error_mids_.size() > 0) {
        /* 分配的这些id一定都属于同一个主任务 !!!!!! */ 
        int count = 0;
        auto front = error_mids_.begin();
        int mainTaskId = front->first;

        auto it = front->second.begin();
        while (it != front->second.end()) {
            mids.emplace_back(*it);
            it = front->second.erase(it);
            if (++count >= g_clientConfig.sub_task_step) {
                break;
            }
        }
        if (it == front->second.end()) {
            error_mids_.erase(front);
        }

        sub_tasks_.emplace(current_sub_task_id_, SubTask(mainTaskId, mids));
        return current_sub_task_id_++;
    }

    /* 2. 分配正常任务 */
    else {
        /* 如果当前从主服务器分配的任务已经下发完毕, 就再次向主服务器申请任务 */
        if (current_mid_ >= current_main_task_.mid_end) {
            if (g_shouldStop) {
                this->flush();  /* 这里应该强制刷新一下，写入数据库,不然客户端可能永远无法退出 */
                return -1;
            }
            if (getMainTask_()) {
                LInfo("Get Task [{0} - {1}]", current_main_task_.mid_start, current_main_task_.mid_end);
                main_tasks_.emplace(current_main_task_.id, current_main_task_);
                current_mid_ = current_main_task_.mid_start;
            }
            else {
                LError("Get Main Task failed");
                /* 申请任务失败, 客户端退出 */
                g_stop(StopMode::Smooth);
                return -1;
            }
        }

        int count = 0;
        while (current_mid_ < current_main_task_.mid_end) {
            mids.emplace_back(current_mid_++);
            if (++count >= g_clientConfig.sub_task_step) {
                break;
            }
        }

        sub_tasks_.emplace(current_sub_task_id_, SubTask(current_main_task_.id, mids));
        return current_sub_task_id_++;
    }
}


/* 提交任务 */
void TaskManager::commitSubTask(int subTaskId,
    const std::vector<bili::UserInfo>& infos,
    const std::vector<int>& noneMids,
    const std::deque<int>& errorMids) 
{
    LInfo("Commit sub task : {} {} {}", infos.size(), noneMids.size(), errorMids.size());
    std::lock_guard<std::mutex> lck(mutex_);
    int okCount = static_cast<int>(infos.size());
    int noneCount = static_cast<int>(noneMids.size());
    int errorCount = static_cast<int>(errorMids.size());

    /* 在子任务列表中 查找该任务 */
    auto subTaskIter = sub_tasks_.find(subTaskId);
    if (subTaskIter == sub_tasks_.end()) {
        LCritical("Won't get here");
        /* Won't get here */
        return;
    }
    SubTask& subTask = subTaskIter->second;

    /* 查找该子任务对应的主任务 */
    auto mainTaskIter = main_tasks_.find(subTask.main_task_id);
    if (mainTaskIter == main_tasks_.end()) {
        LCritical("Won't get here");
        /* Won't get here */
        return;
    }
    MainTask& mainTask = mainTaskIter->second;
    /* 这里不能更新，应该等写入数据库成功时才更新，防止写入数据库失败造成数据遗漏 */
    //mainTask.completed += okCount + noneCount;

    /* 正常的用户信息 */
    /* 先缓存，缓存一定数量后再写入数据库 */
    if (okCount > 0) {
        commit_ok += okCount;
        auto findIt = infos_buff_.find(subTask.main_task_id);
        if (findIt == infos_buff_.end()) {
            infos_buff_.emplace(subTask.main_task_id, infos);
        }
        else {
            findIt->second.insert(findIt->second.end(), infos.begin(), infos.end());
        }
    }
    /*
     * 写入数据库的条件
     *  1. 缓存满
     *  2. 应该退出（平滑）
     *  3. 间隔50个子任务
    **/
    if (infos_buff_.size() > kRecordsPerQuery || g_shouldStop
        || (infos_buff_.size() > 0 && subTaskId % 50 == 0))
    {
        /* 写入数据库失败，致命错误，程序退出 */
        if (!writeOkToDb_()) {
            g_stop(StopMode::Force);
        }
    }

    /* 空用户 */
    if (noneCount > 0) {
        commit_none += noneCount;
        auto findIt = none_mids_buff_.find(subTask.main_task_id);
        if (findIt == none_mids_buff_.end()) {
            none_mids_buff_.emplace(subTask.main_task_id, noneMids);
        }
        else {
            findIt->second.insert(findIt->second.end(), noneMids.begin(), noneMids.end());
        }
    }
    if (none_mids_buff_.size() > kRecordsPerQuery || g_shouldStop
        || (none_mids_buff_.size() > 0 && subTaskId % 50 == 0))
    {
        /* 写入数据库失败，致命错误，程序退出 */
        if (!writeNoneToDb_()) {
            g_stop(StopMode::Force);
        }
    }

    /* 有错误发生 */
    if (errorCount > 0) {
        commit_error += errorCount;
        auto findIt = error_mids_.find(subTask.main_task_id);
        if (findIt == error_mids_.end()) {
            error_mids_.emplace(subTask.main_task_id, errorMids);
        }
        else {
            findIt->second.insert(findIt->second.end(), errorMids.begin(), errorMids.end());
        }
        /* 不写入数据库, 因为很快就会将该mid分配出去,再次尝试 */
        //writeErrorToDb(errorMids);
    }

    /* 从子任务列表中删除该任务 */
    sub_tasks_.erase(subTaskIter);
}


/* 刷新缓存，立即写如数据库 */
void TaskManager::flush() {
    writeOkToDb_();
    writeNoneToDb_();
}

/* 向主任务服务器申请任务 */
bool TaskManager::getMainTask_() {
    for (int i = 0; i < 10 && !g_shouldStop; ++i) {
        /* 例如：开启50个线程，每个线程每次申请5个任务，向服务器申请任务的倍数关系为3 */
        /* 那么，每次向服务器申请的任务数量为 50*5*3=750 */
        int count = g_numThreads * g_clientConfig.sub_task_step * g_clientConfig.main_task_step_times;
        int commitAll = commit_ok + commit_none;
        std::string errMsg;
        int ret = api_get_task(g_svrAddr, g_clientId, count, commitAll, &current_main_task_, &g_clientConfig, &errMsg);
        if (ret == NO_ERROR) {
            return true;
        }
        else if (ret == SERVER_STOPPED) {
            return false;
        }
        else { // REQUEST_ERROR
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    return false;
}


/* 向主任务服务器提交任务 */
bool TaskManager::commitMainTask_(int mainTaskId) {
    for (int i = 0; i < 10 && !g_forceStop; ++i) {
        int commitAll = commit_ok + commit_none;
        std::string errMsg;
        int ret = api_commit_task(g_svrAddr, g_clientId, mainTaskId, commitAll, &g_clientConfig, &errMsg);
        if (ret == NO_ERROR) {
            return true;
        }
        else if (ret == SERVER_STOPPED) {
            /* 服务器停止运行(逻辑上) */
            g_stop(StopMode::Smooth);
            return true;
        }
        else { // REQUEST_ERROR
            LError("Commit Task Failed! {}. retry {}", errMsg, i);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    return false;
}


/* helper function */
/* Append info object to sql statement */
int TaskManager::appendInfoToSql_(char* end, const bili::UserInfo* info, MysqlInstance& mysql) {
    std::string face;
    /* 头像为默认头像的，插入空值 */
    if (info->face.find("noface.jpg") == std::string::npos) {
        face = info->face;
    }
    return sprintf(end,
        "(%u,'%s',%d,'%s','%s',%d,%d,%d,%d,%d,'%s',%d,'%s',%d,'%s',%u,%d),",
        info->mid, mysql.escapeString(info->name).c_str(), info->sex, face.c_str(),
        mysql.escapeString(info->sign.substr(0, 300)).c_str(), info->level, info->attention, info->fans,
        info->vip_status, info->vip_type, info->vip_label.c_str(), info->official_type,
        mysql.escapeString(info->official_desc).c_str(), info->official_role,
        mysql.escapeString(info->official_title).c_str(), info->lid, info->is_deleted);
}


/* 将用户信息写入数据库 */
bool TaskManager::writeOkToDb_() {
    /* 获取一个mysql连接 */
    MysqlInstance mysql(mysql_db_pool_);
    if (mysql.bad()) {
        LError("MysqlInstance is bad");
        return false;
    }

    /* 判断是否跨表 */
    /*           < 表名, 在infos中的索引> */
    std::map<std::string, std::vector<const bili::UserInfo*>> tablesMap;
    for (auto it = infos_buff_.begin(); it != infos_buff_.end(); ++it) {
        std::vector<bili::UserInfo>& infos = it->second;
        size_t size = infos.size();
        for (size_t idx = 0; idx < size; ++idx) {
            int mid = infos[idx].mid;
            int div = mid / kRecordsPerTable;
            int startOfThisTable = div * kRecordsPerTable;
            int endOfThisTable = startOfThisTable + kRecordsPerTable;
            startOfThisTable /= 10000;
            endOfThisTable /= 10000;
            char tableNameTmp[32];
            sprintf(tableNameTmp, "mid_%u_%u", startOfThisTable, endOfThisTable);
            std::string tableName(tableNameTmp);
            auto findIt = tablesMap.find(tableName);
            if (findIt != tablesMap.end()) {
                findIt->second.emplace_back(&(infos[idx]));
            }
            else {
                std::vector<const bili::UserInfo*> infosTmp;
                infosTmp.emplace_back(&infos[idx]);
                tablesMap.emplace(tableName, infosTmp);
            }
        }
    }

    /* 执行写入数据库操作 */
    for (auto it = tablesMap.begin(); it != tablesMap.end(); ++it) {
        bool success = false;
        /* 尝试5次，都出错，就返回false */
        for (int i = 0; i < 5 && !g_forceStop; ++i) {
            if (writeOkToDb_(it->second, it->first, mysql)) {
                success = true;
                break;
            }
        }
        if (!success) {
            return false;
        }
    }

    /* 检查主任务是否完成,如果完成,就提交到服务器 */
    for (auto it = infos_buff_.begin(); it != infos_buff_.end(); ) {
        int mainTaskId = it->first;
        auto findIt = main_tasks_.find(mainTaskId);
        if (findIt == main_tasks_.end()) {
            /* Critical error */
            g_stop(StopMode::Force);
            LCritical("Not found main task id in mainTasks_");
            return false;
        }
        else {
            auto& mainTask = findIt->second;
            mainTask.completed += static_cast<int>(it->second.size());  /* 更新任务完成度 */
            /* 该主任务已经完成,就提交到服务器 */
            if (mainTask.isCompleted()) {
                if (commitMainTask_(mainTaskId)) {
                    LInfo("Commit Task: {0} [{1} - {2}]", mainTaskId, mainTask.mid_start, mainTask.mid_end);
                    LInfo("Remaining tasks: {}", main_tasks_.size());
                    main_tasks_.erase(findIt);
                }
                else {
                    LError("Commit main task {} failed", mainTask.id);
                    return false;
                }
            }
        }
        it = infos_buff_.erase(it);
    }

    return true;
}

bool TaskManager::writeOkToDb_(const std::vector<const bili::UserInfo*>& infos, const std::string& tableName, MysqlInstance& mysql) {
    char sql[1024 * kRecordsPerQuery];
    char* start = sql;
    start += sprintf(start, "INSERT IGNORE INTO %s (`mid`,`name`,`sex`,`face`,`sign`,`level`,"
        "`attention`,`fans`,`vip_status`,`vip_type`,`vip_label`,`official_type`,"
        "`official_desc`,`official_role`,`official_title`,`lid`,`is_deleted`) VALUE",
        tableName.c_str());

    size_t size = infos.size();
    size_t idx = 0;
    while (idx < size) {
        char* end = start;
        int count = 0;
        while (count < kRecordsPerQuery && idx < size) {
            end += appendInfoToSql_(end, infos[idx], mysql);
            ++count;
            ++idx;
        }
        *(end-1) = ';';
        *end = '\0';
        if (!mysql.exec(sql, 3)) {
            return false;
        }
    }

    return true;
}

bool TaskManager::writeNoneToDb_() {
    /* 获取一个mysql连接 */
    MysqlInstance mysql(mysql_db_pool_);
    if (mysql.bad()) {
        LError("MysqlInstance is bad");
        return false;
    }

    /* 展平: map -> vector */
    std::vector<int> noneMidsTmp;
    for (auto it = none_mids_buff_.begin(); it != none_mids_buff_.end(); ++it) {
        noneMidsTmp.insert(noneMidsTmp.end(), it->second.begin(), it->second.end());
    }

    /* 判断是否跨表 */
    /*           < 表名, 在infos中的索引> */
    std::map<std::string, std::vector<int>> tablesMap;
    for (auto it = none_mids_buff_.begin(); it != none_mids_buff_.end(); ++it) {
        std::vector<int>& noneMids = it->second;
        size_t size = noneMids.size();
        for (size_t idx = 0; idx < size; ++idx) {
            int mid = noneMids[idx];
            int div = mid / kRecordsPerTableOfNone;
            int startOfThisTable = div * kRecordsPerTableOfNone;
            int endOfThisTable = startOfThisTable + kRecordsPerTableOfNone;
            startOfThisTable /= 10000;
            endOfThisTable /= 10000;
            char tableNameTmp[64];
            sprintf(tableNameTmp, "mid_none_%u_%u", startOfThisTable, endOfThisTable);
            std::string tableName(tableNameTmp);
            auto findIt = tablesMap.find(tableName);
            if (findIt != tablesMap.end()) {
                findIt->second.emplace_back(noneMids[idx]);
            }
            else {
                std::vector<int> infosTmp;
                infosTmp.emplace_back(noneMids[idx]);
                tablesMap.emplace(tableName, infosTmp);
            }
        }
    }

    /* 执行写入数据库操作 */
    for (auto it = tablesMap.begin(); it != tablesMap.end(); ++it) {
        bool success = false;
        /* 尝试5次，都出错，就返回false */
        for (int i = 0; i < 5 && !g_forceStop; ++i) {
            if (writeNoneToDb_(it->second, it->first, mysql)) {
                success = true;
                break;
            }
        }
        if (!success) {
            return false;
        }
    }

    /* 更新任务完成度 */
    /* 检查主任务是否完成,如果完成,就提交到主服务器 */
    for (auto it = none_mids_buff_.begin(); it != none_mids_buff_.end(); ) {
        int mainTaskId = it->first;
        auto findIt = main_tasks_.find(mainTaskId);
        if (findIt == main_tasks_.end()) {
            /* Critical error */
            g_stop(StopMode::Force);
            LCritical("Not found main task id in mainTasks_");
            return false;
        }
        else {
            auto& mainTask = findIt->second;
            mainTask.completed += static_cast<int>(it->second.size());  /* 更新任务完成度 */
            if (mainTask.isCompleted()) {
                if (commitMainTask_(mainTaskId)) {
                    LInfo("Commit Task {} - {}", mainTask.mid_start, mainTask.mid_end);
                    main_tasks_.erase(findIt);
                }
                else {
                    LError("Critical error, commit main task {} failed", mainTask.id);
                    return false;
                }
            }
        }
        it = none_mids_buff_.erase(it);
    }

    return true;
}

bool TaskManager::writeNoneToDb_(const std::vector<int>& noneMids, const std::string& tableName, MysqlInstance& mysql) {
    char sql[128 * kRecordsPerQuery];
    char* start = sql;
    start += sprintf(sql, "INSERT IGNORE INTO %s (`mid`) VALUE", tableName.c_str());
    size_t size = noneMids.size();
    size_t idx = 0;
    while (idx < size) {
        char* end = start;
        int count = 0;
        while (count < kRecordsPerQuery && idx < size) {
            end += sprintf(end, "(%u),", noneMids[idx]);
            ++count;
            ++idx;
        }
        *(end-1) = ';';
        *end = '\0';
        if (!mysql.exec(sql, 3)) {
            return false;
        }
    }

    return true;
}

