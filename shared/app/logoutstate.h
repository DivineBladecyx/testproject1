#pragma once

#include "telemetryvalue.h"
#include <QElapsedTimer>
#include <QJsonObject>

struct LogoutState
{
    enum class Phase { Idle, Waiting, Uncertain };
    enum class Reply { Ignored, Rejected, Incomplete, Completed };
    Phase phase = Phase::Idle;
    QString requestId, username, roomId, message;
    QJsonValue liveBeam;
    QElapsedTimer beamAge;
    int deliveryResult = -1;
    QElapsedTimer deliveryAge;

    bool busy() const { return phase != Phase::Idle; }
    void observeBeam(const QJsonValue &value) {
        liveBeam = TelemetryValue::binaryFlag(value);
        if (liveBeam.isBool()) beamAge.start();
        else beamAge.invalidate();
    }
    void observeDeliveryResult(const QJsonValue &value) {
        const int code = value.isDouble() ? value.toInt(-1) : -1;
        if (value.isDouble() && value.toDouble() == code
            && (code == 0 || code == 1 || code == 2 || code == 7)) {
            deliveryResult = code;
            deliveryAge.start();
        } else {
            deliveryResult = -1;
            deliveryAge.invalidate();
        }
    }
    void invalidateBeam() {
        liveBeam = QJsonValue(); beamAge.invalidate();
        deliveryResult = -1; deliveryAge.invalidate();
    }
    bool deliveryFresh(qint64 maxAgeMs) const {
        return maxAgeMs > 0 && deliveryAge.isValid() && deliveryAge.elapsed() < maxAgeMs;
    }
    bool beamSafe(bool connected, bool synced, bool workflowBeamActive, qint64 maxAgeMs) const {
        const bool contradictoryBeamOn = liveBeam.isBool() && liveBeam.toBool()
        && beamAge.isValid() && beamAge.elapsed() < maxAgeMs;
        return connected && synced && !workflowBeamActive && deliveryFresh(maxAgeMs)
               && (deliveryResult == 0 || deliveryResult == 2) && !contradictoryBeamOn;
    }
    QJsonObject begin(const QString &id, const QString &user, const QString &room) {
        requestId = id; username = user; roomId = room; message.clear();
        phase = Phase::Waiting;
        return {{"cmd", "LOGOUT"}, {"username", user}, {"room_id", room},
                {"reason", "OPERATOR_LOGOUT"}};
    }
    Reply consume(const QJsonObject &reply) {
        if (!busy() || reply.value("type") != QStringLiteral("TASK")
            || reply.value("cmd") != QStringLiteral("TASK_LOGOUT_SUCCESS")) return Reply::Ignored;
        if (!reply.value("payload").isObject()) {
            phase = Phase::Uncertain;
            return Reply::Incomplete;
        }
        const QJsonObject payload = reply.value("payload").toObject();
        const QString status = payload.value("status").toString();
        if (status == QStringLiteral("SUCCESS") && payload.value("room_id") != roomId)
            return Reply::Ignored;
        message = payload.value("message").toString();
        if (status == QStringLiteral("REJECTED")) {
            phase = Phase::Idle;
            return Reply::Rejected;
        }
        if (status == QStringLiteral("SUCCESS")) {
            phase = Phase::Idle;
            return Reply::Completed;
        }
        phase = Phase::Uncertain;
        return Reply::Incomplete;
    }
};
