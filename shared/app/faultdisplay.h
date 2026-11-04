#pragma once
#include "websocketclient.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMap>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>
#include <utility>
#include <QStringList>

// Read-only FaultCenter display; no acknowledgement or hardware commands.
class FaultDisplay : public QWidget
{
public:
    FaultDisplay(WebSocketClient *client, std::function<QString()> room, QWidget *parent)
        : QWidget(parent), client_(client), room_(std::move(room))
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 4, 8, 4);
        button_ = new QPushButton(tr("故障详情"), this);
        summary_ = new QLabel(tr("故障信息待同步"), this);
        summary_->setTextFormat(Qt::PlainText);
        summary_->setWordWrap(true);
        layout->addWidget(button_);
        layout->addWidget(summary_, 1);
        connect(button_, &QPushButton::clicked, this, [this] { open(); });
        if (client) {
            connect(client, &WebSocketClient::dataReceived, this, [this](const QJsonObject &m) { receive(m); });
            connect(client, &WebSocketClient::disconnected, this, [this] { synced_ = false; render(); });
            connect(client, &WebSocketClient::connected, this, [this] { synced_ = false; render(); query(); });
        }
        QTimer::singleShot(0, this, [this] { query(); });
    }
private:
    static QString roomId(const QJsonValue &v) {
        QString s = v.isString() ? v.toString().trimmed().toUpper() : QString();
        if (v.isDouble() && v.toInt(-1) >= 0 && v.toInt(-1) == v.toDouble()) s = QString::number(v.toInt());
        if (s.startsWith("ROOM_")) s.remove(0, 5);
        bool ok = false; const int n = s.toInt(&ok);
        return ok && n >= 0 ? QStringLiteral("ROOM_%1").arg(n) : QString();
    }
    bool matches(const QJsonObject &r) const {
        if (!r.contains("room_id")) return true;
        const auto v = r.value("room_id");
        if (v.isString() && v.toString().trimmed().isEmpty()) return true;
        return !roomId(v).isEmpty() && roomId(v) == roomId(QJsonValue(room_()));
    }
    static QString id(const QJsonObject &r) {
        const auto v = r.value("fault_id");
        const qint64 n = v.toInteger(-1);
        return v.isDouble() && n >= 0 && double(n) == v.toDouble() ? QString::number(n) : QString();
    }
    void query() {
        if (client_ && client_->isConnected()) client_->sendData(QJsonObject{{"cmd", "QUERY_FAULTS"}});
    }
    void receive(const QJsonObject &m) {
        if (m.value("type") != "TASK" || !m.value("payload").isObject() || !matches(m)) return;
        const auto p = m.value("payload").toObject();
        if (!matches(p)) return;
        const QString cmd = m.value("cmd").toString();
        if (cmd == "TASK_FAULT_SNAPSHOT") {
            if (!p.value("active").isArray() || !p.value("history").isArray()) return;
            QMap<QString, QJsonObject> next;
            for (const auto &v : p.value("active").toArray()) {
                const auto r = v.toObject();
                if (id(r).isEmpty()) return;
                if (matches(r)) next.insert(id(r), r);
            }
            active_ = next; history_ = QJsonArray();
            for (const auto &v : p.value("history").toArray())
                if (v.isObject() && matches(v.toObject())) history_.append(v);
            synced_ = true;
        } else if (cmd == "TASK_FAULT_RAISED") {
            if (id(p).isEmpty() || p.value("active") != true) return;
            active_.insert(id(p), p);
        } else if (cmd == "TASK_FAULT_CLEARED") {
            const QString key = id(p);
            if (key.isEmpty() || p.value("active") != false || !active_.contains(key)) return;
            auto r = active_.take(key);
            for (auto it = p.begin(); it != p.end(); ++it) r.insert(it.key(), it.value());
            history_.append(r);
        } else return;
        while (history_.size() > 200) history_.removeAt(0);
        render();
    }
    static QString describe(const QJsonObject &r) {
        const QList<QPair<QString, QString>> fields = {
                                                       {"code", tr("故障码")}, {"severity", tr("等级")}, {"title", tr("标题")},
                                                       {"source", tr("来源")}, {"stage", tr("阶段")}, {"detail", tr("详情")},
                                                       {"advice", tr("处置建议")}, {"recovery", tr("恢复动作建议")}, {"count", tr("次数")},
                                                       {"first_at", tr("首次时间")}, {"last_at", tr("最近时间")}, {"acknowledged", tr("已确认")},
                                                       {"cleared_at", tr("清除时间")}, {"clear_reason", tr("清除原因")}};
        QStringList lines;
        for (const auto &f : fields) {
            const auto v = r.value(f.first);
            if (v.isUndefined() || v.isNull()) continue;
            const QString s = v.isBool() ? (v.toBool() ? tr("是") : tr("否"))
                              : v.isDouble() ? QString::number(v.toDouble(), 'g', 15) : v.toString();
            lines.append(f.second + QStringLiteral("：") + s.left(4000));
        }
        return lines.join('\n');
    }
    void render() {
        QStringList lines; bool critical = false;
        for (const auto &r : active_) {
            lines.append(describe(r)); critical = critical || r.value("severity") == "CRITICAL";
        }
        button_->setText(tr("故障详情（%1）").arg(active_.size()));
        summary_->setText((synced_ ? QString() : tr("待同步，以下为已收到记录；"))
                          + (active_.isEmpty() ? (synced_ ? tr("当前无活动故障") : tr("故障状态未知"))
                                               : tr("活动故障 %1 项%2：%3").arg(active_.size()).arg(critical ? tr("，含严重故障") : QString(),
                                                                                                    active_.last().value("title").toString().left(160))));
        summary_->setStyleSheet(active_.isEmpty() ? QString() : critical
                                                                ? "color:#B42318;font-weight:bold;" : "color:#875A09;font-weight:bold;");
        if (text_) {
            QStringList history;
            for (const auto &r : history_) history.append(describe(r.toObject()));
            text_->setPlainText(summary_->text() + tr("\n\n活动故障\n") + lines.join("\n\n----------------\n\n")
                                + tr("\n\n历史记录\n") + history.join("\n\n----------------\n\n"));
        }
    }
    void open() {
        if (!dialog_) {
            auto *d = new QDialog(this);
            d->setWindowTitle(tr("故障信息")); d->setAttribute(Qt::WA_DeleteOnClose);
            auto *layout = new QVBoxLayout(d);
            text_ = new QPlainTextEdit(d); text_->setReadOnly(true); layout->addWidget(text_);
            auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, d);
            connect(buttons->addButton(tr("刷新"), QDialogButtonBox::ActionRole), &QPushButton::clicked,
                    this, [this] { query(); });
            connect(buttons, &QDialogButtonBox::rejected, d, &QDialog::close);
            layout->addWidget(buttons); d->resize(760, 620); dialog_ = d;
        }
        render(); dialog_->show(); dialog_->raise(); query();
    }
    QPointer<WebSocketClient> client_;
    std::function<QString()> room_;
    QPushButton *button_ = nullptr;
    QLabel *summary_ = nullptr;
    QPointer<QDialog> dialog_;
    QPointer<QPlainTextEdit> text_;
    QMap<QString, QJsonObject> active_;
    QJsonArray history_;
    bool synced_ = false;
};
