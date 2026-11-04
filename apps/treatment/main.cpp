#include "clientbootstrap.h"
#include "thememanager.h"
#include "treatmentwindow.h"
#include "windowlifecycle.h"

#include <QApplication>
#include <QEvent>
#include <QSettings>
#include <memory>

int main(int argc, char *argv[])
{
    SingleInstanceGuard instance;
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(QString::fromLatin1(kSimmApplicationVersion));
    QApplication::setOrganizationName(QStringLiteral("SIMM"));
    QApplication::setApplicationName(QStringLiteral("SIMM Treatment"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    Q_INIT_RESOURCE(resources);
    if (!allowClientInstance(instance))
        return instance.result() == SingleInstanceGuard::Result::AlreadyRunning ? 0 : 1;
    ThemeManager::instance().loadSavedTheme();
    auto window = std::make_unique<TreatmentWindow>(nullptr, QString(), QString(), QJsonObject(), QJsonObject());
    window->setWindowTitle(window->windowTitle() + QStringLiteral(" V%1").arg(app.applicationVersion()));
    configureClientMainWindow(window.get());
    const auto quitCleanup = QObject::connect(&app, &QCoreApplication::aboutToQuit, &app,
                                              [&window] { shutdownClientWindow(window.get()); });
    window->showMaximized();
    const int exitCode = app.exec();
    QObject::disconnect(quitCleanup);
    shutdownClientWindow(window.get());
    window.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return exitCode;
}
