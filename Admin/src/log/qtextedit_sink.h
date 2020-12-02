/************************************************
**
**  功能: 输出日志到QTextEdit控件中
**         < warn  黑色
**         = warn  黄色
**         > warn  红色
**
**  author: Leopard-C
**
**  update: 2020-11-30
**
************************************************/
#pragma once
#include <QTextEdit>
#include "spdlog/sinks/base_sink.h"
#include "spdlog/formatter.h"

namespace spdlog {
namespace sinks {

template<typename Mutex>
class qtextedit_sink : public base_sink<Mutex> {
public:
    qtextedit_sink(QTextEdit*** textedit)
        : target(textedit) {}

    void sink_it_(const details::log_msg& msg) override {
        if (target && *target && **target) {
            (**target)->moveCursor(QTextCursor::End);
            if (msg.level < level::warn) {
                (**target)->setTextColor(Qt::black);
            }
            else if (msg.level == level::warn) {
                (**target)->setTextColor(Qt::yellow);
            }
            else {
                (**target)->setTextColor(Qt::red);
            }
            spdlog::memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);
            fmt::to_string(formatted);
            QTextCursor tc = (**target)->textCursor();
            tc.insertText(QString::fromStdString(fmt::to_string(formatted)));
            (**target)->setTextCursor(tc);
        }
    }

    void flush_() override {
        if (target && *target && **target) {
            (**target)->update();
        }
    }

private:
    QTextEdit*** target = nullptr;
};

#include "spdlog/details/null_mutex.h"
#include <mutex>

using qtextedit_sink_mt = qtextedit_sink<std::mutex>;
using qtextedit_sink_st = qtextedit_sink<spdlog::details::null_mutex>;

}
}
