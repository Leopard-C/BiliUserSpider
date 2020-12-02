/*********************************************************************************
** class name:  TaskManager
**
** description: 任务分配服务器
**              客户端一次申请一块连续的任务，必须全部完成才能提交，
**              超时，就收回该任务，重新分配
**
** author: Leopard-C
**
** update: 2020/12/02
**********************************************************************************/
#pragma once
#include <mutex>
#include <map>
#include <vector>
#include <deque>
#include "../task/task.h"


class TaskManager {
public:
    TaskManager();
    ~TaskManager();

    /* 读取任务进度和遗留任务文件 */
    bool readData();
    
    /* 申请任务 (连续的用户ID)
     * 返回值:
     *   SERVER_STOPPED
     *   NO_ERROR
     */
    int getTask(int clientId,  /* 客户端ID */
                int count,     /* 累计爬取用户数量 */
                int* taskId,   /* 任务ID */
                int* midStart, /* 起始用户ID（包含） */
                int* midEnd);  /* 结束用户ID（不包含） */

    /* 提交任务 */
    void commit(int clientId, int taskId);

    /* 回收指定客户端的所有任务（客户端已经退出） */
    void recycle(int clientId);

    /* 获取任务进度 */
    void getProgress(int* midStart, int* midEnd, int* midCurrent);

    /* 设置任务范围 */
    void setRange(int midStart, int midEnd, int midCurrent);

private:
    bool writeDataToFile();
    void printTasks();
    void printTasksRecycled();

private:
    std::mutex mutex_;

    /* 正常执行的任务 */
    std::map<int, BiliTask> tasks_;

    /* 回收的任务，优先分配这些任务 */
    //                  < mid start  , mid end     >
    std::deque<std::pair<int, int>> tasks_recycled_;

    /* 客户端id 自动增长 */
    int task_id_current_ = 0;

    /* 当前任务进度 */
    int mid_start_ = 0;
    int mid_current_ = 0;
    int mid_end_ = 0;

    /* 写入文件 */
    std::string data_file_;
    time_t last_time_write_to_file_ = 0;
};

