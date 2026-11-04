#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QString>
#include <QJsonObject>
#include <QTimer>
#include "thememanager.h"
#include "websocketclient.h"
#include <QButtonGroup>
#include <QJsonObject>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    enum class AuthMode {
        Login,
        ConfirmOnly
    };
    explicit LoginDialog(const QString &middlewareUrl = "",AuthMode mode = AuthMode::Login, QWidget *parent = nullptr);
    ~LoginDialog();
    QJsonObject getLoginResponse() const;

    QString getUsername() const;
    QString getAuthToken() const;
    QJsonObject getUserInfo() const;
    bool isRememberPassword() const;
    WebSocketClient* takeClient();  // 新增
    // 获取用户选择的主题
    Theme selectedTheme() const;
    //void setAuthServerUrl(const QString &url);
    //QString getAuthServerUrl() const;
signals:
    void loginSuccess(const QString &username, const QString &authToken ,WebSocketClient *client,const QJsonObject &loginResponse);
    void loginFailed(const QString &error);


private slots:
    void on_pushButton_login_clicked();
    void on_pushButton_cancel_clicked();
    void on_pb_show_passwd_clicked();
    //void onNetworkError(QNetworkReply::NetworkError code);
    void onTimeout();
    //void onServerEditChanged();


    // 新增 WebSocket 相关槽函数
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(const QString &error);
    void onWebSocketDataReceived(const QJsonObject &data);

private:
    Ui::LoginDialog *ui;
    QSettings *settings;
    QTimer *timeoutTimer;
    QButtonGroup *m_themeGroup;     // 用于管理单选按钮组
    QJsonObject m_loginResponse;
    // 新增成员
    WebSocketClient *m_webSocketClient;
    bool m_webSocketConnected;
    QString session_id;
    QString room_id;
    // 认证相关
    QString m_middlewareUrl;
    QString m_authToken;
    QJsonObject m_userInfo;
    AuthMode m_authMode;

    void initUI();
    void initConnections();
    bool validateInput();
    void showLoading(bool show);
    void saveCredentials();
    void loadCredentials();
    void parseAuthResponse(const QByteArray &response);
    void handleAuthSuccess(const QJsonObject &response);
    void handleAuthFailure(const QJsonObject &response);
    void cleanupNetwork();
    QJsonObject buildAuthData(const QString &username, const QString &password) const;
};

#endif // LOGINDIALOG_H
