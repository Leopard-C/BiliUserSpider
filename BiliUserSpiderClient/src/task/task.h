#pragma once
#include <vector>

/* 主任务 */
/* 本地TaskManager向服务器TaskManager申请的任务 */
struct MainTask {
    MainTask() = default;
    MainTask(int id, int start, int end) :
        id(id), mid_start(start), mid_end(end) {}
    bool isCompleted() const { return completed >= mid_end - mid_start; }
    int id = -1;
    int mid_start = 0;
    int mid_end = 0;
    int completed = 0;
};


/* 子任务 */
/* 本地TaskManager下发(分配)的任务 */
struct SubTask {
    SubTask(int mainTaskId, const std::vector<int>& mids) :
        main_task_id(mainTaskId), mids(mids) {}
    int main_task_id;  /* 该任务所属的主任务的id */
    std::vector<int> mids;
};

