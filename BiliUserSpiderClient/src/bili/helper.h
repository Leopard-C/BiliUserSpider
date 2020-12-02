/**************************************************************
** namespace:   bili
**
** description: helper function for bilibili request
**
** author: Leopard-C
**
** update: 2020/12/02
**************************************************************/
#pragma once
#include <string>
#include "../iclient/ic/ic.h"


namespace bili {

    const std::string APP_KEY = "6f90a59ac58a4123";
	const std::string BUILD = "6091000";
	const std::string C_LOCALE = "en_CN";
	const std::string CHANNEL = "yingyongbao";
	const std::string MOBI_APP = "android";
	const std::string PLATFORM = "android";
	const std::string S_LOCALE = "en_CN";
    const std::string SECRET_KEY = "0bfd84cc3940035173f35e6777508326";

    /* 生成签名: App端的API需要鉴权签名 */
	std::string getSign(ic::Url& url);

}
