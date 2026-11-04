#include "clientbootstrap.h"
#include "logindialog.h"
#include "qawindow.h"
#include "thememanager.h"
#include "treatmentwindow.h"
#include "websocketclient.h"
#include "windowlifecycle.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QSettings>
#include <QWidget>

#include <memory>
#include <utility>

namespace {

std::unique_ptr<QWidget> createClientWindow(LoginDialog::ClientMode mode,
                                            const ClientSession &session)
{
    switch (mode) {
    case LoginDialog::ClientMode::Treatment:
        return std::make_unique<TreatmentWindow>(session.webSocketClient,
                                                 session.username,
                                                 session.authToken,
                                                 session.userInfo,
                                                 session.loginResponse,
                                                 session.password);
    case LoginDialog::ClientMode::Qa:
        return std::make_unique<QaWindow>(session.webSocketClient,
                                          session.username,
                                          session.authToken,
                                          session.userInfo,
                                          session.loginResponse);
    }

    return nullptr;
}

} // namespace

int main(int argc, char *argv[])
{
    SingleInstanceGuard instance;
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(QString::fromLatin1(kSimmApplicationVersion));
    Q_INIT_RESOURCE(resources);

    QApplication::setOrganizationName(QStringLiteral("SIMM"));
    QApplication::setApplicationName(QStringLiteral("SIMM"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    if (!allowClientInstance(instance))
        return instance.result() == SingleInstanceGuard::Result::AlreadyRunning ? 0 : 1;
    ThemeManager::instance().loadSavedTheme();

    ClientSession session;
    std::unique_ptr<QWidget> mainWindow = createClientWindow(
        LoginDialog::ClientMode::Treatment,
        session);

    if (!mainWindow) {
        qCritical() << "Failed to create selected SIMM client window";
        return -1;
    }

    if (auto *treatment = qobject_cast<TreatmentWindow *>(mainWindow.get())) {
        treatment->setClientModeSwitchHandler(
            [&app, &mainWindow](LoginDialog::ClientMode mode, const ClientSession &newSession) {
                std::unique_ptr<QWidget> nextWindow = createClientWindow(mode, newSession);
                if (!nextWindow) {
                    qCritical() << "Failed to switch SIMM client mode";
                    app.exit(-1);
                    return;
                }
                newSession.webSocketClient->setParent(nextWindow.get());
                nextWindow->setWindowTitle(nextWindow->windowTitle()
                                           + QStringLiteral(" V%1").arg(QApplication::applicationVersion()));
                configureClientMainWindow(nextWindow.get());
                QWidget *previousWindow = mainWindow.release();
                mainWindow = std::move(nextWindow);
                // Show the new window before retiring the old modal-login owner.
                mainWindow->showMaximized();
                previousWindow->deleteLater();
            });
    }

    mainWindow->setWindowTitle(mainWindow->windowTitle()
                               + QStringLiteral(" V%1").arg(QApplication::applicationVersion()));
    configureClientMainWindow(mainWindow.get());
    const auto quitCleanup = QObject::connect(&app, &QCoreApplication::aboutToQuit, &app,
                                              [&mainWindow] { shutdownClientWindow(mainWindow.get()); });
    mainWindow->showMaximized();
    const int exitCode = app.exec();
    QObject::disconnect(quitCleanup);
    shutdownClientWindow(mainWindow.get());
    mainWindow.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return exitCode;
}
