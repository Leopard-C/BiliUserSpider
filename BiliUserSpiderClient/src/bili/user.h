/*******************************************************
** namespace:   bili
**
** struct:      UserInfo
**
** function:    getUserInfo_Web, getUserInfo_App
**
** description: bilibili user's info
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>

class ProxyManager;
class UserAgentManager;


namespace bili {

struct UserInfo {
	char sex;
	char level;
	char vip_status;
	char vip_type;
	char official_type;
	char official_role;
	char is_deleted;      /* 是否已经注销 */
	int attention;
	int fans;
	int mid;    /* 用户ID, 即 UID */
	int lid;    /* 直播间ID */
	std::string name;
	std::string face;
	std::string sign;
	std::string vip_label;
	std::string official_desc;
	std::string official_title;
};

enum BILI_RET {
	RET_NO_ERORR     = 0,
	RET_RISK_CONTROL = 1,
	RET_ERROR        = 2,
	RET_CRITICAL     = 3,
	RET_NOT_FOUND    = 4
};

/*
 *  获取用户ID为mid的个人信息 
 *
 *    接口类型：
 *      Web: 
 *      App: 返回信息比web详细 (推荐)
 *
 *    返回值:
 *      0: 获取用户信息成功
 *      1: 用户信息为空，用户不存在
 *     -1: 获取信息失败
**/
int getUserInfo_Web(int mid, int timeout_ms, ProxyManager* proxyMgr, UserAgentManager* uaMgr, UserInfo* info);
int getUserInfo_App(int mid, int timeout_ms, ProxyManager* proxyMgr, UserAgentManager* uaMgr, UserInfo* info);

} // namespace bili

