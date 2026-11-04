#include "auditlogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>
#include <initializer_list>

namespace {

constexpr qsizetype kMaximumEmbeddedPayloadBytes = 64 * 1024;

QString jsonValueAsString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    return QString();
}

QString firstString(const QJsonObject &object,
                    std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString value = jsonValueAsString(object.value(QLatin1String(key)));
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

bool isSensitiveKey(const QString &key)
{
    const QString normalized = key.toLower();
    return normalized.contains(QStringLiteral("password"))
           || normalized.contains(QStringLiteral("passwd"))
           || normalized.contains(QStringLiteral("token"))
           || normalized.contains(QStringLiteral("secret"))
           || normalized.contains(QStringLiteral("authorization"))
           || normalized.contains(QStringLiteral("credential"))
           || normalized == QStringLiteral("patient_name")
           || normalized == QStringLiteral("patientname");
}

QJsonValue sanitizedValue(const QJsonValue &value);

QJsonObject sanitizedObject(const QJsonObject &object)
{
    QJsonObject sanitized;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (isSensitiveKey(it.key())) {
            sanitized.insert(it.key(), QStringLiteral("[REDACTED]"));
        } else {
            sanitized.insert(it.key(), sanitizedValue(it.value()));
        }
    }
    return sanitized;
}

QJsonValue sanitizedValue(const QJsonValue &value)
{
    if (value.isObject()) {
        return sanitizedObject(value.toObject());
    }
    if (value.isArray()) {
        QJsonArray array;
        for (const QJsonValue &item : value.toArray()) {
            array.append(sanitizedValue(item));
        }
        return array;
    }
    return value;
}

QString upperSnakeCase(QString value)
{
    static const QRegularExpression separators(QStringLiteral("[^A-Za-z0-9]+"));
    static const QRegularExpression edgeUnderscores(QStringLiteral("^_+|_+$"));
    value = value.trimmed();
    value.replace(separators, QStringLiteral("_"));
    value.remove(edgeUnderscores);
    return value.toUpper();
}

QString normalizedAction(const QString &action)
{
    static const QHash<QString, QString> mappings = {
                                                     {QStringLiteral("treatment_client_started"), QStringLiteral("SERVICE_STARTED")},
                                                     {QStringLiteral("treatment_client_closed"), QStringLiteral("SERVICE_STOPPED")},
                                                     {QStringLiteral("login_success"), QStringLiteral("OPERATOR_LOGIN")},
                                                     {QStringLiteral("task_login_success_received"), QStringLiteral("OPERATOR_LOGIN")},
                                                     {QStringLiteral("login_failed"), QStringLiteral("OPERATOR_LOGIN_FAILED")},
                                                     {QStringLiteral("logout"), QStringLiteral("OPERATOR_LOGOUT")},
                                                     {QStringLiteral("websocket_connected"), QStringLiteral("DEVICE_CONNECTED")},
                                                     {QStringLiteral("websocket_disconnected"), QStringLiteral("DEVICE_DISCONNECTED")},
                                                     {QStringLiteral("websocket_error"), QStringLiteral("DEVICE_ERROR")},
                                                     {QStringLiteral("task_error_stopped"), QStringLiteral("SESSION_ABORTED")},
                                                     {QStringLiteral("plan_confirm_request_received"), QStringLiteral("PLAN_CONFIRMATION_REQUESTED")},
                                                     {QStringLiteral("plan_confirmed"), QStringLiteral("PLAN_CONFIRMATION_SUBMITTED")},
                                                     {QStringLiteral("can_apply_beam_received"), QStringLiteral("BEAM_APPLICATION_ENABLED")},
                                                     {QStringLiteral("apply_beam_clicked"), QStringLiteral("BEAM_REQUESTED")},
                                                     {QStringLiteral("beam_ready_received"), QStringLiteral("BEAM_READY")},
                                                     {QStringLiteral("beam_on_received"), QStringLiteral("BEAM_ON")},
                                                     {QStringLiteral("beam_finished_received"), QStringLiteral("BEAM_OFF")},
                                                     {QStringLiteral("beam_paused_received"), QStringLiteral("BEAM_PAUSE")},
                                                     {QStringLiteral("beam_resumed_received"), QStringLiteral("BEAM_RESUME")},
                                                     {QStringLiteral("chart_generated_received"), QStringLiteral("TREATMENT_REPORT_GENERATED")},
                                                     {QStringLiteral("report_confirmed"), QStringLiteral("TREATMENT_REPORT_CONFIRMED")},
                                                     {QStringLiteral("send_couch_position"), QStringLiteral("BED_POSITION_COMMAND_SENT")},
                                                     {QStringLiteral("state_transition"), QStringLiteral("WORKFLOW_STATE_CHANGED")},
                                                     {QStringLiteral("state_transition_rejected"), QStringLiteral("WORKFLOW_STATE_CHANGE_REJECTED")},
                                                     {QStringLiteral("task_command_sent"), QStringLiteral("TASK_COMMAND_SENT")},
                                                     };

    const QString key = action.trimmed().toLower();
    const auto it = mappings.constFind(key);
    if (it != mappings.constEnd()) {
        return it.value();
    }
    const QString normalized = upperSnakeCase(action);
    return normalized.isEmpty() ? QStringLiteral("UNSPECIFIED") : normalized;
}

QString normalizedCategory(const QString &category,
                           const QString &canonicalAction)
{
    const QString normalized = category.trimmed().toLower();
    if (canonicalAction.startsWith(QStringLiteral("OPERATOR_"))
        || canonicalAction.startsWith(QStringLiteral("TOKEN_"))) {
        return QStringLiteral("auth");
    }
    if (canonicalAction.startsWith(QStringLiteral("BED_"))
        || normalized == QStringLiteral("couch")
        || normalized == QStringLiteral("positioning")) {
        return QStringLiteral("positioning");
    }
    if (canonicalAction.startsWith(QStringLiteral("BEAM_"))
        || normalized == QStringLiteral("beam")) {
        return QStringLiteral("beam");
    }
    if (canonicalAction.startsWith(QStringLiteral("PLAN_"))
        || canonicalAction.startsWith(QStringLiteral("CALIBRATION_"))
        || canonicalAction.startsWith(QStringLiteral("DOSE_CONFIRMATION_"))
        || normalized == QStringLiteral("treatment_plan")
        || normalized == QStringLiteral("plan")) {
        return QStringLiteral("plan");
    }

    if (normalized == QStringLiteral("auth")
        || normalized == QStringLiteral("credential")
        || normalized == QStringLiteral("permission")) {
        return QStringLiteral("auth");
    }
    if (normalized == QStringLiteral("treatment")
        || normalized == QStringLiteral("treatment_flow")
        || normalized == QStringLiteral("workflow")
        || normalized == QStringLiteral("report")) {
        return QStringLiteral("treatment");
    }
    return QStringLiteral("system");
}

QString normalizedOutcome(const QString &result)
{
    const QString normalized = result.trimmed().toLower();
    if (normalized == QStringLiteral("success")
        || normalized == QStringLiteral("info")
        || normalized == QStringLiteral("attempt")) {
        return QStringLiteral("success");
    }
    return QStringLiteral("failure");
}

QString levelForResult(const QString &result)
{
    const QString normalized = result.trimmed().toLower();
    if (normalized == QStringLiteral("failed")
        || normalized == QStringLiteral("failure")
        || normalized == QStringLiteral("error")
        || normalized == QStringLiteral("fatal")) {
        return QStringLiteral("ERROR");
    }
    if (normalized == QStringLiteral("warning")
        || normalized == QStringLiteral("cancelled")
        || normalized == QStringLiteral("canceled")) {
        return QStringLiteral("WARN");
    }
    return QStringLiteral("INFO");
}

} // namespace

AuditLogger &AuditLogger::instance()
{
    static AuditLogger logger;
    return logger;
}

AuditLogger::AuditLogger()
    : m_workstation(QSysInfo::machineHostName())
{
    qInfo().noquote()
    << QStringLiteral("[AuditLogger] 审计日志路径: %1")
            .arg(currentLogFilePath());
}

AuditLogger::~AuditLogger() = default;

QString AuditLogger::logDirectory() const
{
    const QString configuredPath =
        qEnvironmentVariable("TCS_LOG_DIR").trimmed();
    if (!configuredPath.isEmpty()) {
        QDir().mkpath(configuredPath);
        return QDir::cleanPath(configuredPath);
    }

    QString commonDataPath =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (commonDataPath.isEmpty()) {
        commonDataPath = QStringLiteral("C:/ProgramData");
    }

    const QString preferredPath =
        QDir(commonDataPath).filePath(QStringLiteral("TCS/logs"));
    if (QDir().mkpath(preferredPath) || QDir(preferredPath).exists()) {
        return QDir::cleanPath(preferredPath);
    }

    QString fallbackBase =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (fallbackBase.isEmpty()) {
        fallbackBase = QCoreApplication::applicationDirPath();
    }
    const QString fallbackPath =
        QDir(fallbackBase).filePath(QStringLiteral("logs"));
    QDir().mkpath(fallbackPath);
    return QDir::cleanPath(fallbackPath);
}

QString AuditLogger::currentLogFilePath() const
{
    return QDir(logDirectory())
    .filePath(QStringLiteral("tcs-gui-audit.log"));
}

void AuditLogger::setContext(const QString &clientName,
                             const QString &username,
                             const QString &role,
                             const QString &roomId,
                             const QString &operatorSub)
{
    QMutexLocker locker(&m_mutex);
    m_clientName = clientName;
    m_username = username;
    m_role = role;
    m_roomId = roomId;
    m_operatorSub = operatorSub;
    m_workstation = QSysInfo::machineHostName();
}

void AuditLogger::clearTreatmentContext()
{
    QMutexLocker locker(&m_mutex);
    m_medicalId.clear();
    m_courseId.clear();
    m_planVersionId.clear();
    m_fractionId.clear();
    m_sessionId.clear();
    m_tcsSessionId.clear();
}

QJsonObject AuditLogger::contextObject() const
{
    QJsonObject context;

    QString version = QCoreApplication::applicationVersion().trimmed();
    if (version.isEmpty()) {
        version = QStringLiteral("0.1.0");
    }

    context.insert(QStringLiteral("service.name"), QStringLiteral("tcs-gui"));
    context.insert(QStringLiteral("service.version"), version);
    context.insert(QStringLiteral("host.name"), m_workstation);

    if (!m_clientName.isEmpty()) {
        context.insert(QStringLiteral("service.component"), m_clientName);
    }
    if (!m_username.isEmpty()) {
        context.insert(QStringLiteral("operator.username"), m_username);
    }
    if (!m_role.isEmpty()) {
        context.insert(QStringLiteral("operator.roles"), m_role);
    }
    if (!m_operatorSub.isEmpty()) {
        context.insert(QStringLiteral("operator.sub"), m_operatorSub);
    }
    if (!m_roomId.isEmpty()) {
        context.insert(QStringLiteral("pt.room_id"), m_roomId);
    }
    if (!m_medicalId.isEmpty()) {
        context.insert(QStringLiteral("pt.medical_id"), m_medicalId);
    }
    if (!m_courseId.isEmpty()) {
        context.insert(QStringLiteral("pt.course_id"), m_courseId);
    }
    if (!m_planVersionId.isEmpty()) {
        context.insert(QStringLiteral("pt.plan_version_id"),
                       m_planVersionId);
    }
    if (!m_fractionId.isEmpty()) {
        context.insert(QStringLiteral("pt.fraction_id"), m_fractionId);
    }
    if (!m_sessionId.isEmpty()) {
        context.insert(QStringLiteral("pt.session_id"), m_sessionId);
    }
    if (!m_tcsSessionId.isEmpty()) {
        context.insert(QStringLiteral("pt.tcs_session_id"), m_tcsSessionId);
    }

    return context;
}

void AuditLogger::logEvent(const QString &category,
                           const QString &action,
                           const QString &objectType,
                           const QString &objectId,
                           const QString &result,
                           const QString &message,
                           const QJsonObject &payload,
                           const QJsonObject &beforeState,
                           const QJsonObject &afterState)
{
    const QString canonicalAction = normalizedAction(action);
    const QString canonicalCategory = normalizedCategory(category,
                                                         canonicalAction);
    const QString canonicalOutcome = normalizedOutcome(result);
    const QJsonObject safePayload = sanitizedObject(payload);
    const QJsonObject safeBeforeState = sanitizedObject(beforeState);
    const QJsonObject safeAfterState = sanitizedObject(afterState);

    const QString medicalId = firstString(
        payload,
        {"pt.medical_id", "medical_id", "patient_id", "patientId"});
    const QString courseId = firstString(
        payload,
        {"pt.course_id", "course_id", "courseId"});
    const QString planVersionId = firstString(
        payload,
        {"pt.plan_version_id", "plan_version_id", "plan_id"});
    const QString fractionId = firstString(
        payload,
        {"pt.fraction_id", "fraction_id", "fractionId"});
    const QString sessionId = firstString(
        payload,
        {"pt.session_id", "session_id", "treatment_id", "treatmentId"});
    const QString tcsSessionId = firstString(
        payload,
        {"pt.tcs_session_id", "tcs_session_id"});
    const QString payloadRoomId = firstString(
        payload,
        {"pt.room_id", "room_id", "roomId"});

    QJsonObject record;
    QString filePath;
    {
        QMutexLocker locker(&m_mutex);
        if (!medicalId.isEmpty() && medicalId != m_medicalId) {
            m_courseId.clear();
            m_planVersionId.clear();
            m_fractionId.clear();
            m_sessionId.clear();
            m_tcsSessionId.clear();
        }
        if (!fractionId.isEmpty() && fractionId != m_fractionId) {
            m_sessionId.clear();
            m_tcsSessionId.clear();
        }

        if (!medicalId.isEmpty()) {
            m_medicalId = medicalId;
        }
        if (!courseId.isEmpty()) {
            m_courseId = courseId;
        }
        if (!planVersionId.isEmpty()) {
            m_planVersionId = planVersionId;
        }
        if (!fractionId.isEmpty()) {
            m_fractionId = fractionId;
        }
        if (!sessionId.isEmpty()) {
            m_sessionId = sessionId;
        }
        if (!tcsSessionId.isEmpty()) {
            m_tcsSessionId = tcsSessionId;
        }
        if (!payloadRoomId.isEmpty()) {
            m_roomId = payloadRoomId;
        }

        record = contextObject();
    }
    // Directory access must not hold the context mutex.
    filePath = currentLogFilePath();

    record.insert(QStringLiteral("@timestamp"),
                  QDateTime::currentDateTimeUtc()
                      .toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'")));
    record.insert(QStringLiteral("log.level"), levelForResult(result));
    record.insert(QStringLiteral("log.type"), QStringLiteral("audit"));
    record.insert(QStringLiteral("message"),
                  message.isEmpty() ? canonicalAction : message);
    record.insert(QStringLiteral("event.action"), canonicalAction);
    record.insert(QStringLiteral("event.category"), canonicalCategory);
    record.insert(QStringLiteral("event.outcome"), canonicalOutcome);

    if (canonicalAction.compare(action, Qt::CaseInsensitive) != 0) {
        record.insert(QStringLiteral("event.original_action"), action);
    }
    if (canonicalOutcome.compare(result, Qt::CaseInsensitive) != 0) {
        record.insert(QStringLiteral("event.original_outcome"), result);
    }
    if (!objectType.isEmpty()) {
        record.insert(QStringLiteral("event.object.type"), objectType);
    }
    if (!objectId.isEmpty()) {
        record.insert(QStringLiteral("event.object.id"), objectId);
    }

    if (!safeBeforeState.isEmpty()) {
        record.insert(QStringLiteral("event.before_state"), safeBeforeState);
    }
    if (!safeAfterState.isEmpty()) {
        record.insert(QStringLiteral("event.after_state"), safeAfterState);
    }

    if (!safePayload.isEmpty()) {
        const QByteArray compactPayload =
            QJsonDocument(safePayload).toJson(QJsonDocument::Compact);
        if (compactPayload.size() <= kMaximumEmbeddedPayloadBytes) {
            record.insert(QStringLiteral("event.payload"), safePayload);
        } else {
            record.insert(QStringLiteral("event.payload_truncated"), true);
            record.insert(QStringLiteral("event.payload_bytes"),
                          compactPayload.size());
        }
    }

    QByteArray line = QJsonDocument(record).toJson(QJsonDocument::Compact);
    line.append('\n');
    appendRecord(filePath, line);
}

void AuditLogger::appendRecord(const QString &filePath, const QByteArray &line)
{
    QMutexLocker writeLocker(&m_writeMutex);
    const QFileInfo fileInfo(filePath);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        qCritical().noquote()
        << QStringLiteral("[AuditLogger] 无法创建日志目录: %1")
                .arg(fileInfo.absolutePath());
        return;
    }

    constexpr int maximumAttempts = 3;
    QString lastError;
    for (int attempt = 1; attempt <= maximumAttempts; ++attempt) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly
                      | QIODevice::Append
                      | QIODevice::Text)) {
            const qint64 writtenBytes = file.write(line);
            if (writtenBytes == line.size() && file.flush()) {
                return;
            }
            lastError =
                QStringLiteral("%1，期望写入 %2 字节，实际写入 %3 字节")
                    .arg(file.errorString())
                    .arg(line.size())
                    .arg(writtenBytes);
            // Bytes may already be present. Reopening and retrying the entire
            // record could duplicate it or append it after an incomplete line.
            break;
        } else {
            lastError = file.errorString();
        }

        if (attempt < maximumAttempts) {
            QThread::msleep(100);
        }
    }

    qCritical().noquote()
        << QStringLiteral("[AuditLogger] 审计日志写入失败: %1；原因: %2")
               .arg(filePath, lastError);
}
