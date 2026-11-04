#include "simmtcs.h"
#include "logindialog.h"
#include "websocketclient.h"
#include "thememanager.h"
#include <QDebug>
#include <QApplication>
#include <QFileInfo>

#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息（用于QSettings）
    QApplication::setOrganizationName("MyCompany");
    QApplication::setApplicationName("LoginSystem");
    //加载网关配置
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // 加载保存的主题（登录对话框会显示对应风格）
    ThemeManager::instance().loadSavedTheme();
    // 加载配置文件
    QString configPath = QApplication::applicationDirPath() + "/config/app_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString middlewareUrl = settings.value("middleware_url").toString();
    if (middlewareUrl.isEmpty()) {
       //middlewareUrl = "ws://localhost:8765";  // 默认测试地址
       middlewareUrl = "ws://172.16.120.192:8081/tcs";
        qWarning() << "Middleware URL not configured, using default.";
        // 可设置默认值或提示错误
    }
    qDebug() << "Step 1: 开始创建登录对话框";
    // 显示登录对话框
    LoginDialog loginDialog(middlewareUrl);
    qDebug() << "Step 2: 登录对话框创建完成，准备 exec()";
    if (loginDialog.exec() != QDialog::Accepted) {
        qDebug() << "Step 3: 登录对话框被拒绝或关闭，程序退出";
        return 0;   // 用户取消登录，退出程序
    }
    // 获取登录成功后的信息
    QString username = loginDialog.getUsername();   // 假设 LoginDialog 提供了这样的接口
    QString authToken = loginDialog.getAuthToken(); // 假设 LoginDialog 提供了这样的接口
    QJsonObject userInfo =loginDialog.getUserInfo();
    QJsonObject response=loginDialog.getLoginResponse();
    // 从登录对话框获取已经建立连接的 WebSocket 客户端
    WebSocketClient *webSocketClient = loginDialog.takeClient();
    if (!webSocketClient) {
        qCritical() << "Failed to obtain WebSocket client from login dialog";
        return -1;
    }
   /*
    // 设置认证参数
    QJsonObject authParams;
    authParams["token"] = authToken;
    authParams["client_type"] = "desktop";
    authParams["username"] = username;
    webSocketClient->setMiddlewareAuthParams(authParams);
    webSocketClient->setAutoReconnect(true, 5000);
    webSocketClient->connectToServer(middlewareUrl);
   */
    // 创建主窗口，传递必要的信息
    SimmTcs mainWindow(webSocketClient, username, authToken,userInfo,response);
    mainWindow.show();
    qDebug() << "[main] 主窗口已创建，开始事件循环";
    return a.exec();
}
