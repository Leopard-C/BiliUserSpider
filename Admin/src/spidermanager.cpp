#include "spidermanager.h"
#include <QApplication>
#include <QInputDialog>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QContextMenuEvent>

#include <thread>

#include "api/api_config.h"
#include "api/api_proxy.h"
#include "api/api_server.h"
#include "status/status_code.h"
#include "util/string_util.h"
#include "log/logger.h"
#include "global.h"

#define GROUPBOX_QSS \
        "QGroupBox{border-width:1px;border-style:solid;"\
        "border-radius:2px;border-color:gray;margin-top:1ex;}"\
        "QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;"\
        "left:10px;margin-left:3px;padding:0 1px;}"


SpiderManager::SpiderManager(QWidget *parent)
    : QMainWindow(parent)
{
    setupLayout();
    connect(this, &SpiderManager::sigUpdateClientsInfo, this, &SpiderManager::onUpdateClientsInfo);
    connect(this, &SpiderManager::sigUpdateProxyPoolSize, this, &SpiderManager::onUpdateProxyPoolSize);
    connect(this, &SpiderManager::sigUpdateProgress, this, &SpiderManager::onUpdateProgress);

    /* 右键菜单（仅表格中） */
    menuRightClick = new QMenu();
    QAction* actionStopClient = new QAction("Stop");
    QAction* actionForceStopClient = new QAction("ForceStop");
    menuRightClick->addAction(actionStopClient);
    menuRightClick->addAction(actionForceStopClient);
    connect(actionStopClient, &QAction::triggered, this, &SpiderManager::onQuitClient);
    connect(actionForceStopClient, &QAction::triggered, this, &SpiderManager::onForceQuitClient);

    /* Timer(改变控件颜色，起到提醒的作用) */
    timerForProxyPoolSize = new QTimer(this);
    timerForTaskProgress = new QTimer(this);
    connect(timerForTaskProgress, &QTimer::timeout, this, &SpiderManager::onTimerForTaskProgressTimeout);
    connect(timerForProxyPoolSize, &QTimer::timeout, this, &SpiderManager::onTimerForProxyPoolSizeTimeout);
}


SpiderManager::~SpiderManager() {
    {
        std::lock_guard<std::mutex> lck(mutexForMysqlDbPool);
        if (mysqlDbPool) {
            delete mysqlDbPool;
            mysqlDbPool = nullptr;
        }
    }
}

/********************************** 点击按钮 改变服务器状态 ***************************************/

void SpiderManager::onConnect() {
    /* 获取服务器地址 */
    QString serverAddr = editServerAddr->text().trimmed();
    if (serverAddr.isEmpty()) {
        QMessageBox::critical(this, "Error", "Please input server address", QMessageBox::Ok);
        return;
    }
    if (serverAddr.left(4) != "http") {
        serverAddr = "http://" + serverAddr;
    }
    g_svrAddr = serverAddr.toStdString();

    /* 连接服务器 */
    std::string errMsg;
    bool flag = false;
    if (api_connect_to_server(g_svrAddr, &g_isSvrRunning, &errMsg) != NO_ERROR) {
        LError("{}", errMsg);
        LError("Connect to server failed");
        return;
    }

    /* 获取所有配置项 */
    if (!getAllConfig()) {
        LError("Get all configuration failed");
        return;
    }

    /* 创建数据库连接池(1个连接即可) */
    {
        std::lock_guard<std::mutex> lck(mutexForMysqlDbPool);
        if (mysqlDbPool) {
            delete mysqlDbPool;
            mysqlDbPool = nullptr;
        }
        mysqlDbPool = new MysqlDbPool(g_dbConfig.host, g_dbConfig.port, g_dbConfig.user,
            g_dbConfig.password, g_dbConfig.name, 1);
        if (mysqlDbPool->bad()) {
            LError("Connect to mysql database failed");
            delete mysqlDbPool;
            mysqlDbPool = nullptr;
        }
        else {
            LInfo("Connect to mysql database ok");
        }
    }

    /* 连接服务器成功，修改控件状态等工作 */
    onServerConnected();

    /* 启动后台线程 */
    if (isBgThreadQuit) {
        isBgThreadQuit = false;
        std::thread t(backgroundThread, this);
        t.detach();
    }
}

void SpiderManager::onDisconnect() {
    onServerDisconnected();
}

void SpiderManager::onStart() {
    for (int i = 0; i < 3; ++i) {
        std::string errMsg;
        if (api_start_server(g_svrAddr, &errMsg) == NO_ERROR) {
            onServerStarted();
            return;
        }
        LError("{}", errMsg);
    }
    LError("Start server failed");
}

void SpiderManager::onStop() {
    int btn = QMessageBox::question(this, "Confirm", "Confirm to stop the server?", QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        std::string errMsg;
        if (api_stop_server(g_svrAddr, &errMsg) == NO_ERROR) {
            onServerStopped();
            return;
        }
        LError("{}", errMsg);
    }
    LError("Stop server failed!");
}

void SpiderManager::onShutdown() {
    int btn = QMessageBox::question(this, "Confirm", "Confirm to shutdown the server?", QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        std::string errMsg;
        if (api_shutdown_server(g_svrAddr, &errMsg) == NO_ERROR) {
            onServerShutdown();
            return;
        }
        LError("{}", errMsg);
    }
    LError("Shutdown server failed!");
}

void SpiderManager::onSetPassword() {
    QString text = QInputDialog::getText(nullptr, "Prompt", "Input password", QLineEdit::Password, 0);
    if (!text.isEmpty()) {
        g_svrPwd = text.toStdString();
    }
}


/************************************** 服务器状态发生改变时调用以下代码 ******************************************/

/* 连接服务器成功 */
void SpiderManager::onServerConnected() {
    g_isSvrConnected = true;
    editServerAddr->setEnabled(false);
    btnConnect->setEnabled(false);
    btnDisconnect->setEnabled(true);
    btnShutdown->setEnabled(true);
    if (g_isSvrRunning) {
        btnStart->setEnabled(false);
        btnStop->setEnabled(true);
        /* 服务器运行时，只能修改终止mid */
        editMidStart->setEnabled(false);
        editMidCurrent->setEnabled(false);
        editMidEnd->setEnabled(true);
    }
    else {
        btnStart->setEnabled(true);
        btnStop->setEnabled(false);
        /* 服务器停止时，可以修改整个任务范围和进度 */
        editMidStart->setEnabled(true);
        editMidCurrent->setEnabled(true);
        editMidEnd->setEnabled(true);
    }
    clientConfigGroupBox->setEnabled(true);
    dbConfigGroupBox->setEnabled(true);
    proxyConfigGroupBox->setEnabled(true);
    svrConfigGroupBox->setEnabled(true);
    taskRangeGroupBox->setEnabled(true);
    this->update();
    LInfo("Connect to server ok!");
}

/* 服务器断开连接 */
void SpiderManager::onServerDisconnected() {
    g_isSvrConnected = false;
    editServerAddr->setEnabled(true);
    btnConnect->setEnabled(true);
    btnDisconnect->setEnabled(false);
    btnStart->setEnabled(false);
    btnStop->setEnabled(false);
    btnShutdown->setEnabled(false);
    clientsTable->clearContents();
    clientConfigGroupBox->setEnabled(false);
    dbConfigGroupBox->setEnabled(false);
    proxyConfigGroupBox->setEnabled(false);
    svrConfigGroupBox->setEnabled(false);
    taskRangeGroupBox->setEnabled(false);
    this->update();
    LInfo("Disconnect to server ok");
}

/* 服务器启动成功 (逻辑) */
void SpiderManager::onServerStarted() {
    g_isSvrRunning = true;
    editServerAddr->setEnabled(false);
    btnConnect->setEnabled(false);
    btnDisconnect->setEnabled(true);
    btnStart->setEnabled(false);
    btnStop->setEnabled(true);
    btnShutdown->setEnabled(true);
    /* 服务器运行时，只能修改终止mid */
    editMidStart->setEnabled(false);
    editMidCurrent->setEnabled(false);
    editMidEnd->setEnabled(true);
    this->update();
    LInfo("Server started");
}

/* 服务器停止运行 (逻辑) */
void SpiderManager::onServerStopped() {
    g_isSvrRunning = false;
    editServerAddr->setEnabled(false);
    btnConnect->setEnabled(false);
    btnDisconnect->setEnabled(true);
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
    btnShutdown->setEnabled(true);
    /* 服务器停止时，可以修改整个任务范围和进度 */
    editMidStart->setEnabled(true);
    editMidCurrent->setEnabled(true);
    editMidEnd->setEnabled(true);
    this->update();
    //LInfo("Server stopped");
}

/* 服务器关闭 (程序) */
void SpiderManager::onServerShutdown() {
    g_isSvrRunning = false;
    onServerDisconnected();
    LInfo("Server shutdown");
}



/************************************** 获取/修改 配置项 *********************************************/

/* 获取所有配置 */
bool SpiderManager::getAllConfig() {
    ClientConfig clientConfigTmp;
    DatabaseConfig dbConfigTmp;
    ProxyConfig proxyConfigTmp;
    ServerConfig svrConfigTmp;
    for (int i = 0; i < 3; ++i) {
        std::string errMsg;
        if (NO_ERROR == api_get_advance_config(g_svrAddr, &clientConfigTmp, &dbConfigTmp,
            &proxyConfigTmp, &svrConfigTmp, &errMsg))
        {
            LInfo("Get configuration ok");
            updateAllConfig(clientConfigTmp, dbConfigTmp, proxyConfigTmp, svrConfigTmp);
            return true;
        }
        LError("{}", errMsg);
    }
    return false;
}

void SpiderManager::updateAllConfig(const ClientConfig& clientConfig, const DatabaseConfig& dbConfig,
    const ProxyConfig& proxyConfig, const ServerConfig& svrConfig)
{
    /* 客户端配置 */
    g_clientConfig = clientConfig;
    if (clientConfig.bili_api_type == 0) {
        btnClientBiliApiType_Web->setChecked(true); /* Web API */
    }
    else {
        btnClientBiliApiType_App->setChecked(true); /* APP API */
    }
    if (clientConfig.main_task_step_times == 1) {
        btnClientMainTaskTimes_1->setChecked(true);
    }
    else if (clientConfig.main_task_step_times == 2) {
        btnClientMainTaskTimes_2->setChecked(true);
    }
    else {
        btnClientMainTaskTimes_3->setChecked(true);
    }
    editClientSubTaskStep->setText(QString::number(clientConfig.sub_task_step));
    editClientTimeout->setText(QString::number(clientConfig.timeout));
    editClientProxyMaxError->setText(QString::number(clientConfig.proxy_max_error_times));
    editClientMidMaxError->setText(QString::number(clientConfig.mid_max_error_times));
    editClientIntervalProxyPoolSync->setText(QString::number(clientConfig.interval_proxypool_sync));
    pstmDialog->setMap(clientConfig.proxy_sleeptime_map);

    /* 数据库配置 */
    g_dbConfig = dbConfig;
    editDbHost->setText(QString::fromStdString(dbConfig.host));
    editDbPort->setText(QString::number(dbConfig.port));
    editDbUser->setText(QString::fromStdString(dbConfig.user));
    editDbPwd->setText(QString::fromStdString(dbConfig.password));
    editDbName->setText(QString::fromStdString(dbConfig.name));

    /* 代理ip配置 */
    g_proxyConfig = proxyConfig;
    editProxyOrderCode->setText(QString::fromStdString(proxyConfig.order_id));
    editProxyUserId->setText(QString::fromStdString(proxyConfig.user_id));
    editProxyIntervalQuery->setText(QString::number(proxyConfig.interval_query));

    /* 服务器配置 */
    g_svrConfig = svrConfig;
    editSvrMaxLiveHeartbeat->setText(QString::number(svrConfig.client_live_heartbeat));
    editSvrIntervalDetectClientLive->setText(QString::number(svrConfig.interval_detect_client_live));
    editSvrMainTaskTimeout->setText(QString::number(svrConfig.main_task_timeout));
}


/* 修改客户端配置项 */
void SpiderManager::onSetClientConfig() {
    QString subTaskStep = editClientSubTaskStep->text();
    QString timeout = editClientTimeout->text();
    QString proxyMaxErrorCount = editClientProxyMaxError->text();
    QString midMaxErrorCount = editClientMidMaxError->text();
    QString intvProxyPoolSync = editClientIntervalProxyPoolSync->text();
    if (subTaskStep.isEmpty() || timeout.isEmpty() || proxyMaxErrorCount.isEmpty()
        || midMaxErrorCount.isEmpty() || intvProxyPoolSync.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Please fill in all the input box first.", QMessageBox::Ok);
        return;
    }

    ClientConfig clientConfigTmp;
    clientConfigTmp.bili_api_type = btnClientBiliApiType_Web->isChecked() ? 0 : 1;
    if (btnClientMainTaskTimes_1->isChecked()) {
        clientConfigTmp.main_task_step_times = 1;
    }
    else if (btnClientMainTaskTimes_2->isChecked()) {
        clientConfigTmp.main_task_step_times = 2;
    }
    else {
        clientConfigTmp.main_task_step_times = 3;
    }
    clientConfigTmp.sub_task_step = subTaskStep.toInt();
    clientConfigTmp.timeout = timeout.toInt();
    clientConfigTmp.proxy_max_error_times = proxyMaxErrorCount.toInt();
    clientConfigTmp.mid_max_error_times = midMaxErrorCount.toInt();
    clientConfigTmp.interval_proxypool_sync = intvProxyPoolSync.toInt();
    clientConfigTmp.proxy_sleeptime_map = pstmDialog->getMap();

    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_set_client_config(g_svrAddr, clientConfigTmp, &errMsg);
        if (ret == NO_ERROR) {
            g_clientConfig = clientConfigTmp;
            LInfo("Set database configuration ok!");
            return;
        }
        LError("{}", errMsg);
    }
    LError("Set databse configuration failed");
}

/* 修改数据库配置项 */
void SpiderManager::onSetDbConfig() {
    QString host = editDbHost->text();
    QString port = editDbPort->text();
    QString user = editDbUser->text();
    QString pwd = editDbPwd->text();
    QString name = editDbName->text();
    if (host.isEmpty() || port.isEmpty() || user.isEmpty() || pwd.isEmpty() || name.isEmpty()) {
        QMessageBox::critical(this, "Error", "Please fill in all the input box first.", QMessageBox::Ok);
        return;
    }

    DatabaseConfig dbConfigTmp;
    dbConfigTmp.host = host.toStdString();
    dbConfigTmp.port = port.toUInt();
    dbConfigTmp.user = user.toStdString();
    dbConfigTmp.password = pwd.toStdString();
    dbConfigTmp.name = name.toStdString();

    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_set_database_config(g_svrAddr, dbConfigTmp, &errMsg);
        if (ret == NO_ERROR) {
            LInfo("Set database configuration ok!");
            std::lock_guard<std::mutex> lck(mutexForMysqlDbPool);
            g_dbConfig = dbConfigTmp;
            if (mysqlDbPool) {
                delete mysqlDbPool;
            }
            mysqlDbPool = new MysqlDbPool(g_dbConfig.host, g_dbConfig.port, g_dbConfig.user,
                g_dbConfig.password, g_dbConfig.name, 2);
            if (mysqlDbPool->bad()) {
                LError("Connect to mysql database failed!");
                delete mysqlDbPool;
                mysqlDbPool = nullptr;
            }
            return;
        }
        LError("{}", errMsg);
    }
    LError("Set database configuration failed!");
}

/* 修改代理IP配置项 */
void SpiderManager::onSetProxyConfig() {
    QString orderCode = editProxyOrderCode->text();
    QString userId = editProxyUserId->text();
    QString intvQuery = editProxyIntervalQuery->text();
    if (orderCode.isEmpty() || /*userId.isEmpty() ||*/ intvQuery.isEmpty()) {
        QMessageBox::critical(this, "Error", "Please fill in all the input box first.", QMessageBox::Ok);
        return;
    }

    ProxyConfig proxyConfigTmp;
    proxyConfigTmp.order_id = orderCode.toStdString();
    proxyConfigTmp.user_id = userId.toStdString();
    proxyConfigTmp.interval_query = intvQuery.toInt();

    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_set_proxy_config(g_svrAddr, proxyConfigTmp, &errMsg);
        if (ret == NO_ERROR) {
            g_proxyConfig = proxyConfigTmp;
            LInfo("Set proxy configuration ok");
            return;
        }
        else {
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    LError("Set proxy configuration failed");
}

/* 修改服务器配置项 */
void SpiderManager::onSetServerConfig() {
    QString maxLiveHeartbeat = editSvrMaxLiveHeartbeat->text();
    QString intvDetectClientLive = editSvrIntervalDetectClientLive->text();
    QString mainTaskTimeout = editSvrMainTaskTimeout->text();
    if (maxLiveHeartbeat.isEmpty() || intvDetectClientLive.isEmpty() || mainTaskTimeout.isEmpty()) {
        QMessageBox::critical(this, "Error", "Please fill in all the input box first.", QMessageBox::Ok);
        return;
    }

    ServerConfig svrConfigTmp;
    svrConfigTmp.client_live_heartbeat = maxLiveHeartbeat.toInt();
    svrConfigTmp.interval_detect_client_live = intvDetectClientLive.toInt();
    svrConfigTmp.main_task_timeout = mainTaskTimeout.toInt();

    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_set_server_config(g_svrAddr, svrConfigTmp, &errMsg);
        if (ret == NO_ERROR) {
            LInfo("Set server configuration ok");
            g_svrConfig = svrConfigTmp;
            return;
        }
        else {
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    LError("Set server configuration failed");
}



/*************************************** 更新客户端列表 ***************************************************/

/* 更新客户端信息  */
void SpiderManager::onUpdateClientsInfo() {
    clientsInfo.clear();
    std::string errMsg;
    bool status;
    for (int i = 0; i < 3 && g_isSvrConnected; ++i) {
        int ret = api_get_clients_info(g_svrAddr, &clientsInfo, &status, &errMsg);
        if (ret == NO_ERROR) {
            if (!status) {
                onServerStopped();
            }
            showClientsInfo(true);
            return;
        }
        LError("{}", errMsg);
    }
    
    /* 获取失败，断开连接 */
    LError("Update clients info failed");
    onDisconnect();
}

/* 显示客户端爬虫状态 */
void SpiderManager::showClientsInfo(bool isNew) {
    /* 尝试加锁 */
    if (!mutexForClientsInfo.try_lock()) {
        return;
    }
    /* 排序 */
    switch (sortCol) {
    default: break;
    case 0: sortClientsInfoById(clientsInfo, isASC); break;
    case 1: sortClientsInfoByHost(clientsInfo, isASC); break;
    case 2: sortClientsInfoByOS(clientsInfo, isASC); break;
    case 3: sortClientsInfoByJoinTime(clientsInfo, isASC); break;
    case 4: sortClientsInfoByThreads(clientsInfo, isASC); break;
    case 5: sortClientsInfoByCount(clientsInfo, isASC); break;
    case 6: sortClientsInfoByAvgSpeed_3m(clientsInfo, isASC); break;
    case 7: sortClientsInfoByAvgSpeed_5m(clientsInfo, isASC); break;
    case 8: sortClientsInfoByAvgSpeed_10m(clientsInfo, isASC); break;
    case 9: sortClientsInfoByAvgSpeed_30m(clientsInfo, isASC); break;
    case 10: sortClientsInfoByAvgSpeed(clientsInfo, isASC); break;
    }

    int count = clientsInfo.size();
    clientsTable->clearContents();
    clientsTable->setRowCount(count + 1);

    time_t now = time(NULL);
    int countTotal = 0;
    int threadsTotal = 0;
    double speedTotal = 0;
    double speedTotal_3m = 0;
    double speedTotal_5m = 0;
    double speedTotal_10m = 0;
    double speedTotal_30m = 0;

    for (int row = 0; row < count; ++row) {
        SpiderClientInfo& info = clientsInfo[row];

        /* 0. 编号 */
        QTableWidgetItem* itemId = new QTableWidgetItem();
        itemId->setText(QString::number(info.id));
        itemId->setTextAlignment(Qt::AlignCenter);
        clientsTable->setItem(row, 0, itemId);

        /* 1. 客户端IP地址 */
        QTableWidgetItem* itemHost = new QTableWidgetItem();
        itemHost->setText(QString::fromStdString(info.host));
        itemHost->setTextAlignment(Qt::AlignCenter);
        clientsTable->setItem(row, 1, itemHost);

        /* 2. 客户端操作系统 */
        QTableWidgetItem* itemOS = new QTableWidgetItem();
        itemOS->setText(QString::fromStdString(info.os));
        itemOS->setTextAlignment(Qt::AlignCenter);
        clientsTable->setItem(row, 2, itemOS);

        /* 3. 客户端运行时间 */
        QTableWidgetItem* itemTime = new QTableWidgetItem();
        itemTime->setText(QString::fromStdString(util::formatTimeSpan(now - info.join_time)));
        itemTime->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 3, itemTime);

        /* 4. 客户端启动的线程数量 */
        QTableWidgetItem* itemThreads = new QTableWidgetItem();
        itemThreads->setText(QString::number(info.threads));
        itemThreads->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 4, itemThreads);
        threadsTotal += info.threads;

        /* 5. 客户端累计爬虫数量 */
        QTableWidgetItem* itemCount = new QTableWidgetItem();
        itemCount->setText(QString::number(info.count));
        itemCount->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 5, itemCount);
        countTotal += info.count;

        /* 6. 客户端3分钟内爬虫平均速度 */
        QTableWidgetItem* itemSpeed_3m = new QTableWidgetItem();
        itemSpeed_3m->setText(QString::number(info.average_speed_3m, 'f', 3));
        itemSpeed_3m->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 6, itemSpeed_3m);
        speedTotal_3m += info.average_speed_3m;
        if (isNew) {
            auto speed_3m_it = speeds_3m.find(info.id);
            if (speed_3m_it == speeds_3m.end()) {
                speeds_3m.emplace(info.id, info.average_speed_3m);
            }
            else {
                if (enableColorSpeeds_3m) {
                    if (info.average_speed_3m > speed_3m_it->second) {
                        itemSpeed_3m->setBackgroundColor(qRgb(178, 40, 40));
                    }
                    else {
                        itemSpeed_3m->setBackgroundColor(qRgb(68, 150, 79));
                    }
                }
                speed_3m_it->second = info.average_speed_3m;
            }
        }

        /* 7. 客户端5分钟内爬虫平均速度 */
        QTableWidgetItem* itemSpeed_5m = new QTableWidgetItem();
        itemSpeed_5m->setText(QString::number(info.average_speed_5m, 'f', 3));
        itemSpeed_5m->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 7, itemSpeed_5m);
        speedTotal_5m += info.average_speed_5m;
        auto speed_3m_it = speeds_3m.find(info.id);
        if (isNew) {
            auto speed_5m_it = speeds_5m.find(info.id);
            if (speed_5m_it == speeds_5m.end()) {
                speeds_5m.emplace(info.id, info.average_speed_5m);
            }
            else {
                if (enableColorSpeeds_5m) {
                    if (info.average_speed_5m > speed_5m_it->second) {
                        itemSpeed_5m->setBackgroundColor(qRgb(178, 40, 40));
                    }
                    else {
                        itemSpeed_5m->setBackgroundColor(qRgb(68, 150, 79));
                    }
                }
                speed_5m_it->second = info.average_speed_5m;
            }
        }

        /* 8. 客户端10分钟内爬虫平均速度 */
        QTableWidgetItem* itemSpeed_10m = new QTableWidgetItem();
        itemSpeed_10m->setText(QString::number(info.average_speed_10m, 'f', 3));
        itemSpeed_10m->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 8, itemSpeed_10m);
        speedTotal_10m += info.average_speed_10m;
        if (isNew) {
            auto speed_10m_it = speeds_10m.find(info.id);
            if (speed_10m_it == speeds_10m.end()) {
                speeds_10m.emplace(info.id, info.average_speed_10m);
            }
            else {
                if (enableColorSpeeds_10m) {
                    if (info.average_speed_10m > speed_10m_it->second) {
                        itemSpeed_10m->setBackgroundColor(qRgb(178, 40, 40));
                    }
                    else {
                        itemSpeed_10m->setBackgroundColor(qRgb(68, 150, 79));
                    }
                }
                speed_10m_it->second = info.average_speed_10m;
            }
        }

        /* 9. 客户端30分钟内爬虫平均速度 */
        QTableWidgetItem* itemSpeed_30m = new QTableWidgetItem();
        itemSpeed_30m->setText(QString::number(info.average_speed_30m, 'f', 3));
        itemSpeed_30m->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 9, itemSpeed_30m);
        speedTotal_30m += info.average_speed_30m;
        if (isNew) {
            auto speed_30m_it = speeds_30m.find(info.id);
            if (speed_30m_it == speeds_30m.end()) {
                speeds_30m.emplace(info.id, info.average_speed_30m);
            }
            else {
                if (enableColorSpeeds_30m) {
                    if (info.average_speed_30m > speed_30m_it->second) {
                        itemSpeed_30m->setBackgroundColor(qRgb(178, 40, 40));
                    }
                    else {
                        itemSpeed_30m->setBackgroundColor(qRgb(68, 150, 79));
                    }
                }
                speed_30m_it->second = info.average_speed_30m;
            }
        }

        /* 10. 客户端平均爬虫速度 */
        QTableWidgetItem* itemSpeed = new QTableWidgetItem();
        itemSpeed->setText(QString::number(info.average_speed, 'f', 3));
        itemSpeed->setTextAlignment(Qt::AlignRight);
        clientsTable->setItem(row, 10, itemSpeed);
        speedTotal += info.average_speed;
        if (isNew) {
            auto speed_it = speeds.find(info.id);
            if (speed_it == speeds.end()) {
                speeds.emplace(info.id, info.average_speed);
            }
            else {
                if (enableColorSpeeds) {
                    if (info.average_speed > speed_it->second) {
                        itemSpeed->setBackgroundColor(qRgb(178, 40, 40));
                    }
                    else {
                        itemSpeed->setBackgroundColor(qRgb(68, 150, 79));
                    }
                }
                speed_it->second = info.average_speed;
            }
        }
    }

    /*** 汇总信息 ***/

    /* 0. 编号 (-> 爬虫客户端数量) */
    QTableWidgetItem* itemIdTotal = new QTableWidgetItem();
    //itemIdTotal->setFlags(itemIdTotal->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
    itemIdTotal->setText(QString("Total(%1)").arg(count));
    itemIdTotal->setTextAlignment(Qt::AlignCenter);
    clientsTable->setItem(count, 0, itemIdTotal);

    /* 4. 客户端累计启动的线程数量 */
    QTableWidgetItem* itemThreadsTotal = new QTableWidgetItem();
    itemThreadsTotal->setText(QString::number(threadsTotal));
    itemThreadsTotal->setTextAlignment(Qt::AlignRight);
    clientsTable->setItem(count, 4, itemThreadsTotal);

    /* 5. 客户端累计爬虫数量 */
    QTableWidgetItem* itemCountTotal = new QTableWidgetItem();
    itemCountTotal->setText(QString::number(countTotal));
    clientsTable->setItem(count, 5, itemCountTotal);
    itemCountTotal->setTextAlignment(Qt::AlignRight);

    /* 6. 客户端累计爬虫速度(3分钟) */
    QTableWidgetItem* itemSpeedTotal_3m = new QTableWidgetItem();
    itemSpeedTotal_3m->setText(QString::number(speedTotal_3m, 'f', 3));
    itemSpeedTotal_3m->setTextAlignment(Qt::AlignRight);
    clientsTable->setItem(count, 6, itemSpeedTotal_3m);

    /* 7. 客户端累计爬虫速度(5分钟) */
    QTableWidgetItem* itemSpeedTotal_5m = new QTableWidgetItem();
    itemSpeedTotal_5m->setText(QString::number(speedTotal_5m, 'f', 3));
    itemSpeedTotal_5m->setTextAlignment(Qt::AlignRight);
    clientsTable->setItem(count, 7, itemSpeedTotal_5m);

    /* 8. 客户端累计爬虫速度(3分钟) */
    QTableWidgetItem* itemSpeedTotal_10m = new QTableWidgetItem();
    itemSpeedTotal_10m->setText(QString::number(speedTotal_10m, 'f', 3));
    itemSpeedTotal_10m->setTextAlignment(Qt::AlignRight);
    clientsTable->setItem(count, 8, itemSpeedTotal_10m);

    /* 9. 客户端累计爬虫速度(30分钟) */
    QTableWidgetItem* itemSpeedTotal_30m = new QTableWidgetItem();
    itemSpeedTotal_30m->setTextAlignment(Qt::AlignRight);
    itemSpeedTotal_30m->setText(QString::number(speedTotal_30m, 'f', 3));
    clientsTable->setItem(count, 9, itemSpeedTotal_30m);

    /* 10. 客户端累计爬虫速度 */
    QTableWidgetItem* itemSpeedTotal = new QTableWidgetItem();
    itemSpeedTotal->setText(QString::number(speedTotal, 'f', 3));
    itemSpeedTotal->setTextAlignment(Qt::AlignRight);
    clientsTable->setItem(count, 10, itemSpeedTotal);

    /* 解锁 */
    mutexForClientsInfo.unlock();
}

/* 单击某一列 */
/* 排序 */
void SpiderManager::onHorizontalHeaderClicked(int col) {
    if (col == sortCol) {
        isASC = !isASC;
    }
    else {
        isASC = col < 3;
    }
    sortCol = col;
    showClientsInfo(false);
}

/* 双击某一列 */
/* 只对 速度相关的列有效 */
/* 开启/关闭 颜色 */
void SpiderManager::onHorizontalHeaderDoubleClicked(int col) {
    switch (col) {
    default: return;
    case 6: enableColorSpeeds_3m = !enableColorSpeeds_3m; break;
    case 7: enableColorSpeeds_5m = !enableColorSpeeds_5m; break;
    case 8: enableColorSpeeds_10m = !enableColorSpeeds_10m; break;
    case 9: enableColorSpeeds_30m = !enableColorSpeeds_30m; break;
    case 10: enableColorSpeeds = !enableColorSpeeds; break;
    }
    emit sigUpdateClientsInfo();
}

/*************************************** 后台线程 *****************************************/

/*
 * 后台线程，每隔5s
 *   获取代理IP池大小
 *   获取所有爬虫客户端状态
**/
void SpiderManager::backgroundThread(SpiderManager* sm) {  
    while (!sm->shouldClose && g_isSvrConnected) {
        emit sm->sigUpdateClientsInfo();
        emit sm->sigUpdateProxyPoolSize();
        emit sm->sigUpdateProgress();
        for (int i = 0; i < 50 && !sm->shouldClose; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    sm->isBgThreadQuit = true;
}



/************************************** 事件 *******************************************/

/* 关闭事件 */
void SpiderManager::closeEvent(QCloseEvent *ev) {
    g_isSvrConnected = false;
    shouldClose = true;
    int count = 20;
    while (--count && !isBgThreadQuit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/* 右键点击 */
void SpiderManager::onCustomContextMenuRequestedOnClientsTable(const QPoint& pos) {
    /* 尝试加锁 */
    if (!mutexForClientsInfo.try_lock()) {
        return;
    }

    QTableWidgetItem* item = clientsTable->itemAt(pos);
    if (item && item->row() != clientsTable->rowCount() - 1) {
        item = clientsTable->item(item->row(), 0);  /* 第0列，客户端ID */
        if (item && !item->text().isEmpty()) {
            currentClientId = item->text().toInt();
            menuRightClick->exec(QCursor::pos());
        }
    }

    mutexForClientsInfo.unlock();
    /* 解锁 */
}



/**************************************** 任务范围/进度 *****************************************/

/* 更新任务进度 */
void SpiderManager::onUpdateProgress() {
    int midStart, midEnd, midCurrent;
    bool status;
    std::string errMsg;
    for (int i = 0; i < 3; ++i) {
        int ret = api_get_task_progress(g_svrAddr, &midStart, &midEnd, &midCurrent, &status, &errMsg);
        if (ret == NO_ERROR) {
            if (!status) {
                onServerStopped();
            }
            editMidStart->setText(QString::number(midStart));
            editMidEnd->setText(QString::number(midEnd));
            editMidCurrent->setText(QString::number(midCurrent));
            double progress = double(midCurrent - midStart) / (midEnd - midStart);
            editMidProgress->setText(QString::number(progress * 100, 'f', 4) + "%");
            setWidgetBgColor(editMidCurrent, qRgb(255, 180, 90));
            setWidgetBgColor(editMidProgress, qRgb(255, 180, 90));
            timerForTaskProgress->start(1000);
            return;
        }
        LError("{}", errMsg);
    }

    /* 获取失败，断开连接 */
    LError("Update task progress failed");
    onDisconnect();
}

/* 修改Task范围 */
void SpiderManager::onSetTaskRange() {
    int midStart = editMidStart->text().toInt();
    int midEnd = editMidEnd->text().toInt();
    int midCurrent = editMidCurrent->text().toInt();
    if (midStart < 1 || midEnd < 1 || midCurrent < 1) {
        LError("Invalid range: start={} end={} current={}", editMidStart->text().toStdString(),
            editMidEnd->text().toStdString(), editMidCurrent->text().toStdString());
        return;
    }
    for (int i = 0; i < 3; ++i) {
        std::string errMsg;
        if (api_set_task_range(g_svrAddr, midStart, midEnd, midCurrent, &errMsg) == NO_ERROR) {
            LInfo("Set task range ok");
            return;
        }
        LError("{}", errMsg);
    }
    LError("Set task range failed");
}



/************************************** 代理IP池大小 *****************************************/

/* 更新代理IP数量 */
void SpiderManager::onUpdateProxyPoolSize() {
    std::lock_guard<std::mutex> lck(mutexForMysqlDbPool);
    if (!mysqlDbPool || mysqlDbPool->bad()) {
        return;
    }
    MysqlInstance mysql(mysqlDbPool);
    if (mysql.bad()) {
        LError("Mysql instance is bad");
        return;
    }
    int num;
    std::string errMsg;
    for (int i = 0; i < 10; ++i) {
        int ret = api_get_num_proxies(mysql, &num, &errMsg);
        if (ret == NO_ERROR) {
            editProxyPoolSize->setText(QString::number(num));
            setWidgetBgColor(editProxyPoolSize, qRgb(255, 180, 90));
            timerForProxyPoolSize->start(1000);
            return;
        }
        LError("{}", errMsg);
    }
    LError("Update proxy pool size failed");
}

/* 清空代理池 */
void SpiderManager::onEmptyPorxyPool() {
    int btn = QMessageBox::question(this, "Confirm", "Confirm to empty the proxy pool?", QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
        return;
    }
    std::lock_guard<std::mutex> lck(mutexForMysqlDbPool);
    if (!mysqlDbPool || mysqlDbPool->bad()) {
        LError("Mysql db pool is bad");
        return;
    }
    MysqlInstance mysql(mysqlDbPool);
    if (mysql.bad()) {
        LError("Mysql instance is bad");
        return;
    }

    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        if (api_empty_proxy_pool(mysql, &errMsg) == 0) {
            LInfo("Empty proxy pool ok");
            editProxyPoolSize->setText("0");
            return;
        }
        LError("{}", errMsg);
    }
    LError("Empty proxy pool failed");
}



/************************************* 通知客户端退出 ****************************************/

/* 通知客户端平滑退出 */
void SpiderManager::onQuitClient() {
    int btn = QMessageBox::question(this, "Confirm", QString("Notify client(id=%0) quit").arg(currentClientId),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (btn != QMessageBox::Yes) {
        return;
    }
    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        if (api_quit_client(g_svrAddr, currentClientId, false, &errMsg) == NO_ERROR) {
            LInfo("Notify client:{} stop", currentClientId);
            return;
        }
        LError("{}", errMsg);
    }
    LInfo("Notify client:{} stop failed", currentClientId);
}

/* 通知客户端强制退出 */
void SpiderManager::onForceQuitClient() {
    int btn = QMessageBox::question(this, "Confirm", QString("Notify client(id=%0) force quit").arg(currentClientId),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (btn != QMessageBox::Yes) {
        return;
    }
    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        if (api_quit_client(g_svrAddr, currentClientId, true, &errMsg) == NO_ERROR) {
            LInfo("Notify client:{} force stop", currentClientId);
            return;
        }
        LError("{}", errMsg);
    }
    LInfo("Notify client:{} force stop failed", currentClientId);
}


/************************************************  Timer ****************************************************/

void SpiderManager::onTimerForProxyPoolSizeTimeout() {
    setWidgetBgColor(editProxyPoolSize, qRgb(240, 240, 240));
}

void SpiderManager::onTimerForTaskProgressTimeout() {
    setWidgetBgColor(editMidCurrent, qRgb(240, 240, 240));
    setWidgetBgColor(editMidProgress, qRgb(240, 240, 240));
}

void SpiderManager::setWidgetBgColor(QWidget* widget, const QColor& color) {
    QPalette pal(widget->palette());
    auto c = pal.background().color();
    int r=c.red();
    int g =c.green();
    int b = c.blue();
    pal.setColor(QPalette::Base, color);
    widget->setAutoFillBackground(true);
    widget->setPalette(pal);
    widget->update();
}


/******************************************** 以下全部为布局代码 *********************************************/

/* 服务器信息 */
void SpiderManager::createServerGroupBox() {
    /* 服务器地址 + 按钮  横向布局 */
    QHBoxLayout* serverLayout = new QHBoxLayout();

    /* 0. 地址(IP/Domain + [Port]) */
    editServerAddr = new QLineEdit(this);
    editServerAddr->setText(QString::fromStdString(g_svrAddr));
    editServerAddr->setMinimumWidth(180);
    serverLayout->addStretch();
    serverLayout->addWidget(editServerAddr);

    /* 1. 通信密码 */
    btnSetSvrPwd = new QPushButton("PWD", this);
    btnSetSvrPwd->setMaximumWidth(40);
    connect(btnSetSvrPwd, &QPushButton::clicked, this, &SpiderManager::onSetPassword);
    //serverLayout->addStretch();
    serverLayout->addWidget(btnSetSvrPwd);

    /* 2. 连接 */
    btnConnect = new QPushButton("Connect", this);
    connect(btnConnect, &QPushButton::clicked, this, &SpiderManager::onConnect);
    serverLayout->addStretch();
    serverLayout->addWidget(btnConnect);

    /* 3. 断开连接 */
    btnDisconnect = new QPushButton("Disconnect", this);
    btnDisconnect->setEnabled(false);
    connect(btnDisconnect, &QPushButton::clicked, this, &SpiderManager::onDisconnect);
    serverLayout->addStretch();
    serverLayout->addWidget(btnDisconnect);

    /* 4. 启动 (逻辑)*/
    btnStart = new QPushButton("Start", this);
    btnStart->setEnabled(false);
    connect(btnStart, &QPushButton::clicked, this, &SpiderManager::onStart);
    serverLayout->addStretch();
    serverLayout->addWidget(btnStart);

    /* 5. 停止 (逻辑) */
    btnStop = new QPushButton("Stop", this);
    btnStop->setEnabled(false);
    connect(btnStop, &QPushButton::clicked, this, &SpiderManager::onStop);
    serverLayout->addStretch();
    serverLayout->addWidget(btnStop);

    /* 6. 关闭 (程序) */
    btnShutdown = new QPushButton("Shutdown", this);
    btnShutdown->setEnabled(false);
    connect(btnShutdown, &QPushButton::clicked, this, &SpiderManager::onShutdown);
    serverLayout->addStretch();
    serverLayout->addWidget(btnShutdown);
    serverLayout->addStretch();

    serverGroupBox = new QGroupBox("Server", this);
    serverGroupBox->setLayout(serverLayout);
    serverGroupBox->setStyleSheet(GROUPBOX_QSS);
}

/* 0. 客户端配置信息 */
void SpiderManager::createClientConfigGroupBox() {
    QGridLayout* clientConfigLayout = new QGridLayout();

    /* 0. Bilibili API 类型：Web/App */
    QLabel* label000 = new QLabel("BiliApiType", this);
    label000->setAlignment(Qt::AlignRight);
    QButtonGroup* btnGroupBiliApiType = new QButtonGroup();
    btnClientBiliApiType_Web = new QRadioButton("Web", this);
    btnClientBiliApiType_App = new QRadioButton("App", this);
    btnClientBiliApiType_Web->setMaximumWidth(50);
    btnClientBiliApiType_App->setMaximumWidth(50);
    btnGroupBiliApiType->addButton(btnClientBiliApiType_Web);
    btnGroupBiliApiType->addButton(btnClientBiliApiType_App);
    btnClientBiliApiType_Web->setChecked(true);
    clientConfigLayout->addWidget(label000, 0, 0, 1, 1);
    clientConfigLayout->addWidget(btnClientBiliApiType_Web, 0, 1, 1, 1);
    clientConfigLayout->addWidget(btnClientBiliApiType_App, 0, 2, 1, 1);

    /* 1. 主任务倍数 */
    QLabel* label010 = new QLabel("MainTaskTimes:", this);
    label010->setAlignment(Qt::AlignRight);
    QButtonGroup* btnGroupMainTaskTimes = new QButtonGroup();
    btnClientMainTaskTimes_1 = new QRadioButton("1", this);
    btnClientMainTaskTimes_2 = new QRadioButton("2", this);
    btnClientMainTaskTimes_3 = new QRadioButton("3", this);
    btnGroupMainTaskTimes->addButton(btnClientMainTaskTimes_1);
    btnGroupMainTaskTimes->addButton(btnClientMainTaskTimes_2);
    btnGroupMainTaskTimes->addButton(btnClientMainTaskTimes_3);
    btnClientMainTaskTimes_1->setChecked(true);
    clientConfigLayout->addWidget(label010, 1, 0, 1, 1);
    clientConfigLayout->addWidget(btnClientMainTaskTimes_1, 1, 1, 1, 1);
    clientConfigLayout->addWidget(btnClientMainTaskTimes_2, 1, 2, 1, 1);
    clientConfigLayout->addWidget(btnClientMainTaskTimes_3, 1, 3, 1, 1);

    /* 2.子任务步长 */
    QLabel* label020 = new QLabel("SubTaskStep:", this);
    label020->setAlignment(Qt::AlignRight);
    editClientSubTaskStep = new QLineEdit(this);
    editClientSubTaskStep->setMaximumWidth(50);
    clientConfigLayout->addWidget(label020, 2, 0, 1, 1);
    clientConfigLayout->addWidget(editClientSubTaskStep, 2, 1, 1, 3);

    /* 3. 超时 */
    QLabel* label030 = new QLabel("Timeout(ms):", this);
    label030->setAlignment(Qt::AlignRight);
    editClientTimeout = new QLineEdit(this);
    editClientTimeout->setMaximumWidth(50);
    clientConfigLayout->addWidget(label030, 3, 0, 1, 1);
    clientConfigLayout->addWidget(editClientTimeout, 3, 1, 1, 3);

    /* 4. 代理IP最大失败次数 */
    QLabel* label040 = new QLabel("ProxyMaxError:", this);
    label040->setAlignment(Qt::AlignRight);
    editClientProxyMaxError = new QLineEdit(this);
    editClientProxyMaxError->setMaximumWidth(50);
    clientConfigLayout->addWidget(label040, 4, 0, 1, 1);
    clientConfigLayout->addWidget(editClientProxyMaxError, 4, 1, 1, 3);

    /* 5. 爬取一个用户信息时最大重试次数 */
    QLabel* label050 = new QLabel("MidMaxError:", this);
    label050->setAlignment(Qt::AlignRight);
    editClientMidMaxError = new QLineEdit(this);
    editClientMidMaxError->setMaximumWidth(50);
    clientConfigLayout->addWidget(label050, 5, 0, 1, 1);
    clientConfigLayout->addWidget(editClientMidMaxError, 5, 1, 1, 3);

    /* 6. 同步代理IP池的间隔 */
    QLabel* label060 = new QLabel("IntvSyncProxyPool(s):", this);
    label060->setAlignment(Qt::AlignRight);
    editClientIntervalProxyPoolSync = new QLineEdit(this);
    editClientIntervalProxyPoolSync->setMaximumWidth(50);
    clientConfigLayout->addWidget(label060, 6, 0, 1, 1);
    clientConfigLayout->addWidget(editClientIntervalProxyPoolSync, 6, 1, 1, 3);

    /* 7. 代理IP数量 <--> 暂停时间  映射 */
    QLabel* label070 = new QLabel("ProxyNumSleepMap:", this);
    label070->setAlignment(Qt::AlignRight);
    btnSetProxyNumSleeptimeMap = new QPushButton("Show");
    btnSetProxyNumSleeptimeMap->setMaximumWidth(50);
    clientConfigLayout->addWidget(label070, 7, 0, 1, 1);
    clientConfigLayout->addWidget(btnSetProxyNumSleeptimeMap, 7, 1, 1, 1);
    pstmDialog = new ProxySleepTimeMapDialog(this);
    connect(btnSetProxyNumSleeptimeMap, &QPushButton::clicked, this, [this] {
        this->pstmDialog->show();
    });

    /* 8. 提交按钮 */
    btnSetClientConfig = new QPushButton("Set", this);
    connect(btnSetClientConfig, &QPushButton::clicked, this, &SpiderManager::onSetClientConfig);
    clientConfigLayout->addWidget(btnSetClientConfig, 8, 0, 1, 4);

    clientConfigGroupBox = new QGroupBox("Client", this);
    clientConfigGroupBox->setLayout(clientConfigLayout);
    clientConfigGroupBox->setStyleSheet(GROUPBOX_QSS);
    clientConfigGroupBox->setEnabled(false);
}

/* 1. 数据库配置信息 */
void SpiderManager::createDatabaseConfigGroupBox() {
    QGridLayout* dbConfigLayout = new QGridLayout();

    /* 0. 主机 */
    QLabel* label100 = new QLabel("Host:", this);
    label100->setAlignment(Qt::AlignRight);
    editDbHost = new QLineEdit(this);
    editDbHost->setMinimumWidth(120);

    /* 1. 端口 */
    QLabel* label110 = new QLabel("Port:", this);
    label110->setAlignment(Qt::AlignRight);
    editDbPort = new QLineEdit(this);
    editDbPort->setMinimumWidth(120);
    dbConfigLayout->addWidget(label100, 0, 0, 1, 1);
    dbConfigLayout->addWidget(editDbHost, 0, 1, 1, 1);
    dbConfigLayout->addWidget(label110, 1, 0, 1, 1);
    dbConfigLayout->addWidget(editDbPort, 1, 1, 1, 1);

    /* 2. 用户名 */
    QLabel* label120 = new QLabel("User:", this);
    label120->setAlignment(Qt::AlignRight);
    editDbUser = new QLineEdit(this);
    editDbUser->setMinimumWidth(120);
    dbConfigLayout->addWidget(label120, 2, 0, 1, 1);
    dbConfigLayout->addWidget(editDbUser, 2, 1, 1, 1);

    /* 3. 密码 */
    QLabel* label130 = new QLabel(" Pwd:", this);
    label130->setAlignment(Qt::AlignRight);
    editDbPwd = new QLineEdit(this);
    editDbPwd->setMinimumWidth(120);
    editDbPwd->setEchoMode(QLineEdit::Password);
    dbConfigLayout->addWidget(label130, 3, 0, 1, 1);
    dbConfigLayout->addWidget(editDbPwd, 3, 1, 1, 1);

    /* 4. 数据库名称 */
    QLabel* label140 = new QLabel("Name:", this);
    label140->setAlignment(Qt::AlignRight);
    editDbName = new QLineEdit(this);
    editDbName->setMinimumWidth(120);
    dbConfigLayout->addWidget(label140, 4, 0, 1, 1);
    dbConfigLayout->addWidget(editDbName, 4, 1, 1, 1);

    /* 5. 提交按钮 */
    btnSetDbConfig = new QPushButton("Set", this);
    connect(btnSetDbConfig, &QPushButton::clicked, this, &SpiderManager::onSetDbConfig);
    dbConfigLayout->addWidget(btnSetDbConfig, 5, 0, 1, 2);

    dbConfigGroupBox = new QGroupBox("Database", this);
    dbConfigGroupBox->setLayout(dbConfigLayout);
    dbConfigGroupBox->setStyleSheet(GROUPBOX_QSS);
    dbConfigGroupBox->setEnabled(false);
}

/* 2. 代理IP配置信息 */
void SpiderManager::createProxyConfigGroupBox() {
    QGridLayout* proxyConfigLayout = new QGridLayout();

    /* 0. 订单号 */
    QLabel* label200 = new QLabel("OrderCode:", this);
    label200->setAlignment(Qt::AlignRight);
    editProxyOrderCode = new QLineEdit(this);
    editProxyOrderCode->setMinimumWidth(150);
    proxyConfigLayout->addWidget(label200, 0, 0, 1, 1);
    proxyConfigLayout->addWidget(editProxyOrderCode, 0, 1, 1, 2);

    /* 1. 用户ID/Token(可选) */
    QLabel* label210 = new QLabel("UserID:", this);
    label210->setAlignment(Qt::AlignRight);
    editProxyUserId = new QLineEdit(this);
    editProxyUserId->setMinimumWidth(150);
    proxyConfigLayout->addWidget(label210, 1, 0, 1, 1);
    proxyConfigLayout->addWidget(editProxyUserId, 1, 1, 1, 2);

    /* 2. 提取代理IP间隔 */
    QLabel* label220 = new QLabel("IntvQuery(s):", this);
    label220->setAlignment(Qt::AlignRight);
    editProxyIntervalQuery = new QLineEdit(this);
    editProxyIntervalQuery->setMinimumWidth(150);
    proxyConfigLayout->addWidget(label220, 2, 0, 1, 1);
    proxyConfigLayout->addWidget(editProxyIntervalQuery, 2, 1, 1, 2);

    /* 3. 代理IP数量(不可编辑) */
    QLabel* label230 = new QLabel("PoolSize:", this);
    label230->setAlignment(Qt::AlignRight);
    editProxyPoolSize = new QLineEdit(this);
    editProxyPoolSize->setEnabled(false);
    editProxyPoolSize->setMaximumWidth(60);
    btnEmptyProxyPool = new QPushButton("Empty", this);
    connect(btnEmptyProxyPool, &QPushButton::clicked, this, &SpiderManager::onEmptyPorxyPool);
    proxyConfigLayout->addWidget(label230, 3, 0, 1, 1);
    proxyConfigLayout->addWidget(editProxyPoolSize, 3, 1, 1, 1);
    proxyConfigLayout->addWidget(btnEmptyProxyPool, 3, 2, 1, 1);

    /* 4. 提交按钮 */
    btnSetProxyConfig = new QPushButton("Set", this);
    connect(btnSetProxyConfig, &QPushButton::clicked, this, &SpiderManager::onSetProxyConfig);
    proxyConfigLayout->addWidget(btnSetProxyConfig, 4, 0, 1, 3);

    proxyConfigGroupBox = new QGroupBox("Proxy", this);
    proxyConfigGroupBox->setLayout(proxyConfigLayout);
    proxyConfigGroupBox->setStyleSheet(GROUPBOX_QSS);
    proxyConfigGroupBox->setEnabled(false);
}

/* 3. 服务器配置信息 */
void SpiderManager::createServerConfigGroupBox() {
    QGridLayout* svrConfigLayout = new QGridLayout();

    /* 0. 客户端心跳最大间隔（超过就踢出） */
    QLabel* label300 = new QLabel("MaxLiveHeartbeat(s):", this);
    label300->setAlignment(Qt::AlignRight);
    editSvrMaxLiveHeartbeat = new QLineEdit(this);
    editSvrMaxLiveHeartbeat->setMaximumWidth(80);
    svrConfigLayout->addWidget(label300, 0, 0, 1, 1);
    svrConfigLayout->addWidget(editSvrMaxLiveHeartbeat, 0, 1, 1, 1);

    /* 1. 检测客户端存活的时间间隔 */
    QLabel* label310 = new QLabel("IntvDetectHeartbeat(s):", this);
    label310->setAlignment(Qt::AlignRight);
    editSvrIntervalDetectClientLive = new QLineEdit(this);
    editSvrIntervalDetectClientLive->setMaximumWidth(80);
    svrConfigLayout->addWidget(label310, 1, 0, 1, 1);
    svrConfigLayout->addWidget(editSvrIntervalDetectClientLive, 1, 1, 1, 1);

    /* 2. 主任务最长存活时间 */
    QLabel* label320 = new QLabel("MainTaskTimeout(s):", this);
    label320->setAlignment(Qt::AlignRight);
    editSvrMainTaskTimeout = new QLineEdit(this);
    editSvrMainTaskTimeout->setMaximumWidth(80);
    svrConfigLayout->addWidget(label320, 2, 0, 1, 1);
    svrConfigLayout->addWidget(editSvrMainTaskTimeout, 2, 1, 1, 1);

    /* 3. 提交按钮 */
    btnSetServerConfig = new QPushButton("Set", this);
    connect(btnSetServerConfig, &QPushButton::clicked, this, &SpiderManager::onSetServerConfig);
    svrConfigLayout->addWidget(btnSetServerConfig, 3, 0, 1, 2);

    svrConfigGroupBox = new QGroupBox("Server", this);
    svrConfigGroupBox->setLayout(svrConfigLayout);
    svrConfigGroupBox->setStyleSheet(GROUPBOX_QSS);
    svrConfigGroupBox->setEnabled(false);
}

/* 4. 任务范围和进度 */
void SpiderManager::createTaskRangeGroupBox() {
    QGridLayout* taskRangeLayout = new QGridLayout();

    /* 0. 任务起始mid */
    editMidStart = new QLineEdit(this);
    editMidStart->setMaximumWidth(80);
    editMidStart->setAlignment(Qt::AlignRight);
    taskRangeLayout->addWidget(editMidStart, 0, 0, 1, 1);

    /* 1. 横杠 */
    QLabel* label410 = new QLabel("-", this);
    label410->setFixedWidth(10);
    taskRangeLayout->addWidget(label410, 0, 1, 1, 1);

    /* 2. 任务结束mid */
    editMidEnd = new QLineEdit(this);
    editMidEnd->setMaximumWidth(80);
    editMidEnd->setAlignment(Qt::AlignRight);
    taskRangeLayout->addWidget(editMidEnd, 0, 2, 1, 1);

    /* 3. 任务当前mid */
    editMidCurrent = new QLineEdit(this);
    editMidCurrent->setMaximumWidth(80);
    editMidCurrent->setAlignment(Qt::AlignRight);
    taskRangeLayout->addWidget(editMidCurrent, 1, 0, 1, 1);

    /* 3. 任务完成进度 */
    editMidProgress = new QLineEdit(this);
    editMidProgress->setMaximumWidth(80);
    editMidProgress->setAlignment(Qt::AlignRight);
    editMidProgress->setEnabled(false);
    taskRangeLayout->addWidget(editMidProgress, 1, 2, 1, 1);

    /* 4. 提交按钮 */
    btnSetTaskRange = new QPushButton("Set", this);
    taskRangeLayout->addWidget(btnSetTaskRange, 2, 0, 1, 3);

    taskRangeGroupBox = new QGroupBox("Task", this);
    taskRangeGroupBox->setLayout(taskRangeLayout);
    taskRangeGroupBox->setStyleSheet(GROUPBOX_QSS);
    taskRangeGroupBox->setEnabled(false);
}


/* 表格形式展示爬虫客户端信息 */
void SpiderManager::createClientsInfoTableWidget() {
    clientsTable = new QTableWidget(this);
    clientsTable->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:#DAD6D6;}");
    clientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); /* 禁止编辑 */
    clientsTable->verticalHeader()->setHidden(true); /* 隐藏左侧默认序号列 */
    clientsTable->setAlternatingRowColors(true); /* 隔行变色 */
    clientsTable->setSelectionMode(QAbstractItemView::SingleSelection); /* 只能单选 */
    clientsTable->setContextMenuPolicy(Qt::CustomContextMenu); /* 自定义右键菜单 */
    connect(clientsTable, &QTableWidget::customContextMenuRequested,
        this, &SpiderManager::onCustomContextMenuRequestedOnClientsTable);
    clientsTable->setColumnCount(11);
    /* 表头 */
    QStringList header;
    header << "ID" << "Host" << "OS" << "RunTime" << "Threads" << "Count"
        << "Speed_3m" << "Speed_5m" << "Speed_10m" << "Speed_30m" << "Speed";
    clientsTable->setHorizontalHeaderLabels(header);
    connect(clientsTable->horizontalHeader(), &QHeaderView::sectionClicked,
        this, &SpiderManager::onHorizontalHeaderClicked);
    connect(clientsTable->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
        this, &SpiderManager::onHorizontalHeaderDoubleClicked);
    clientsTable->setColumnWidth(0,  60);
    clientsTable->setColumnWidth(1,  120);
    clientsTable->setColumnWidth(2,  80);
    clientsTable->setColumnWidth(3,  100);
    clientsTable->setColumnWidth(4,  60);
    clientsTable->setColumnWidth(5,  80);
    clientsTable->setColumnWidth(6,  80);
    clientsTable->setColumnWidth(7,  80);
    clientsTable->setColumnWidth(8,  80);
    clientsTable->setColumnWidth(9,  80);
    clientsTable->setColumnWidth(10, 80);
}

/** 布局 **/
void SpiderManager::setupLayout() {
    QWidget* widget = new QWidget(this);
    this->setCentralWidget(widget);

    /* 0. 服务器相关信息 */
    createServerGroupBox();

    /* 1. 各种配置 */
    createClientConfigGroupBox();
    createDatabaseConfigGroupBox();
    createProxyConfigGroupBox();
    createServerConfigGroupBox();
    createTaskRangeGroupBox();
    QHBoxLayout* configLayout = new QHBoxLayout();
    configLayout->addWidget(clientConfigGroupBox);
    configLayout->addWidget(dbConfigGroupBox);
    configLayout->addWidget(proxyConfigGroupBox);
    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addWidget(svrConfigGroupBox);
    vLayout->addWidget(taskRangeGroupBox);
    configLayout->addLayout(vLayout);
    configLayout->addStretch();
    
    /* 2. 客户端爬虫信息表格 */
    createClientsInfoTableWidget();

    /* 3. 日志 */
    logTextEdit = new QTextEdit(this);
    logTextEdit->setMinimumHeight(120);
    logTextEdit->document()->setMaximumBlockCount(1000);  /* 最多显示1000行 */
    /* 绑定spdlog日志输出 */
    Logger::GetInstance().setQTextEditTarget(&logTextEdit);

    /* 主布局 */
    QVBoxLayout* mainLayout = new QVBoxLayout(widget);
    mainLayout->addWidget(serverGroupBox, 1);
    mainLayout->addLayout(configLayout, 3);
    mainLayout->addWidget(clientsTable, 10);
    mainLayout->addWidget(logTextEdit, 2);
}

