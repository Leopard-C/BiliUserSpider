/*******************************************************
** class name:  SpiderManager
**
** description: 程序主界面
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once

#include <QtWidgets/QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QRadioButton>
#include <QMenu>
#include <QAction>
#include <QTimer>

#include <map>
#include <mutex>

#include "client/spider_client_info.h"
#include "database/mysql_instance.h"
#include "config/config.h"
#include "dialog/proxy_sleeptime_map_dialog.h"


class SpiderManager : public QMainWindow
{
    Q_OBJECT

public:
    SpiderManager(QWidget *parent = Q_NULLPTR);
    ~SpiderManager();

signals:
    void sigUpdateClientsInfo();
    void sigUpdateProxyPoolSize();
    void sigUpdateProgress();

public slots:
    /* 连接/启动/关闭服务器 */
    void onConnect();
    void onDisconnect();
    void onStart();
    void onStop();
    void onShutdown();
    void onSetPassword();

    /* 修改各种配置 */
    bool getAllConfig();
    void onSetClientConfig();
    void onSetDbConfig();
    void onSetProxyConfig();
    void onSetServerConfig();

    /* 清空数据库中所有代理IP */
    void onEmptyPorxyPool();

    /* 更新客户端信息 */
    void onUpdateClientsInfo();

    /* 表格某一列表头被点击 */
    void onHorizontalHeaderClicked(int col);
    void onHorizontalHeaderDoubleClicked(int col);

    /* 更新代理IP池大小 */
    void onUpdateProxyPoolSize();

    /* 更新任务进度 */
    void onUpdateProgress();

    /* 通知客户端平滑退出 */
    void onQuitClient();

    /* 通知客户端强制退出 */
    void onForceQuitClient();

    /* 修改Task Range */
    void onSetTaskRange();

    /* 右键菜单 */
    void onCustomContextMenuRequestedOnClientsTable(const QPoint& pos);

    /* Timer */
    void onTimerForProxyPoolSizeTimeout();
    void onTimerForTaskProgressTimeout();

public slots:
    /* 连接服务器成功 */
    void onServerConnected();

    /* 服务器断开连接 */
    void onServerDisconnected();

    /* 服务器启动成功 (逻辑) */
    void onServerStarted();

    /* 服务器停止运行 (逻辑) */
    void onServerStopped();

    /* 服务器关闭 (程序) */
    void onServerShutdown();

public:
    static void backgroundThread(SpiderManager* sm);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void showClientsInfo(bool isNew);
    void updateAllConfig(const ClientConfig& clientConfig, const DatabaseConfig& dbConfig,
        const ProxyConfig& proxyConfig, const ServerConfig& svrConfig);
    void setWidgetBgColor(QWidget* widget, const QColor& color);

private:
    /* 布局 */
    void setupLayout();
    void createServerGroupBox();
    void createClientConfigGroupBox();
    void createDatabaseConfigGroupBox();
    void createProxyConfigGroupBox();
    void createServerConfigGroupBox();
    void createClientsInfoTableWidget();
    void createTaskRangeGroupBox();

private:
    /* 记录上一次的速度，用于对比 */
    /* <clientId, speed> */
    /* 潜在的BUG：即使客户端退出，记录也不会删除，但是客户端基本上是稳定的，问题不大 */
    std::map<int, double> speeds;
    std::map<int, double> speeds_3m;
    std::map<int, double> speeds_5m;
    std::map<int, double> speeds_10m;
    std::map<int, double> speeds_30m;

    /* 是否通过颜色显示爬虫速度的变化 */
    bool enableColorSpeeds = true;
    bool enableColorSpeeds_3m = false;
    bool enableColorSpeeds_5m = false;
    bool enableColorSpeeds_10m = false;
    bool enableColorSpeeds_30m = false;

    /* 服务器控制 */
    QPushButton* btnSetSvrPwd;
    QLineEdit* editServerAddr;
    QPushButton* btnConnect;
    QPushButton* btnDisconnect;
    QPushButton* btnStart;
    QPushButton* btnStop;
    QPushButton* btnShutdown;
    QGroupBox* serverGroupBox;

    /* 0. 客户端配置信息 */
    QRadioButton* btnClientBiliApiType_Web;
    QRadioButton* btnClientBiliApiType_App;
    QRadioButton* btnClientMainTaskTimes_1;
    QRadioButton* btnClientMainTaskTimes_2;
    QRadioButton* btnClientMainTaskTimes_3;
    QLineEdit* editClientSubTaskStep;
    QLineEdit* editClientTimeout;
    QLineEdit* editClientProxyMaxError;
    QLineEdit* editClientMidMaxError;
    QLineEdit* editClientIntervalProxyPoolSync;
    QPushButton* btnSetProxyNumSleeptimeMap;
    QPushButton* btnSetClientConfig;
    QGroupBox* clientConfigGroupBox;

    /* 1. 数据库配置信息 */
    QLineEdit* editDbHost;
    QLineEdit* editDbPort;
    QLineEdit* editDbUser;
    QLineEdit* editDbPwd;
    QLineEdit* editDbName;
    QPushButton* btnSetDbConfig;
    QGroupBox* dbConfigGroupBox;

    /* 2. 代理IP配置信息 */
    QLineEdit* editProxyOrderCode;
    QLineEdit* editProxyUserId;
    QLineEdit* editProxyIntervalQuery;
    QLineEdit* editProxyPoolSize;
    QPushButton* btnEmptyProxyPool;
    QPushButton* btnSetProxyConfig;
    QGroupBox* proxyConfigGroupBox;
    QTimer* timerForProxyPoolSize;

    /* 3. 服务器配置信息 */
    QLineEdit* editSvrMaxLiveHeartbeat;
    QLineEdit* editSvrIntervalDetectClientLive;
    QLineEdit* editSvrMainTaskTimeout;
    QPushButton* btnSetServerConfig;
    QGroupBox* svrConfigGroupBox;

    /* 4. 任务范围和进度 */
    QLineEdit* editMidStart;
    QLineEdit* editMidEnd;
    QLineEdit* editMidCurrent;
    QLineEdit* editMidProgress;
    QPushButton* btnSetTaskRange;
    QGroupBox* taskRangeGroupBox;
    QTimer* timerForTaskProgress;

    /* 客户端信息 */
    QTableWidget* clientsTable;
    std::vector<SpiderClientInfo> clientsInfo;
    int sortCol = 0;
    bool isASC = true;
    
    /* 日志窗口 */
    QTextEdit* logTextEdit;
    int logId = 0;

    /* 右键菜单 */
    QMenu* menuRightClick;
    int currentClientId = -1;  /* 当前右键选中的客户端ID */

    /* 右键菜单，加锁，暂停更新客户端信息 */
    std::mutex mutexForClientsInfo;

    /* 数据库连接池 */
    MysqlDbPool* mysqlDbPool = nullptr;
    std::mutex mutexForMysqlDbPool;

    /* 后台线程是否退出 */
    bool isBgThreadQuit = true;

    /* 点击了关闭按钮 */
    bool shouldClose = false;

    ProxySleepTimeMapDialog* pstmDialog;
};

