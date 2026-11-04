#pragma once

#include <QJsonObject>
#include <QString>

struct StartupState
{
    enum class Notification { Ignored, Ready, Failed };

    bool authenticated = false;
    bool initialized = false;
    QString failure;

    bool canOperate() const { return authenticated && initialized; }

    Notification observe(const QJsonObject &message, const QString &completeCommand,
                         const QString &failedCommand, const QString &roomId)
    {
        if (message.value(QStringLiteral("type")) != QStringLiteral("TASK"))
            return Notification::Ignored;
        const QString command = message.value(QStringLiteral("cmd")).toString();
        if (command == QStringLiteral("TASK_LOGIN_SUCCESS")
            || command == QStringLiteral("TASK_LOGIN_FAILED")) return Notification::Ignored;
        const QString expectedComplete = completeCommand.isEmpty()
                                             ? QStringLiteral("TASK_INITIALIZATION_COMPLETED") : completeCommand;
        const bool complete = command == expectedComplete;
        const bool failed = !failedCommand.isEmpty() && command == failedCommand;
        if (!complete && !failed) return Notification::Ignored;
        const QJsonObject payload = message.value(QStringLiteral("payload")).toObject();
        for (const auto &scope : {message, payload}) {
            if (scope.contains(QStringLiteral("room_id"))) {
                const QJsonValue room = scope.value(QStringLiteral("room_id"));
                if (!room.isString() || (!room.toString().isEmpty() && room.toString() != roomId))
                    return Notification::Ignored;
            }
        }
        bool rejected = failed;
        for (const auto &scope : {message, payload}) {
            for (const QString &key : {QStringLiteral("success"), QStringLiteral("result")}) {
                if (!scope.contains(key)) continue;
                if (!scope.value(key).isBool()) return Notification::Ignored;
                rejected = rejected || !scope.value(key).toBool();
            }
        }
        initialized = !rejected;
        failure = rejected ? payload.value(QStringLiteral("message")).toString(
                                 message.value(QStringLiteral("message")).toString()) : QString();
        return rejected ? Notification::Failed : Notification::Ready;
    }
};
