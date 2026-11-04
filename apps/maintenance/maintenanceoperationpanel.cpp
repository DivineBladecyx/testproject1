#include "maintenanceoperationpanel.h"
#include "telemetryvalue.h"
#include "roomcapabilities.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>

#include <cmath>

namespace {
QString deviceTitle(const QString &device)
{
    if (device == "couch") return QStringLiteral("Couch 治疗床");
    if (device == "dr") return QStringLiteral("DR");
    return QStringLiteral("Ridge Filter 脊形过滤器");
}

void setTone(QWidget *widget, const char *tone)
{
    if (widget->property("engTone").toString() == QLatin1String(tone)) return;
    widget->setProperty("engTone", tone);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

template<typename T>
T *requiredChild(QWidget *parent, const QString &name)
{
    auto *child = parent->findChild<T *>(name);
    if (!child) qFatal("Missing Designer widget: %s", qPrintable(name));
    return child;
}

void configureTable(QTableWidget *table)
{
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setDefaultSectionSize(36);
}

QString valueText(const QJsonValue &value)
{
    if (value.isBool()) return value.toBool() ? QStringLiteral("是") : QStringLiteral("否");
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 10);
    if (value.isString()) return value.toString();
    return QStringLiteral("未知");
}

QJsonObject firstObject(const QJsonObject &object, const QStringList &names)
{
    for (const QString &name : names) {
        if (object.value(name).isObject()) return object.value(name).toObject();
    }
    return {};
}

} // namespace

MaintenanceOperationPanel::MaintenanceOperationPanel(QWidget *parent)
    : QWidget(parent)
{
}

void MaintenanceOperationPanel::initialize(const QString &roomId)
{
    // uic creates the promoted container before its children; bind after setupUi.
    if (m_refreshTimer) return;
    m_roomId = roomId;
    m_deviceTabs = requiredChild<QTabWidget>(this, QStringLiteral("deviceTabs"));
    m_deviceTabs->setTabVisible(1, RoomCapabilities::hasDrLift(m_roomId));
    m_cabinet = requiredChild<QSpinBox>(this, QStringLiteral("cabinetSpinBox"));
    for (int axis = 1; axis <= 3; ++axis)
        m_axes.append(requiredChild<QCheckBox>(this, QStringLiteral("axis%1CheckBox").arg(axis)));
    for (const QString &axis : {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"),
                               QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}) {
        m_absolute.insert(axis, requiredChild<QDoubleSpinBox>(this, QStringLiteral("absolute_") + axis));
        m_offset.insert(QStringLiteral("d") + axis,
                        requiredChild<QDoubleSpinBox>(this, QStringLiteral("offset_") + axis));
    }
    bindRpcButton("moveCouchPoint", [this] {
        QJsonObject params{{"room_id", m_roomId}};
        for (auto it = m_absolute.constBegin(); it != m_absolute.constEnd(); ++it) params.insert(it.key(), it.value()->value());
        return params;
    });
    bindRpcButton("moveCouchOffset", [this] {
        QJsonObject params{{"room_id", m_roomId}};
        for (auto it = m_offset.constBegin(); it != m_offset.constEnd(); ++it) params.insert(it.key(), it.value()->value());
        return params;
    });
    const auto room = [this] { return QJsonObject{{"room_id", m_roomId}}; };
    for (const QString &method : {QStringLiteral("moveCouchBedPos"), QStringLiteral("moveCouchPlacePos"),
                                  QStringLiteral("resetCouchAlarm"), QStringLiteral("moveDrToZero"),
                                  QStringLiteral("moveDrToWork"), QStringLiteral("resetDrAlarm")})
        bindRpcButton(method, room);
    const auto axes = [this] { return filterParams(); };
    for (const QString &method : {QStringLiteral("executeInsert"), QStringLiteral("executeExit"),
                                  QStringLiteral("motorEnable"), QStringLiteral("stop"), QStringLiteral("resetFault")})
        bindRpcButton(method, axes);
    requiredChild<QPushButton>(this, QStringLiteral("motorDisableButton"))->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    for (const QString &name : {QStringLiteral("jogPlusButton"), QStringLiteral("jogMinusButton")})
        requiredChild<QPushButton>(this, name)->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

    for (const QString &device : {QStringLiteral("couch"), QStringLiteral("dr"), QStringLiteral("filter")}) {
        bindStatus(device);
        auto *split = requiredChild<QSplitter>(this, device + QStringLiteral("Splitter"));
        split->setSizes({520, 780});
        split->setStretchFactor(0, 2);
        split->setStretchFactor(1, 3);
    }
    for (auto *surface : findChildren<QWidget *>()) {
        if (!surface->property("engRole").toString().isEmpty())
            surface->setAttribute(Qt::WA_StyledBackground);
    }
    connect(m_deviceTabs, &QTabWidget::currentChanged, this, &MaintenanceOperationPanel::selectDevice);
    connect(m_cabinet, &QSpinBox::valueChanged, this, [this] { refreshStatus(); });
    auto *freshness = new QTimer(this);
    freshness->setInterval(1000);
    connect(freshness, &QTimer::timeout, this, &MaintenanceOperationPanel::refreshStatus);
    freshness->start();
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(250);
    connect(m_refreshTimer, &QTimer::timeout, this, &MaintenanceOperationPanel::refreshStatus);
    selectDevice(m_deviceTabs->currentIndex());
}

void MaintenanceOperationPanel::bindRpcButton(const QString &method, const std::function<QJsonObject()> &params)
{
    auto *button = requiredChild<QPushButton>(this, method + QStringLiteral("Button"));
    button->setProperty("engRpcKey", QStringLiteral("device/") + method);
    button->setIcon(style()->standardIcon(method == "stop" ? QStyle::SP_MediaStop
        : method.contains("reset", Qt::CaseInsensitive) ? QStyle::SP_DialogResetButton : QStyle::SP_MediaPlay));
    button->setEnabled(false);
    m_buttons.append(button);
    connect(button, &QPushButton::clicked, this, [this, method, params] { request(method, params()); });
}

QJsonObject MaintenanceOperationPanel::filterParams() const
{
    QJsonObject params{{"cabinet_id", m_cabinet->value()}};
    for (int i = 0; i < m_axes.size(); ++i) params.insert(QStringLiteral("axis%1").arg(i + 1), m_axes.at(i)->isChecked());
    return params;
}

void MaintenanceOperationPanel::request(const QString &method, const QJsonObject &params)
{
    if (!RoomCapabilities::supportsRpc(m_roomId, QStringLiteral("device"), method)) return;
    if (params.contains("axis1") && !params.value("axis1").toBool() && !params.value("axis2").toBool()
        && !params.value("axis3").toBool()) {
        QMessageBox::warning(this, tr("未选择电机轴"), tr("请选择需要操作的轴。"));
        return;
    }
    emit rpcRequested({{"cmd", "ENG_INVOKE_RPC"}, {"target", "device"}, {"method", method}, {"params", params}});
}

void MaintenanceOperationPanel::addField(const QString &device, const QString &key,
                                        const QStringList &aliases, bool faultWhenTrue, bool positiveWhenTrue)
{
    auto *value = requiredChild<QLabel>(this, device + QLatin1Char('_') + key + QStringLiteral("_Value"));
    value->setToolTip(aliases.isEmpty() ? tr("当前协议未提供已确认的字段映射。") : aliases.join(QStringLiteral(" / ")));
    m_fields.insert(device + QLatin1Char('.') + key, {device, aliases, QJsonValue(), QDateTime(), value, faultWhenTrue, positiveWhenTrue});
}

void MaintenanceOperationPanel::bindStatus(const QString &device)
{
    if (device == "couch") {
        addField(device, "connected", {"connected"}, false, true);
        addField(device, "ready", {"system_ready", "systemReady"}, false, true);
        addField(device, "remote", {"remote_mode", "remoteMode"});
        addField(device, "estop", {"estop"}, true);
        addField(device, "bedState", {});
        addField(device, "positionNumber", {});
        m_couchPosition = requiredChild<QTableWidget>(this, QStringLiteral("couchPositionTable"));
        m_couchAxes = requiredChild<QTableWidget>(this, QStringLiteral("couchAxesTable"));
        configureTable(m_couchPosition);
        configureTable(m_couchAxes);
        for (const QString &axis : {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"), QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")})
            m_fields.insert(device + QLatin1Char('.') + axis, {device, {axis}, QJsonValue(), QDateTime(), nullptr});
    } else if (device == "dr") {
        addField(device, "connected", {"connected"}, false, true);
        addField(device, "systemError", {"system_error", "systemError"}, true);
        addField(device, "systemCode", {});
        addField(device, "axisError", {"axis_error", "axisError"}, true);
        addField(device, "axisCode", {});
        addField(device, "moving", {"axis_moving", "axisMoving"});
        addField(device, "zero", {"axis_at_zero_position", "axisAtZeroPosition"});
        addField(device, "work", {"axis_at_work_position", "axisAtWorkPosition"});
        addField(device, "position", {});
        addField(device, "angle", {});
        addField(device, "errorMessage", {"system_error_message_zh"});
        addField(device, "advice", {});
    } else {
        addField(device, "connected", {"connected"}, false, true);
        addField(device, "faultWord", {"motor_fault_word_40134", "motorFaultWord40134"});
        addField(device, "stopWord", {});
        m_filterAxes = requiredChild<QTableWidget>(this, QStringLiteral("filterAxesTable"));
        configureTable(m_filterAxes);
        for (int cabinet = 1; cabinet <= 3; ++cabinet) {
            for (int axis = 1; axis <= 3; ++axis) {
                const QString key = QStringLiteral("cabinet%1_axis%2_in_position").arg(cabinet).arg(axis);
                const QString camel = QStringLiteral("cabinet%1Axis%2InPosition").arg(cabinet).arg(axis);
                m_fields.insert(device + QLatin1Char('.') + key, {device, {key, camel}, QJsonValue(), QDateTime(), nullptr});
            }
        }
    }
}

void MaintenanceOperationPanel::setEnabledMethods(const QSet<QString> &methods)
{
    for (auto *button : m_buttons) {
        const QString key = button->property("engRpcKey").toString();
        button->setEnabled(methods.contains(key)
            && RoomCapabilities::supportsRpc(m_roomId, key.section('/', 0, 0), key.section('/', 1)));
    }
}

void MaintenanceOperationPanel::updateTelemetry(const QJsonObject &payload)
{
    const QString room = payload.value("room_id").toString();
    if (!room.isEmpty() && room != m_roomId) return;
    QJsonObject status = firstObject(payload, {"status_update", "statusUpdate", "status", "device_status", "deviceStatus"});
    if (status.isEmpty()) status = payload;
    const QString statusRoom = status.value("room_id").toString();
    if (!statusRoom.isEmpty() && statusRoom != m_roomId) return;
    const QMap<QString, QJsonObject> devices = {
        {"couch", firstObject(status, {"couch", "couch_status"})},
        {"dr", firstObject(status, {"dr", "dr_state"})},
        {"filter", firstObject(status, {"ridge_filter_stand", "ridgeFilterStand", "filter_state"})}
    };
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (auto device = devices.constBegin(); device != devices.constEnd(); ++device) {
        if (device.key() == QStringLiteral("dr") && !RoomCapabilities::hasDrLift(m_roomId)) continue;
        if (device.value().isEmpty()) continue;
        m_deviceTimes.insert(device.key(), now);
        const QJsonValue connection = TelemetryValue::binaryFlag(device.value().value("connected"));
        m_connected.insert(device.key(), connection);
        for (auto it = m_fields.begin(); it != m_fields.end(); ++it) {
            Field &field = it.value();
            if (field.device != device.key()) continue;
            if (field.aliases.contains("connected")) {
                field.value = connection;
                field.received = connection.isBool() ? now : QDateTime();
                continue;
            }
            const QSet<QString> booleanFields = {"connected", "ready", "remote", "estop", "systemError", "axisError", "moving", "zero", "work"};
            const bool boolean = booleanFields.contains(it.key().section('.', 1)) || it.key().endsWith("_in_position");
            const bool text = it.key().endsWith(".errorMessage");
            for (const QString &alias : field.aliases) {
                if (!device.value().contains(alias)) continue;
                const QJsonValue value = device.value().value(alias);
                if ((boolean && value.isBool()) || (text && value.isString())
                    || (!boolean && !text && value.isDouble() && std::isfinite(value.toDouble()))) {
                    field.value = value;
                    field.received = now;
                } else {
                    field.value = QJsonValue();
                    field.received = QDateTime();
                }
                break;
            }
        }
    }
    if (!m_refreshTimer->isActive()) m_refreshTimer->start();
}

void MaintenanceOperationPanel::invalidateStatus()
{
    m_deviceTimes.clear();
    m_refreshTimer->stop();
    m_connected.clear();
    for (auto &field : m_fields) { field.value = QJsonValue(); field.received = QDateTime(); }
    refreshStatus();
}

void MaintenanceOperationPanel::refreshStatus()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const auto fieldText = [this, now](const Field &field) {
        if (field.aliases.isEmpty()) return tr("待接入");
        if (!field.received.isValid()) return tr("未知");
        if (field.received.msecsTo(now) > 10000) return tr("已过期");
        if (!field.aliases.contains("connected") && m_connected.value(field.device).isBool()
            && !m_connected.value(field.device).toBool()) return tr("离线 / 未知");
        return valueText(field.value);
    };
    for (auto it = m_fields.begin(); it != m_fields.end(); ++it) {
        const Field &field = it.value();
        if (!field.label) continue;
        field.label->setText(fieldText(field));
        const bool fresh = field.received.isValid() && field.received.msecsTo(now) <= 10000;
        const bool offline = !field.aliases.contains("connected") && m_connected.value(field.device).isBool()
            && !m_connected.value(field.device).toBool();
        const char *tone = "unknown";
        if (fresh && !offline && field.value.isBool()) {
            if (it.key().endsWith(".connected")) tone = field.value.toBool() ? "good" : "fault";
            else if (field.faultWhenTrue) tone = field.value.toBool() ? "fault" : "good";
            else if (field.positiveWhenTrue) tone = field.value.toBool() ? "good" : "warning";
        }
        if (fresh && !offline && field.device == "filter" && it.key().endsWith("faultWord") && field.value.isDouble())
            tone = field.value.toDouble() != 0 ? "fault" : "unknown";
        setTone(field.label, tone);
    }
    const QStringList coordinates = {"x", "y", "z", "a", "b", "c"};
    for (int col = 0; col < coordinates.size(); ++col)
        m_couchPosition->item(0, col)->setText(fieldText(m_fields.value(QStringLiteral("couch.") + coordinates.at(col))));
    for (int axis = 1; axis <= 3; ++axis) {
        const QString key = QStringLiteral("filter.cabinet%1_axis%2_in_position").arg(m_cabinet->value()).arg(axis);
        m_filterAxes->item(4, axis - 1)->setText(m_fields.contains(key) ? fieldText(m_fields.value(key)) : tr("待接入"));
    }
    const QDateTime time = m_deviceTimes.value(m_device);
    if (!time.isValid()) m_age->setText(tr("尚未收到数据"));
    else {
        const qint64 seconds = qMax<qint64>(0, time.secsTo(now));
        m_age->setText(seconds > 10 ? tr("数据已过期 · %1 秒前").arg(seconds) : tr("%1 秒前更新").arg(seconds));
    }
}

void MaintenanceOperationPanel::selectDevice(int index)
{
    const QStringList devices = {"couch", "dr", "filter"};
    if (index < 0 || index >= devices.size()) return;
    m_device = devices.at(index);
    m_deviceTabs->setCurrentIndex(index);
    m_controlTitle = requiredChild<QLabel>(this, m_device + QStringLiteral("ControlTitle"));
    m_statusTitle = requiredChild<QLabel>(this, m_device + QStringLiteral("StatusTitle"));
    m_age = requiredChild<QLabel>(this, m_device + QStringLiteral("Age"));
    m_controlTitle->setText(deviceTitle(m_device) + tr(" 控制"));
    m_statusTitle->setText(deviceTitle(m_device) + tr(" 状态"));
    refreshStatus();
}

