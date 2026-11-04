#include "clientbootstrap.h"
#include "maintenancewindow.h"

#include <memory>

int main(int argc, char *argv[])
{
    return runAuthenticatedClient(
        argc,
        argv,
        QStringLiteral("SIMM Maintenance"),
        [](const ClientSession &session) {
            return std::make_unique<MaintenanceWindow>(session.webSocketClient,
                                                       session.username,
                                                       session.authToken,
                                                       session.userInfo,
                                                       session.loginResponse);
        });
}
