#include "clientbootstrap.h"
#include "qawindow.h"
#include "windowlifecycle.h"

#include <QApplication>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTimer>

#include <memory>

int main(int argc, char *argv[])
{
    if (QProcessEnvironment::systemEnvironment().value(QStringLiteral("SIMM_QA_SMOKE")) == QStringLiteral("1")) {
        SingleInstanceGuard instance;
        QApplication app(argc, argv);
        if (!allowClientInstance(instance))
            return instance.result() == SingleInstanceGuard::Result::AlreadyRunning ? 0 : 1;

        QJsonObject userInfo;
        userInfo[QStringLiteral("display_name")] = QStringLiteral("smoke");
        userInfo[QStringLiteral("role")] = QStringLiteral("admin");

        QaWindow window(nullptr,
                        QStringLiteral("smoke"),
                        QString(),
                        userInfo,
                        QJsonObject());
        window.show();
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
        return app.exec();
    }

    return runAuthenticatedClient(
        argc,
        argv,
        QStringLiteral("SIMM QA"),
        [](const ClientSession &session) {
            return std::make_unique<QaWindow>(session.webSocketClient,
                                              session.username,
                                              session.authToken,
                                              session.userInfo,
                                              session.loginResponse);
        });
}
