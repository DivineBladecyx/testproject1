#pragma once

#include <QJsonObject>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QStringList>

class QCheckBox;
class QAction;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class WebSocketClient;
class MaintenanceOperationPanel;

class MaintenanceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MaintenanceWindow(WebSocketClient *webSocketClient,
                               const QString &username,
                               const QString &authToken,
                               const QJsonObject &userInfo,
                               const QJsonObject &loginResponse,
                               QWidget *parent = nullptr);
    ~MaintenanceWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi(const QString &displayName);
    void showDebugTools(int page);
    void rebuildMethods();
    void filterMethods();
    void selectMethod();
    void updatePreview();
    QJsonObject selectedRpc(QString *error = nullptr) const;
    bool canControl() const;
    bool canSendPriority(const QString &key = QString()) const;
    void sendPriority(const QJsonObject &request);
    void updateControls();
    void requestMode();
    void invokeRpc();
    void executeRpc(const QJsonObject &request, const QString &error = QString());
    void forceState();
    void resetHardware();
    void releaseUncertainOperation();
    bool confirmCommand(const QJsonObject &request, const QString &warning);
    void sendControl(const QJsonObject &request);
    void sendQuery(const QString &cmd);
    void handleMessage(const QJsonObject &message);
    void handleManifest(const QJsonObject &payload);
    void connectionChanged(bool connected);
    void operationTimedOut();
    void finishPending(const QString &outcome, const QJsonObject &reply);
    void appendRecord(const QString &direction, const QString &outcome,
                      const QJsonObject &message);
    void audit(const QString &action, const QString &outcome,
               const QJsonObject &details = QJsonObject());
    void exportRecords();

    QPointer<WebSocketClient> m_client;
    QString m_username;
    QString m_role;
    QString m_roomId;
    QString m_mode;
    bool m_authorized = false;
    bool m_hasManifest = false;
    bool m_uncertain = false;
    bool m_confirming = false;
    quint64 m_revision = 0;
    QSet<QString> m_serverMethods;
    QJsonObject m_pending;
    QJsonObject m_priorityPending;
    QJsonObject m_unconfirmedPriority;
    QSet<QString> m_ambiguousPriorityKeys;
    QMap<QString, QTimer *> m_priorityTimers;
    // No protocol request ID: retire timed-out RPC identities for this window.
    QSet<QString> m_retiredRpcKeys;
    QMap<QString, QWidget *> m_paramEditors;

    QTimer *m_operationTimer = nullptr;
    QTimer *m_queryTimer = nullptr;
    QSet<QString> m_queries;
    QLabel *m_connectionLabel = nullptr;
    QLabel *m_modeLabel = nullptr;
    QLabel *m_notice = nullptr;
    QLabel *m_methodTitle = nullptr;
    QLabel *m_resultLabel = nullptr;
    QLabel *m_priorityLabel = nullptr;
    QLineEdit *m_search = nullptr;
    QLineEdit *m_roomEdit = nullptr;
    QTreeWidget *m_methods = nullptr;
    QWidget *m_parameterPanel = nullptr;
    QFormLayout *m_parameters = nullptr;
    QPlainTextEdit *m_preview = nullptr;
    QPlainTextEdit *m_result = nullptr;
    QPlainTextEdit *m_status = nullptr;
    QPlainTextEdit *m_telemetry = nullptr;
    QTimer *m_telemetryTimer = nullptr;
    QJsonObject m_lastTelemetry;
    QPlainTextEdit *m_wire = nullptr;
    QTableWidget *m_records = nullptr;
    QComboBox *m_state = nullptr;
    QPushButton *m_modeButton = nullptr;
    QPushButton *m_invokeButton = nullptr;
    QPushButton *m_forceButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_releaseButton = nullptr;
    QPushButton *m_statusButton = nullptr;
    QPushButton *m_manifestButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    MaintenanceOperationPanel *m_operations = nullptr;
    QDockWidget *m_debugDock = nullptr;
    QTabWidget *m_debugTabs = nullptr;
    QAction *m_recordAction = nullptr;
};
