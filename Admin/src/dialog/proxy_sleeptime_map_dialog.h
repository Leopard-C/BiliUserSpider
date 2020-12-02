/**************************************************************
** class name:  ProxySleepTimeMapDialog
**
** description: 对话框，用于修改代理IP数量对应的线程暂停时间
**
** author: Leopard-C
**
** update: 2020/12/02
***************************************************************/
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <map>


class ProxySleepTimeMapDialog : public QDialog {
    Q_OBJECT
public:
    ProxySleepTimeMapDialog(QWidget* parent = nullptr);

    std::map<int, int> getMap() const;

    void setMap(const std::map<int, int>& m);

private:
    void setupLayout();

private:
    QLineEdit* editProxiesCount0;
    QLineEdit* editSleepTime0;

    QLineEdit* editProxiesCount1;
    QLineEdit* editSleepTime1;

    QLineEdit* editProxiesCount2;
    QLineEdit* editSleepTime2;
};

