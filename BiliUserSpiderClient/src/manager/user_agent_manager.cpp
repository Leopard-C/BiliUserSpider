#include "user_agent_manager.h"
#include "../log/logger.h"
#include "../json/json.h"
#include <fstream>

UserAgentManager::UserAgentManager() {
    LogF("Create instance of UserAgentServer");
}

UserAgentManager::~UserAgentManager() {
	LogF("Destroy instance of UserAgentServer");
}

bool UserAgentManager::readConfig(const std::string& uaFile) {
    std::ifstream ifs(uaFile);
    if (!ifs) {
        LError("Open file: {} failed", uaFile);
        return false;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root, false)) {
        LError("Parse file: {} failed", uaFile);
        return false;
    }

    int size = root.size();
    if (size == 0) {
        LError("No user agent!");
        return false;
    }

    UAs.reserve(size);
    for (int i = 0; i < size; ++i) {
        UAs.emplace_back(root[i].asString());
    }

    return true;
}

const std::string& UserAgentManager::setRandomUA(ic::Request& request) const {
    const std::string& userAgent = UAs[rand() % UAs.size()];
    request.setHeader("User-Agent", userAgent);
    return userAgent;
}

