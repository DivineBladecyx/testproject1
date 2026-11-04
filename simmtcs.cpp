#include "simmtcs.h"
#include "ui_simmtcs.h"

#include "logindialog.h"
#include "websocketclient.h"
#include "thememanager.h"
#include "todayschedule.h"
#include "DataProvider.h"

#include <QMessageBox>
#include <QStatusBar>
#include <QDebug>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextEdit>
#include <QDateTime>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QQuickItem>
#include <QTimer>
#include <QLocale>
#include <QDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QSize>

SimmTcs::SimmTcs(WebSocketClient *webSocketClient,
                 const QString &username,
                 const QString &authToken,
                 const QJsonObject &userInfo,
                 const QJsonObject &loginResponse,
                 QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SimmTcs)
    , m_settings(new QSettings("config/app_config.ini", QSettings::IniFormat, this))
    , m_webSocketClient(webSocketClient)
    , m_currentUsername(username)
    , m_authToken(authToken)
    , m_userInfo(userInfo)
{
    ui->setupUi(this);

    initUiLayoutTuning();
    initTables();
    const QJsonObject payload = loginResponse.value("payload").toObject();
    m_roomId = payload.value("room_id").toString("Unknown");

    QJsonArray entries = payload.value("entries").toArray(
        payload.value("data").toArray());

    if (entries.isEmpty()) {
        entries = loginResponse.value("entries").toArray(
            loginResponse.value("data").toArray());
    }

    updateTodaySchedule(entries);
    initStatusIndicators();
    loadMiddlewareSettings();

    m_settings->setValue("username", username);
    m_settings->setValue("auth_token", authToken);

    m_bedController = new BedController(this);

    initTheme();
    initWebSocket();
    initBedView();
    initStepView();
    setTreatmentState(TreatmentState::Idle, "init");
    initConnectionState();
    updateUserInfoUi();
    initClock();
}

SimmTcs::~SimmTcs()
{
    delete ui;
}
void SimmTcs::initUiLayoutTuning()
{
    resize(1600, 980);
if (ui->mainGrid) {
        ui->mainGrid->setColumnStretch(0, 5);
        ui->mainGrid->setColumnStretch(1, 4);
        ui->mainGrid->setColumnStretch(2, 4);
        ui->mainGrid->setColumnStretch(3, 3);
        ui->mainGrid->setColumnStretch(4, 4);

        ui->mainGrid->setRowStretch(0, 0);
        ui->mainGrid->setRowStretch(1, 0);
        ui->mainGrid->setRowStretch(2, 1);
        ui->mainGrid->setRowStretch(3, 0);
        ui->mainGrid->setRowStretch(4, 0);

        ui->mainGrid->setHorizontalSpacing(10);
        ui->mainGrid->setVerticalSpacing(10);
        ui->mainGrid->setContentsMargins(10, 10, 10, 10);
    }

    for (QGroupBox *box : {ui->groupBox_8, ui->groupBox_2, ui->groupBox_5, ui->groupBox_info, ui->groupBox}) {
        if (box) {
            box->setMinimumHeight(110);
            box->setMaximumHeight(124);
            box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    ui->groupBox_8->setMinimumWidth(220);
    ui->groupBox_2->setMinimumWidth(210);
    ui->groupBox_5->setMinimumWidth(190);
    ui->groupBox_info->setMinimumWidth(260);
    ui->groupBox->setMinimumWidth(420);

    ui->groupBox_10->setMinimumWidth(430);
    ui->groupBox_10->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->groupBox_6->setMinimumHeight(190);
    ui->groupBox_6->setMaximumHeight(220);
    ui->groupBox_6->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->groupBox_3->setMinimumWidth(340);
    ui->groupBox_3->setMaximumWidth(430);
    ui->groupBox_3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->groupBox_4->setMinimumHeight(108);
    ui->groupBox_4->setMaximumHeight(124);
    ui->groupBox_4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->groupBox_12->setMinimumWidth(360);
    ui->groupBox_12->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->groupBox_12->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->comboBox->setMinimumSize(150, 40);
    ui->comboBox->setMaximumHeight(40);

    ui->todays_pushButton->setMinimumSize(132, 40);
    ui->theme_pushButton->setMinimumSize(132, 40);
    ui->logout_pushButton->setMinimumSize(108, 40);
    ui->todays_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/schedule_action.svg")));
    ui->theme_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/theme.svg")));
    ui->logout_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/logout.svg")));

    for (QPushButton *button : {ui->todays_pushButton, ui->theme_pushButton, ui->logout_pushButton}) {
        if (button) {
            button->setIconSize(QSize(18, 18));
        }
    }

    ui->confirmPlan_pushButton->setMinimumSize(132, 38);
    ui->confirmPlan_cancel_pushButton->setMinimumSize(132, 38);

    ui->beamApply_pushButton->setFixedSize(118, 42);
    ui->beamApply_cancel_pushButton->setFixedSize(118, 42);

    ui->label_30->setMinimumHeight(34);
    ui->label_30->setAlignment(Qt::AlignCenter);

    const QString statusItemStyle = QStringLiteral(
        "QLabel {"
        "background: #F4F8FE;"
        "border: 1px solid #D4E2F2;"
        "border-radius: 7px;"
        "color: #1E4E82;"
        "font-weight: 700;"
        "padding: 3px 12px;"
        "}");

    const std::initializer_list<QPair<QLabel*, QLabel*>> statusItems = {
        {ui->label_34, ui->label_9},
        {ui->label_36, ui->label_35},
        {ui->label_37, ui->label_46},
        {ui->label_48, ui->label_47}
    };
    for (const auto &item : statusItems) {
        QLabel *statusText = item.first;
        QLabel *statusDot = item.second;
        if (statusDot) {
            statusDot->hide();
        }
        if (statusText) {
            statusText->setMinimumWidth(132);
            statusText->setFixedHeight(30);
            statusText->setAlignment(Qt::AlignCenter);
            statusText->setTextFormat(Qt::RichText);
            statusText->setText(QStringLiteral("<span style='color:#30B27A;'>&#9679;</span>&nbsp;%1").arg(statusText->text()));
            statusText->setStyleSheet(statusItemStyle);
        }
    }

    if (ui->label_icon_doctor) {
        ui->label_icon_doctor->setFixedSize(18, 18);
        ui->label_icon_doctor->setStyleSheet("image: url(:/icons/icons/therapist_new.svg);");
    }

    if (ui->horizontalLayout) {
        ui->horizontalLayout->setContentsMargins(10, 8, 10, 8);
        ui->horizontalLayout->setSpacing(10);
    }

    if (ui->verticalLayout_plan) {
        ui->verticalLayout_plan->setContentsMargins(10, 10, 10, 10);
        ui->verticalLayout_plan->setSpacing(8);
    }

    if (ui->horizontalLayout_7) {
        ui->horizontalLayout_7->setContentsMargins(36, 0, 36, 0);
        ui->horizontalLayout_7->setSpacing(12);
    }

    if (ui->horizontalLayout_6) {
        ui->horizontalLayout_6->setContentsMargins(8, 8, 8, 8);
        ui->horizontalLayout_6->setSpacing(0);
    }

    if (ui->horizontalLayout_4) {
        ui->horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        ui->horizontalLayout_4->setSpacing(0);
    }

    if (ui->verticalLayout_ops) {
        ui->verticalLayout_ops->setContentsMargins(12, 10, 12, 12);
        ui->verticalLayout_ops->setSpacing(8);
}

    if (ui->horizontalLayout_3) {
        ui->horizontalLayout_3->setContentsMargins(12, 2, 12, 2);
        ui->horizontalLayout_3->setSpacing(12);
    }
    if (ui->verticalLayout_statusBox) {
        ui->verticalLayout_statusBox->setContentsMargins(10, 6, 10, 8);
        ui->verticalLayout_statusBox->setSpacing(4);
    }

    constexpr int bedPositionFieldWidth = 212;

    if (ui->verticalLayout_bedBox) {
        ui->verticalLayout_bedBox->setContentsMargins(10, 8, 10, 10);
        ui->verticalLayout_bedBox->setSpacing(4);
    }
    if (ui->gridLayout_2) {
        ui->gridLayout_2->setContentsMargins(12, 0, 12, 10);
        ui->gridLayout_2->setHorizontalSpacing(10);
        ui->gridLayout_2->setVerticalSpacing(7);
        ui->gridLayout_2->setColumnMinimumWidth(0, 28);
        ui->gridLayout_2->setColumnMinimumWidth(1, bedPositionFieldWidth);
        ui->gridLayout_2->setColumnMinimumWidth(2, bedPositionFieldWidth);
        ui->gridLayout_2->setColumnStretch(0, 0);
        ui->gridLayout_2->setColumnStretch(1, 1);
        ui->gridLayout_2->setColumnStretch(2, 1);
        for (int row = 0; row <= 6; ++row) {
            ui->gridLayout_2->setRowStretch(row, 0);
        }
        ui->gridLayout_2->setRowStretch(7, 1);
        ui->gridLayout_2->setAlignment(Qt::AlignTop);
    }

    if (ui->verticalLayout_preview) {
        ui->verticalLayout_preview->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout_preview->setSpacing(0);
    }

    if (ui->verticalLayout_previewPanel) {
        ui->verticalLayout_previewPanel->setContentsMargins(10, 8, 10, 10);
        ui->verticalLayout_previewPanel->setSpacing(8);
    }

    if (ui->panelFrame) {
        ui->panelFrame->setMinimumWidth(460);
        ui->panelFrame->setMaximumWidth(QWIDGETSIZE_MAX);
        ui->panelFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    if (ui->horizontalLayout_bed) {
        ui->horizontalLayout_bed->setContentsMargins(0, 0, 0, 0);
        ui->horizontalLayout_bed->setSpacing(0);
    }

    if (ui->qmlStepContainer) {
        ui->qmlStepContainer->setMinimumHeight(140);
        ui->qmlStepContainer->setMaximumHeight(150);
        ui->qmlStepContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    if (ui->tableWidget_4) {
        ui->tableWidget_4->setAlternatingRowColors(true);
        ui->tableWidget_4->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_4->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableWidget_4->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget_4->setShowGrid(true);
        ui->tableWidget_4->verticalHeader()->setVisible(false);
        ui->tableWidget_4->horizontalHeader()->setStretchLastSection(true);
        ui->tableWidget_4->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->tableWidget_4->horizontalHeader()->setMinimumHeight(34);
        ui->tableWidget_4->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    }

    if (ui->tableWidget) {
        ui->tableWidget->setAlternatingRowColors(true);
        ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget->verticalHeader()->setVisible(false);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    if (ui->tableWidget_2) {
        ui->tableWidget_2->setAlternatingRowColors(true);
        ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget_2->verticalHeader()->setVisible(false);
        ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    if (ui->tableWidget_3) {
        ui->tableWidget_3->setAlternatingRowColors(true);
        ui->tableWidget_3->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_3->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget_3->verticalHeader()->setVisible(false);
        ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    for (QLabel *header : {ui->label_38, ui->label_39}) {
        if (header) {
            header->setFixedSize(bedPositionFieldWidth, 24);
            header->setAlignment(Qt::AlignCenter);
            header->setStyleSheet(QStringLiteral("color: #4A5F78; font-weight: 700; background: transparent;"));
            ui->gridLayout_2->setAlignment(header, Qt::AlignCenter);
        }
    }

    for (QLabel *axisLabel : {ui->label_40, ui->label_41, ui->label_42,
                              ui->label_43, ui->label_44, ui->label_45}) {
        if (axisLabel) {
            axisLabel->setFixedWidth(28);
            axisLabel->setAlignment(Qt::AlignCenter);
            axisLabel->setStyleSheet(QStringLiteral("color: #005BAC; font-weight: 700; background: transparent;"));
            ui->gridLayout_2->setAlignment(axisLabel, Qt::AlignCenter);
        }
    }

    const QString positionValueStyle = QStringLiteral(
        "QLabel {"
        "background: #EAF3FF;"
        "border: 1px solid #D2E2F4;"
        "border-radius: 6px;"
        "color: #0E2847;"
        "padding: 0 8px;"
        "}");

    const QString positionEditStyle = QStringLiteral(
        "QLineEdit {"
        "background: #FFFFFF;"
        "border: 1px solid #D2E2F4;"
        "border-radius: 6px;"
        "color: #0E2847;"
        "padding: 0 8px;"
        "selection-background-color: #2F80ED;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid #7FB2F0;"
        "}");

    for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz}) {
        if (label) {
            label->setFixedSize(bedPositionFieldWidth, 34);
            label->setAlignment(Qt::AlignCenter);
            if (label->text() == QStringLiteral("0.0")) {
                label->setText(QStringLiteral("0.00"));
            }
            label->setStyleSheet(positionValueStyle);
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->gridLayout_2->setAlignment(label, Qt::AlignCenter);
        }
    }

    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) {
        if (edit) {
            edit->setFixedSize(bedPositionFieldWidth, 34);
            edit->setAlignment(Qt::AlignCenter);
            if (edit->text().isEmpty()) {
                edit->setText(QStringLiteral("0.00"));
            }
            edit->setStyleSheet(positionEditStyle);
            edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->gridLayout_2->setAlignment(edit, Qt::AlignCenter);
        }
    }
}


void SimmTcs::initTables()
{
    auto setupTable = [](QTableWidget *table) {
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setShowGrid(true);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setMinimumHeight(36);
    };

    setupTable(ui->tableWidget_4);
    setupTable(ui->tableWidget);
    setupTable(ui->tableWidget_2);
    setupTable(ui->tableWidget_3);

    ui->tableWidget_4->setColumnCount(4);
    ui->tableWidget_4->setHorizontalHeaderLabels(
        QStringList() << tr("项目")
                      << tr("计划参数")
                      << tr("治疗信息")
                      << tr("状态"));

    ui->tableWidget_4->setRowCount(6);
    ui->tableWidget_4->setVerticalHeaderLabels(
        QStringList() << tr("CBCT")
                      << tr("2D-kV")
                      << tr("Plan1")
                      << tr("Plan2")
                      << tr("Plan3")
                      << tr("Plan4"));

    ui->tableWidget_4->setWordWrap(true);
    ui->tableWidget_4->setTextElideMode(Qt::ElideNone);
    ui->tableWidget_4->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_4->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_4->horizontalHeader()->setMinimumHeight(36);
    ui->tableWidget_4->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    for (int row = 0; row < ui->tableWidget_4->rowCount(); ++row) {
        for (int col = 0; col < ui->tableWidget_4->columnCount(); ++col) {
            auto *item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget_4->setItem(row, col, item);
        }
    }

    ui->tableWidget->setColumnCount(1);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << tr("射野 MU"));

    ui->tableWidget_2->setColumnCount(1);
    ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << tr("场实现"));

    ui->tableWidget_3->setColumnCount(1);
    ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << tr("场实现"));
}

void SimmTcs::initStatusIndicators()
{
    const QString greenStyle = QStringLiteral("background-color: #30B27A; border: 2px solid #E8FFF5; border-radius: 7px;");
    ui->label_9->setStyleSheet(greenStyle);
    ui->label_35->setStyleSheet(greenStyle);
    ui->label_46->setStyleSheet(greenStyle);
    ui->label_47->setStyleSheet(greenStyle);

    ui->label_9->setToolTip(tr("全局报警正常"));
    ui->label_35->setToolTip(tr("系统连接正常"));
    ui->label_46->setToolTip(tr("束流状态正常"));
    ui->label_47->setToolTip(tr("床状态正常"));
}

void SimmTcs::updateUserInfoUi()
{
    // 按你后端实际字段名改
    const QString displayName =
        m_userInfo.value("display_name").toString(
            m_userInfo.value("name").toString(m_currentUsername));

    const QString role = m_userInfo.value("role").toString("普通用户");

    // 这些控件名请和 .ui 里一致

    ui->label_6->setText(tr("姓名：%1").arg(displayName));
    ui->label_user->setText(tr("角色：%1").arg(role));

    ui->label_7->setText(tr("治疗室：%1").arg(m_roomId));
    ui->label_room->setText(tr("状态：可用"));
}
void SimmTcs::initTheme()
{
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &SimmTcs::onThemeChanged, Qt::UniqueConnection);

    onThemeChanged(ThemeManager::instance().currentTheme());
}

void SimmTcs::initWebSocket()
{
    connectWebSocketSignals();
}

void SimmTcs::initBedView()
{
    m_bedView = new QQuickWidget(this);
    m_bedView->rootContext()->setContextProperty("bedController", m_bedController);
    m_bedView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_bedView->setSource(QUrl("qrc:/models/Bed3DView.qml"));

    ui->horizontalLayout_bed->addWidget(m_bedView);
}

void SimmTcs::initStepView()
{
    m_stepView = new QQuickWidget(ui->qmlStepContainer);
    m_stepView->setClearColor(Qt::transparent);
    m_stepView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_stepView->rootContext()->setContextProperty("themeManager", &ThemeManager::instance());
    m_stepView->setSource(QUrl("qrc:/models/StepProgressBar.qml"));

    if (ui->qmlStepContainer->layout()) {
        ui->qmlStepContainer->layout()->addWidget(m_stepView);
    } else {
        auto *layout = new QVBoxLayout(ui->qmlStepContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_stepView);
    }

    if (QObject *root = m_stepView->rootObject()) {
        root->setProperty("stepTitles", QStringList{"预览", "准备", "就绪", "治疗中", "记录"});
    }

    updateStepBar();
}

void SimmTcs::initConnectionState()
{
    if (!m_webSocketClient) {
        qWarning() << "WebSocketClient 为空";
        return;
    }

    if (m_webSocketClient->isConnected()) {
        onWebSocketConnected();
        return;
    }

    const bool autoConnect = m_settings->value("middleware_auto_connect", true).toBool();
    if (autoConnect) {
        connectToMiddlewareWebSocket();
    }
}
void SimmTcs::updateTodaySchedule(const QJsonArray &entries)
{
    m_todayScheduleEntries = entries;

    qDebug() << "缓存今日排程数量:" << m_todayScheduleEntries.size();

    if (m_todayScheduleDialog) {
        m_todayScheduleDialog->updateScheduleEntries(m_todayScheduleEntries);
    }
}
void SimmTcs::connectWebSocketSignals()
{
    if (!m_webSocketClient)
        return;

    connect(m_webSocketClient, &WebSocketClient::connected,
            this, &SimmTcs::onWebSocketConnected, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::disconnected,
            this, &SimmTcs::onWebSocketDisconnected, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::errorOccurred,
            this, &SimmTcs::onWebSocketError, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::textMessageReceived,
            this, [](const QString &message) {
                qDebug() << "收到文本消息:" << message;
            });

    connect(m_webSocketClient, &WebSocketClient::dataReceived,
            this, &SimmTcs::onWebSocketDataReceived, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::dataSent,
            this, &SimmTcs::onWebSocketDataSent, Qt::UniqueConnection);
}

void SimmTcs::showLoginDialog()
{
    if (loginDialog) {
        delete loginDialog;
        loginDialog = nullptr;
    }
    hide();

    loginDialog = new LoginDialog(m_middlewareUrl, LoginDialog::AuthMode::Login, this);

    connect(loginDialog, &LoginDialog::loginSuccess,
            this, &SimmTcs::onLoginSuccess, Qt::UniqueConnection);

    connect(loginDialog, &LoginDialog::loginFailed,
            this, &SimmTcs::onLoginFailed, Qt::UniqueConnection);

    connect(loginDialog, &LoginDialog::rejected,
            this, &QWidget::close, Qt::UniqueConnection);

    loginDialog->exec();
}

void SimmTcs::onThemeChanged(Theme newTheme)
{
    Q_UNUSED(newTheme)

    for (QWidget *widget : findChildren<QWidget*>()) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }

    style()->unpolish(this);
    style()->polish(this);
    update();
}

void SimmTcs::connectToMiddlewareWebSocket()
{
    if (!m_webSocketClient) {
        qWarning() << "WebSocketClient 为空";
        return;
    }

    if (m_webSocketClient->isConnected()) {
        qDebug() << "WebSocket 已连接，跳过重复连接";
        return;
    }

    if (m_authToken.isEmpty() || m_wsMiddlewareUrl.isEmpty()) {
        qWarning() << "认证信息或 WebSocket 地址为空";
        statusBar()->showMessage("连接参数不完整", 5000);
        return;
    }

    const QUrl url(m_wsMiddlewareUrl);
    if (!url.isValid() || (url.scheme() != "ws" && url.scheme() != "wss")) {
        qWarning() << "无效的 WebSocket 地址:" << m_wsMiddlewareUrl;
        statusBar()->showMessage("网关地址无效", 5000);
        return;
    }

    QJsonObject authParams;
    authParams["token"] = m_authToken;
    authParams["client_type"] = "desktop";
    if (!m_currentUsername.isEmpty()) {
        authParams["username"] = m_currentUsername;
    }

    m_webSocketClient->setMiddlewareAuthParams(authParams);
    m_webSocketClient->setAutoReconnect(true, 5000);

    statusBar()->showMessage("正在连接网关...", 3000);
    qDebug() << "开始连接 WebSocket:" << m_wsMiddlewareUrl;

    m_webSocketClient->connectToServer(m_wsMiddlewareUrl);
}

void SimmTcs::onLoginSuccess(const QString &username, const QString &authToken, WebSocketClient *client, const QJsonObject &loginResponse)
{
    m_currentUsername = username;
    m_authToken = authToken;
    const QJsonObject payload = loginResponse.value("payload").toObject();

    m_todayScheduleEntries = payload.value("entries").toArray();

    if (m_todayScheduleEntries.isEmpty()) {
        m_todayScheduleEntries = loginResponse.value("entries").toArray();
    }

    qDebug() << "登录成功携带今日排程数量:" << m_todayScheduleEntries.size();

    if (m_webSocketClient) {
        disconnect(m_webSocketClient, nullptr, this, nullptr);
        m_webSocketClient->disconnectFromServer();
        m_webSocketClient = nullptr;
    }

    m_webSocketClient = client;
    m_webSocketClient->setParent(this);

    connectWebSocketSignals();

    m_settings->setValue("username", username);
    m_settings->setValue("auth_token", authToken);
    m_settings->setValue("token_expire", QDateTime::currentDateTime().addSecs(3600));
    m_settings->sync();

    show();

    if (m_webSocketClient->isConnected()) {
        onWebSocketConnected();
    } else {
        connectToMiddlewareWebSocket();
    }

    if (loginDialog) {
        loginDialog->deleteLater();
        loginDialog = nullptr;
    }
}

void SimmTcs::onLoginFailed(const QString &error)
{
    qDebug() << "登录失败:" << error;

    static int failedAttempts = 0;
    ++failedAttempts;

    if (failedAttempts >= 3) {
        QMessageBox::critical(this, "登录失败",
                              "连续登录失败次数过多，请检查网络连接和网关地址");
        failedAttempts = 0;
    }
}

void SimmTcs::onWebSocketConnected()
{
    QJsonObject connectMsg = {
        {"type", "client_connect"},
        {"client", "SIMM-TCS-Desktop"},
        {"version", "1.0.0"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"username", m_currentUsername}
    };

    qDebug() << "准备发送 connectMsg，内容：" << connectMsg;
    m_webSocketClient->sendData(connectMsg);
    statusBar()->showMessage("已连接到网关", 3000);
    applyTreatmentStateToUi();
}

void SimmTcs::onWebSocketDisconnected()
{
    qDebug() << "WebSocket 已断开";
    statusBar()->showMessage("WebSocket 已断开", 3000);
}

void SimmTcs::onWebSocketError(const QString &error)
{
    qDebug() << "WebSocket 错误:" << error;
    statusBar()->showMessage("连接错误: " + error, 5000);

    if (error.contains("401") ||
        error.contains("auth", Qt::CaseInsensitive) ||
        error.contains("token", Qt::CaseInsensitive)) {
        QMessageBox::warning(this, "认证错误", "网关认证失败，请重新登录");
    }
}

void SimmTcs::onWebSocketDataReceived(const QJsonObject &data)
{
    const QString type = data.value("type").toString();

    if (type == "RSP") {
        handleRspMessage(data);
    } else if (type == "TELEMETRY") {
        handleTelemetryMessage(data);
    } else if (type == "TASK") {
        handleTaskMessage(data);
    } else if (type == "TASK_ERROR_STOPPED") {
        handleTaskErrorStopped(data);
    } else {
        qDebug() << "未处理的消息类型:" << type << data;
    }
}
void SimmTcs::handleRspMessage(const QJsonObject &data)
{
    Q_UNUSED(data);
    // 普通响应直接忽略
}
void SimmTcs::handleTelemetryMessage(const QJsonObject &data)
{
    if (data.value("cmd").toString() != "DEVICE_TELEMETRY_STREAM") {
        return;
    }

    const QJsonObject payload = data.value("payload").toObject();
    if (payload.isEmpty()) {
        return;
    }

    const QJsonObject couch = payload.value("couch").toObject();
    if (couch.isEmpty()) {
        return;
    }

    ui->label_posa->setText(QString::number(couch.value("a").toDouble(), 'f', 2));
    ui->label_posb->setText(QString::number(couch.value("b").toDouble(), 'f', 2));
    ui->label_posc->setText(QString::number(couch.value("c").toDouble(), 'f', 2));
    ui->label_posx->setText(QString::number(couch.value("x").toDouble(), 'f', 2));
    ui->label_posy->setText(QString::number(couch.value("y").toDouble(), 'f', 2));
    ui->label_posz->setText(QString::number(couch.value("z").toDouble(), 'f', 2));
}
/*
void SimmTcs::handleTaskMessage(const QJsonObject &data)
{
    const QString cmd = data.value("cmd").toString();
    const QJsonObject payload = data.value("payload").toObject();

    if (cmd == "TASK_LOGIN_SUCCESS") {
        const QString roomId = payload.value("room_id").toString("Unknown");
        const QJsonArray entries = payload.value("entries").toArray();
        if (!m_todayScheduleDialog) {
            m_todayScheduleDialog = new TodaySchedule(this);
            m_todayScheduleDialog->setAttribute(Qt::WA_DeleteOnClose);
            m_todayScheduleDialog->setWebSocketClient(m_webSocketClient);
            connect(m_todayScheduleDialog, &QObject::destroyed, this, [this]() {
                m_todayScheduleDialog = nullptr;
            });
        }

        m_todayScheduleDialog->updateScheduleEntries(entries);
        statusBar()->showMessage(QString("登录成功，治疗室: %1").arg(roomId), 3000);
        ui->label_room->setText(roomId);
    } else if (cmd == "TASK_PLAN_CONFIRM") {
        currentStep =0;
        updateStepBar();
        const QString patientId = payload.value("patient_id").toString("Unknown");
        const QString patientName = payload.value("patient_name").toString("Unknown");
        const QString planId = payload.value("plan_id").toString("Unknown");
        const QString fractionId = payload.value("fraction_id").toString("Unknown");

        const QJsonObject planSummary = payload.value("plan_summary").toObject();
        const double targetDose = planSummary.value("target_dose").toDouble(0.0);
        const int layers = planSummary.value("layers").toInt(0);
        const int estimatedTime = planSummary.value("estimated_time").toInt(0);

        const int row = 0; // Plan1 那一行：0=CBCT, 1=2D-kV, 2=Plan1

        auto setTableItem = [this](int row, int col, const QString &text) {
            QTableWidgetItem *item = ui->tableWidget_4->item(row, col);
            if (!item) {
                item = new QTableWidgetItem;
                ui->tableWidget_4->setItem(row, col, item);
            }
            item->setText(text);
            item->setTextAlignment(Qt::AlignCenter);
        };

        setTableItem(row, 0,
                     QString("患者ID: %1\n患者姓名: %2\n计划ID: %3\n分次ID: %4")
                         .arg(patientId, patientName, planId, fractionId));

        setTableItem(row, 1,
                     QString("靶区剂量: %1 Gy").arg(targetDose, 0, 'f', 2));

        setTableItem(row, 2,
                     QString("层数: %1\n预计时间: %2 s").arg(layers).arg(estimatedTime));

        setTableItem(row, 3, "待确认");

        ui->tableWidget_4->resizeRowToContents(row);

        statusBar()->showMessage(QString("请确认患者 %1 的治疗计划").arg(patientName), 5000);

        QMessageBox::information(
            this,
            tr("计划确认"),
            tr("请确认患者 %1 的治疗计划。\n\n患者ID: %2\n计划ID: %3\n分次ID: %4\n靶区剂量: %5 Gy\n层数: %6\n预计时间: %7 秒")
                .arg(patientName)
                .arg(patientId)
                .arg(planId)
                .arg(fractionId)
                .arg(targetDose, 0, 'f', 2)
                .arg(layers)
                .arg(estimatedTime)
            );
    }
   else if (cmd == "TASK_CAN_APPLY_BEAM") {
        statusBar()->showMessage("自动摆位完成，可申请束流", 5000);
        currentStep = 2;
        updateStepBar();
        QMessageBox::information(
            this,
            tr("束流已就绪"),
            tr("束流已就绪，请点击申请束流。")
            );
    }
    else if (cmd == "TASK_BEAM_READY") {

        statusBar()->showMessage("束流已就绪，准备出束", 3000);
        currentStep = 2;
        updateStepBar();


    } else if (cmd == "TASK_BEAM_ON") {
        statusBar()->showMessage("治疗已开始，正在出束", 3000);
        currentStep = 3;
        updateStepBar();
    } else if (cmd == "TASK_BEAM_FINISHED") {
        statusBar()->showMessage("治疗结束，请确认", 5000);
        currentStep = 4;
        updateStepBar();
        QJsonObject flagMessage;
        flagMessage["type"] = "TASK";
        flagMessage["cmd"] = "REPLY_BEAM_FINISHED";

        m_webSocketClient->sendData(flagMessage);

        qDebug() << "已发送 beam flag:" << flagMessage;
        statusBar()->showMessage("已发送 beam flag", 2000);

    } else if (cmd == "TASK_CHART_GENERATED") {
        statusBar()->showMessage("治疗报告已生成，流程完成", 5000);
        QString reportText;
        if (payload.value("report").isString()) {
            reportText = payload.value("report").toString();
        } else if (payload.value("report").isObject()) {
            reportText = QString::fromUtf8(
                QJsonDocument(payload.value("report").toObject()).toJson(QJsonDocument::Indented));
        } else {
            reportText = tr("治疗报告已生成，患者流程闭环。");
        }

        if (!showReportConfirmDialog(reportText)) {
            statusBar()->showMessage("报告确认已取消", 3000);
            return;
        }

        QJsonObject flagMessage;
        flagMessage["type"] = "TASK";
        flagMessage["cmd"] = "START_NEXT_TREATMENT";

        m_webSocketClient->sendData(flagMessage);
        currentStep = 0;
        updateStepBar();

    } else {
        qDebug() << "未处理的 TASK 指令:" << cmd << data;
    }
}*/
void SimmTcs::handleTaskMessage(const QJsonObject &data)
{
    const QString cmd = data.value("cmd").toString();
    const QJsonObject payload = data.value("payload").toObject();

    if (cmd == "TASK_LOGIN_SUCCESS") {
        const QString roomId = payload.value("room_id").toString("Unknown");
        const QJsonArray entries = payload.value("entries").toArray();

        updateTodaySchedule(entries);
        statusBar()->showMessage(QString("登录成功，治疗室: %1").arg(roomId), 3000);
        ui->label_room->setText(roomId);

        setTreatmentState(TreatmentState::Idle, cmd);
        return;
    }

    if (cmd == "TASK_PLAN_CONFIRM") {
        setTreatmentState(TreatmentState::PlanConfirm, cmd);

        const QString patientId = payload.value("patient_id").toString("Unknown");
        const QString patientName = payload.value("patient_name").toString("Unknown");
        const QString planId = payload.value("plan_id").toString("Unknown");
        const QString fractionId = payload.value("fraction_id").toString("Unknown");

        const QJsonObject planSummary = payload.value("plan_summary").toObject();
        const double targetDose = planSummary.value("target_dose").toDouble(0.0);
        const int layers = planSummary.value("layers").toInt(0);
        const int estimatedTime = planSummary.value("estimated_time").toInt(0);

        const int row = 0;

        auto setTableItem = [this](int row, int col, const QString &text) {
            QTableWidgetItem *item = ui->tableWidget_4->item(row, col);
            if (!item) {
                item = new QTableWidgetItem;
                ui->tableWidget_4->setItem(row, col, item);
            }
            item->setText(text);
            item->setTextAlignment(Qt::AlignCenter);
        };

        setTableItem(row, 0,
                     QString("患者ID: %1\n患者姓名: %2\n计划ID: %3\n分次ID: %4")
                         .arg(patientId, patientName, planId, fractionId));

        setTableItem(row, 1,
                     QString("靶区剂量: %1 Gy").arg(targetDose, 0, 'f', 2));

        setTableItem(row, 2,
                     QString("层数: %1\n预计时间: %2 s")
                         .arg(layers)
                         .arg(estimatedTime));

        setTableItem(row, 3, "待确认");

        ui->tableWidget_4->resizeRowToContents(row);

        QMessageBox::information(
            this,
            tr("计划确认"),
            tr("请确认患者 %1 的治疗计划。\n\n患者ID: %2\n计划ID: %3\n分次ID: %4\n靶区剂量: %5 Gy\n层数: %6\n预计时间: %7 秒")
                .arg(patientName)
                .arg(patientId)
                .arg(planId)
                .arg(fractionId)
                .arg(targetDose, 0, 'f', 2)
                .arg(layers)
                .arg(estimatedTime));

        return;
    }

    if (cmd == "TASK_CAN_APPLY_BEAM") {
        setTreatmentState(TreatmentState::ReadyToApplyBeam, cmd);

        QMessageBox::information(
            this,
            tr("可申请束流"),
            tr("自动摆位完成，可以申请束流。"));

        return;
    }

    if (cmd == "TASK_BEAM_READY") {
        setTreatmentState(TreatmentState::BeamReady, cmd);
        return;
    }

    if (cmd == "TASK_BEAM_ON") {
        setTreatmentState(TreatmentState::Treating, cmd);
        return;
    }

    if (cmd == "TASK_BEAM_FINISHED") {
        setTreatmentState(TreatmentState::BeamFinished, cmd);

        QJsonObject flagMessage;
        flagMessage["type"] = "TASK";
        flagMessage["cmd"] = "REPLY_BEAM_FINISHED";
        m_webSocketClient->sendData(flagMessage);

        return;
    }

    if (cmd == "TASK_CHART_GENERATED") {
        setTreatmentState(TreatmentState::ReportConfirm, cmd);

        QString reportText;
        if (payload.value("report").isString()) {
            reportText = payload.value("report").toString();
        } else if (payload.value("report").isObject()) {
            reportText = QString::fromUtf8(
                QJsonDocument(payload.value("report").toObject())
                    .toJson(QJsonDocument::Indented));
        } else {
            reportText = tr("治疗报告已生成，患者流程闭环。");
        }

        if (!showReportConfirmDialog(reportText)) {
            statusBar()->showMessage("报告确认已取消", 3000);
            return;
        }

        QJsonObject flagMessage;
        flagMessage["type"] = "TASK";
        flagMessage["cmd"] = "START_NEXT_TREATMENT";
        m_webSocketClient->sendData(flagMessage);

        setTreatmentState(TreatmentState::Completed, "report confirmed");
        setTreatmentState(TreatmentState::Idle, "start next treatment");

        return;
    }

    qDebug() << "未处理的 TASK 指令:" << cmd << data;
}
void SimmTcs::handleTaskErrorStopped(const QJsonObject &data)
{
    const QJsonObject payload = data.value("payload").toObject();
    const QString errorMsg = payload.value("message").toString("Unknown error");
    setTreatmentState(TreatmentState::ErrorStopped, errorMsg);

    QMessageBox::critical(this,
                          tr("流程阻断"),
                          tr("流程发生致命异常而阻断:\n%1").arg(errorMsg));
    statusBar()->showMessage("流程发生致命异常", 5000);
}

void SimmTcs::onWebSocketDataSent(const QString &message)
{
    qDebug() << "WebSocket data sent:" << message;
}

void SimmTcs::onWebSocketReconnecting()
{
    statusBar()->showMessage("正在重新连接网关...", 3000);
    qDebug() << "正在重新连接网关...";
}

void SimmTcs::loadMiddlewareSettings()
{
    if (!m_settings) {
        m_settings = new QSettings("YourCompany", "SIMM", this);
    }

    const QString savedMiddlewareUrl = m_settings->value("middleware_url").toString();
    if (!savedMiddlewareUrl.isEmpty()) {
        m_middlewareUrl = savedMiddlewareUrl;
        m_wsMiddlewareUrl = savedMiddlewareUrl;
        qDebug() << "Loaded middleware URL from settings:" << m_middlewareUrl;
    } else {
        m_middlewareUrl.clear();
        m_wsMiddlewareUrl.clear();
        qDebug() << "Using default middleware URL:" << m_middlewareUrl;
    }

    const QString authToken = m_settings->value("auth_token").toString();
    if (!authToken.isEmpty()) {
        m_authToken = authToken;
        qDebug() << "Loaded auth token from settings";
    }

    const QString username = m_settings->value("username").toString();
    if (!username.isEmpty()) {
        m_currentUsername = username;
        qDebug() << "Loaded username from settings:" << m_currentUsername;
    }

    const bool autoConnect = m_settings->value("middleware_auto_connect", true).toBool();
    const int reconnectInterval = m_settings->value("middleware_reconnect_interval", 5000).toInt();
    const int timeout = m_settings->value("middleware_timeout", 30000).toInt();

    qDebug() << "middleware settings loaded - AutoConnect:" << autoConnect
             << "ReconnectInterval:" << reconnectInterval << "ms"
             << "Timeout:" << timeout << "ms";
}



void SimmTcs::on_bed_pushButton_clicked()
{
    QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("确认操作"),
                              tr("确定要发送床数据吗？此操作需要再次确认。"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    if (!m_webSocketClient || !m_webSocketClient->isConnected()) {
        QMessageBox::warning(this, tr("错误"),
                             tr("未连接到服务器，无法发送数据。"));
        return;
    }

    bool ok1, ok2, ok3, ok4, ok5, ok6;
    const double a = ui->lineEdit_a->text().toDouble(&ok1);
    const double b = ui->lineEdit_b->text().toDouble(&ok2);
    const double c = ui->lineEdit_c->text().toDouble(&ok3);
    const double x = ui->lineEdit_x->text().toDouble(&ok4);
    const double y = ui->lineEdit_y->text().toDouble(&ok5);
    const double z = ui->lineEdit_z->text().toDouble(&ok6);


    if (!(ok1 && ok2 && ok3 && ok4 && ok5 && ok6)) {
        QMessageBox::warning(this, tr("输入错误"), tr("请输入合法的床位参数。"));
        return;
    }

    QJsonObject data;
    data["type"] = "bed_data";
    data["username"] = m_currentUsername;
    data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["value1"] = a;
    data["value2"] = b;
    data["value3"] = c;
    data["value4"] = x;
    data["value5"] = y;
    data["value6"] = z;

    m_webSocketClient->sendData(data);

    statusBar()->showMessage(tr("床数据已发送"), 2000);
    qDebug() << "床数据已发送:" << data;
}

void SimmTcs::on_todays_pushButton_clicked()
{
    if (!m_todayScheduleDialog) {
        m_todayScheduleDialog = new TodaySchedule(this);
        m_todayScheduleDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_todayScheduleDialog, &QObject::destroyed, this, [this]() {
            m_todayScheduleDialog = nullptr;
        });
    }

    qDebug() << "打开今日排程，缓存数量:" << m_todayScheduleEntries.size();
    m_todayScheduleDialog->setHeaderInfo(m_currentUsername, m_roomId);

    m_todayScheduleDialog->updateScheduleEntries(m_todayScheduleEntries);

    m_todayScheduleDialog->show();
    m_todayScheduleDialog->raise();
    m_todayScheduleDialog->activateWindow();
}

void SimmTcs::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateStepBar();
}

void SimmTcs::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void SimmTcs::updateStepBar()
{
    if (!m_stepView)
        return;

    QObject *root = m_stepView->rootObject();
    if (!root)
        return;

    root->setProperty("currentStep", treatmentStateToStep(m_treatmentState));
}
int SimmTcs::treatmentStateToStep(TreatmentState state) const
{
    switch (state) {
    case TreatmentState::Idle:
    case TreatmentState::PlanConfirm:
        return 0;   // 预览

    case TreatmentState::Preparing:
        return 1;   // 准备

    case TreatmentState::ReadyToApplyBeam:
    case TreatmentState::BeamReady:
        return 2;   // 就绪

    case TreatmentState::Treating:
        return 3;   // 治疗中

    case TreatmentState::BeamFinished:
    case TreatmentState::ReportConfirm:
    case TreatmentState::Completed:
        return 4;   // 记录

    case TreatmentState::ErrorStopped:
        return treatmentStateToStep(m_previousTreatmentState);
    }

    return 0;
}

QString SimmTcs::treatmentStateText(TreatmentState state) const
{
    switch (state) {
    case TreatmentState::Idle:
        return tr("等待任务");
    case TreatmentState::PlanConfirm:
        return tr("计划确认");
    case TreatmentState::Preparing:
        return tr("准备中");
    case TreatmentState::ReadyToApplyBeam:
        return tr("可申请束流");
    case TreatmentState::BeamReady:
        return tr("束流已就绪");
    case TreatmentState::Treating:
        return tr("治疗中");
    case TreatmentState::BeamFinished:
        return tr("治疗结束");
    case TreatmentState::ReportConfirm:
        return tr("报告确认");
    case TreatmentState::Completed:
        return tr("流程完成");
    case TreatmentState::ErrorStopped:
        return tr("异常中止");
    }

    return tr("未知状态");
}
bool SimmTcs::canTransitTo(TreatmentState from, TreatmentState to) const
{
    if (from == to) {
        return true;
    }

    // 异常中止可以从任何状态进入
    if (to == TreatmentState::ErrorStopped) {
        return true;
    }

    // 异常状态下，只允许回到空闲或重新进入计划确认
    if (from == TreatmentState::ErrorStopped) {
        return to == TreatmentState::Idle || to == TreatmentState::PlanConfirm;
    }

    switch (from) {
    case TreatmentState::Idle:
        return to == TreatmentState::PlanConfirm;

    case TreatmentState::PlanConfirm:
        return to == TreatmentState::Preparing;

    case TreatmentState::Preparing:
        return to == TreatmentState::ReadyToApplyBeam;

    case TreatmentState::ReadyToApplyBeam:
        return to == TreatmentState::BeamReady ||
               to == TreatmentState::Treating;

    case TreatmentState::BeamReady:
        return to == TreatmentState::Treating;

    case TreatmentState::Treating:
        return to == TreatmentState::BeamFinished;

    case TreatmentState::BeamFinished:
        return to == TreatmentState::ReportConfirm;

    case TreatmentState::ReportConfirm:
        return to == TreatmentState::Completed ||
               to == TreatmentState::Idle;

    case TreatmentState::Completed:
        return to == TreatmentState::Idle ||
               to == TreatmentState::PlanConfirm;

    case TreatmentState::ErrorStopped:
        return to == TreatmentState::Idle ||
               to == TreatmentState::PlanConfirm;
    }

    return false;
}
void SimmTcs::setTreatmentState(TreatmentState newState, const QString &reason)
{
    if (m_treatmentState == newState) {
        applyTreatmentStateToUi();
        return;
    }

    if (!canTransitTo(m_treatmentState, newState)) {
        qWarning() << "非法状态跳转:"
                   << treatmentStateText(m_treatmentState)
                   << "->"
                   << treatmentStateText(newState)
                   << "reason =" << reason;

        statusBar()->showMessage(
            tr("非法流程跳转：%1 -> %2")
                .arg(treatmentStateText(m_treatmentState))
                .arg(treatmentStateText(newState)),
            5000);

        return;
    }

    m_previousTreatmentState = m_treatmentState;
    m_treatmentState = newState;

    qDebug() << "治疗状态切换:"
             << treatmentStateText(m_previousTreatmentState)
             << "->"
             << treatmentStateText(m_treatmentState)
             << "reason =" << reason;

    applyTreatmentStateToUi();
}
void SimmTcs::applyTreatmentStateToUi()
{
    updateStepBar();

    const bool connected =
        m_webSocketClient && m_webSocketClient->isConnected();

    // 计划确认按钮
    if (ui->confirmPlan_pushButton) {
        ui->confirmPlan_pushButton->setEnabled(
            connected && m_treatmentState == TreatmentState::PlanConfirm);
    }

    // 发送床数据按钮
    if (ui->confirmPlan_cancel_pushButton) {
        ui->confirmPlan_cancel_pushButton->setEnabled(
            connected &&
            (m_treatmentState == TreatmentState::Preparing ||
             m_treatmentState == TreatmentState::PlanConfirm));
    }

    // 申请束流按钮
    if (ui->beamApply_pushButton) {
        ui->beamApply_pushButton->setEnabled(
            connected &&
            (m_treatmentState == TreatmentState::ReadyToApplyBeam ||
             m_treatmentState == TreatmentState::BeamReady));
    }


    // 异常状态下锁定高风险按钮
    if (m_treatmentState == TreatmentState::ErrorStopped) {
        if (ui->confirmPlan_pushButton) ui->confirmPlan_pushButton->setEnabled(false);
        if (ui->confirmPlan_cancel_pushButton) ui->confirmPlan_cancel_pushButton->setEnabled(false);
        if (ui->beamApply_pushButton) ui->beamApply_pushButton->setEnabled(false);
    }

    statusBar()->showMessage(
        tr("当前治疗状态：%1").arg(treatmentStateText(m_treatmentState)),
        3000);
}
bool SimmTcs::showReportConfirmDialog(const QString &reportText)
{
    QDialog reportDialog(this);
    reportDialog.setWindowTitle(tr("治疗报告确认"));
    reportDialog.resize(700, 500);

    auto *layout = new QVBoxLayout(&reportDialog);

    auto *label = new QLabel(tr("请查看治疗报告内容，确认后继续："), &reportDialog);

    auto *textEdit = new QTextEdit(&reportDialog);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(reportText);

    auto *buttonLayout = new QHBoxLayout;
    auto *confirmButton = new QPushButton(tr("确认"), &reportDialog);
    auto *cancelButton = new QPushButton(tr("取消"), &reportDialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(confirmButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addWidget(textEdit);
    layout->addLayout(buttonLayout);

    connect(confirmButton, &QPushButton::clicked, &reportDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &reportDialog, &QDialog::reject);

    return reportDialog.exec() == QDialog::Accepted;
}
bool SimmTcs::ensureConnected() const
{
    if (!m_webSocketClient || !m_webSocketClient->isConnected()) {
        QMessageBox::warning(
            const_cast<SimmTcs *>(this),
            tr("连接错误"),
            tr("未连接到服务器，无法发送指令。"));
        return false;
    }

    return true;
}

bool SimmTcs::ensureStateAllowed(const QList<TreatmentState> &allowedStates,
                                 const QString &operationName) const
{
    if (allowedStates.contains(m_treatmentState)) {
        return true;
    }

    QMessageBox::warning(
        const_cast<SimmTcs *>(this),
        tr("操作禁止"),
        tr("当前状态为【%1】，不允许执行【%2】。")
            .arg(treatmentStateText(m_treatmentState))
            .arg(operationName));

    return false;
}

void SimmTcs::sendTaskCommand(const QString &cmd)
{
    if (!ensureConnected()) {
        return;
    }

    QJsonObject msg;
    msg["type"] = "TASK";
    msg["cmd"] = cmd;
    msg["username"] = m_currentUsername;
    msg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_webSocketClient->sendData(msg);

    qDebug() << "已发送 TASK 指令:" << msg;
    statusBar()->showMessage(tr("已发送指令：%1").arg(cmd), 2000);
}


void SimmTcs::initClock()
{
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000); // 1秒刷新
    connect(m_clockTimer, &QTimer::timeout, this, &SimmTcs::updateTimeLabel);

    updateTimeLabel();   // 先立即显示一次
    m_clockTimer->start();
}

void SimmTcs::updateTimeLabel()
{
    // 例如：2026-04-03 15:42:10
    const QString nowText = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ui->label_time->setText(nowText);
}

void SimmTcs::on_theme_pushButton_clicked()
{
    const Theme current = ThemeManager::instance().currentTheme();
    const Theme next = (current == Theme::Light) ? Theme::Dark : Theme::Light;
    ThemeManager::instance().setTheme(next);
    m_settings->setValue("theme", static_cast<int>(next));
}


void SimmTcs::on_logout_pushButton_clicked()
{
    if (m_webSocketClient) {
        disconnect(m_webSocketClient, nullptr, this, nullptr);
        m_webSocketClient->disconnectFromServer();
    }


    showLoginDialog();
}


void SimmTcs::on_beamApply_pushButton_clicked()
{
    if (!ensureStateAllowed(
            {TreatmentState::ReadyToApplyBeam, TreatmentState::BeamReady},
            tr("申请束流"))) {
        return;
    }

    sendTaskCommand("APPLY_BEAM");
}


void SimmTcs::on_beamApply_cancel_pushButton_clicked()
{

}


void SimmTcs::on_confirmPlan_pushButton_clicked()
{
    if (!ensureStateAllowed({TreatmentState::PlanConfirm}, tr("计划确认"))) {
        return;
    }

    sendTaskCommand("REPLY_PLAN_CONFIRMED");

    setTreatmentState(TreatmentState::Preparing, "operator confirmed plan");
}


void SimmTcs::on_confirmPlan_cancel_pushButton_clicked()
{

}

