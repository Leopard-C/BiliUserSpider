#include <QtWidgets/QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <fstream>

#include "global.h"
#include "log/logger.h"
#include "util/path.h"
#include "config/config.h"
#include "spidermanager.h"


/* 各种目录 */
std::string g_projDir = util::getProjDir();
std::string g_srcDir = g_projDir + "/src";
std::string g_cfgDir = g_projDir + "/config";
std::string g_logsDir = g_projDir + "/logs";

/* 各种配置 */
DatabaseConfig g_dbConfig;
ClientConfig   g_clientConfig;
ServerConfig   g_svrConfig;
ProxyConfig    g_proxyConfig;

/* 服务器地址 */
std::string g_svrAddr;

/* 通信密码 */
std::string g_svrPwd;

/* 服务器状态 */
bool g_isSvrRunning;

/* 是否与服务器连接 */
bool g_isSvrConnected;

/* 读取服务器地址 */
bool readServerInfo();


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    /* 创建相关文件夹 */
    if (!util::createDir(g_logsDir)) {
        QMessageBox::critical(nullptr, "Error", "Create logs dir failed", QMessageBox::Ok);
        return 1;
    }
    if (!util::createDir(g_cfgDir)) {
        QMessageBox::critical(nullptr, "Error", "Create config dir failed", QMessageBox::Ok);
        return 1;
    }

    /* 从配置文件读取服务器地址 */
    if (!readServerInfo()) {
        //QMessageBox::critical(nullptr, "Error", "Read server info failed", QMessageBox::Ok);
        //return 1;
    }

    /* 输入密码 */
    QString text = QInputDialog::getText(nullptr, "Prompt", "Input password", QLineEdit::Password, 0);
    if (text.isEmpty()) {
        QMessageBox::critical(nullptr, "Error", "No password was input, quit.", QMessageBox::Ok);
        return 1;
    }
    g_svrPwd = text.toStdString();

    SpiderManager w;
    w.show();
    return a.exec();
}

bool readServerInfo() {
    std::ifstream ifs(g_cfgDir + "/server.json");
    if (!ifs) {
        LError("Open configuration file failed");
        return false;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root, false)) {
        LError("Parse configuration file failed");
        return false;
    }

    if (checkNull(root, "address")) {
        LError("Missing key:address");
        return false;
    }

    g_svrAddr = getStrVal(root, "address");

    return true;
}

