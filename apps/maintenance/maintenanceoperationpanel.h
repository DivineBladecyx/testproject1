#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QWidget>

#include <functional>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;
class QTimer;

class MaintenanceOperationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MaintenanceOperationPanel(QWidget *parent = nullptr);
    void initialize(const QString &roomId);
    void setEnabledMethods(const QSet<QString> &methods);
    void updateTelemetry(const QJsonObject &payload);
    void invalidateStatus();

signals:
    void rpcRequested(const QJsonObject &request);

private:
    struct Field {
        QString device;
        QStringList aliases;
        QJsonValue value;
        QDateTime received;
        QLabel *label = nullptr;
        bool faultWhenTrue = false;
        bool positiveWhenTrue = false;
    };

    void bindStatus(const QString &device);
    void bindRpcButton(const QString &method, const std::function<QJsonObject()> &params);
    void addField(const QString &device, const QString &key, const QStringList &aliases,
                  bool faultWhenTrue = false, bool positiveWhenTrue = false);
    QJsonObject filterParams() const;
    void request(const QString &method, const QJsonObject &params);
    void refreshStatus();
    void selectDevice(int index);

    QString m_roomId;
    QString m_device = QStringLiteral("couch");
    QMap<QString, Field> m_fields;
    QMap<QString, QDateTime> m_deviceTimes;
    QMap<QString, QJsonValue> m_connected;
    QList<QPushButton *> m_buttons;
    QTabWidget *m_deviceTabs = nullptr;
    QLabel *m_controlTitle = nullptr;
    QLabel *m_statusTitle = nullptr;
    QLabel *m_age = nullptr;
    QSpinBox *m_cabinet = nullptr;
    QList<QCheckBox *> m_axes;
    QTableWidget *m_couchPosition = nullptr;
    QTableWidget *m_couchAxes = nullptr;
    QTableWidget *m_filterAxes = nullptr;
    QMap<QString, QDoubleSpinBox *> m_absolute;
    QMap<QString, QDoubleSpinBox *> m_offset;
    QTimer *m_refreshTimer = nullptr;
};
