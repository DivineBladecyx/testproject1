#include "clientbootstrap.h"

#include "logindialog.h"
#include "thememanager.h"
#include "websocketclient.h"
#include "windowlifecycle.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QSettings>
#include <QWidget>

#include <memory>

namespace {

QString middlewareUrlFromSettings()
{
    const QString configPath = QApplication::applicationDirPath()
    + QStringLiteral("/config/app_config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    return settings.value(QStringLiteral("Middleware/Url"))
        .toString()
        .trimmed();
}

ClientSession createSession(LoginDialog &loginDialog,
                            WebSocketClient *webSocketClient)
{
    ClientSession session;
    session.webSocketClient = webSocketClient;
    session.username = loginDialog.getUsername();
    session.password = loginDialog.getPassword();
    session.authToken = loginDialog.getAuthToken();
    session.userInfo = loginDialog.getUserInfo();
    session.loginResponse = loginDialog.getLoginResponse();
    return session;
}

} // namespace

int runAuthenticatedClient(int argc,
                           char *argv[],
                           const QString &applicationName,
                           ClientWindowFactory createWindow)
{
    SingleInstanceGuard instance;
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(QString::fromLatin1(kSimmApplicationVersion));
    Q_INIT_RESOURCE(resources);

    QApplication::setOrganizationName(QStringLiteral("SIMM"));
    QApplication::setApplicationName(applicationName);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    if (!allowClientInstance(instance))
        return instance.result() == SingleInstanceGuard::Result::AlreadyRunning ? 0 : 1;

    if (!createWindow) {
        qCritical() << "Client window factory is not configured";
        return -1;
    }

    ThemeManager::instance().loadSavedTheme();

    LoginDialog loginDialog(middlewareUrlFromSettings(),
                            LoginDialog::AuthMode::Login,
                            nullptr,
                            false);
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    // The login dialog releases ownership after successful authentication.
    // Keep temporary ownership until a main window can adopt the client.
    std::unique_ptr<WebSocketClient> webSocketClient(loginDialog.takeClient());
    if (!webSocketClient) {
        qCritical() << "Failed to obtain WebSocket client from login dialog";
        return -1;
    }

    ClientSession session = createSession(loginDialog, webSocketClient.get());
    std::unique_ptr<QWidget> mainWindow = createWindow(session);

    // The factory has consumed the credentials; reduce their lifetime here.
    loginDialog.clearPassword();
    session.password.fill(QChar(u'\0'));
    session.password.clear();

    if (!mainWindow) {
        qCritical() << "Client window factory returned null";
        return -1;
    }

    webSocketClient->setParent(mainWindow.get());
    webSocketClient.release();

    mainWindow->setWindowTitle(mainWindow->windowTitle()
                               + QStringLiteral(" V%1").arg(QApplication::applicationVersion()));
    configureClientMainWindow(mainWindow.get());
    const auto quitCleanup = QObject::connect(&app, &QCoreApplication::aboutToQuit, &app,
                                              [&mainWindow] { shutdownClientWindow(mainWindow.get()); });
    mainWindow->show();
    const int exitCode = app.exec();
    QObject::disconnect(quitCleanup);
    shutdownClientWindow(mainWindow.get());
    mainWindow.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return exitCode;
}
