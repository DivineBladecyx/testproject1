#pragma once

#include <QString>
#include <QStringList>

namespace RoomCapabilities {

inline bool hasDrLift(const QString &roomId)
{
    return roomId.trimmed().compare(QStringLiteral("ROOM_9"), Qt::CaseInsensitive) != 0;
}

inline bool supportsRpc(const QString &roomId, const QString &target, const QString &method)
{
    if (hasDrLift(roomId) || target != QStringLiteral("device")) return true;
    static const QStringList drLiftMethods = {
        QStringLiteral("moveDrToZero"), QStringLiteral("moveDrToWork"),
        QStringLiteral("resetDrAlarm"), QStringLiteral("setDrEstop"),
        QStringLiteral("confirmDrMotionEnable")
    };
    return !drLiftMethods.contains(method);
}

} // namespace RoomCapabilities
