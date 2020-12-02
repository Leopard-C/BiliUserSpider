#include "helper.h"
#include "../util/md5.h"

namespace bili {

std::string getSign(ic::Url& url) {
    url.setParam("appkey", APP_KEY);
    url.setParam("build", BUILD);
    url.setParam("c_locale", C_LOCALE);
    url.setParam("channel", CHANNEL);
    url.setParam("mobi_app", MOBI_APP);
    url.setParam("platform", PLATFORM);
    url.setParam("s_locale", S_LOCALE);
    std::string oriSign = url.toParamString() + bili::SECRET_KEY;
    std::string sign = util::MD5(oriSign).toStr();
    return sign;
}

}
