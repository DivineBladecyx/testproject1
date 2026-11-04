#ifndef AUDITLOGGER_H
#define AUDITLOGGER_H

#include <QJsonObject>
#include <QByteArray>
#include <QMutex>
#include <QString>

class AuditLogger
{
public:
    static AuditLogger &instance();
    ~AuditLogger();

    void setContext(const QString &clientName,
                    const QString &username,
                    const QString &role,
                    const QString &roomId,
                    const QString &operatorSub = QString());
    void clearTreatmentContext();

    void logEvent(const QString &category,
                  const QString &action,
                  const QString &objectType,
                  const QString &objectId,
                  const QString &result,
                  const QString &message = QString(),
                  const QJsonObject &payload = QJsonObject(),
                  const QJsonObject &beforeState = QJsonObject(),
                  const QJsonObject &afterState = QJsonObject());

    QString logDirectory() const;

private:
    AuditLogger();
    AuditLogger(const AuditLogger &) = delete;
    AuditLogger &operator=(const AuditLogger &) = delete;

    QString currentLogFilePath() const;
    QJsonObject contextObject() const;
    void appendRecord(const QString &filePath, const QByteArray &line);

    mutable QMutex m_mutex;
    QString m_clientName;
    QString m_username;
    QString m_role;
    QString m_roomId;
    QString m_operatorSub;
    QString m_workstation;
    QString m_medicalId;
    QString m_courseId;
    QString m_planVersionId;
    QString m_fractionId;
    QString m_sessionId;
    QString m_tcsSessionId;
    QMutex m_writeMutex;
};

#endif // AUDITLOGGER_H
