/*********************************************************************************
** class name:  TaskManager
**
** description: 任务调度管理器
**              从服务器获取一大块任务，然后爬虫线程每次从这里获取一小部分任务。
**              记录从服务器申请的任务的完成度，如果全部完成，就提交到服务器。
**
** author: Leopard-C
**
** update: 2020/12/02
**********************************************************************************/
#pragma once
#include <mutex>
#include <map>
#include <deque>
#include "../bili/user.h"
#include "../database/mysql_instance.h"
#include "../config/config.h"
#include "../task/task.h"


class TaskManager {
public:
    TaskManager();
    ~TaskManager();

public:
    /* 启动 */
    void start(MysqlDbPool* mysqlDbPool);

    /* 申请任务, 返回任务id,为-1说明申请失败或者没有剩余任务可做 */
    int getSubTask(std::vector<int>& mids);

    /* 提交任务 */
    void commitSubTask(int subTaskId,
        const std::vector<bili::UserInfo>& infos,
        const std::vector<int>& noneMids,
        const std::deque<int>& errorMids);

    /* 子任务数量 */
    int numSubTasks() const { return static_cast<int>(sub_tasks_.size()); }

    /* 主任务数量 */
    int numMainTasks() const { return static_cast<int>(main_tasks_.size()); }

    /* 刷新缓存，立即写如数据库 */
    void flush();

private:
    int appendInfoToSql_(char* end, const bili::UserInfo* info, MysqlInstance& mysql);

    /* 向主任务服务器申请任务 */
    bool getMainTask_();

    /* 向主任务服务器提交任务 */
    bool commitMainTask_(int mainTaskId);

    /* 写入数据库 */
    bool writeOkToDb_();
    bool writeNoneToDb_();
    bool writeOkToDb_(const std::vector<const bili::UserInfo*>& infos, const std::string& tableName, MysqlInstance& mysql);
    bool writeNoneToDb_(const std::vector<int>& noneMids, const std::string& tableName, MysqlInstance& mysql);

public:
    /* 统计量 */
    int commit_ok = 0;
    int commit_none = 0;
    int commit_error = 0;
    int write_to_db_ok = 0;
    int write_to_db_none = 0;
    int write_to_db_error = 0;

private:
    enum {
        kRecordsPerQuery = 128,  /* 缓存大小，即每次向数据库中插入128条记录 */
        kRecordsPerTable = 2000000,       /* 200万条数据 */
        kRecordsPerTableOfNone = 2000000  /* 200万条数据 */
    };

private:
    /* 主任务列表 */
    /* <id, mainTask> */
    std::map<int, MainTask> main_tasks_;
    MainTask current_main_task_;

    /* 子任务列表 */
    /* <id, subTask> */
    std::map<int, SubTask> sub_tasks_;
    int current_mid_ = 0;
    int current_sub_task_id_ = 0;

private:
    std::mutex mutex_;

    /* 获取到的用户信息 缓存 */
    // <mainTaskId, infos>
    std::map<int, std::vector<bili::UserInfo>> infos_buff_;

    /* 获取到的空用户信息 缓存 */
    // <mainTaskId, none info mids>
    std::map<int, std::vector<int>> none_mids_buff_;

    /* 获取用户信息失败的mid */
    // <mainTaskId, error mids>
    std::map<int, std::deque<int>> error_mids_;

    /* mysql数据库连接池 */
    /* 只有使用权，不负责管理 */
    MysqlDbPool* mysql_db_pool_ = nullptr;
};
