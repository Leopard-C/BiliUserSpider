#pragma once

struct BiliTask {
    BiliTask() : client_id(-1), mid_start(0), mid_end(0), ttl(0) {}
	BiliTask(int clientId, int start, int end, time_t ttl) :
		client_id(clientId), mid_start(start), mid_end(end), ttl(ttl) {}
    int client_id;
	int mid_start;
	int mid_end;
    time_t ttl;  /* 最晚提交时间, 超时回收 */
};

