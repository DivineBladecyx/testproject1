#include "logindialog.h"
#include "ui_logindialog.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QButtonGroup>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QLineEdit>
#include <QIcon>
#include <QSize>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>

namespace {

constexpr int kHeartbeatIntervalMs = 10'000;
constexpr int kPongTimeoutMs = 5'000;

QJsonObject loginReplyMetadata(const QJsonObject &object, int depth = 0)
{
    // Only protocol identifiers/status flags are logged; never arbitrary response or auth values.
    QJsonObject summary;
    const QStringList keys = object.keys();
    summary.insert(QStringLiteral("field_names"), QJsonArray::fromStringList(keys.mid(0, 32)));
    static const QRegularExpression identifier(QStringLiteral("^[A-Za-z0-9_.:-]{1,96}$"));
    for (const QString &key : {QStringLiteral("type"), QStringLiteral("cmd"), QStringLiteral("status"),
                               QStringLiteral("code"), QStringLiteral("success"), QStringLiteral("error_code"),
                               QStringLiteral("result")}) {
        const QJsonValue value = object.value(key);
        if (value.isBool() || value.isDouble()) summary.insert(key, value);
        else if (value.isString()) summary.insert(key, identifier.match(value.toString()).hasMatch()
                                    ? value : QJsonValue(QStringLiteral("[non-identifier omitted]")));
    }
    if (depth < 2) {
        for (const QString &key : {QStringLiteral("payload"), QStringLiteral("data"),
                                   QStringLiteral("result"), QStringLiteral("error")}) {
            if (object.value(key).isObject())
                summary.insert(key, loginReplyMetadata(object.value(key).toObject(), depth + 1));
        }
    }
    return summary;
}

} // namespace

QString clientModeToString(LoginDialog::ClientMode mode)
{
    switch (mode) {
    case LoginDialog::ClientMode::Treatment:
        return QStringLiteral("treatment");
    case LoginDialog::ClientMode::Qa:
        return QStringLiteral("qa");
    }

    return QStringLiteral("treatment");
}

QString clientModeToAppName(LoginDialog::ClientMode mode)
{
    switch (mode) {
    case LoginDialog::ClientMode::Treatment:
        return QStringLiteral("simm-treatment");
    case LoginDialog::ClientMode::Qa:
        return QStringLiteral("simm-qa");
    }

    return QStringLiteral("simm-treatment");
}

LoginDialog::LoginDialog(const QString &middlewareUrl,
                         AuthMode mode,
                         QWidget *parent,
                         bool showClientModeSelection)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , settings(new QSettings("MyCompany", "LoginSystem", this))
    , timeoutTimer(new QTimer(this))
    , m_themeGroup(new QButtonGroup(this))
    , m_clientModeGroup(new QButtonGroup(this))
    , m_webSocketClient(new WebSocketClient(this))
    , m_webSocketConnected(false)
    , m_middlewareUrl(middlewareUrl.trimmed())
    , m_authMode(mode)
    , m_showClientModeSelection(showClientModeSelection)
{
    ui->setupUi(this);

    m_themeGroup->addButton(ui->radioLight, static_cast<int>(Theme::Light));
    m_themeGroup->addButton(ui->radioDark, static_cast<int>(Theme::Dark));

    m_clientModeGroup->setExclusive(true);
    m_clientModeGroup->addButton(ui->treatmentModeButton, static_cast<int>(ClientMode::Treatment));
    m_clientModeGroup->addButton(ui->qaModeButton, static_cast<int>(ClientMode::Qa));
    ui->treatmentModeButton->setChecked(true);

    const int savedClientMode = settings->value(
                                            QStringLiteral("client_mode"), static_cast<int>(ClientMode::Treatment)).toInt();
    if (QAbstractButton *button = m_clientModeGroup->button(savedClientMode)) {
        button->setChecked(true);
    } else {
        // Retired modes must not be restored from a previous installation.
        settings->setValue(QStringLiteral("client_mode"), static_cast<int>(ClientMode::Treatment));
    }

    const Theme savedTheme = ThemeManager::instance().currentTheme();
    if (savedTheme != Theme::Dark) {
        ui->radioLight->setChecked(true);
    } else {
        ui->radioDark->setChecked(true);
    }

    initUI();
    initConnections();

    if (m_authMode == AuthMode::Login) {
        loadCredentials();
    }

    connect(m_webSocketClient, &WebSocketClient::connected,
            this, &LoginDialog::onWebSocketConnected);
    connect(m_webSocketClient, &WebSocketClient::disconnected,
            this, &LoginDialog::onWebSocketDisconnected);
    connect(m_webSocketClient, &WebSocketClient::errorOccurred,
            this, &LoginDialog::onWebSocketError);
    connect(m_webSocketClient, &WebSocketClient::dataReceived,
            this, &LoginDialog::onWebSocketDataReceived);

    m_webSocketClient->setHeartbeatInterval(kHeartbeatIntervalMs,
                                            kPongTimeoutMs);

    const QUrl serverUrl(m_middlewareUrl);
    if (!serverUrl.isValid() || serverUrl.host().isEmpty()
        || (serverUrl.scheme() != QStringLiteral("ws")
            && serverUrl.scheme() != QStringLiteral("wss"))) {
        qCritical() << "Middleware URL is not configured.";
        QTimer::singleShot(0, this, [this]() {
            QMessageBox::critical(this,
                                  tr("配置错误"),
                                  tr("中间层地址无效，请检查程序目录下的 config/app_config.ini，地址应为 ws:// 或 wss://。"));
            reject();
        });
    } else {
        m_webSocketClient->connectToServer(m_middlewareUrl);
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::initUI()
{
    if (m_authMode == AuthMode::Login) {
        setWindowTitle(tr("SIMM 系统登录 V%1")
                           .arg(QCoreApplication::applicationVersion()));
        ui->pushButton_login->setText("登录");
        ui->checkBox_remember->setText("记住用户名");
        ui->checkBox_remember->show();
        ui->modeSelectorWidget->setVisible(m_showClientModeSelection);
        ui->label_title->setText(m_showClientModeSelection ? "SIMM治疗控制系统" : "SIMM 系统登录");
        ui->label_subtitle->setText(m_showClientModeSelection
                                        ? "请选择工作模式并输入账号信息"
                                        : "请输入账号信息以进入系统");
    } else {
        setWindowTitle("治疗结束认证");
        ui->pushButton_login->setText("确认");
        ui->checkBox_remember->hide();
        ui->modeSelectorWidget->hide();
    }

    ui->lineEdit_username->setPlaceholderText("请输入用户名");
    ui->lineEdit_password->setPlaceholderText("请输入密码");
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    ui->lineEdit_username->addAction(QIcon(":/icons/icons/user.svg"), QLineEdit::LeadingPosition);
    ui->lineEdit_password->addAction(QIcon(":/icons/icons/lock.svg"), QLineEdit::LeadingPosition);
    if (m_authMode == AuthMode::Login) {
        ui->lineEdit_password->setFocus(Qt::OtherFocusReason);
    } else {
        ui->lineEdit_username->setFocus(Qt::OtherFocusReason);
    }

    ui->pb_show_passwd->setText("");
    ui->pb_show_passwd->setIcon(QIcon(":/icons/icons/eye-slash.svg"));
    ui->pb_show_passwd->setIconSize(QSize(20, 20));

    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(15000);
}

void LoginDialog::initConnections()
{
    // Covers Cancel, Escape and the title-bar close button.
    connect(this, &QDialog::rejected, this, [this]() {
        timeoutTimer->stop();
        if (m_webSocketClient) {
            disconnect(m_webSocketClient, nullptr, this, nullptr);
            m_webSocketClient->setAutoReconnect(false);
            m_webSocketClient->disconnectFromServer();
        }
        m_webSocketConnected = false;
    });
    connect(timeoutTimer, &QTimer::timeout,
            this, &LoginDialog::onTimeout);

    connect(ui->lineEdit_password, &QLineEdit::returnPressed,
            ui->pushButton_login, &QPushButton::click);

    connect(ui->lineEdit_username, &QLineEdit::returnPressed,
            this, [this]() {
                ui->lineEdit_password->setFocus();
            });

    connect(m_themeGroup, &QButtonGroup::buttonClicked,
            this, [this](QAbstractButton *button) {
                ThemeManager::instance().setTheme(
                    static_cast<Theme>(m_themeGroup->id(button)));
            });

    connect(m_clientModeGroup, &QButtonGroup::buttonClicked,
            this, [this](QAbstractButton *button) {
                settings->setValue(QStringLiteral("client_mode"), m_clientModeGroup->id(button));
            });
}

Theme LoginDialog::selectedTheme() const
{
    return m_themeGroup->checkedId() == static_cast<int>(Theme::Dark)
    ? Theme::Dark : Theme::Light;
}

LoginDialog::ClientMode LoginDialog::selectedClientMode() const
{
    return m_clientModeGroup->checkedId() == static_cast<int>(ClientMode::Qa)
    ? ClientMode::Qa : ClientMode::Treatment;
}

void LoginDialog::setQaModeAvailable(bool available)
{
    m_qaModeAvailable = available;
    ui->qaModeButton->setEnabled(available && !timeoutTimer->isActive());
    ui->qaModeButton->setToolTip(available ? QString() : tr("等待后台初始化完成后才能进入 QA 模式"));
    if (!available && ui->qaModeButton->isChecked()) ui->treatmentModeButton->setChecked(true);
}

QString LoginDialog::getUsername() const
{
    return ui->lineEdit_username->text();
}

QString LoginDialog::getPassword() const
{
    return ui->lineEdit_password->text();
}

void LoginDialog::clearPassword()
{
    ui->lineEdit_password->clear();
}

QString LoginDialog::getAuthToken() const
{
    return m_authToken;
}

QJsonObject LoginDialog::getUserInfo() const
{
    return m_userInfo;
}

QJsonObject LoginDialog::getLoginResponse() const
{
    return m_loginResponse;
}

bool LoginDialog::isRememberPassword() const
{
    return ui->checkBox_remember->isChecked();
}

WebSocketClient *LoginDialog::takeClient()
{
    qDebug() << "[LoginDialog::takeClient] 取出客户端:";

    WebSocketClient *client = m_webSocketClient;
    if (client) {
        disconnect(client, nullptr, this, nullptr);
        client->setParent(nullptr);
        m_webSocketClient = nullptr;
        qDebug() << "[LoginDialog::takeClient] 取出客户端:" << client;
    }

    return client;
}

void LoginDialog::on_pushButton_login_clicked()
{
    if (timeoutTimer->isActive() || !m_webSocketClient) {
        return;
    }
    const QString username = getUsername();
    const QString password = ui->lineEdit_password->text();

    if (username.trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "用户名不能为空！");
        ui->lineEdit_username->setFocus();
        return;
    }

    if (password.isEmpty()) {
        QMessageBox::warning(this, "警告", "密码不能为空！");
        ui->lineEdit_password->setFocus();
        return;
    }

    if (!m_webSocketClient->isConnected()) {
        QMessageBox::information(this, "提示", "正在连接服务器，请稍后重试。");
        m_webSocketClient->connectToServer(m_middlewareUrl);
        return;
    }

    QJsonObject loginMsg = buildAuthData(username, password);
    if (loginMsg.value(QStringLiteral("room_id")).toString().isEmpty()) {
        QMessageBox::warning(this, tr("缺少治疗室配置"),
                             tr("请在 %1 中设置 [Treatment] 下的 RoomId，再重新登录。")
                                 .arg(QCoreApplication::applicationDirPath()
                                      + QStringLiteral("/config/app_config.ini")));
        return;
    }
    showLoading(true);
    /*if (m_authMode == AuthMode::Login && selectedClientMode() == ClientMode::Qa) {
        loginMsg = QJsonObject{{"cmd", "LOGIN"}, {"username", username}, {"password", password},
                               {"room_id", loginMsg.value("room_id")},
                               {"session_id", QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz")}};
    } else {
        loginMsg["type"] = "TASK";
        loginMsg["cmd"] = "REPLY_LOGIN";
    }
   */
    loginMsg["type"] = "TASK";
    loginMsg["cmd"] = "REPLY_LOGIN";
    // sendData can emit errorOccurred synchronously; start first so it can stop us.
    if (m_authMode == AuthMode::Login) m_webSocketClient->beginWorkflowHandoff();
    timeoutTimer->start();
    m_webSocketClient->sendData(loginMsg);
}

void LoginDialog::on_pushButton_cancel_clicked()
{
    reject();
}

void LoginDialog::on_pb_show_passwd_clicked()
{
    const bool showPassword =
        ui->lineEdit_password->echoMode() == QLineEdit::Password;

    ui->lineEdit_password->setEchoMode(
        showPassword ? QLineEdit::Normal : QLineEdit::Password);

    ui->pb_show_passwd->setIcon(
        QIcon(showPassword
                  ? ":/icons/icons/eye.svg"
                  : ":/icons/icons/eye-slash.svg"));
}

void LoginDialog::onWebSocketConnected()
{
    m_webSocketConnected = true;
    qDebug() << "登录对话框：WebSocket 已连接到" << m_middlewareUrl;
}

void LoginDialog::onWebSocketDisconnected()
{
    const bool wasWaitingLogin = timeoutTimer->isActive();
    m_webSocketConnected = false;

    if (wasWaitingLogin) {
        timeoutTimer->stop();
        showLoading(false);
        QMessageBox::warning(this, "连接断开", "与服务器的连接已断开，登录已取消。");
        emit loginFailed("连接已断开");
    }

    qDebug() << "登录对话框：WebSocket 已断开";
}

void LoginDialog::onWebSocketError(const QString &error)
{
    qDebug() << "登录对话框：WebSocket 错误" << error;

    if (timeoutTimer->isActive()) {
        timeoutTimer->stop();
        showLoading(false);
        QMessageBox::warning(this, "连接错误", "登录过程中连接中间层失败：\n" + error);
        emit loginFailed(error);
        return;
    }

    if (!m_webSocketConnected) {
        QMessageBox::warning(this, "连接错误", "无法连接到中间层服务器:\n" + error);
    }
}

void LoginDialog::onWebSocketDataReceived(const QJsonObject &data)
{
    // Ignore unsolicited responses and responses received after timeout/cancel.
    if (!timeoutTimer->isActive()) {
        return;
    }
    const QString type = data.value("type").toString();
    const QString cmd = data.value("cmd").toString();

    if (type != "TASK") {
        if (type == QStringLiteral("RSP")) {
            qWarning().noquote() << "[LoginReply] RSP metadata (not login success):"
                                 << QString::fromUtf8(QJsonDocument(loginReplyMetadata(data)).toJson(QJsonDocument::Compact));
            if (cmd == QStringLiteral("REPLY_LOGIN") && data.value("result").isBool()
                && !data.value("result").toBool()) {
                timeoutTimer->stop();
                QJsonObject failure;
                failure.insert(QStringLiteral("message"),
                               tr("中间层返回登录请求失败（REPLY_LOGIN: result=false），但未提供具体原因。\n"
                                  "请检查服务端的登录处理日志，包括账号权限、room_id 和当前流程状态；不能仅据此判断为密码错误。"));
                handleAuthFailure(failure);
            }
            return;
        }
        if (type == QStringLiteral("TELEMETRY")) return;
        qDebug() << "登录对话框：忽略非 TASK 消息，type =" << type;
        return;
    }

    if (cmd == "TASK_LOGIN_SUCCESS") {
        if (timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }
        handleAuthSuccess(data);
        return;
    }

    if (cmd == "TASK_LOGIN_FAILED") {
        if (timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }
        handleAuthFailure(data);
        return;
    }

    if (cmd == QStringLiteral("TASK_DDS_FAULT_ALERT")) return;
    qDebug() << "登录对话框：忽略非登录响应消息，cmd =" << cmd;
}

void LoginDialog::onTimeout()
{
    const bool transportConnected = m_webSocketClient && m_webSocketClient->isConnected();
    showLoading(false);

    if (m_webSocketClient && m_webSocketClient->isConnected()) {
        m_webSocketClient->disconnectFromServer();
    }

    qWarning() << "[LoginReply] Login timed out without TASK_LOGIN_SUCCESS/TASK_LOGIN_FAILED;"
               << "WebSocket was connected:" << transportConnected;
    QMessageBox::warning(this, tr("请求超时"), transportConnected
                                                       ? tr("WebSocket 已连接，但未收到登录成功或失败回执。\n请检查中间层对 REPLY_LOGIN 的处理，以及应用程序输出中的 [LoginReply] 摘要。")
                                                       : tr("登录请求超时，连接已中断，请检查网络及中间层服务后重试。"));
    emit loginFailed("请求超时");
}

void LoginDialog::handleAuthSuccess(const QJsonObject &response)
{
    m_loginResponse = response;

    qDebug() << "Login authentication succeeded";

    m_authToken = response.value("auth_token").toString();
    if (m_authToken.isEmpty()) {
        m_authToken = response.value("token").toString();
    }

    m_userInfo = response.value("user").toObject();

    if (m_authMode == AuthMode::ConfirmOnly) {
        showLoading(false);
        accept();
        return;
    }

    saveCredentials();

    const int expiresIn = response.value("expires_in").toInt(3600);
    const QDateTime expireTime = QDateTime::currentDateTime().addSecs(expiresIn);
    settings->setValue("token_expire_time", expireTime);

    disconnect(m_webSocketClient, nullptr, this, nullptr);
    showLoading(false);
    accept();
    emit loginSuccess(getUsername(), m_authToken, m_webSocketClient, response);
}

void LoginDialog::handleAuthFailure(const QJsonObject &response)
{
    const QJsonObject payload = response.value(QStringLiteral("payload")).toObject();
    QString errorMsg = response.value(QStringLiteral("message")).toString().trimmed();
    if (errorMsg.isEmpty()) errorMsg = payload.value(QStringLiteral("message")).toString().trimmed();
    if (errorMsg.isEmpty()) errorMsg = tr("登录失败，请检查用户名和密码。后台未提供具体失败原因。");
    showLoading(false);
    ui->lineEdit_password->clear();
    QMessageBox::warning(this, m_authMode == AuthMode::Login ? tr("登录失败") : tr("认证失败"), errorMsg);
    ui->lineEdit_password->setFocus();
    emit loginFailed(errorMsg);
}

QJsonObject LoginDialog::buildAuthData(const QString &username, const QString &password) const
{
    QJsonObject authData;
    authData["username"] = username;
    authData["password"] = password;
    const QString configPath = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/config/app_config.ini");
    QSettings config(configPath, QSettings::IniFormat);
    authData[QStringLiteral("room_id")] = config.value(
                                                    QStringLiteral("Treatment/RoomId")).toString().trimmed();
    authData["grant_type"] = "password";
    authData["client_type"] = "desktop";
    authData["client_version"] = QCoreApplication::applicationVersion();
    authData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (m_showClientModeSelection) {
        const ClientMode mode = selectedClientMode();
        authData["client_mode"] = clientModeToString(mode);
        authData["client_app"] = clientModeToAppName(mode);
    }

    QJsonObject deviceInfo;
    deviceInfo["os"] = QSysInfo::prettyProductName();
    deviceInfo["platform"] = QSysInfo::productType();

    const QByteArray hashResult = QCryptographicHash::hash(
        QSysInfo::machineUniqueId(),
        QCryptographicHash::Sha256);

    deviceInfo["device_id"] = QString::fromUtf8(hashResult.toHex());

    authData["device_info"] = deviceInfo;
    return authData;
}

void LoginDialog::showLoading(bool show)
{
    ui->pushButton_login->setEnabled(!show);
    ui->pushButton_cancel->setEnabled(true);
    ui->lineEdit_username->setEnabled(!show);
    ui->lineEdit_password->setEnabled(!show);
    ui->checkBox_remember->setEnabled(!show);
    ui->pb_show_passwd->setEnabled(!show);
    for (QAbstractButton *button : m_clientModeGroup->buttons()) {
        button->setEnabled(!show && (button != ui->qaModeButton || m_qaModeAvailable));
    }

    if (m_authMode == AuthMode::Login) {
        ui->pushButton_login->setText(show ? "登录中..." : "登录");
    } else {
        ui->pushButton_login->setText(show ? "认证中..." : "确认");
    }
}

void LoginDialog::loadCredentials()
{
    const bool remember = settings->value("remember_username", false).toBool();
    if (!remember) {
        return;
    }

    const QString username = settings->value("username").toString();
    if (!username.isEmpty()) {
        ui->lineEdit_username->setText(username);
        ui->checkBox_remember->setChecked(true);
    }
}

void LoginDialog::saveCredentials()
{
    const QString username = getUsername();
    const bool remember = ui->checkBox_remember->isChecked();

    if (remember) {
        settings->setValue("username", username);
        settings->setValue("remember_username", true);
    } else {
        settings->remove("username");
        settings->remove("remember_username");
    }

    settings->sync();
}
