#include "maintenancewindow.h"
#include "ui_maintenancewindow.h"
#include "maintenanceoperationpanel.h"

#include "auditlogger.h"
#include "thememanager.h"
#include "websocketclient.h"
#include "roomcapabilities.h"

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVector>

#include <limits>

namespace {
struct MethodSpec {
    QString target;
    QString group;
    QString name;
    QString title;
    QStringList fields;
    bool readOnly = false;
};

const QVector<MethodSpec> &methodSpecs()
{
    static const QVector<MethodSpec> specs = [] {
        QVector<MethodSpec> result;
        const auto add = [&result](const char *target, const QString &group,
                                  const char *name, const QString &title,
                                  const QStringList &fields, bool readOnly = false) {
            result.append({QString::fromLatin1(target), group, QString::fromLatin1(name),
                           title, fields, readOnly});
        };
        const QStringList room = {QStringLiteral("room_id")};
        const QStringList toggle = {QStringLiteral("room_id"), QStringLiteral("state")};
        const QStringList axes = {QStringLiteral("cabinet_id"), QStringLiteral("axis1"),
                                  QStringLiteral("axis2"), QStringLiteral("axis3")};
        const QStringList treatment = {QStringLiteral("treatment_id")};
        const QString bed = QStringLiteral("治疗床与 DR");
        add("device", bed, "moveCouchPoint", QStringLiteral("治疗床绝对定位"),
            {"room_id", "x", "y", "z", "a", "b", "c"});
        add("device", bed, "moveCouchOffset", QStringLiteral("治疗床相对移动"),
            {"room_id", "dx", "dy", "dz", "da", "db", "dc"});
        add("device", bed, "moveCouchBedPos", QStringLiteral("移动到上下床位置"), room);
        add("device", bed, "moveCouchPlacePos", QStringLiteral("移动到治疗摆位位置"), room);
        add("device", bed, "resetCouchAlarm", QStringLiteral("治疗床警报复位"), room);
        add("device", bed, "moveDrToZero", QStringLiteral("DR 回零位"), room);
        add("device", bed, "moveDrToWork", QStringLiteral("DR 去工作位"), room);
        add("device", bed, "resetDrAlarm", QStringLiteral("DR 警报复位"), room);
        const QString lamps = QStringLiteral("指示灯与联锁");
        add("device", lamps, "setPresetStatusLamp", QStringLiteral("预置灯（黄）"), toggle);
        add("device", lamps, "setPrepareStatusLamp", QStringLiteral("准备灯（绿）"), toggle);
        add("device", lamps, "setBeamOutStatusLamp", QStringLiteral("出束灯（红）"), toggle);
        add("device", lamps, "setInterlockStatusLamp", QStringLiteral("联锁状态灯"), toggle);
        add("device", lamps, "setTcsStatusLamp", QStringLiteral("TCS 运行灯"), toggle);
        add("device", lamps, "setEmergencyStopStatusLamp", QStringLiteral("急停指示灯"), toggle);
        add("device", lamps, "resetTreatmentControlState", QStringLiteral("复位治疗控制状态"), room);
        add("device", lamps, "resetAllStatusLamps", QStringLiteral("熄灭所有状态灯"), room);
        add("device", lamps, "setKeepDoorClosed", QStringLiteral("防护门闭锁设置"), toggle);
        add("device", lamps, "setDrEstop", QStringLiteral("DR 急停设置"), toggle);
        add("device", lamps, "setMedicalbedEstop", QStringLiteral("医疗床急停设置"), toggle);
        add("device", lamps, "setTcsInterlock", QStringLiteral("TCS 联锁设置"), toggle);
        const QString plc = QStringLiteral("PLC 确认信号");
        add("device", plc, "confirmEnable", QStringLiteral("使能确认"), room);
        add("device", plc, "confirmMotion", QStringLiteral("运动确认"), room);
        add("device", plc, "confirmXray", QStringLiteral("X 射线曝光确认"), room);
        add("device", plc, "confirmReady", QStringLiteral("就绪确认"), room);
        add("device", plc, "confirmParticleBeam", QStringLiteral("粒子束出束确认"), room);
        add("device", plc, "confirmPositionVerification", QStringLiteral("位置验证确认"), room);
        add("device", plc, "confirmDrMotionEnable", QStringLiteral("DR 运动使能确认"), room);
        const QString motor = QStringLiteral("机柜与电机轴");
        add("device", motor, "executeInsert", QStringLiteral("插入到位闭环"), axes);
        add("device", motor, "executeExit", QStringLiteral("退出到位闭环"), axes);
        add("device", motor, "motorEnable", QStringLiteral("电机使能"), axes);
        add("device", motor, "insert", QStringLiteral("插入动作"), axes);
        add("device", motor, "exit", QStringLiteral("退出动作"), axes);
        add("device", motor, "jog1", QStringLiteral("点动 1"), axes);
        add("device", motor, "jog2", QStringLiteral("点动 2"), axes);
        add("device", motor, "stop", QStringLiteral("停止轴运动"), axes);
        add("device", motor, "resetFault", QStringLiteral("轴故障复位"), axes);
        const QString flow = QStringLiteral("设备流程动作");
        add("device", flow, "isIdle", QStringLiteral("查询设备空闲状态"), room, true);
        add("device", flow, "requestPositioning", QStringLiteral("请求摆位定位"), treatment);
        add("device", flow, "loadPlan", QStringLiteral("下发设备计划"), treatment);
        add("device", flow, "sendPositionReady", QStringLiteral("发送摆位就绪"), treatment);
        add("device", flow, "waitExposureReleased", QStringLiteral("等待曝光释放"), treatment);
        add("device", flow, "ackPositionResultReceived", QStringLiteral("确认收到位置结果"), treatment);
        add("device", flow, "waitRegistrationFinished", QStringLiteral("等待配准完成"), treatment);
        const QString dispatch = QStringLiteral("束流调度");
        add("dispatch", dispatch, "requestAdmission", QStringLiteral("取号申请准入"), {"room_id", "treatment_id"});
        add("dispatch", dispatch, "waitAdmission", QStringLiteral("等待叫号"), {"ticket", "wait_seconds"}, true);
        add("dispatch", dispatch, "cancelAdmission", QStringLiteral("取消排队叫号"), {"ticket"});
        add("dispatch", dispatch, "requestBeam", QStringLiteral("请求出束置位"), room);
        add("dispatch", dispatch, "startAccelerator", QStringLiteral("启动加速器出束"), {"room_id", "energy", "beamline"});
        add("dispatch", dispatch, "stopBeam", QStringLiteral("请求停束"), room);
        add("dispatch", dispatch, "pauseBeam", QStringLiteral("暂停出束"), room);
        add("dispatch", dispatch, "resumeBeam", QStringLiteral("恢复出束"), room);
        add("dispatch", dispatch, "endTreatment", QStringLiteral("治疗结束释放设备"), room);
        const QString ois = QStringLiteral("OIS 业务");
        add("ois", ois, "getTodaySchedule", QStringLiteral("查询今日排程"), {"room_id", "date"}, true);
        add("ois", ois, "requestStartAuthorization", QStringLiteral("请求治疗启动授权"), {"room_id", "fraction_id"});
        add("ois", ois, "sessionStarted", QStringLiteral("通知会话开始"), {"session_id"});
        add("ois", ois, "sessionCompleted", QStringLiteral("通知治疗结案"), {"session_id", "outcome"});
        add("ois", ois, "sessionPaused", QStringLiteral("通知会话暂停"), {"session_id"});
        return result;
    }();
    return specs;
}

QString methodKey(const QString &target, const QString &method)
{
    return target + QLatin1Char('/') + method;
}

const MethodSpec *findSpec(const QString &key)
{
    for (const auto &spec : methodSpecs()) {
        if (methodKey(spec.target, spec.name) == key) return &spec;
    }
    return nullptr;
}

bool isPriorityRpc(const QJsonObject &request)
{
    if (request.value("cmd").toString() != "ENG_INVOKE_RPC") return false;
    const QString key = methodKey(request.value("target").toString(), request.value("method").toString());
    return key == "dispatch/stopBeam" || key == "dispatch/pauseBeam" || key == "device/stop";
}

bool hasEngineerRole(const QJsonObject &info)
{
    const QSet<QString> allowed = {"engineer", "admin", "administrator",
                                   QStringLiteral("工程师"), QStringLiteral("管理员"),
                                   QStringLiteral("工程人员")};
    QStringList roles;
    roles.append(info.value("role").toString());
    for (const QJsonValue &role : info.value("roles").toArray()) roles.append(role.toString());
    for (const QJsonValue &role : info.value("realm_access").toObject().value("roles").toArray())
        roles.append(role.toString());
    for (const QString &role : roles) {
        if (allowed.contains(role.trimmed().toLower())) return true;
    }
    return false;
}

QJsonValue redacted(const QJsonValue &value)
{
    if (value.isObject()) {
        QJsonObject result;
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            const QString key = it.key().toLower();
            result.insert(it.key(), key.contains("password") || key.contains("token")
                || key.contains("secret") || key.contains("authorization")
                ? QJsonValue(QStringLiteral("[REDACTED]")) : redacted(it.value()));
        }
        return result;
    }
    if (value.isArray()) {
        QJsonArray result;
        for (const auto &item : value.toArray()) result.append(redacted(item));
        return result;
    }
    return value;
}

QString jsonText(const QJsonObject &object)
{
    return QString::fromUtf8(QJsonDocument(redacted(object).toObject()).toJson(QJsonDocument::Indented));
}

void tone(QWidget *widget, const char *value)
{
    widget->setProperty("engTone", value);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}


} // namespace

MaintenanceWindow::MaintenanceWindow(WebSocketClient *client, const QString &username,
                                     const QString &authToken, const QJsonObject &userInfo,
                                     const QJsonObject &loginResponse, QWidget *parent)
    : QMainWindow(parent), m_client(client), m_username(username)
{
    Q_UNUSED(authToken)
    m_role = userInfo.value("role").toString(tr("未提供角色"));
    m_authorized = hasEngineerRole(userInfo);
    QSettings settings(QApplication::applicationDirPath() + QStringLiteral("/config/app_config.ini"),
                       QSettings::IniFormat);
    m_roomId = loginResponse.value("payload").toObject().value("room_id").toString().trimmed();
    if (m_roomId.isEmpty()) m_roomId = settings.value("Treatment/RoomId").toString().trimmed();
    const QString displayName = userInfo.value("display_name").toString(userInfo.value("name").toString(username));
    buildUi(displayName);

    m_operationTimer = new QTimer(this);
    m_operationTimer->setSingleShot(true);
    connect(m_operationTimer, &QTimer::timeout, this, &MaintenanceWindow::operationTimedOut);
    m_queryTimer = new QTimer(this);
    m_queryTimer->setSingleShot(true);
    connect(m_queryTimer, &QTimer::timeout, this, [this] {
        m_queries.clear();
        updateControls();
    });
    m_telemetryTimer = new QTimer(this);
    m_telemetryTimer->setSingleShot(true);
    connect(m_telemetryTimer, &QTimer::timeout, this, [this] {
        if (!m_lastTelemetry.isEmpty()) {
            m_telemetry->setPlainText(jsonText(m_lastTelemetry).left(65536));
        }
    });
    if (client) {
        connect(client, &WebSocketClient::dataReceived, this, &MaintenanceWindow::handleMessage);
        connect(client, &WebSocketClient::connected, this, [this] { connectionChanged(true); });
        connect(client, &WebSocketClient::disconnected, this, [this] { connectionChanged(false); });
        connect(client, &WebSocketClient::errorOccurred, this, [this](const QString &error) {
            statusBar()->showMessage(tr("连接错误：%1").arg(error));
        });
        connect(client, &QObject::destroyed, this, [this] { connectionChanged(false); });
    }
    rebuildMethods();
    AuditLogger::instance().clearTreatmentContext();
    AuditLogger::instance().setContext("maintenance", username, m_role, m_roomId,
                                      userInfo.value("sub").toString());
    audit("ENGINEER_CLIENT_STARTED", "success");
    connectionChanged(client && client->isConnected());
    setWindowState(windowState() | Qt::WindowMaximized);
}

MaintenanceWindow::~MaintenanceWindow()
{
    // The client is parented to the window; detach callbacks before QWidget destroys children.
    if (m_client) disconnect(m_client, nullptr, this, nullptr);
}

void MaintenanceWindow::buildUi(const QString &displayName)
{
    Ui::MaintenanceWindow form;
    form.setupUi(this);
    form.identityLabel->setText(tr("执行人：%1　角色：%2").arg(displayName, m_role));
    m_connectionLabel = form.connectionLabel;
    m_modeLabel = form.modeLabel;
    m_roomEdit = form.roomEdit;
    m_roomEdit->setText(m_roomId);
    m_modeButton = form.modeButton;
    m_resetButton = form.resetButton;
    m_stopButton = form.stopBeamButton;
    m_notice = form.noticeLabel;
    m_priorityLabel = form.priorityLabel;
    m_operations = form.maintenanceOperations;
    m_debugDock = form.engineerDebugDock;
    m_debugTabs = form.debugTabs;
    m_recordAction = form.recordAction;
    m_statusButton = form.statusButton;
    m_manifestButton = form.manifestButton;
    m_search = form.searchEdit;
    m_methods = form.methodsTree;
    m_parameterPanel = form.parameterPanel;
    m_parameters = form.parametersLayout;
    m_methodTitle = form.methodTitle;
    m_preview = form.previewEdit;
    m_invokeButton = form.invokeButton;
    m_resultLabel = form.resultLabel;
    m_result = form.resultEdit;
    m_state = form.stateComboBox;
    m_forceButton = form.forceButton;
    m_releaseButton = form.releaseButton;
    m_status = form.statusEdit;
    m_telemetry = form.telemetryEdit;
    m_records = form.recordsTable;
    m_wire = form.wireEdit;

    form.themeButton->setIcon(QIcon(":/icons/icons/theme.svg"));
    connect(form.themeButton, &QPushButton::clicked, this, [] {
        auto &manager = ThemeManager::instance();
        manager.setTheme(manager.currentTheme() == Theme::Light ? Theme::Dark : Theme::Light);
    });
    form.engineerDebugTools->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    auto *debugMenu = new QMenu(form.engineerDebugTools);
    debugMenu->addAction(form.rpcAction);
    debugMenu->addAction(form.rawAction);
    debugMenu->addAction(m_recordAction);
    form.engineerDebugTools->setMenu(debugMenu);
    connect(form.rpcAction, &QAction::triggered, this, [this] { showDebugTools(0); });
    connect(form.rawAction, &QAction::triggered, this, [this] { showDebugTools(1); });
    connect(m_recordAction, &QAction::triggered, this, [this] { showDebugTools(2); });

    m_modeButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(m_modeButton, &QPushButton::clicked, this, &MaintenanceWindow::requestMode);
    m_statusButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_statusButton, &QPushButton::clicked, this, [this] { sendQuery("ENG_GET_ALL_STATUS"); });
    m_manifestButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    connect(m_manifestButton, &QPushButton::clicked, this, [this] { sendQuery("ENG_LIST_ALL_METHODS"); });
    m_resetButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    connect(m_resetButton, &QPushButton::clicked, this, &MaintenanceWindow::resetHardware);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    connect(m_stopButton, &QPushButton::clicked, this, [this] {
        sendPriority({{"cmd", "ENG_INVOKE_RPC"}, {"target", "dispatch"}, {"method", "stopBeam"},
                      {"params", QJsonObject{{"room_id", m_roomId}}}});
    });
    m_operations->initialize(m_roomId);
    connect(m_operations, &MaintenanceOperationPanel::rpcRequested, this,
            [this](const QJsonObject &request) { executeRpc(request); });
    m_priorityLabel->hide();
    m_debugDock->setAttribute(Qt::WA_DeleteOnClose, false);
    m_debugDock->hide();

    connect(m_search, &QLineEdit::textChanged, this, &MaintenanceWindow::filterMethods);
    connect(m_methods, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *, QTreeWidgetItem *) { selectMethod(); });
    m_invokeButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(m_invokeButton, &QPushButton::clicked, this, &MaintenanceWindow::invokeRpc);
    m_forceButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
    connect(m_forceButton, &QPushButton::clicked, this, &MaintenanceWindow::forceState);
    m_releaseButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    connect(m_releaseButton, &QPushButton::clicked, this, &MaintenanceWindow::releaseUncertainOperation);
    form.rpcSplitter->setSizes({310, 600, 420});
    form.rpcSplitter->setStretchFactor(0, 2);
    form.rpcSplitter->setStretchFactor(1, 4);
    form.rpcSplitter->setStretchFactor(2, 3);

    form.exportButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(form.exportButton, &QPushButton::clicked, this, &MaintenanceWindow::exportRecords);
    m_records->verticalHeader()->hide();
    m_records->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_records->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    form.recordSplitter->setSizes({350, 280});
    connect(m_records, &QTableWidget::itemSelectionChanged, this, [this] {
        const auto *item = m_records->item(m_records->currentRow(), 0);
        if (item) m_wire->setPlainText(item->data(Qt::UserRole).toString());
    });
    statusBar()->showMessage(tr("未发送任何设备控制指令"));
}

void MaintenanceWindow::showDebugTools(int page)
{
    if (!m_debugDock || !m_debugTabs || page < 0 || page >= m_debugTabs->count()) return;
    m_debugTabs->setCurrentIndex(page);
    if (!m_debugDock->isVisible()) {
        m_debugDock->setFloating(true);
        const QRect available = screen() ? screen()->availableGeometry() : geometry();
        m_debugDock->resize(qMin(1120, qMax(400, available.width() - 80)),
                            qMin(760, qMax(320, available.height() - 100)));
        m_debugDock->move(available.center() - m_debugDock->rect().center());
        m_debugDock->show();
    }
    m_debugDock->raise();
    m_debugDock->activateWindow();
}

void MaintenanceWindow::rebuildMethods()
{
    QString selection;
    if (m_methods->currentItem()) selection = m_methods->currentItem()->data(0, Qt::UserRole).toString();
    const QSignalBlocker blocker(m_methods);
    m_methods->clear();
    QMap<QString, QTreeWidgetItem *> groups;
    QTreeWidgetItem *selected = nullptr;
    const auto add = [&](const QString &key, const QString &group, const QString &label) {
        if (!groups.contains(group)) groups.insert(group, new QTreeWidgetItem(m_methods, {group}));
        auto *item = new QTreeWidgetItem(groups.value(group), {label});
        item->setData(0, Qt::UserRole, key);
        item->setToolTip(0, key);
        if (key == selection) selected = item;
    };
    for (const auto &spec : methodSpecs()) {
        const QString key = methodKey(spec.target, spec.name);
        if (!RoomCapabilities::supportsRpc(m_roomId, spec.target, spec.name)) continue;
        if (!m_hasManifest || m_serverMethods.contains(key))
            add(key, spec.target + QStringLiteral(" / ")
                + (!RoomCapabilities::hasDrLift(m_roomId) && spec.group == QStringLiteral("治疗床与 DR")
                    ? tr("治疗床") : spec.group), spec.title);
    }
    if (m_hasManifest) {
        for (const QString &key : m_serverMethods) {
            if (!findSpec(key)) add(key, tr("未定义参数协议"), key);
        }
    }
    m_methods->expandAll();
    if (selected) m_methods->setCurrentItem(selected);
    filterMethods();
    selectMethod();
}

void MaintenanceWindow::filterMethods()
{
    const QString needle = m_search->text().trimmed();
    for (int i = 0; i < m_methods->topLevelItemCount(); ++i) {
        auto *group = m_methods->topLevelItem(i);
        bool visible = false;
        for (int j = 0; j < group->childCount(); ++j) {
            auto *item = group->child(j);
            const bool match = (group->text(0) + item->text(0) + item->toolTip(0)).contains(needle, Qt::CaseInsensitive);
            item->setHidden(!match);
            visible |= match;
        }
        group->setHidden(!visible);
    }
}

void MaintenanceWindow::selectMethod()
{
    while (m_parameters->rowCount()) m_parameters->removeRow(0);
    m_paramEditors.clear();
    const auto *item = m_methods->currentItem();
    const auto *spec = item ? findSpec(item->data(0, Qt::UserRole).toString()) : nullptr;
    if (!spec) {
        m_methodTitle->setText(item && !item->data(0, Qt::UserRole).toString().isEmpty()
            ? tr("该接口尚无参数协议，禁止调用") : tr("未选择接口"));
        updatePreview();
        return;
    }
    m_methodTitle->setText(spec->title + QStringLiteral("\n") + methodKey(spec->target, spec->name));
    const QSet<QString> boolKeys = {"state", "axis1", "axis2", "axis3"};
    const QSet<QString> integerKeys = {"cabinet_id", "ticket", "wait_seconds", "beamline"};
    const QSet<QString> numberKeys = {"x", "y", "z", "a", "b", "c", "dx", "dy", "dz", "da", "db", "dc", "energy"};
    for (const QString &key : spec->fields) {
        QWidget *edit = nullptr;
        if (boolKeys.contains(key)) {
            auto *check = new QCheckBox(QStringLiteral("true"));
            connect(check, &QCheckBox::toggled, this, [this] { updatePreview(); });
            edit = check;
        } else if (integerKeys.contains(key)) {
            auto *spin = new QSpinBox;
            spin->setRange(key == "cabinet_id" ? 1 : 0, std::numeric_limits<int>::max());
            spin->setValue(key == "cabinet_id" ? 1 : key == "wait_seconds" ? 5 : 0);
            connect(spin, &QSpinBox::valueChanged, this, [this] { updatePreview(); });
            edit = spin;
        } else if (numberKeys.contains(key)) {
            auto *spin = new QDoubleSpinBox;
            spin->setDecimals(6);
            spin->setRange(key == "energy" ? 0.0 : -1.0e12, 1.0e12);
            spin->setKeyboardTracking(false);
            connect(spin, &QDoubleSpinBox::valueChanged, this, [this] { updatePreview(); });
            edit = spin;
        } else {
            auto *line = new QLineEdit;
            if (key == "room_id") {
                line->setText(m_roomId);
                line->setReadOnly(true);
            } else if (key == "date") {
                line->setText(QDate::currentDate().toString(Qt::ISODate));
            }
            connect(line, &QLineEdit::textChanged, this, [this] { updatePreview(); });
            edit = line;
        }
        m_paramEditors.insert(key, edit);
        m_parameters->addRow(key, edit);
    }
    updatePreview();
}

QJsonObject MaintenanceWindow::selectedRpc(QString *error) const
{
    const auto fail = [error](const QString &message) { if (error) *error = message; return QJsonObject(); };
    const auto *item = m_methods->currentItem();
    const auto *spec = item ? findSpec(item->data(0, Qt::UserRole).toString()) : nullptr;
    if (!spec) return fail(tr("请先选择参数协议已定义的接口。"));
    if (!RoomCapabilities::supportsRpc(m_roomId, spec->target, spec->name))
        return fail(tr("当前治疗室未配置 DR 起落架，该接口不可用。"));
    QJsonObject params;
    for (auto it = m_paramEditors.constBegin(); it != m_paramEditors.constEnd(); ++it) {
        if (auto *check = qobject_cast<QCheckBox *>(it.value())) params.insert(it.key(), check->isChecked());
        else if (auto *spin = qobject_cast<QSpinBox *>(it.value())) params.insert(it.key(), spin->value());
        else if (auto *spin = qobject_cast<QDoubleSpinBox *>(it.value())) params.insert(it.key(), spin->value());
        else if (auto *line = qobject_cast<QLineEdit *>(it.value())) {
            const QString text = line->text().trimmed();
            if (text.isEmpty()) return fail(tr("参数 %1 不能为空。").arg(it.key()));
            if (it.key() == "date" && !QDate::fromString(text, Qt::ISODate).isValid())
                return fail(tr("日期应为有效的 YYYY-MM-DD。"));
            params.insert(it.key(), text);
        }
    }
    if (params.contains("axis1") && !params.value("axis1").toBool()
        && !params.value("axis2").toBool() && !params.value("axis3").toBool())
        return fail(tr("至少选择一个电机轴。"));
    return {{"cmd", "ENG_INVOKE_RPC"}, {"target", spec->target}, {"method", spec->name}, {"params", params}};
}

void MaintenanceWindow::updatePreview()
{
    QString error;
    const QJsonObject request = selectedRpc(&error);
    m_preview->setPlainText(request.isEmpty() ? error : jsonText(request));
    updateControls();
}

bool MaintenanceWindow::canControl() const
{
    return m_client && m_client->isConnected() && m_authorized && !m_roomId.isEmpty()
        && m_mode == QStringLiteral("ENGINEER") && m_pending.isEmpty() && m_priorityPending.isEmpty()
        && !m_uncertain && !m_confirming;
}

bool MaintenanceWindow::canSendPriority(const QString &key) const
{
    return m_client && m_client->isConnected() && m_authorized && !m_roomId.isEmpty()
        && m_mode == "ENGINEER" && (key.isEmpty() || !m_priorityPending.contains(key)) && !m_confirming;
}

void MaintenanceWindow::sendPriority(const QJsonObject &request)
{
    const QString key = methodKey(request.value("target").toString(), request.value("method").toString());
    if (!canSendPriority(key) || !isPriorityRpc(request)) return;
    ++m_revision;
    m_priorityPending.insert(key, request);
    m_priorityLabel->show();
    m_priorityLabel->setText(tr("%1：停止类指令已发送，等待接口回执；尚未确认设备或束流停止。")
        .arg(request.value("method").toString()));
    appendRecord("TX", tr("独立停止类指令，未确认"), request);
    audit("ENGINEER_PRIORITY_SENT", "attempt", request);
    if (!m_priorityTimers.contains(key)) {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_priorityTimers.insert(key, timer);
        connect(timer, &QTimer::timeout, this, [this, key] {
            const QJsonObject pending = m_priorityPending.value(key).toObject();
            if (pending.isEmpty()) return;
            ++m_revision;
            appendRecord("LOCAL", tr("停止类指令超时，结果未知"), pending);
            audit("ENGINEER_PRIORITY_TIMEOUT", "warning", pending);
            m_unconfirmedPriority.insert(key, pending);
            m_ambiguousPriorityKeys.insert(key);
            m_uncertain = true;
            m_priorityPending.remove(key);
            m_priorityLabel->setText(tr("%1 回执超时，执行结果未知；请现场核实，必要时使用物理急停。").arg(key));
            updateControls();
        });
    }
    m_priorityTimers.value(key)->start(15000);
    updateControls();
    m_client->sendData(request);
}

void MaintenanceWindow::updateControls()
{
    const bool connected = m_client && m_client->isConnected();
    const bool unlocked = connected && m_authorized && !m_roomId.isEmpty()
        && m_pending.isEmpty() && m_priorityPending.isEmpty() && !m_uncertain && !m_confirming;
    m_connectionLabel->setText(connected ? tr("中间层：已连接") : tr("中间层：未连接"));
    tone(m_connectionLabel, connected ? "good" : "unknown");
    m_modeLabel->setText(m_mode == "ENGINEER" ? tr("后端：工程模式")
        : m_mode == "TREATMENT" ? tr("后端：治疗模式") : tr("后端模式：待确认"));
    tone(m_modeLabel, m_mode == "ENGINEER" ? "warning" : "unknown");
    m_modeButton->setText(m_mode == "ENGINEER" ? tr("切回治疗模式") : tr("切换工程模式"));
    m_modeButton->setEnabled(unlocked);
    m_statusButton->setEnabled(connected && m_authorized && !m_queries.contains("ENG_GET_ALL_STATUS"));
    m_manifestButton->setEnabled(connected && m_authorized && !m_queries.contains("ENG_LIST_ALL_METHODS"));
    const QJsonObject request = selectedRpc();
    const QString key = methodKey(request.value("target").toString(), request.value("method").toString());
    m_invokeButton->setEnabled(!request.isEmpty() && m_hasManifest && m_serverMethods.contains(key)
        && (isPriorityRpc(request) ? canSendPriority(key) : canControl() && !m_retiredRpcKeys.contains(key)));
    m_stopButton->setEnabled(canSendPriority(QStringLiteral("dispatch/stopBeam")));
    QSet<QString> enabledMethods;
    if (m_hasManifest) {
        for (const auto &spec : methodSpecs()) {
            if (!RoomCapabilities::supportsRpc(m_roomId, spec.target, spec.name)) continue;
            const QString key = methodKey(spec.target, spec.name);
            const QJsonObject rpc{{"cmd", "ENG_INVOKE_RPC"}, {"target", spec.target}, {"method", spec.name}};
            if (m_serverMethods.contains(key) && (isPriorityRpc(rpc)
                    ? canSendPriority(key) : canControl() && !m_retiredRpcKeys.contains(key)))
                enabledMethods.insert(key);
        }
    }
    m_operations->setEnabledMethods(enabledMethods);
    m_forceButton->setEnabled(canControl());
    m_resetButton->setEnabled(canControl());
    m_releaseButton->setEnabled(connected && m_authorized && m_uncertain && !m_confirming);
    m_parameterPanel->setEnabled(!m_confirming);
    m_state->setEnabled(canControl());
    QString notice;
    if (!m_authorized) notice = tr("当前登录信息未提供工程师或管理员权限，控制操作已锁定。服务端仍须独立鉴权。");
    else if (!connected) notice = tr("连接已断开，后端模式与设备状态均待重新确认；不会重发控制指令。");
    else if (m_roomId.isEmpty()) notice = tr("缺少治疗室配置，请检查 app_config.ini 中 [Treatment] RoomId。");
    else if (m_uncertain) notice = tr("上一次操作结果未确认，普通控制已锁定；停止类指令独立可用。请核实后端与设备现场。");
    else if (!m_pending.isEmpty() || !m_priorityPending.isEmpty()) notice = tr("请求已发送，等待后端确认；普通控制暂时锁定，停止类指令使用独立通道。");
    else if (m_mode != "ENGINEER") notice = tr("尚未确认后端处于工程模式，设备控制已锁定。");
    else if (!m_hasManifest) notice = tr("工程模式已确认，等待有效接口清单；单点调用暂未开放。");
    else notice = tr("工程模式：可影响设备运动、曝光、束流和联锁。远程回执不等于设备到位或停束确认。");
    m_notice->setText(notice);
    tone(m_notice, m_mode == "ENGINEER" || m_uncertain ? "warning" : "unknown");
}

bool MaintenanceWindow::confirmCommand(const QJsonObject &request, const QString &warning)
{
    const quint64 revision = m_revision;
    m_confirming = true;
    updateControls();
    QMessageBox box(QMessageBox::Warning, tr("确认工程操作"), warning,
                    QMessageBox::Yes | QMessageBox::No, this);
    box.setTextFormat(Qt::PlainText);
    box.setInformativeText(tr("执行人：%1\n治疗室：%2\n远程操作不替代物理急停。")
                               .arg(m_username, m_roomId));
    box.setDetailedText(jsonText(request));
    box.setDefaultButton(QMessageBox::No);
    const bool accepted = box.exec() == QMessageBox::Yes;
    m_confirming = false;
    updateControls();
    return accepted && revision == m_revision && m_client && m_client->isConnected();
}

void MaintenanceWindow::requestMode()
{
    if (!m_client || !m_client->isConnected() || !m_authorized || m_roomId.isEmpty()
        || !m_pending.isEmpty() || !m_priorityPending.isEmpty() || m_uncertain || m_confirming) return;
    const QString destination = m_mode == "ENGINEER" ? QStringLiteral("TREATMENT") : QStringLiteral("ENGINEER");
    const QJsonObject request = {{"cmd", "SWITCH_MODE"}, {"mode", destination}};
    const QString warning = destination == "ENGINEER"
        ? tr("切换后将开放单点硬件调用和状态强置。请确认现场已隔离，不处于临床治疗中。")
        : tr("服务端将切回治疗模式并执行协议定义的复位操作。确认切换？");
    if (confirmCommand(request, warning)) sendControl(request);
}

void MaintenanceWindow::invokeRpc()
{
    QString error;
    const QJsonObject request = selectedRpc(&error);
    executeRpc(request, error);
}

void MaintenanceWindow::executeRpc(const QJsonObject &request, const QString &error)
{
    const QString key = methodKey(request.value("target").toString(), request.value("method").toString());
    const auto *spec = findSpec(key);
    if (!spec || request.value("cmd").toString() != "ENG_INVOKE_RPC" || !request.value("params").isObject()) return;
    if (!RoomCapabilities::supportsRpc(m_roomId, spec->target, spec->name)) return;
    const QJsonObject params = request.value("params").toObject();
    if (params.contains("room_id") && params.value("room_id").toString() != m_roomId) return;
    for (const QString &field : spec->fields) {
        if (!params.contains(field)) return;
    }
    if (isPriorityRpc(request)) {
        if (m_hasManifest && m_serverMethods.contains(methodKey(request.value("target").toString(), request.value("method").toString())))
            sendPriority(request);
        return;
    }
    if (!canControl()) return;
    if (request.isEmpty() || !m_hasManifest || !m_serverMethods.contains(key) || m_retiredRpcKeys.contains(key)) {
        QMessageBox::warning(this, tr("无法调用"), error.isEmpty()
            ? tr("接口不在有效清单中，或该接口存在超时回执歧义；本窗口不再重复调用。") : error);
        return;
    }
    if (spec && !spec->readOnly && !confirmCommand(request,
        tr("将执行 %1。\n参数单位、行程和允许范围应由设备协议及服务端校验，界面数值范围不是设备安全限值。\n确认执行？")
            .arg(spec->title))) return;
    if (canControl()) sendControl(request);
}

void MaintenanceWindow::forceState()
{
    if (!canControl()) return;
    const QString state = m_state->currentText().trimmed();
    if (!QRegularExpression(QStringLiteral("^[A-Z][A-Z0-9_]*$")).match(state).hasMatch()) {
        QMessageBox::warning(this, tr("目标状态无效"), tr("请输入服务端支持的状态标识。"));
        return;
    }
    const QJsonObject request = {{"cmd", "ENG_FORCE_SET_STATE"}, {"state", state}};
    if (confirmCommand(request, tr("强置状态会绕过正常治疗时序，不代表相关设备已准备完成。\n目标状态：%1\n确认执行？").arg(state))
        && canControl()) sendControl(request);
}

void MaintenanceWindow::resetHardware()
{
    if (!canControl()) return;
    const QJsonObject request = {{"cmd", "ENG_RESET_HARDWARE"}, {"room_id", m_roomId}};
    if (confirmCommand(request, tr("将请求熄灭状态灯并复位控制状态。该指令不是已确认的停束结果。确认执行？"))
        && canControl()) sendControl(request);
}

void MaintenanceWindow::sendControl(const QJsonObject &request)
{
    if (!m_client || !m_client->isConnected() || !m_authorized || m_roomId.isEmpty()
        || !m_pending.isEmpty() || !m_priorityPending.isEmpty() || m_uncertain || m_confirming) return;
    const QString cmd = request.value("cmd").toString();
    if (cmd != "SWITCH_MODE" && m_mode != "ENGINEER") return;
    if (cmd == "ENG_INVOKE_RPC") {
        const QString key = methodKey(request.value("target").toString(), request.value("method").toString());
        if (!RoomCapabilities::supportsRpc(m_roomId, request.value("target").toString(), request.value("method").toString())) return;
        if (!m_hasManifest || !m_serverMethods.contains(key) || !findSpec(key) || m_retiredRpcKeys.contains(key)) return;
    }
    m_pending = request;
    m_resultLabel->setText(tr("已发送，等待后端确认"));
    tone(m_resultLabel, "warning");
    m_result->setPlainText(jsonText(request));
    appendRecord("TX", tr("已发送，未确认"), request);
    audit("ENGINEER_COMMAND_SENT", "attempt", request);
    int timeout = 15000;
    if (cmd == "ENG_INVOKE_RPC" && request.value("method").toString() == "waitAdmission") {
        const qint64 seconds = request.value("params").toObject().value("wait_seconds").toInt();
        timeout = static_cast<int>(qMin<qint64>(std::numeric_limits<int>::max(), seconds * 1000 + 15000));
    }
    m_operationTimer->start(timeout);
    updateControls();
    m_client->sendData(request);
}

void MaintenanceWindow::sendQuery(const QString &cmd)
{
    if (!m_client || !m_client->isConnected() || !m_authorized || m_queries.contains(cmd)) return;
    if (cmd != "ENG_LIST_ALL_METHODS" && cmd != "ENG_GET_ALL_STATUS") return;
    m_queries.insert(cmd);
    m_queryTimer->start(5000);
    const QJsonObject request = {{"cmd", cmd}};
    appendRecord("TX", tr("只读查询已发送"), request);
    updateControls();
    m_client->sendData(request);
}

void MaintenanceWindow::handleManifest(const QJsonObject &payload)
{
    QSet<QString> methods;
    for (const QString &target : {QStringLiteral("device"), QStringLiteral("dispatch"), QStringLiteral("ois")}) {
        if (!payload.value(target).isArray()) {
            m_hasManifest = false;
            m_serverMethods.clear();
            ++m_revision;
            rebuildMethods();
            statusBar()->showMessage(tr("接口清单结构不符合协议，未更新可调用接口"));
            return;
        }
        for (const QJsonValue &item : payload.value(target).toArray()) {
            if (!item.isString() || item.toString().trimmed().isEmpty()) {
                m_hasManifest = false;
                m_serverMethods.clear();
                ++m_revision;
                rebuildMethods();
                statusBar()->showMessage(tr("接口清单必须为方法名字符串数组，未更新可调用接口"));
                return;
            }
            methods.insert(methodKey(target, item.toString()));
        }
    }
    m_serverMethods = methods;
    ++m_revision;
    m_hasManifest = true;
    m_queries.remove("ENG_LIST_ALL_METHODS");
    rebuildMethods();
    statusBar()->showMessage(tr("已接收 %1 个服务端接口").arg(methods.size()), 5000);
}

void MaintenanceWindow::handleMessage(const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    const QString cmd = message.value("cmd").toString();
    if (type == "TASK" && cmd == "TASK_SYSTEM_MODE_CHANGED") {
        const QJsonObject payload = message.value("payload").toObject();
        const QString mode = payload.value("mode").toString();
        if (mode != "ENGINEER" && mode != "TREATMENT") return;
        ++m_revision;
        m_mode = mode;
        m_hasManifest = false;
        m_serverMethods.clear();
        rebuildMethods();
        appendRecord("RX", tr("后端模式广播"), message);
        audit("ENGINEER_MODE_RECEIVED", "success", payload);
        if (m_pending.value("cmd").toString() == "SWITCH_MODE") {
            if (m_pending.value("mode").toString() == mode) finishPending(tr("模式切换已确认"), message);
            else {
                m_operationTimer->stop();
                m_uncertain = true;
                m_resultLabel->setText(tr("后端模式与请求不一致，请核实"));
            }
        } else if (!m_pending.isEmpty() && mode != "ENGINEER") {
            m_operationTimer->stop();
            m_uncertain = true;
            if (m_pending.value("cmd").toString() == "ENG_INVOKE_RPC")
                m_retiredRpcKeys.insert(methodKey(m_pending.value("target").toString(), m_pending.value("method").toString()));
            m_resultLabel->setText(tr("操作期间后端模式发生变化，结果待核实"));
        }
        updateControls();
        if (mode == "ENGINEER") {
            sendQuery("ENG_LIST_ALL_METHODS");
            sendQuery("ENG_GET_ALL_STATUS");
        }
        return;
    }
    if (type == "TASK" && cmd == "TASK_ENG_METHODS_MANIFEST") {
        appendRecord("RX", tr("接口清单"), message);
        handleManifest(message.value("payload").toObject());
        return;
    }
    if (type == "TASK" && cmd == "TASK_ENG_RPC_RESULT") {
        const QJsonObject payload = message.value("payload").toObject();
        const bool valid = payload.value("target").isString() && payload.value("method").isString()
            && payload.value("success").isBool();
        const QJsonObject rpc = {{"cmd", "ENG_INVOKE_RPC"}, {"target", payload.value("target")}, {"method", payload.value("method")}};
        if (valid && isPriorityRpc(rpc)) {
            ++m_revision;
            // There is no request ID: a stop reply is an interface report, not a per-click completion proof.
            appendRecord("RX", tr("停止类接口回执"), message);
            audit("ENGINEER_PRIORITY_REPLY", payload.value("success").toBool() ? "success" : "failed", message);
            const QString key = methodKey(payload.value("target").toString(), payload.value("method").toString());
            const bool ambiguous = m_ambiguousPriorityKeys.contains(key);
            if (!payload.value("success").toBool()) {
                m_unconfirmedPriority.insert(key, m_priorityPending.value(key).toObject());
                m_uncertain = true;
            }
            if (m_priorityPending.contains(key) && !ambiguous) {
                m_priorityTimers.value(key)->stop();
                m_priorityPending.remove(key);
            }
            m_priorityLabel->show();
            m_priorityLabel->setText(ambiguous
                ? tr("收到 %1 回执，但该接口曾超时或断线，无法关联本次请求；普通控制仍需现场核实后解锁。").arg(key)
                : payload.value("success").toBool()
                ? tr("收到 %1 成功回执；实际束流/设备停止状态仍须核实。").arg(payload.value("method").toString())
                : tr("收到 %1 失败回执，请核实现场：%2").arg(payload.value("method").toString(), payload.value("error").toString()));
            m_result->setPlainText(jsonText(message));
            updateControls();
            return;
        }
        const bool matches = valid && m_pending.value("cmd").toString() == "ENG_INVOKE_RPC"
            && payload.value("target") == m_pending.value("target")
            && payload.value("method") == m_pending.value("method");
        appendRecord("RX", !valid ? tr("回执格式不完整")
            : matches ? tr("收到匹配接口回执") : tr("非当前接口回执"), message);
        if (matches) finishPending(payload.value("success").toBool()
            ? tr("服务端报告调用成功（设备状态仍需核实）") : tr("服务端报告调用失败"), message);
        return;
    }
    if (type == "TASK" && cmd == "TASK_ERROR") {
        appendRecord("RX", tr("后端错误"), message);
        m_result->setPlainText(jsonText(message));
        // TASK_ERROR has no correlation identity; it cannot complete a pending command.
        if (!m_pending.isEmpty()) {
            m_uncertain = true;
            m_operationTimer->stop();
            if (m_pending.value("cmd").toString() == "ENG_INVOKE_RPC")
                m_retiredRpcKeys.insert(methodKey(m_pending.value("target").toString(), m_pending.value("method").toString()));
            m_resultLabel->setText(tr("收到后端错误，操作结果待核实"));
        }
        updateControls();
        return;
    }
    if (type == "TELEMETRY" || cmd == "DISPATCH_TELEMETRY_STREAM" || type == "DISPATCH_TELEMETRY_STREAM") {
        m_operations->updateTelemetry(message.value("payload").toObject());
        m_lastTelemetry = {{"received_at", QDateTime::currentDateTime().toString(Qt::ISODateWithMs)}, {"message", message}};
        if (!m_telemetryTimer->isActive()) m_telemetryTimer->start(250);
        return;
    }
    if (type == "TASK" && cmd == "TASK_WORKFLOW_RESET") {
        ++m_revision;
        m_mode.clear();
        m_operations->invalidateStatus();
        if (!m_pending.isEmpty()) {
            m_uncertain = true;
            m_operationTimer->stop();
            if (m_pending.value("cmd").toString() == "ENG_INVOKE_RPC")
                m_retiredRpcKeys.insert(methodKey(m_pending.value("target").toString(), m_pending.value("method").toString()));
        }
        updateControls();
    }
    if (!cmd.isEmpty() && cmd != "TASK_LOGIN_SUCCESS" && cmd != "TASK_LOGIN_FAILED") {
        m_status->setPlainText(tr("接收时间：%1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                               + jsonText(message).left(65536));
        // Status/reset response schemas are not specified: preserve the data, do not infer success.
        appendRecord("RX", tr("状态原文（未作完成判定）"), message);
    }
}

void MaintenanceWindow::connectionChanged(bool connected)
{
    ++m_revision;
    m_mode.clear();
    m_hasManifest = false;
    m_serverMethods.clear();
    m_queries.clear();
    m_operations->invalidateStatus();
    m_queryTimer->stop();
    for (QTimer *timer : m_priorityTimers) timer->stop();
    if (!m_priorityPending.isEmpty()) {
        audit("ENGINEER_PRIORITY_CONNECTION_LOST", "warning", m_priorityPending);
        for (auto it = m_priorityPending.constBegin(); it != m_priorityPending.constEnd(); ++it) {
            m_unconfirmedPriority.insert(it.key(), it.value());
            m_ambiguousPriorityKeys.insert(it.key());
        }
        m_uncertain = true;
        m_priorityPending = QJsonObject();
        m_priorityLabel->setText(tr("连接发生变化，停止类指令执行结果未知；不会自动重发，请现场核实。"));
    }
    m_telemetryTimer->stop();
    m_lastTelemetry = QJsonObject();
    m_telemetry->setPlainText(tr("遥测待更新；旧数据已失效。"));
    if (!m_pending.isEmpty()) {
        m_operationTimer->stop();
        m_uncertain = true;
        if (m_pending.value("cmd").toString() == "ENG_INVOKE_RPC")
            m_retiredRpcKeys.insert(methodKey(m_pending.value("target").toString(), m_pending.value("method").toString()));
        m_resultLabel->setText(tr("连接变化，上一次操作结果未知；未自动重发"));
    }
    m_status->setPlainText(tr("连接已变化，之前的状态数据已失效，等待新的后端报文。"));
    rebuildMethods();
    updateControls();
    if (connected) {
        QTimer::singleShot(0, this, [this] {
            sendQuery("ENG_GET_ALL_STATUS");
            sendQuery("ENG_LIST_ALL_METHODS");
        });
    }
}

void MaintenanceWindow::operationTimedOut()
{
    if (m_pending.isEmpty()) return;
    ++m_revision;
    m_uncertain = true;
    if (m_pending.value("cmd").toString() == "ENG_INVOKE_RPC")
        m_retiredRpcKeys.insert(methodKey(m_pending.value("target").toString(), m_pending.value("method").toString()));
    m_resultLabel->setText(tr("回执超时，执行结果未知；没有自动重发"));
    tone(m_resultLabel, "warning");
    appendRecord("LOCAL", tr("超时，结果未知"), m_pending);
    audit("ENGINEER_COMMAND_TIMEOUT", "warning", m_pending);
    updateControls();
}

void MaintenanceWindow::finishPending(const QString &outcome, const QJsonObject &reply)
{
    ++m_revision;
    m_operationTimer->stop();
    m_pending = QJsonObject();
    m_uncertain = !m_unconfirmedPriority.isEmpty();
    m_resultLabel->setText(outcome);
    m_result->setPlainText(jsonText(reply));
    const QJsonValue success = reply.value("payload").toObject().value("success");
    tone(m_resultLabel, success.isBool() && !success.toBool() ? "fault" : "good");
    audit("ENGINEER_COMMAND_REPLY", success.isBool() && !success.toBool() ? "failed" : "success", reply);
    updateControls();
}

void MaintenanceWindow::releaseUncertainOperation()
{
    if (!m_uncertain || !m_authorized || !m_client || !m_client->isConnected()) return;
    const QJsonObject previous = {{"ordinary_request", m_pending}, {"stop_requests_unknown", m_unconfirmedPriority},
                                 {"stop_requests_pending", m_priorityPending}};
    if (!confirmCommand(previous, tr("请先通过后端日志和现场设备确认上次操作的实际结果。\n"
        "此操作仅解除客户端发送锁，不代表硬件复位、停束或到位成功，也不会重发指令。\n"
        "超时 RPC 在本窗口内仍禁止重复调用。确认已核实？"))) return;
    audit("ENGINEER_SEND_LOCK_RELEASED", "warning", previous);
    m_pending = QJsonObject();
    for (auto it = m_priorityPending.constBegin(); it != m_priorityPending.constEnd(); ++it)
        m_ambiguousPriorityKeys.insert(it.key());
    for (QTimer *timer : m_priorityTimers) timer->stop();
    m_priorityPending = QJsonObject();
    m_unconfirmedPriority = QJsonObject();
    m_uncertain = false;
    m_operationTimer->stop();
    m_resultLabel->setText(tr("已人工解除发送锁；上次操作未获得协议确认"));
    tone(m_resultLabel, "warning");
    updateControls();
}

void MaintenanceWindow::appendRecord(const QString &direction, const QString &outcome,
                                     const QJsonObject &message)
{
    if (m_records->rowCount() >= 200) m_records->removeRow(0);
    const int row = m_records->rowCount();
    m_records->insertRow(row);
    const QString detail = jsonText(message).left(65536);
    QString command = message.value("cmd").toString();
    if (message.contains("method")) command += QStringLiteral(" / ") + message.value("method").toString();
    const QStringList values = {QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"), direction, command, outcome};
    for (int column = 0; column < values.size(); ++column) {
        auto *item = new QTableWidgetItem(values.at(column));
        if (column == 0) item->setData(Qt::UserRole, detail);
        m_records->setItem(row, column, item);
    }
    m_recordAction->setText(tr("通信记录 (%1)").arg(m_records->rowCount()));
}

void MaintenanceWindow::audit(const QString &action, const QString &outcome, const QJsonObject &details)
{
    AuditLogger::instance().logEvent("engineering", action, "room", m_roomId,
                                    outcome, action, redacted(details).toObject());
}

void MaintenanceWindow::exportRecords()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("导出通信记录"),
        QStringLiteral("engineer-records-%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")),
        tr("JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    QJsonArray records;
    for (int row = 0; row < m_records->rowCount(); ++row) {
        const auto text = [this, row](int column) { return m_records->item(row, column)->text(); };
        records.append(QJsonObject{{"time", text(0)}, {"direction", text(1)}, {"outcome", text(3)},
            {"message", m_records->item(row, 0)->data(Qt::UserRole).toString()}});
    }
    const QByteArray bytes = QJsonDocument(records).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        QMessageBox::warning(this, tr("导出失败"), file.errorString());
        return;
    }
    audit("ENGINEER_RECORDS_EXPORTED", "success", {{"record_count", records.size()}});
    statusBar()->showMessage(tr("通信记录已导出"), 4000);
}

void MaintenanceWindow::closeEvent(QCloseEvent *event)
{
    if (m_mode == "ENGINEER" || !m_pending.isEmpty() || !m_priorityPending.isEmpty() || m_uncertain) {
        QMessageBox box(QMessageBox::Warning, tr("关闭工程界面"),
            tr("关闭界面不会自动切回治疗模式，也不代表设备或束流已停止。仍要关闭？"),
            QMessageBox::Yes | QMessageBox::No, this);
        box.setDefaultButton(QMessageBox::No);
        if (box.exec() != QMessageBox::Yes) { event->ignore(); return; }
    }
    audit("ENGINEER_CLIENT_CLOSED", "success", {{"last_confirmed_mode", m_mode}, {"pending", !m_pending.isEmpty()}});
    if (m_debugDock) m_debugDock->hide();
    QMainWindow::closeEvent(event);
}
