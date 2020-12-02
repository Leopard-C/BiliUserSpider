#include "proxy_sleeptime_map_dialog.h"
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

ProxySleepTimeMapDialog::ProxySleepTimeMapDialog(QWidget* parent /*= nullptr*/)
    : QDialog(parent)
{
    setupLayout();
}


std::map<int, int> ProxySleepTimeMapDialog::getMap() const {
    std::map<int, int> result;
    if (!editProxiesCount0->text().isEmpty() && !editSleepTime0->text().isEmpty()) {
        result.emplace(editProxiesCount0->text().toInt(), editSleepTime0->text().toInt());
    }
    if (!editProxiesCount1->text().isEmpty() && !editSleepTime1->text().isEmpty()) {
        result.emplace(editProxiesCount1->text().toInt(), editSleepTime1->text().toInt());
    }
    if (!editProxiesCount2->text().isEmpty() && !editSleepTime2->text().isEmpty()) {
        result.emplace(editProxiesCount2->text().toInt(), editSleepTime2->text().toInt());
    }
    return result;
}

void ProxySleepTimeMapDialog::setMap(const std::map<int, int>& m) {
    auto it = m.begin();
    if (it == m.end()) {
        return;
    }
    editProxiesCount0->setText(QString::number(it->first));
    editSleepTime0->setText(QString::number(it->second));

    ++it;
    if (it == m.end()) {
        return;
    }
    editProxiesCount1->setText(QString::number(it->first));
    editSleepTime1->setText(QString::number(it->second));

    ++it;
    if (it == m.end()) {
        return;
    }
    editProxiesCount2->setText(QString::number(it->first));
    editSleepTime2->setText(QString::number(it->second));
}

void ProxySleepTimeMapDialog::setupLayout() {
    /* Group0 */
    QHBoxLayout* hLayout0 = new QHBoxLayout();
    QLabel* label00 = new QLabel("If proxies number < ", this);
    label00->setAlignment(Qt::AlignRight);
    editProxiesCount0 = new QLineEdit(this);
    editProxiesCount0->setMaximumWidth(50);
    QLabel* label01 = new QLabel("then sleep (ms)", this);
    label01->setAlignment(Qt::AlignRight);
    editSleepTime0 = new QLineEdit(this);
    editSleepTime0->setMaximumWidth(50);
    hLayout0->addWidget(label00);
    hLayout0->addWidget(editProxiesCount0);
    hLayout0->addWidget(label01);
    hLayout0->addWidget(editSleepTime0);

    /* Group1 */
    QHBoxLayout* hLayout1 = new QHBoxLayout();
    QLabel* label10 = new QLabel("If proxies number < ", this);
    label10->setAlignment(Qt::AlignRight);
    editProxiesCount1 = new QLineEdit(this);
    editProxiesCount1->setMaximumWidth(50);
    QLabel* label11 = new QLabel("then sleep (ms)", this);
    label11->setAlignment(Qt::AlignRight);
    editSleepTime1 = new QLineEdit(this);
    editSleepTime1->setMaximumWidth(50);
    hLayout1->addWidget(label10);
    hLayout1->addWidget(editProxiesCount1);
    hLayout1->addWidget(label11);
    hLayout1->addWidget(editSleepTime1);

    /* Group2 */
    QHBoxLayout* hLayout2 = new QHBoxLayout();
    QLabel* label20 = new QLabel("If proxies number < ", this);
    label20->setAlignment(Qt::AlignRight);
    editProxiesCount2 = new QLineEdit(this);
    editProxiesCount2->setMaximumWidth(50);
    QLabel* label21 = new QLabel("then sleep (ms)", this);
    label21->setAlignment(Qt::AlignRight);
    editSleepTime2 = new QLineEdit(this);
    editSleepTime2->setMaximumWidth(50);
    hLayout2->addWidget(label20);
    hLayout2->addWidget(editProxiesCount2);
    hLayout2->addWidget(label21);
    hLayout2->addWidget(editSleepTime2);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(hLayout0);
    mainLayout->addLayout(hLayout1);
    mainLayout->addLayout(hLayout2);
}

