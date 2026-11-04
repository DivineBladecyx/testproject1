#pragma once

#include <QJsonObject>
#include <QString>

#include <functional>
#include <memory>

class QWidget;
class WebSocketClient;

inline constexpr char kSimmApplicationVersion[] = "1.0.4.0930";

struct ClientSession
{
    WebSocketClient *webSocketClient = nullptr;
    QString username;
    QString password;
    QString authToken;
    QJsonObject userInfo;
    QJsonObject loginResponse;
};

using ClientWindowFactory = std::function<std::unique_ptr<QWidget>(const ClientSession &)>;

int runAuthenticatedClient(int argc,
                           char *argv[],
                           const QString &applicationName,
                           ClientWindowFactory createWindow);
