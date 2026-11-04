#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace TelemetryValue {

// Only explicit binary values are valid; arbitrary numbers and strings stay unknown.
inline QJsonValue binaryFlag(const QJsonValue &value)
{
    if (value.isBool()) return value;
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (number == 0.0 || number == 1.0) return QJsonValue(number == 1.0);
    }
    return QJsonValue();
}

inline QJsonValue binaryFlag(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = binaryFlag(object.value(key));
        if (value.isBool()) return value;
    }
    return QJsonValue();
}

} // namespace TelemetryValue
