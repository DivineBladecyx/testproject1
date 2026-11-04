#include "qawindow.h"
#include "ui_qawindow.h"
#include "faultdisplay.h"
#include "websocketclient.h"
#include "thememanager.h"
#include "telemetryvalue.h"
#include "roomcapabilities.h"
#include "auditlogger.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QSaveFile>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QFormLayout>
#include <cmath>
#include <limits>

namespace {
const QStringList protocolIds = {QStringLiteral("MECHANICAL_IMAGING"),
                                 QStringLiteral("DOSIMETRY_BEAM"), QStringLiteral("PLAN_VERIFICATION"), QStringLiteral("OIS_DIRECT_BEAM")};

QString protocolName(const QString &id)
{
    if (id == "MECHANICAL_IMAGING") return QObject::tr("机械与影像摆位质控");
    if (id == "DOSIMETRY_BEAM") return QObject::tr("束流与绝对剂量质控");
    if (id == "PLAN_VERIFICATION") return QObject::tr("计划预演验证（可能真实出束）");
    if (id == "OIS_DIRECT_BEAM") return QObject::tr("OIS 叫号直通供束质控");
    return QObject::tr("未知预案");
}

QJsonObject firstObject(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonObject value = object.value(key).toObject();
        if (!value.isEmpty()) return value;
    }
    return {};
}

bool finiteNumber(const QJsonValue &value)
{
    return value.isDouble() && std::isfinite(value.toDouble());
}

QString qaRoom(const QJsonValue &value)
{
    QString text;
    if (value.isString()) text = value.toString().trimmed().toUpper();
    else if (value.isDouble() && value.toInt(-1) >= 0
             && value.toDouble() == value.toInt(-1)) text = QString::number(value.toInt());
    else return {};
    if (text.startsWith("ROOM_")) text.remove(0, 5);
    bool ok = false;
    const int id = text.toInt(&ok);
    return ok && id >= 0 ? QStringLiteral("ROOM_%1").arg(id) : QString();
}

bool qaRoomMatches(const QJsonObject &object, const QString &room)
{
    return !object.contains("room_id") || (!qaRoom(object.value("room_id")).isEmpty()
                                           && qaRoom(object.value("room_id")) == qaRoom(QJsonValue(room)));
}

QString number(const QJsonObject &data, const QString &key)
{
    const QJsonValue value = data.value(key);
    return finiteNumber(value) ? QString::number(value.toDouble(), 'g', 10) : QStringLiteral("-");
}

void polish(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString stageName(const QString &stage)
{
    static const QMap<QString, QString> stages = {
                                                  {"INITIALIZING", QObject::tr("初始化")}, {"COUCH_MOVING", QObject::tr("治疗床移动中")},
                                                  {"IGRT_VERIFYING", QObject::tr("IGRT 配准验证中")},
                                                  {"APPLYING_BEAM", QObject::tr("申请束流中")}, {"BEAM_DELIVERING", QObject::tr("出束中")},
                                                  {"WAITING_LOGIN", QObject::tr("操作员授权登入")},
                                                  {"PLAN_PREPARING", QObject::tr("计划 / 硬件准备")},
                                                  {"POSITIONING", QObject::tr("机械运动摆位")},
                                                  {"SUBMIT_REVIEW", QObject::tr("配准 / 标定审核")},
                                                  {"REQUEST_ADMISSION", QObject::tr("申请准入排队")},
                                                  {"SENDING_PLAN_TO_DDS", QObject::tr("调度下发计划")},
                                                  {"SENDING_TO_DDS", QObject::tr("调度下发计划")},
                                                  {"STARTING_BEAM", QObject::tr("建立出束通道")},
                                                  {"BEAM_IN_PROGRESS", QObject::tr("出束中")},
                                                  {"WAITING_OIS_CALL", QObject::tr("等待 OIS 叫号")},
                                                  {"FETCHING_PLAN_PACKAGE", QObject::tr("拉取计划包")},
                                                  {"FETCHING_BEAM_DELIVERY", QObject::tr("拉取束流投照数据")},
                                                  {"QA_COMPLETED", QObject::tr("安全结案（等待完成回执）")}};
    return stages.value(stage, stage.isEmpty() ? QObject::tr("未提供") : stage.left(120));
}

QString resultText(const QJsonValue &value)
{
    if (value.isBool()) return value.toBool() ? QObject::tr("是") : QObject::tr("否");
    if (finiteNumber(value)) return QString::number(value.toDouble(), 'g', 12);
    if (value.isString()) return value.toString().left(2000);
    return QObject::tr("未提供有效值");
}

QJsonObject knownResultData(const QJsonObject &data)
{
    QJsonObject result;
    const QStringList fields = {"status", "shift_couch_x", "shift_couch_y", "shift_couch_z",
                                "isocenter_deviation_mm", "is_isocenter_aligned", "nominal_energy", "target_mu", "delivered_mu",
                                "dose_deviation_pct", "plan_id", "gamma_pass_rate_pct", "qa_conclusion"};
    for (const auto &key : fields) {
        const auto value = data.value(key);
        if (value.isString() || value.isBool() || finiteNumber(value)) result.insert(key, value);
    }
    return result;
}
}

QaWindow::QaWindow(WebSocketClient *client, const QString &username,
                   const QString &authToken, const QJsonObject &userInfo,
                   const QJsonObject &loginResponse, QWidget *parent)
    : QMainWindow(parent), m_ui(std::make_unique<Ui::QaWindow>()), m_client(client)
{
    Q_UNUSED(authToken)
    m_ui->setupUi(this);
    m_authenticated = !username.trimmed().isEmpty() && !loginResponse.isEmpty();
    m_roomId = qaRoom(loginResponse.value("payload").toObject().value("room_id"));
    if (m_roomId.isEmpty()) {
        QSettings settings(QApplication::applicationDirPath() + QStringLiteral("/config/app_config.ini"), QSettings::IniFormat);
        m_roomId = settings.value(QStringLiteral("Treatment/RoomId")).toString().trimmed();
    }
    m_ui->roomLabel->setText(tr("治疗室：%1").arg(m_roomId.isEmpty() ? tr("未配置") : m_roomId));
    m_ui->mainLayout->insertWidget(2, new FaultDisplay(m_client, [this] { return m_roomId; }, this));
    m_ui->identityLabel->setText(tr("执行人：%1").arg(userInfo.value("display_name").toString(
        userInfo.value("name").toString(username))));
    AuditLogger::instance().setContext("qa", username, userInfo.value("role").toString("unknown"), m_roomId);
    for (auto *label : findChildren<QLabel *>()) label->setTextFormat(Qt::PlainText);
    const QStringList fields = {"beam", "result", "current_dose", "sum_dose", "layer_sum_dose", "dose_rate",
                                "current_layer", "current_point", "next_layer", "therapy_id", "timestamp"};
    for (const auto &key : fields) m_fields.insert(key, findChild<QLabel *>(key + "Value"));
    const QStringList lamps = {"couch", "dr", "igrt", "filter", "plc", "g01", "g02", "accelerator", "dds", "fault"};
    for (const auto &key : lamps) m_lamps.insert(key, findChild<QLabel *>(key + "Lamp"));
    if (!RoomCapabilities::hasDrLift(m_roomId)) {
        m_ui->drName->hide();
        m_ui->drLamp->hide();
    }
    m_positions = m_ui->positionTable;
    m_positions->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_positions->verticalHeader()->hide();
    m_positions->setFont(QFont(QStringLiteral("Consolas"), 12));
    m_ui->resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->resultTable->verticalHeader()->hide();
    m_ui->resultTable->setWordWrap(true);
    m_ui->resultTable->hide();
    m_ui->detailsTabs->hide();
    for (auto *layout : {m_ui->mechanicalPageLayout, m_ui->dosimetryPageLayout, m_ui->verificationPageLayout})
        layout->setAlignment(Qt::AlignTop);
    m_ui->workspaceLayout->setAlignment(Qt::AlignTop);
    m_ui->monitorLayout->setAlignment(Qt::AlignTop);
    m_ui->parametersLayout->setAlignment(Qt::AlignTop);
    m_ui->protocolTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_ui->communicationLog->setMaximumBlockCount(500);
    m_ui->beamlineInput->setMinimum(-1);
    m_ui->beamlineInput->setSpecialValueText(tr("未设置"));
    m_ui->beamlineInput->setValue(-1);
    m_ui->planIdInput->setMaxLength(256);
    m_directPage = new QWidget(this);
    m_directPage->setObjectName("oisDirectPage");
    auto *directLayout = new QFormLayout(m_directPage);
    m_directPlan = new QLineEdit(m_directPage);
    m_directPlan->setObjectName("directPlanIdInput");
    m_directPlan->setMaxLength(256);
    m_directPlan->setPlaceholderText(tr("留空：等待 OIS 叫号"));
    m_directTreatment = new QLineEdit(m_directPage);
    m_directTreatment->setObjectName("directTreatmentIdInput");
    m_directTreatment->setMaxLength(256);
    m_directMu = new QDoubleSpinBox(m_directPage);
    m_directMu->setObjectName("directMuInput");
    m_directMu->setDecimals(3);
    m_directMu->setRange(0, 300);
    m_directMu->setSuffix(" MU");
    directLayout->addRow(tr("计划 ID"), m_directPlan);
    directLayout->addRow(tr("治疗会话 ID"), m_directTreatment);
    directLayout->addRow(tr("目标 MU"), m_directMu);
    auto *directWarning = new QLabel(tr("真实出束；跳过计划确认与自动摆位。\n本地上限 300 MU，后端更低限额优先。"), m_directPage);
    directWarning->setWordWrap(true);
    directLayout->addRow(directWarning);
    m_ui->protocolTabs->addTab(m_directPage, tr("OIS 直通供束"));
    m_registrationLabel = new QLabel(tr("配准结果：未收到"), this);
    m_registrationLabel->setTextFormat(Qt::PlainText);
    m_registrationLabel->setWordWrap(true);
    m_ui->monitorLayout->addWidget(m_registrationLabel);
    m_ddsAlert = new QLabel(this);
    m_ddsAlert->setTextFormat(Qt::PlainText);
    m_ddsAlert->setWordWrap(true);
    m_ddsAlert->hide();
    m_ui->monitorLayout->addWidget(m_ddsAlert);
    m_ddsReset = new QPushButton(tr("DDS 故障复位"), this);
    m_ddsReset->setObjectName("ddsResetButton");
    m_ddsReset->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_ui->commandLayout->addWidget(m_ddsReset);
    m_ddsResetTimer = new QTimer(this);
    m_ddsResetTimer->setSingleShot(true);
    m_ddsResetTimer->setInterval(15000);
    connect(m_ddsReset, &QPushButton::clicked, this, &QaWindow::resetDdsFault);
    connect(m_ddsResetTimer, &QTimer::timeout, this, [this] {
        m_ddsAlert->setText(tr("DDS 复位回执超时，故障是否解除未知；请核实现场。"));
        m_ddsAlert->show();
        updateControls();
    });
    connect(m_directPlan, &QLineEdit::textChanged, this, [this] { updateControls(); });
    connect(m_directTreatment, &QLineEdit::textChanged, this, [this] { updateControls(); });
    connect(m_directMu, &QDoubleSpinBox::valueChanged, this, [this] { updateControls(); });
    m_ui->enterModeButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_ui->queryStatusButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_ui->abortButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_ui->executeButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_ui->saveSnapshotButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_ui->themeButton->setIcon(QIcon(":/icons/icons/theme.svg"));
    m_ui->themeButton->setMaximumWidth(140);
    connect(m_ui->enterModeButton, &QPushButton::clicked, this, &QaWindow::enterQaMode);
    connect(m_ui->queryStatusButton, &QPushButton::clicked, this, &QaWindow::queryStatus);
    connect(m_ui->executeButton, &QPushButton::clicked, this, &QaWindow::executeProtocol);
    connect(m_ui->abortButton, &QPushButton::clicked, this, &QaWindow::abortQa);
    connect(m_ui->resultButton, &QPushButton::clicked, this, &QaWindow::showResult);
    connect(m_ui->saveSnapshotButton, &QPushButton::clicked, this, &QaWindow::saveSnapshot);
    connect(m_ui->detailsButton, &QToolButton::toggled, this, [this](bool checked) {
        m_ui->detailsTabs->setVisible(checked);
        m_ui->detailsButton->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
    });
    connect(m_ui->protocolTabs, &QTabWidget::currentChanged, this, [this] { updateSelection(); });
    for (auto *input : {m_ui->target_x, m_ui->target_y, m_ui->target_z, m_ui->energyInput, m_ui->target_muInput})
        connect(input, &QDoubleSpinBox::valueChanged, this, [this] { updateControls(); });
    connect(m_ui->beamlineInput, &QSpinBox::valueChanged, this, [this] { updateControls(); });
    connect(m_ui->planIdInput, &QLineEdit::textChanged, this, [this] { updateControls(); });
    connect(m_ui->themeButton, &QPushButton::clicked, this, [] {
        auto &manager = ThemeManager::instance();
        manager.setTheme(manager.currentTheme() == Theme::Light ? Theme::Dark : Theme::Light);
    });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { applyTheme(); });
    m_requestTimer = new QTimer(this);
    m_requestTimer->setObjectName(QStringLiteral("qaRequestTimer"));
    m_requestTimer->setSingleShot(true);
    m_requestTimer->setInterval(15000);
    connect(m_requestTimer, &QTimer::timeout, this, &QaWindow::requestTimedOut);
    m_queryTimer = new QTimer(this);
    m_queryTimer->setSingleShot(true);
    m_queryTimer->setInterval(15000);
    connect(m_queryTimer, &QTimer::timeout, this, [this] {
        m_statusKnown = false;
        m_problem = tr("QA 状态查询超时；请同步状态，不能据此认定设备已停止。");
        ++m_revision;
        updateControls();
    });
    auto *poll = new QTimer(this);
    poll->setInterval(20000);
    connect(poll, &QTimer::timeout, this, &QaWindow::queryStatus);
    poll->start();
    m_timeout = new QTimer(this);
    m_timeout->setInterval(1000);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (auto it = m_updated.begin(); it != m_updated.end();) {
            if (now - it.value() > 10000) { m_data.remove(it.key()); it = m_updated.erase(it); }
            else ++it;
        }
        refresh();
        if (m_data.isEmpty()) m_fields.value("timestamp")->setText(tr("遥测超时，状态未知"));
    });
    if (client) {
        connect(client, &WebSocketClient::dataReceived, this, &QaWindow::receiveMessage);
        connect(client, &WebSocketClient::connected, this, [this] {
            connectionLost();
            m_problem = tr("连接已恢复，请重新确认 QA 模式与现场状态。");
            queryStatus();
            updateControls();
        });
        connect(client, &WebSocketClient::disconnected, this, &QaWindow::connectionLost);
        connect(client, &QObject::destroyed, this, &QaWindow::connectionLost);
        connect(client, &WebSocketClient::errorOccurred, this, [this](const QString &) {
            m_statusKnown = false;
            m_uncertain = m_uncertain || !m_pending.isEmpty() || m_running;
            m_problem = tr("通信异常，操作是否执行待核实；请同步状态，必要时中止 QA。");
            ++m_revision;
            updateControls();
        });
    }
    clearTelemetry();
    applyTheme();
    updateSelection();
    setWindowState(windowState() | Qt::WindowMaximized);
    QTimer::singleShot(0, this, &QaWindow::queryStatus);
}

QaWindow::~QaWindow()
{
    if (m_client) disconnect(m_client, nullptr, this, nullptr);
}

void QaWindow::applyTheme()
{
    const bool dark = ThemeManager::instance().currentTheme() == Theme::Dark;
    setStyleSheet(QStringLiteral(
                      "QMainWindow#QaWindow { background:%1; }"
                      "QMainWindow#QaWindow QWidget { font-size:18px; }"
                      "QMainWindow#QaWindow QLabel { color:%2; background:transparent; }"
                      "QMainWindow#QaWindow #titleLabel {font-size:24px;font-weight:bold;}"
                      "QMainWindow#QaWindow #header,QMainWindow#QaWindow #commandBar {background:%3;}"
                      "QMainWindow#QaWindow QTabWidget::pane {border:1px solid %4;background:%3;}"
                      "QMainWindow#QaWindow QTabBar::tab {padding:10px 20px;color:%2;}"
                      "QMainWindow#QaWindow QDoubleSpinBox,QMainWindow#QaWindow QSpinBox,QMainWindow#QaWindow QLineEdit {min-height:36px;}"
                      "QMainWindow#QaWindow QPushButton {min-height:32px;padding:6px 12px;}"
                      "QMainWindow#QaWindow #abortButton {background:#B42318;color:white;font-weight:bold;border:1px solid #B42318;}"
                      "QMainWindow#QaWindow #abortButton:disabled {background:%4;color:%2;}"
                      "QMainWindow#QaWindow #blockingLabel {background:%5;color:%6;padding:8px;}"
                      "QMainWindow#QaWindow #muLabel {font-family:Consolas;font-size:28px;font-weight:bold;}"
                      "QMainWindow#QaWindow #beamValue {font-size:24px;font-weight:bold;}"
                      "QMainWindow#QaWindow QLabel[qaTone=good] {color:%7;font-weight:bold;}"
                      "QMainWindow#QaWindow QLabel[qaTone=fault] {color:%8;font-weight:bold;}"
                      "QMainWindow#QaWindow QLabel[qaTone=warning] {color:%6;font-weight:bold;}"
                      "QMainWindow#QaWindow QLabel[qaTone=unknown] {color:%9;}"
                      "QMainWindow#QaWindow QTableWidget {font-size:16px;}"
                      "QMainWindow#QaWindow #communicationLog {font-family:Consolas;font-size:16px;}"
                      "QMainWindow#QaWindow QScrollArea {border:none;}")
                      .arg(dark ? "#171E22" : "#F3F6FA", dark ? "#E1E8EC" : "#203F54",
                           dark ? "#212A2F" : "#FFFFFF", dark ? "#46565F" : "#CCD7E0",
                           dark ? "#393120" : "#FFF5E4", dark ? "#F0CE7B" : "#875A09",
                           dark ? "#77D4A1" : "#167347", dark ? "#FF9C98" : "#B42318",
                           dark ? "#AFBDC5" : "#657581"));
}

QString QaWindow::selectedProtocol() const
{
    return protocolIds.value(m_ui->protocolTabs->currentIndex());
}

QJsonObject QaWindow::selectedParams() const
{
    QJsonObject params{{"room_id", m_roomId}};
    const QString id = selectedProtocol();
    if (id == "MECHANICAL_IMAGING") {
        params.insert("target_couch_x", m_ui->target_x->value());
        params.insert("target_couch_y", m_ui->target_y->value());
        params.insert("target_couch_z", m_ui->target_z->value());
    } else if (id == "DOSIMETRY_BEAM") {
        params.insert("energy", m_ui->energyInput->value());
        params.insert("beamline", m_ui->beamlineInput->value());
        params.insert("target_mu", m_ui->target_muInput->value());
    } else if (id == "PLAN_VERIFICATION") params.insert("plan_id", m_ui->planIdInput->text().trimmed());
    else if (id == "OIS_DIRECT_BEAM") {
        params.insert("target_mu", m_directMu->value());
        if (!m_directPlan->text().trimmed().isEmpty()) {
            params.insert("plan_id", m_directPlan->text().trimmed());
            params.insert("treatment_id", m_directTreatment->text().trimmed());
        }
    }
    return params;
}

QString QaWindow::validationError() const
{
    const QString id = selectedProtocol();
    if (!m_authenticated) return tr("操作员未登录或授权已失效，请重新登录。");
    if (m_roomId.isEmpty()) return tr("请先在配置文件设置本机治疗室 RoomId。");
    if (!m_protocols.contains(id)) return tr("后端尚未提供此预案的有效能力声明。");
    if (id == "DOSIMETRY_BEAM") {
        const double limit = m_protocols.value(id).value("max_dose_mu").toDouble();
        if (m_ui->energyInput->value() <= 0) return tr("请输入大于 0 的能量。");
        if (m_ui->beamlineInput->value() < 0) return tr("请填写束线编号。");
        if (m_ui->target_muInput->value() <= 0 || m_ui->target_muInput->value() > limit)
            return tr("目标 MU 必须大于 0 且不超过 %1。").arg(limit);
    }
    if (id == "PLAN_VERIFICATION" && m_ui->planIdInput->text().trimmed().isEmpty())
        return tr("请输入已在后端准备好的 QA 计划 ID。");
    if (id == "OIS_DIRECT_BEAM") {
        const double limit = m_protocols.value(id).value("max_dose_mu").toDouble(300);
        if (m_directMu->value() <= 0 || m_directMu->value() > qMin(300.0, limit))
            return tr("目标 MU 必须大于 0 且不超过 %1。").arg(qMin(300.0, limit));
        if (m_directPlan->text().trimmed().isEmpty() != m_directTreatment->text().trimmed().isEmpty())
            return tr("指定计划时请同时填写计划 ID 和治疗会话 ID；等待叫号时两项均留空。");
    }
    return {};
}

bool QaWindow::canExecute() const
{
    return m_client && m_client->isConnected() && m_modeConfirmed && m_enabled && m_statusKnown
           && !m_running && !m_uncertain && m_pending.isEmpty() && !m_confirmationOpen && validationError().isEmpty();
}

void QaWindow::updateSelection()
{
    const QString id = selectedProtocol();
    const auto protocol = m_protocols.value(id);
    m_ui->protocolDescription->setText(protocol.isEmpty() ? tr("预案能力：等待后端提供")
                                                          : tr("后端预案：%1\n%2").arg(protocol.value("name").toString(protocolName(id)),
                                                                                            protocol.value("description").toString()));
    const double limit = m_protocols.value("DOSIMETRY_BEAM").value("max_dose_mu").toDouble();
    m_ui->doseLimitLabel->setText(limit > 0 ? tr("本次上限：%1 MU（取后端限额与 300 MU 的较小值）").arg(limit)
                                            : tr("后端剂量限额：未确认"));
    updateControls();
}

void QaWindow::updateControls()
{
    const QString displayed = m_activeProtocol.isEmpty() ? selectedProtocol() : m_activeProtocol;
    const bool doseVisible = displayed != "MECHANICAL_IMAGING";
    m_ui->muLabel->setVisible(doseVisible);
    m_ui->doseProgress->setVisible(doseVisible);
    m_ui->qaEnergyLabel->setVisible(doseVisible);
    const bool connected = m_client && m_client->isConnected();
    const bool busy = m_running || !m_pending.isEmpty() || m_uncertain;
    m_ui->paramsOriginLabel->setText(!m_submittedParams.isEmpty() && m_activeProtocol == selectedProtocol()
                                         ? tr("已提交参数；动作完成以回执为准") : busy ? tr("本地草稿；非后端恢复的执行参数") : tr("本地待提交参数"));
    m_ui->connectionLabel->setText(connected ? tr("中间层：已连接") : tr("中间层：未连接"));
    m_ui->modeLabel->setText(m_modeConfirmed ? tr("后端 QA 模式：已确认") : tr("后端 QA 模式：未确认"));
    m_ui->enterModeButton->setEnabled(connected && m_authenticated && !busy && !m_confirmationOpen && !m_roomId.isEmpty());
    m_ddsReset->setEnabled(connected && m_authenticated && !m_confirmationOpen && !m_ddsResetTimer->isActive());
    m_ui->queryStatusButton->setEnabled(connected && !m_queryTimer->isActive());
    // Abort remains available when execution is uncertain, even before the mode/capability handshake.
    m_ui->abortButton->setEnabled(connected && m_pending != "abort"
                                  && (m_modeConfirmed || m_enabled || busy));
    m_ui->executeButton->setEnabled(canExecute());
    m_ui->executeButton->setText(tr("执行：%1").arg(protocolName(selectedProtocol())));
    const bool editable = !busy && !m_confirmationOpen;
    for (auto *page : {m_ui->mechanicalPage, m_ui->dosimetryPage, m_ui->verificationPage}) page->setEnabled(editable);
    m_directPage->setEnabled(editable);
    m_ui->resultButton->setEnabled(!m_result.isEmpty());
    m_ui->saveSnapshotButton->setEnabled(!m_result.isEmpty() || !m_progress.isEmpty() || !m_data.isEmpty());
    QString reason = m_problem;
    if (!connected) reason = tr("连接中断，无法发送控制指令；现场是否停止必须核实。");
    else if (m_roomId.isEmpty()) reason = validationError();
    else if (!m_modeConfirmed) reason = tr("请选择“进入 QA 模式”，等待后端模式与预案能力确认。") + " " + reason;
    else if (!m_statusKnown) reason = tr("QA 现场状态未确认，新任务已锁定。") + " " + reason;
    else if (!m_enabled) reason = tr("后端 QA 模块未启用，新任务已锁定。") + " " + reason;
    else if (reason.isEmpty()) reason = busy ? tr("任务处理中；中止须等待后端安全截断回执。") : validationError();
    if (reason.isEmpty()) reason = tr("QA 真实设备操作；执行前请确认假体、测量装置和现场条件。服务端仍需独立鉴权与安全校验。");
    m_ui->blockingLabel->setText(reason);
    QString state = !m_pending.isEmpty() ? tr("等待后端回执") : m_running ? tr("执行中")
                                                                  : m_uncertain ? tr("执行状态待核实") : !m_statusKnown ? tr("未知")
                                                                  : !m_enabled ? tr("未启用") : tr("空闲");
    m_ui->workflowStateLabel->setText(tr("QA 状态：%1").arg(state));
    m_ui->currentActionLabel->setText(tr("当前动作：%1").arg(m_stageText.isEmpty() ? tr("等待操作或状态同步") : m_stageText));
}

bool QaWindow::send(const QJsonObject &request)
{
    if (!m_client || !m_client->isConnected()) { connectionLost(); return false; }
    const QString cmd = request.value("cmd").toString();
    record(tr("提交 %1（仅提交，不代表执行完成）").arg(cmd));
    AuditLogger::instance().logEvent("qa", "QA_COMMAND_SUBMITTED", "qa_protocol",
                                     request.value("protocol").toString(), "attempt", tr("已提交 QA 请求，等待后端确认"), request);
    m_client->sendData(request);
    return true;
}

void QaWindow::enterQaMode()
{
    if (!m_ui->enterModeButton->isEnabled()) return;
    m_pending = QStringLiteral("mode");
    m_statusKnown = false;
    m_modeConfirmed = false;
    m_protocols.clear();
    m_stageText = tr("请求进入 QA 模式");
    m_problem.clear();
    ++m_revision;
    m_requestTimer->start();
    updateControls();
    send({{"cmd", "SWITCH_MODE"}, {"mode", "QA"}});
}

void QaWindow::queryStatus()
{
    if (!m_client || !m_client->isConnected() || m_queryTimer->isActive()) return;
    m_queryTimer->start();
    updateControls();
    send({{"cmd", "QA_GET_STATUS"}});
}

void QaWindow::executeProtocol()
{
    if (!canExecute()) return;
    const QString protocol = selectedProtocol();
    const QJsonObject params = selectedParams();
    const quint64 revision = m_revision;
    const QJsonObject capability = m_protocols.value(protocol);
    m_confirmationOpen = true;
    updateControls();
    QMessageBox confirmation(QMessageBox::Warning, tr("确认执行 QA"),
                             tr("治疗室：%1\n预案：%2\n%3\n\n参数：\n%4")
                                 .arg(m_roomId, protocolName(protocol), capability.value("beam_allowed").toBool()
                                                                            ? tr("可能发生真实设备运动与真实出束。请确认 QA 假体及现场条件。")
                                                                            : tr("真实设备运动；该预案严格不出束。"),
                                      QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Indented))),
                             QMessageBox::Ok | QMessageBox::Cancel, this);
    confirmation.setObjectName(QStringLiteral("qaExecuteConfirmation"));
    confirmation.setTextFormat(Qt::PlainText);
    confirmation.setDefaultButton(QMessageBox::Cancel);
    const bool confirmed = confirmation.exec() == QMessageBox::Ok;
    m_confirmationOpen = false;
    // Modal dialogs process incoming messages: recheck connection, state and capabilities after approval.
    if (!confirmed || revision != m_revision || protocol != selectedProtocol()
        || capability != m_protocols.value(protocol) || !canExecute()) { updateControls(); return; }
    m_pending = QStringLiteral("execute");
    m_activeProtocol = protocol;
    m_submittedParams = params;
    m_progress = {};
    m_result = {};
    m_registration = {};
    m_registrationLabel->setText(tr("配准结果：未收到"));
    m_ui->resultTable->setRowCount(0);
    m_ui->resultTable->hide();
    m_ui->resultSummary->setText(tr("等待后端执行与完成回执"));
    m_ui->activeProtocolLabel->setText(tr("执行预案：%1").arg(protocolName(protocol)));
    m_ui->stageLabel->setText(tr("阶段：等待后端执行反馈"));
    m_ui->qaEnergyLabel->setText(tr("回读能量：-"));
    m_ui->muLabel->setText(tr("累计 MU / 目标：- / -"));
    m_ui->doseProgress->setValue(0);
    m_ui->doseProgress->setFormat(tr("等待剂量反馈"));
    m_stageText = tr("已提交 %1，等待后端执行反馈").arg(protocolName(protocol));
    m_problem.clear();
    ++m_revision;
    m_requestTimer->start();
    updateControls();
    send({{"cmd", "QA_EXECUTE_PROTOCOL"}, {"protocol", protocol}, {"params", params}});
}

void QaWindow::abortQa()
{
    if (!m_ui->abortButton->isEnabled()) return;
    m_pending = QStringLiteral("abort");
    m_uncertain = true;
    m_stageText = tr("已请求中止，等待 TASK_QA_ABORTED；尚未确认关束或硬件复位");
    ++m_revision;
    m_requestTimer->start();
    updateControls();
    send({{"cmd", "QA_ABORT"}});
}

void QaWindow::resetDdsFault()
{
    if (!m_ddsReset->isEnabled()) return;
    m_confirmationOpen = true;
    updateControls();
    const auto answer = QMessageBox::warning(this, tr("确认 DDS 故障复位"),
                                             tr("请确认现场异常已排除。复位不代表停止出束，也不会自动重启 QA。"),
                                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    m_confirmationOpen = false;
    updateControls();
    if (answer != QMessageBox::Ok || !m_ddsReset->isEnabled()) return;
    m_ddsResetTimer->start();
    m_ddsAlert->setText(tr("DDS 故障复位：等待后端回执"));
    m_ddsAlert->show();
    updateControls();
    send({{"cmd", "DDS_FAULT_RESET"}, {"room_id", m_roomId}});
}

void QaWindow::connectionLost()
{
    m_ddsResetTimer->stop();
    m_uncertain = m_uncertain || m_running || !m_pending.isEmpty();
    m_pending.clear();
    m_modeConfirmed = false;
    m_enabled = false;
    m_statusKnown = false;
    m_protocols.clear();
    m_requestTimer->stop();
    m_queryTimer->stop();
    m_activeProtocol.clear();
    m_submittedParams = {};
    m_progress = {};
    m_ui->activeProtocolLabel->setText(tr("执行预案：待后端提供"));
    m_ui->stageLabel->setText(tr("阶段：待同步"));
    m_ui->muLabel->setText(tr("累计 MU / 目标：- / -"));
    m_ui->qaEnergyLabel->setText(tr("回读能量：-"));
    m_ui->doseProgress->setValue(0);
    m_ui->doseProgress->setFormat(tr("等待有效剂量反馈"));
    m_stageText = tr("现场状态待核实");
    m_problem = tr("状态待核实；重连不会自动重发执行或中止指令。");
    ++m_revision;
    clearTelemetry();
    updateControls();
}

void QaWindow::requestTimedOut()
{
    const QString pending = m_pending;
    m_pending.clear();
    if (pending != "mode") m_uncertain = true;
    m_statusKnown = false;
    m_problem = tr("操作等待超时；不代表失败或停止。请同步状态，必要时再次请求中止。");
    ++m_revision;
    record(m_problem);
    updateControls();
    queryStatus();
}

void QaWindow::applyProtocols(const QJsonObject &payload)
{
    m_protocols.clear();
    QSet<QString> duplicates;
    for (const auto &value : payload.value("protocols").toArray()) {
        const auto p = value.toObject();
        const QString id = p.value("id").toString();
        if (!protocolIds.contains(id) || !p.value("beam_allowed").isBool()) continue;
        if (m_protocols.contains(id) || duplicates.contains(id)) {
            m_protocols.remove(id); duplicates.insert(id); continue;
        }
        if (id == "MECHANICAL_IMAGING" && p.value("beam_allowed").toBool()) continue;
        if (id != "MECHANICAL_IMAGING" && !p.value("beam_allowed").toBool()) continue;
        if (id == "OIS_DIRECT_BEAM" && p.contains("max_dose_mu")
            && (!finiteNumber(p.value("max_dose_mu")) || p.value("max_dose_mu").toDouble() <= 0)) continue;
        QJsonObject accepted = p;
        if (id == "DOSIMETRY_BEAM") {
            if (!p.value("beam_allowed").toBool() || !finiteNumber(p.value("max_dose_mu"))
                || p.value("max_dose_mu").toDouble() <= 0) continue;
            accepted.insert("max_dose_mu", qMin(300.0, p.value("max_dose_mu").toDouble()));
        }
        m_protocols.insert(id, accepted);
    }
}

void QaWindow::receiveMessage(const QJsonObject &message)
{
    if (message.value("type").toString() == "TELEMETRY") { receiveTelemetry(message); return; }
    if (message.value("type").toString() != "TASK") return;
    if (!message.value("payload").isObject() && message.value("cmd") != "TASK_LOGIN_FAILED") return;
    const auto payload = message.value("payload").toObject();
    if (!qaRoomMatches(message, m_roomId) || !qaRoomMatches(payload, m_roomId)) return;
    const QString cmd = message.value("cmd").toString();
    if (cmd == "TASK_LOGIN_FAILED") {
        m_authenticated = false;
        m_problem = tr("登录授权失效，请重新登录。");
        ++m_revision;
        updateControls();
    } else if (cmd == "TASK_REGISTRATION_RESULT") {
        if (payload.value("protocol") != "MECHANICAL_IMAGING"
            || m_activeProtocol != "MECHANICAL_IMAGING" || !m_running) return;
        m_registration = payload;
        const auto shift = payload.value("shift_couch").toObject();
        m_registrationLabel->setText(tr("配准结论：%1\nX / Y / Z（mm）：%2 / %3 / %4\n旋转 / 俯仰 / 滚转（°）：%5 / %6 / %7\n%8\n完成时间：%9")
                                         .arg(payload.value("decision").toString(tr("未知")), number(shift, "x_mm"),
                                              number(shift, "y_mm"), number(shift, "z_mm"), number(shift, "rotate_deg"),
                                              number(shift, "pitch_deg"), number(shift, "roll_deg"),
                                              payload.value("message").toString().left(1000), payload.value("completed_at").toString()));
        m_registrationLabel->setProperty("qaTone", payload.value("decision") == "PASSED" ? "good" : "warning");
        polish(m_registrationLabel);
        record(m_registrationLabel->text());
    } else if (cmd == "TASK_DDS_FAULT_ALERT") {
        const QString text = tr("DDS 故障：%1\n故障码：%2")
                                 .arg(payload.value("fault_desc").toString(tr("未提供详情")), resultText(payload.value("therapy_fault_code")));
        if (text != m_ddsAlert->text()) record(text);
        m_ddsAlert->setText(text);
        m_ddsAlert->setProperty("qaTone", "fault");
        m_ddsAlert->show();
        polish(m_ddsAlert);
    } else if (cmd == "TASK_DDS_FAULT_RESET_RESULT") {
        if (!payload.value("success").isBool()) return;
        m_ddsResetTimer->stop();
        const bool success = payload.value("success").toBool();
        m_ddsAlert->setText(tr("DDS 复位%1：%2").arg(success ? tr("成功") : tr("失败"),
                                                        payload.value(success ? "message" : "error").toString(payload.value("message").toString()).left(2000)));
        m_ddsAlert->setProperty("qaTone", success ? "good" : "fault");
        m_ddsAlert->show();
        polish(m_ddsAlert);
        record(m_ddsAlert->text());
        updateControls();
    } else if (cmd == "TASK_SYSTEM_MODE_CHANGED") {
        if (!payload.value("mode").isString()) return;
        ++m_revision;
        m_modeConfirmed = payload.value("mode").toString() == "QA";
        if (m_pending == "mode") { m_pending.clear(); m_requestTimer->stop(); }
        m_statusKnown = false;
        if (m_modeConfirmed) applyProtocols(payload);
        else { m_protocols.clear(); m_enabled = false; m_uncertain = m_uncertain || m_running; }
        record(tr("后端模式通知：%1").arg(payload.value("mode").toString().left(80)));
        m_problem.clear();
        updateSelection();
        queryStatus();
    } else if (cmd == "TASK_QA_STATUS_SYNC") {
        if (!payload.value("is_enabled").isBool() || !payload.value("is_running").isBool()) return;
        ++m_revision;
        m_queryTimer->stop();
        m_enabled = payload.value("is_enabled").toBool();
        m_statusKnown = true;
        const bool running = payload.value("is_running").toBool();
        // Status has no task ID: an idle reply cannot cancel an unacknowledged control command.
        if (m_pending.isEmpty()) {
            m_running = running;
            if (!running) {
                m_uncertain = false;
                m_activeProtocol.clear();
                m_stageText = m_result.isEmpty() ? tr("后端空闲，等待选择预案") : tr("后端已空闲，可查看结果或选择下一预案");
                if (!m_result.contains("failed_step"))
                    m_ui->stageLabel->setText(tr("阶段：后端空闲（非完成判定）"));
            }
            else if (m_activeProtocol.isEmpty()) m_stageText = tr("后端 QA 正在执行，快照未提供预案和进度详情");
        } else if (running) m_running = true;
        m_problem.clear();
        updateControls();
    } else if (cmd == "TASK_QA_PROGRESS") {
        updateProgress(payload);
    } else if (cmd == "TASK_QA_COMPLETED") {
        finishProtocol(payload, false);
    } else if (cmd == "TASK_QA_ABORTED") {
        finishProtocol(payload, true);
    }
}

void QaWindow::updateProgress(const QJsonObject &payload)
{
    const QString id = payload.value("protocol").toString();
    if (!protocolIds.contains(id) || !payload.value("stage").isString()) return;
    if (!m_activeProtocol.isEmpty() && id != m_activeProtocol) { record(tr("忽略其他预案的进度回执")); return; }
    ++m_revision;
    m_activeProtocol = id;
    m_running = true;
    m_uncertain = false;
    if (m_pending == "execute") { m_pending.clear(); m_requestTimer->stop(); }
    m_progress = {{"protocol", id}, {"stage", payload.value("stage")},
                  {"message", payload.value("message").toString().left(2000)}};
    for (const auto &key : {"delivered_mu", "target_mu", "energy"})
        if (finiteNumber(payload.value(key)) && payload.value(key).toDouble() >= 0) m_progress.insert(key, payload.value(key));
    const QString action = stageName(payload.value("stage").toString());
    if (m_ui->stageLabel->text() != tr("阶段：%1").arg(action))
        record(tr("%1：%2").arg(protocolName(id), action));
    m_stageText = action + " " + payload.value("message").toString().left(1000);
    m_ui->activeProtocolLabel->setText(tr("执行预案：%1").arg(protocolName(id)));
    m_ui->stageLabel->setText(tr("阶段：%1").arg(action));
    const QString stage = payload.value("stage").toString();
    if (stage == "WAITING_LOGIN") m_authenticated = false;
    m_ui->stageLabel->setProperty("qaTone", stage == "BEAM_DELIVERING" || stage == "BEAM_IN_PROGRESS" ? "warning" : "unknown");
    polish(m_ui->stageLabel);
    m_ui->muLabel->setText(tr("累计 MU / 目标：%1 / %2").arg(number(m_progress, "delivered_mu"), number(m_progress, "target_mu")));
    m_ui->qaEnergyLabel->setText(tr("回读能量：%1").arg(number(m_progress, "energy")));
    const double delivered = m_progress.value("delivered_mu").toDouble(-1);
    const double target = m_progress.value("target_mu").toDouble(-1);
    m_ui->doseProgress->setValue(target > 0 && delivered >= 0 ? qBound(0, int(qMin(100.0, delivered / target * 100)), 100) : 0);
    m_ui->doseProgress->setFormat(target > 0 && delivered >= 0 ? (delivered > target ? tr("超过目标 MU，需核实") : QStringLiteral("%p%")) : tr("等待有效剂量反馈"));
    if (m_pending == "abort") m_stageText = tr("中止请求待确认；最新反馈：%1").arg(m_stageText);
    updateControls();
}

void QaWindow::finishProtocol(const QJsonObject &payload, bool aborted)
{
    const QString id = payload.value("protocol").toString();
    if (aborted) { if (payload.value("status").toString() != "ABORTED") return; }
    else if (!protocolIds.contains(id) || !payload.value("success").isBool()
             || (!m_activeProtocol.isEmpty() && id != m_activeProtocol)) return;
    QJsonObject result = aborted ? QJsonObject{{"status", "ABORTED"}, {"reason", payload.value("reason").toString().left(2000)}}
                                 : QJsonObject{{"protocol", id}, {"success", payload.value("success")},
                                               {"summary", payload.value("summary").toString().left(2000)},
                                               {"data", knownResultData(payload.value("data").toObject())}};
    if (!aborted) {
        for (const auto &key : {"failed_step", "failed_item", "error_detail", "action_advice", "timestamp"})
            if (payload.value(key).isString()) result.insert(key, payload.value(key).toString().left(2000));
    }
    if (result == m_result && !m_running && m_pending.isEmpty() && !m_uncertain) return;
    m_result = result;
    m_ui->resultSummary->setText(aborted ? tr("后端回执：QA 已中止\n%1").arg(result.value("reason").toString())
                                         : tr("后端执行结果：%1\n%2").arg(result.value("success").toBool() ? tr("成功") : tr("失败"), result.value("summary").toString()));
    m_ui->resultSummary->setProperty("qaTone", aborted ? "warning" : result.value("success").toBool() ? "good" : "fault");
    if (!aborted && !result.value("success").toBool()) {
        m_ui->resultSummary->setText(m_ui->resultSummary->text()
                                     + tr("\n失败环节：%1\n失败项目：%2\n详情：%3\n处理建议：%4")
                                          .arg(stageName(result.value("failed_step").toString()), result.value("failed_item").toString(),
                                               result.value("error_detail").toString(), result.value("action_advice").toString()));
        m_ui->stageLabel->setText(tr("失败阶段：%1").arg(stageName(result.value("failed_step").toString())));
        m_ui->stageLabel->setProperty("qaTone", "fault");
        polish(m_ui->stageLabel);
        if (result.value("failed_step") == "WAITING_LOGIN") m_authenticated = false;
    }
    polish(m_ui->resultSummary);
    const auto data = result.value("data").toObject();
    const QMap<QString, QString> titles = {{"status", tr("后端判定")}, {"shift_couch_x", tr("X 偏移（接口单位）")},
                                           {"shift_couch_y", tr("Y 偏移（接口单位）")}, {"shift_couch_z", tr("Z 偏移（接口单位）")},
                                           {"isocenter_deviation_mm", tr("等中心偏差（mm）")}, {"is_isocenter_aligned", tr("等中心对齐")},
                                           {"nominal_energy", tr("标称能量（接口单位）")}, {"target_mu", tr("目标 MU")},
                                           {"delivered_mu", tr("累计 MU")}, {"dose_deviation_pct", tr("剂量偏差（%）")},
                                           {"plan_id", tr("QA 计划 ID")}, {"gamma_pass_rate_pct", tr("Gamma 通过率（%）")},
                                           {"qa_conclusion", tr("QA 结论")}};
    m_ui->resultTable->setRowCount(data.size());
    m_ui->resultTable->setVisible(!data.isEmpty());
    int row = 0;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it, ++row) {
        m_ui->resultTable->setItem(row, 0, new QTableWidgetItem(titles.value(it.key(), it.key())));
        m_ui->resultTable->setItem(row, 1, new QTableWidgetItem(resultText(it.value())));
    }
    m_ui->resultTable->resizeRowsToContents();
    if (finiteNumber(data.value("delivered_mu")))
        m_ui->muLabel->setText(tr("累计 MU / 目标：%1 / %2").arg(number(data, "delivered_mu"), number(data, "target_mu")));
    // A completion racing with QA_ABORT does not prove the requested reset was performed.
    const bool awaitingAbort = !aborted && m_pending == "abort";
    m_running = false;
    if (!awaitingAbort) { m_pending.clear(); m_requestTimer->stop(); m_uncertain = false; }
    m_statusKnown = false;
    m_stageText = awaitingAbort ? tr("执行完成回执已到达，仍等待中止／硬件复位回执")
                  : aborted ? tr("后端报告已中止；等待状态复核") : tr("已收到完成回执；等待状态复核");
    ++m_revision;
    record(m_ui->resultSummary->text());
    AuditLogger::instance().logEvent("qa", aborted ? "QA_ABORT_ACKNOWLEDGED" : "QA_COMPLETED",
                                     "qa_protocol", id, aborted ? "info" : result.value("success").toBool() ? "success" : "failure",
                                     m_ui->resultSummary->text(), result);
    updateControls();
    queryStatus();
    if (m_resultDialog) {
        m_resultDialog->close();
        m_resultDialog->deleteLater();
        m_resultDialog.clear();
    }
    showResult();
}

void QaWindow::showResult()
{
    if (m_result.isEmpty()) return;
    if (m_resultDialog) { m_resultDialog->show(); m_resultDialog->raise(); return; }
    auto *dialog = new QDialog(this);
    dialog->setObjectName(QStringLiteral("qaResultDialog"));
    dialog->setWindowTitle(tr("后端 QA 结果"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_ShowWithoutActivating);
    auto *layout = new QVBoxLayout(dialog);
    auto *text = new QPlainTextEdit(dialog);
    text->setReadOnly(true);
    text->setPlainText(m_ui->resultSummary->text() + "\n\n"
                       + QString::fromUtf8(QJsonDocument(m_result).toJson(QJsonDocument::Indented)));
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);
    dialog->resize(600, 420);
    m_resultDialog = dialog;
    dialog->show();
}

void QaWindow::record(const QString &text)
{
    m_ui->communicationLog->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss ") + text.left(2500));
}

void QaWindow::saveSnapshot()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("保存 QA 本地记录"),
                                                      QStringLiteral("QA_%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")), tr("JSON (*.json)"));
    if (path.isEmpty()) return;
    const QJsonObject record{{"record_type", "qa_frontend_record"}, {"room_id", m_roomId},
                             {"saved_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                             {"backend_mode_confirmed", m_modeConfirmed}, {"status_known", m_statusKnown},
                             {"submitted_params", m_submittedParams}, {"progress", m_progress}, {"backend_result", m_result},
                             {"registration_result", m_registration},
                             {"telemetry_received_at_utc", m_receivedAt}, {"telemetry", m_data}};
    const QByteArray bytes = QJsonDocument(record).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        QMessageBox::warning(this, tr("保存失败"), file.errorString());
    else statusBar()->showMessage(tr("本地记录已保存；不代表临床批准或 OIS 已归档"), 5000);
}

void QaWindow::closeEvent(QCloseEvent *event)
{
    if (m_running || m_uncertain || !m_pending.isEmpty()) {
        QMessageBox message(QMessageBox::Warning, tr("QA 现场状态待核实"),
                            tr("关闭界面不会确认关束或中止后端流程。仍要仅关闭界面吗？"), QMessageBox::Yes | QMessageBox::Cancel, this);
        message.setDefaultButton(QMessageBox::Cancel);
        if (message.exec() != QMessageBox::Yes) { event->ignore(); return; }
    }
    QMainWindow::closeEvent(event);
}

void QaWindow::indicator(const QString &key, const QString &text, const QString &tone, const QString &details)
{
    auto *label = m_lamps.value(key);
    if (!label) return;
    label->setText(QStringLiteral("● ") + text);
    label->setProperty("qaTone", tone);
    label->setToolTip(details.isEmpty() ? text : details);
    label->setAccessibleName(text);
    polish(label);
}

void QaWindow::receiveTelemetry(const QJsonObject &message)
{
    if (m_roomId.isEmpty()) return;
    if (message.value("type").toString() != QStringLiteral("TELEMETRY")) return;
    const QStringList commands = {"DEVICE_TELEMETRY_STREAM", "DISPATCH_TELEMETRY_STREAM",
                                  "DEVICE_STATUS_STREAM", "DISPATCH_DEVICE_STATUS_STREAM", "DISPATCH_STATUS_STREAM", "STATUS_UPDATE"};
    if (!commands.contains(message.value("cmd").toString())) return;
    QJsonObject payload = message.value("payload").toObject();
    if (!qaRoomMatches(message, m_roomId) || !qaRoomMatches(payload, m_roomId)) return;
    if (payload.value("beam_delivery_result").isString()) {
        static const QMap<QString, int> codes = {{"BEAM_WAITING", 0}, {"BEAM_TREATING", 1},
                                                 {"BEAM_COMPLETED", 2}, {"BEAM_FAULT", 7}};
        payload.insert("beam_delivery_result", codes.value(payload.value("beam_delivery_result").toString(), -1));
    }
    QJsonObject status = firstObject(payload, {"status_update", "statusUpdate", "status", "device_status", "deviceStatus"});
    if (status.isEmpty()) status = payload;
    if (!qaRoomMatches(status, m_roomId)) return;
    // Only hardware telemetry is accepted; no task or QA conclusion is inferred from it.
    const QStringList keys = {"beam_on", "beam_delivery_result", "current_dose", "sum_dose",
                              "layer_sum_dose", "dose_rate", "current_layer", "current_point", "next_layer", "therapy_id",
                              "interlock_ok", "dds_ilk_en_rb", "accel_ready", "g01_homed", "g02_homed",
                              "therapy_fault_code", "therapy_fault_info", "point_fault_code", "layer_fault_code"};
    bool updated = false;
    for (const QString &key : keys) {
        if (!payload.contains(key)) continue;
        const QJsonValue value = payload.value(key);
        const bool boolean = key == "beam_on" || key == "interlock_ok" || key == "dds_ilk_en_rb"
                             || key == "accel_ready" || key == "g01_homed" || key == "g02_homed";
        const bool integer = key == "beam_delivery_result" || key == "current_layer" || key == "current_point"
                             || key == "next_layer" || key.endsWith("_code") || key == "therapy_fault_info";
        const double number = value.toDouble();
        const bool validNumber = value.isDouble() && std::isfinite(number)
                                 && (!integer || (std::trunc(number) == number && number >= std::numeric_limits<int>::min()
                                                  && number <= std::numeric_limits<int>::max()));
        const bool valid = boolean ? value.isBool() : key == "therapy_id" ? value.isString() : validNumber;
        if (valid) {
            m_data.insert(key, value);
            m_updated.insert(key, QDateTime::currentMSecsSinceEpoch());
        } else {
            m_data.remove(key);
            m_updated.remove(key);
        }
        updated = true;
    }
    for (const QString &key : {QStringLiteral("couch"), QStringLiteral("dr"), QStringLiteral("igrt"),
                               QStringLiteral("filter"), QStringLiteral("plc")}) {
        if (key == QStringLiteral("dr") && !RoomCapabilities::hasDrLift(m_roomId)) continue;
        QJsonObject device;
        if (key == QStringLiteral("filter"))
            device = firstObject(status, {"ridge_filter_stand", "ridgeFilterStand", "filter_state", "filter"});
        else if (key == QStringLiteral("couch"))
            device = firstObject(status, {"couch", "couch_status", "couch_state"});
        else if (key == QStringLiteral("igrt"))
            device = firstObject(status, {"igrt", "igrt_status", "igrt_state"});
        else
            device = firstObject(status, {key, key + QStringLiteral("_state")});
        if (!device.isEmpty()) {
            device.insert("connected", TelemetryValue::binaryFlag(device.value("connected")));
            m_data.insert(key, device);
            m_updated.insert(key, QDateTime::currentMSecsSinceEpoch());
            updated = true;
        }
    }
    if (!updated) return;
    m_receivedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    m_fields.value("timestamp")->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_timeout->start();
    refresh();
}

void QaWindow::refresh()
{
    for (const QString &key : {QStringLiteral("couch"), QStringLiteral("dr"), QStringLiteral("igrt"),
                               QStringLiteral("filter"), QStringLiteral("plc")}) {
        const QJsonValue connected = m_data.value(key).toObject().value("connected");
        indicator(key, connected.isBool() ? (connected.toBool() ? tr("已连接") : tr("未连接")) : tr("未知"),
                  connected.isBool() ? (connected.toBool() ? "good" : "fault") : "unknown",
                  tr("仅显示通信连接反馈，不代表设备就绪或安全。"));
    }
    const auto flag = [this](const QString &field, const QString &key, const QString &yes, const QString &no) {
        const QJsonValue v = m_data.value(field);
        indicator(key, v.isBool() ? (v.toBool() ? yes : no) : tr("未知"),
                  v.isBool() ? (v.toBool() ? "good" : "warning") : "unknown");
    };
    flag("g01_homed", "g01", tr("到位"), tr("未到位"));
    flag("g02_homed", "g02", tr("到位"), tr("未到位"));
    flag("accel_ready", "accelerator", tr("就绪"), tr("未就绪"));
    const QJsonValue ok = m_data.value("interlock_ok"), enabled = m_data.value("dds_ilk_en_rb");
    if (ok.isBool() && !ok.toBool()) indicator("dds", tr("联锁触发"), "fault");
    else if (enabled.isBool() && !enabled.toBool()) indicator("dds", tr("保护未使能"), "warning");
    else if (ok.isBool() && enabled.isBool()) indicator("dds", tr("正常"), "good");
    else indicator("dds", tr("未知"), "unknown");
    const QJsonValue beam = m_data.value("beam_on");
    m_fields.value("beam")->setText(beam.isBool() ? (beam.toBool() ? tr("供束中") : tr("未供束")) : tr("未知"));
    m_fields.value("beam")->setProperty("qaTone", beam.isBool() && beam.toBool() ? "warning" : "unknown");
    polish(m_fields.value("beam"));
    const QMap<int, QString> states = {{0, tr("等待")}, {1, tr("照射中")},
                                       {2, tr("正常完成（设备反馈，非 QA 判定）")}, {7, tr("出束故障")}};
    const QJsonValue result = m_data.value("beam_delivery_result");
    const int code = result.isDouble() && (result.toDouble() == 0 || result.toDouble() == 1
                                           || result.toDouble() == 2 || result.toDouble() == 7) ? result.toInt(-1) : -1;
    m_fields.value("result")->setText(states.value(code, tr("未知")));
    QString details = states.value(code, tr("等待遥测"));
    if (code == 7) {
        const QStringList types = {tr("无"), tr("急停"), tr("层扫描故障")};
        const QStringList infos = {tr("无"), tr("急停"), tr("能量异常"), tr("剂量超限"), tr("位置偏离"), tr("点剂量累积超时")};
        const auto decode = [this](const QString &key, const QStringList &names) {
            const QJsonValue v = m_data.value(key);
            const int i = v.isDouble() ? v.toInt(-1) : -1;
            return i >= 0 && i < names.size() ? names.at(i) : tr("未知");
        };
        details += tr("\n治疗故障：%1\n详情：%2\n治疗码：%3 / 详情码：%4 / 点码：%5 / 层码：%6")
                       .arg(decode("therapy_fault_code", types), decode("therapy_fault_info", infos),
                            number(m_data, "therapy_fault_code"), number(m_data, "therapy_fault_info"),
                            number(m_data, "point_fault_code"), number(m_data, "layer_fault_code"));
    }
    indicator("fault", code == 7 ? tr("故障") : states.contains(code) ? tr("未报告故障") : tr("未知"),
              code == 7 ? "fault" : states.contains(code) ? "good" : "unknown", details);
    for (const QString &key : {QStringLiteral("current_dose"), QStringLiteral("sum_dose"), QStringLiteral("layer_sum_dose"),
                               QStringLiteral("dose_rate"), QStringLiteral("current_layer"), QStringLiteral("current_point"), QStringLiteral("next_layer")})
        m_fields.value(key)->setText(number(m_data, key));
    m_fields.value("therapy_id")->setText(m_data.value("therapy_id").toString(QStringLiteral("-")));
    const QJsonObject couch = m_data.value("couch").toObject();
    const QStringList coordinates = {"x", "y", "z", "a", "b", "c"};
    const bool connected = couch.value("connected").isBool() && couch.value("connected").toBool();
    for (int i = 0; i < coordinates.size(); ++i)
        m_positions->item(0, i)->setText(connected ? number(couch, coordinates.at(i)) : tr("未知"));
}

void QaWindow::clearTelemetry()
{
    if (m_timeout) m_timeout->stop();
    m_data = QJsonObject();
    m_updated.clear();
    m_receivedAt.clear();
    for (auto it = m_lamps.cbegin(); it != m_lamps.cend(); ++it) indicator(it.key(), tr("未知"), "unknown");
    refresh();
    m_fields.value("timestamp")->setText(tr("等待遥测"));
}
