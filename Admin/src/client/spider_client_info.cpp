#include "spider_client_info.h"
#include "../log/logger.h"
#include <algorithm>

bool SpiderClientInfo::parseFromJson(Json::Value& data) {
    if (checkNull(data, "id", "os", "host", "join_time", "heartbeat", "threads", "count",
        "avg", "avg_3m", "avg_5m", "avg_10m", "avg_30m"))
    {
        LError("Missing key:id/host/join_time/...");
        return false;
    }
    id = getIntVal(data, "id");
    os = getStrVal(data, "os");
    host = getStrVal(data, "host");
    threads = getIntVal(data, "threads");
    count = getIntVal(data, "count");
    join_time = getInt64Val(data, "join_time");
    /* 服务器发送来的速度是乘以1000后的整数，这里需要除以1000.0得到真实速度 */
    average_speed = getIntVal(data, "avg") / 1000.0;
    average_speed_3m = getIntVal(data, "avg_3m") / 1000.0;
    average_speed_5m = getIntVal(data, "avg_5m") / 1000.0;
    average_speed_10m = getIntVal(data, "avg_10m") / 1000.0;
    average_speed_30m = getIntVal(data, "avg_30m") / 1000.0;
    /* 由于精度问题，有可能出现负数（偏向负无穷的负数） */
    if (average_speed < 0) { average_speed = .001; }
    if (average_speed_3m < 0) { average_speed_3m = .001; }
    if (average_speed_5m < 0) { average_speed_5m = .001; }
    if (average_speed_10m < 0) { average_speed_10m = .001; }
    if (average_speed_30m < 0) { average_speed_30m = .001; }
    return true;
}


void sortClientsInfoById_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.id < info2.id;
    });
}

void sortClientsInfoById_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.id > info2.id;
    });
}

void sortClientsInfoById(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoById_ASC(infos) : sortClientsInfoById_DESC(infos);
}


void sortClientsInfoByThreads_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.threads < info2.threads;
    });
}

void sortClientsInfoByThreads_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.threads > info2.threads;
    });
}

void sortClientsInfoByThreads(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByThreads_ASC(infos) : sortClientsInfoByThreads_DESC(infos);
}


void sortClientsInfoByCount_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.count < info2.count;
    });
}

void sortClientsInfoByCount_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.count > info2.count;
    });
}

void sortClientsInfoByCount(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByCount_ASC(infos) : sortClientsInfoByCount_DESC(infos);
}


void sortClientsInfoByJoinTime_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.join_time < info2.join_time;
    });
}

void sortClientsInfoByJoinTime_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.join_time > info2.join_time;
    });
}

void sortClientsInfoByJoinTime(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByJoinTime_ASC(infos) : sortClientsInfoByJoinTime_DESC(infos);
}


void sortClientsInfoByHost_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.host < info2.host;
    });
}

void sortClientsInfoByHost_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.host > info2.host;
    });
}

void sortClientsInfoByHost(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByHost_ASC(infos) : sortClientsInfoByHost_DESC(infos);
}


void sortClientsInfoByOS_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.os < info2.os;
    });
}

void sortClientsInfoByOS_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.os > info2.os;
    });
}

void sortClientsInfoByOS(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByOS_ASC(infos) : sortClientsInfoByOS_DESC(infos);
}


void sortClientsInfoByAvgSpeed_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed < info2.average_speed;
    });
}

void sortClientsInfoByAvgSpeed_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed > info2.average_speed;
    });
}

void sortClientsInfoByAvgSpeed(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByAvgSpeed_ASC(infos) : sortClientsInfoByAvgSpeed_DESC(infos);
}


void sortClientsInfoByAvgSpeed_3m_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_3m < info2.average_speed_3m;
    });
}

void sortClientsInfoByAvgSpeed_3m_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_3m > info2.average_speed_3m;
    });
}

void sortClientsInfoByAvgSpeed_3m(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByAvgSpeed_3m_ASC(infos) : sortClientsInfoByAvgSpeed_3m_DESC(infos);
}


void sortClientsInfoByAvgSpeed_5m_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_5m < info2.average_speed_5m;
    });
}

void sortClientsInfoByAvgSpeed_5m_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_5m > info2.average_speed_5m;
    });
}

void sortClientsInfoByAvgSpeed_5m(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByAvgSpeed_5m_ASC(infos) : sortClientsInfoByAvgSpeed_5m_DESC(infos);
}


void sortClientsInfoByAvgSpeed_10m_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_10m < info2.average_speed_10m;
    });
}

void sortClientsInfoByAvgSpeed_10m_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_10m > info2.average_speed_10m;
    });
}

void sortClientsInfoByAvgSpeed_10m(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByAvgSpeed_10m_ASC(infos) : sortClientsInfoByAvgSpeed_10m_DESC(infos);
}


void sortClientsInfoByAvgSpeed_30m_ASC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_30m < info2.average_speed_30m;
    });
}

void sortClientsInfoByAvgSpeed_30m_DESC(std::vector<SpiderClientInfo>& infos) {
    std::sort(infos.begin(), infos.end(), [](SpiderClientInfo& info1, SpiderClientInfo& info2) {
        return info1.average_speed_30m > info2.average_speed_30m;
    });
}

void sortClientsInfoByAvgSpeed_30m(std::vector<SpiderClientInfo>& infos, bool isASC) {
    return isASC ? sortClientsInfoByAvgSpeed_30m_ASC(infos) : sortClientsInfoByAvgSpeed_30m_DESC(infos);
}

