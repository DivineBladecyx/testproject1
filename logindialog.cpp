#include "logindialog.h"
#include "ui_logindialog.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QIcon>
#include <QSize>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>

const QString DEFAULT_MIDDLEWARE_URL = "ws://172.16.120.192:8081/tcs";
//const QString DEFAULT_MIDDLEWARE_URL = "ws://localhost:8765";

LoginDialog::LoginDialog(const QString &middlewareUrl,
                         AuthMode mode,
                         QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , settings(new QSettings("MyCompany", "LoginSystem", this))
    , timeoutTimer(new QTimer(this))
    , m_themeGroup(new QButtonGroup(this))
    , m_webSocketClient(new WebSocketClient(this))
    , m_webSocketConnected(false)
    , m_middlewareUrl(middlewareUrl.isEmpty() ? DEFAULT_MIDDLEWARE_URL : middlewareUrl)
    , m_authMode(mode)
{
    ui->setupUi(this);

    m_themeGroup->addButton(ui->radioLight, static_cast<int>(Theme::Light));
    m_themeGroup->addButton(ui->radioDark, static_cast<int>(Theme::Dark));

    Theme savedTheme = static_cast<Theme>(settings->value("theme", 0).toInt());
    if (savedTheme == Theme::Light) {
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

    m_webSocketClient->connectToServer(m_middlewareUrl);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

Theme LoginDialog::selectedTheme() const
{
    return static_cast<Theme>(m_themeGroup->checkedId());
}

void LoginDialog::initUI()
{
    if (m_authMode == AuthMode::Login) {
        setWindowTitle("SIMM TCS 系统登录");
        ui->pushButton_login->setText("登录");
        ui->checkBox_remember->setText("记住用户名");
        ui->checkBox_remember->show();
    } else {
        setWindowTitle("治疗结束认证");
        ui->pushButton_login->setText("确认");
        ui->checkBox_remember->hide();
    }

    ui->lineEdit_username->setPlaceholderText("请输入用户名");
    ui->lineEdit_password->setPlaceholderText("请输入密码");
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    ui->lineEdit_username->addAction(QIcon(":/icons/icons/user.svg"), QLineEdit::LeadingPosition);
    ui->lineEdit_password->addAction(QIcon(":/icons/icons/lock.svg"), QLineEdit::LeadingPosition);
    ui->lineEdit_username->setFocus();

    ui->pb_show_passwd->setText("");
    ui->pb_show_passwd->setIcon(QIcon(":/icons/icons/eye-slash.svg"));
    ui->pb_show_passwd->setIconSize(QSize(20, 20));

    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(15000);
}
QJsonObject LoginDialog::getLoginResponse() const
{
    return m_loginResponse;
}
void LoginDialog::initConnections()
{
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
}

QString LoginDialog::getUsername() const
{
    return ui->lineEdit_username->text();
}

QString LoginDialog::getAuthToken() const
{
    return m_authToken;
}

QJsonObject LoginDialog::getUserInfo() const
{
    return m_userInfo;
}

bool LoginDialog::isRememberPassword() const
{
    return ui->checkBox_remember->isChecked();
}

void LoginDialog::on_pushButton_login_clicked()
{
    const QString username = getUsername();
    const QString password = ui->lineEdit_password->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, "警告", "用户名不能为空！");
        ui->lineEdit_username->setFocus();
        return;
    }

    if (password.isEmpty()) {
        QMessageBox::warning(this, "警告", "密码不能为空！");
        ui->lineEdit_password->setFocus();
        return;
    }

    if (!m_webSocketConnected) {
        QMessageBox::information(this, "提示", "正在连接服务器，请稍后重试。");
        m_webSocketClient->connectToServer(m_middlewareUrl);
        return;
    }

    showLoading(true);

    QJsonObject loginMsg = buildAuthData(username, password);
    loginMsg["type"] = "TASK";
    loginMsg["cmd"] = "REPLY_LOGIN";

    m_webSocketClient->sendData(loginMsg);
    timeoutTimer->start();
}

void LoginDialog::onWebSocketDataReceived(const QJsonObject &data)
{
    const QString type = data.value("type").toString();
    const QString cmd = data.value("cmd").toString();

    if (type != "TASK") {
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

    qDebug() << "登录对话框：忽略非登录响应消息，cmd =" << cmd;
}

void LoginDialog::on_pushButton_cancel_clicked()
{
    if (timeoutTimer->isActive()) {
        timeoutTimer->stop();
    }

    showLoading(false);

    if (m_webSocketClient && m_webSocketClient->isConnected()) {
        m_webSocketClient->disconnectFromServer();
    }

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

void LoginDialog::onTimeout()
{
    showLoading(false);

    if (m_webSocketClient && m_webSocketClient->isConnected()) {
        m_webSocketClient->disconnectFromServer();
    }

    QMessageBox::warning(this, "请求超时", "登录请求超时，请检查网络后重试。");
    emit loginFailed("请求超时");
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

void LoginDialog::showLoading(bool show)
{
    ui->pushButton_login->setEnabled(!show);
    ui->pushButton_cancel->setEnabled(true);
    ui->lineEdit_username->setEnabled(!show);
    ui->lineEdit_password->setEnabled(!show);
    ui->checkBox_remember->setEnabled(!show);
    ui->pb_show_passwd->setEnabled(!show);

    if (m_authMode == AuthMode::Login) {
        ui->pushButton_login->setText(show ? "登录中..." : "登录");
    } else {
        ui->pushButton_login->setText(show ? "认证中..." : "确认");
    }
}

void LoginDialog::saveCredentials()
{
    const QString username = getUsername();
    const bool remember = ui->checkBox_remember->isChecked();

    if (remember) {
        settings->setValue("username", username);
        settings->setValue("middleware_url", m_middlewareUrl);
        settings->setValue("remember_username", true);
    } else {
        settings->remove("username");
        settings->remove("remember_username");
    }

    settings->sync();
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

void LoginDialog::handleAuthSuccess(const QJsonObject &response)
{
    m_loginResponse = response;

    qDebug().noquote() << "登录成功完整响应:"
                       << QString::fromUtf8(
                              QJsonDocument(response).toJson(QJsonDocument::Indented));

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
    m_webSocketClient->setParent(nullptr);

    emit loginSuccess(getUsername(), m_authToken, m_webSocketClient, response);

    showLoading(false);
    accept();
}

void LoginDialog::handleAuthFailure(const QJsonObject &response)
{
    const QString errorMsg = response.value("message").toString("登录失败");
    QMessageBox::warning(this, "认证失败", errorMsg);
    emit loginFailed(errorMsg);

    ui->lineEdit_password->clear();
    ui->lineEdit_password->setFocus();
    showLoading(false);
}

WebSocketClient *LoginDialog::takeClient()
{
    qDebug() << "[LoginDialog::takeClient] 取出客户端:";

    WebSocketClient *client = m_webSocketClient;
    if (client) {
        m_webSocketClient = nullptr;
        qDebug() << "[LoginDialog::takeClient] 取出客户端:" << client;
    }

    return client;
}

QJsonObject LoginDialog::buildAuthData(const QString &username, const QString &password) const
{
    QJsonObject authData;
    authData["username"] = username;
    authData["password"] = password;
    authData["grant_type"] = "password";
    authData["client_type"] = "desktop";
    authData["client_version"] = "1.0.0";
    authData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

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
