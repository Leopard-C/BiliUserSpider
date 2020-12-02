#pragma once
#include <string>
#include <vector>
#include "../iclient/ic/ic_request.h"

class UserAgentManager {
public:
    UserAgentManager();
    ~UserAgentManager();

public:
    bool  readConfig(const std::string& uaFile);
    const std::string& setRandomUA(ic::Request& request) const;

private:
    std::vector<std::string> UAs;
};
