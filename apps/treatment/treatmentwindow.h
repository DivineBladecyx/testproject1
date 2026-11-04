#ifndef TREATMENTWINDOW_H
#define TREATMENTWINDOW_H
#include "logindialog.h"
#include "clientbootstrap.h"
#include "startupstate.h"
#include "logoutstate.h"
#include "websocketclient.h"
#include <QMainWindow>
#include <QSettings>
#include "thememanager.h"
#include <QShowEvent>
#include "bedcontroller.h"
#include <QQuickWidget>
#include <QPointer>
#include <QTimer>
#include <QHeaderView>
#include <QTableWidget>
#include <QTabBar>
#include <QSplitter>
#include <QJsonArray>
#include <QSet>
#include <QVector>
#include <QStringList>

class QLabel;
class QPushButton;
class QProgressBar;
class QCloseEvent;
class QTabWidget;
class QTreeWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class TreatmentWindow;
}
QT_END_NAMESPACE

class LoginDialog;
class TodaySchedule;
class WebSocketClient;

enum class TreatmentState {
    Idle,              // 等待任务
    PlanConfirm,       // 计划确认
    Preparing,         // 准备 / 摆位
    ReadyToApplyBeam,  // 可申请束流
    BeamRequested,     // 束流申请中
    BeamReady,         // 束流已就绪
    Treating,          // 治疗中
    Paused,            // 后端确认暂停
    BeamFinished,      // 出束结束
    ReportConfirm,     // 报告确认
    Completed,         // 完成
    ErrorStopped       // 异常中止
};


class TreatmentWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TreatmentWindow(WebSocketClient *webSocketClient,
                             const QString &username,
                             const QString &authToken,
                             const QJsonObject &userInfo,
                             const QJsonObject &loginResponse,
                             const QString &loginPassword = QString(),
                             QWidget *parent = nullptr);
    ~TreatmentWindow();
    using ClientModeSwitchHandler = std::function<void(LoginDialog::ClientMode, const ClientSession &)>;
    void setClientModeSwitchHandler(ClientModeSwitchHandler handler);

private slots:
    void onThemeChanged(Theme newTheme);
    void onLoginSuccess(const QString &username,
                        const QString &authToken,
                        WebSocketClient *client,
                        const QJsonObject &loginResponse);
    void onLoginFailed(const QString &error);

    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(const QString &error);
    void onWebSocketDataReceived(const QJsonObject &data);
    void onWebSocketDataSent(const QString &messageId);
    void onWorkflowStageSelected(int stageIndex);
    void onWorkflowStepSelected(int stepIndex);

    void initTheme();
    void initWebSocket();
    void initBedView();
    void initStepView();
    void initConnectionState();
    void syncWorkflowStep(int stepIndex);
    void connectWorkflowViews();

    void handleRspMessage(const QJsonObject &data);
    void handleTelemetryMessage(const QJsonObject &data);
    void handleTaskMessage(const QJsonObject &data);
    void handleGrpcCall(const QJsonObject &data);
    void handleTaskErrorStopped(const QJsonObject &data);

    void on_bed_pushButton_clicked();

    void on_todays_pushButton_clicked();

    void on_theme_pushButton_clicked();

    void on_logout_pushButton_clicked();

    void on_beamApply_pushButton_clicked();

    void on_beamApply_cancel_pushButton_clicked();

    void on_confirmPlan_pushButton_clicked();

    void on_confirmPlan_cancel_pushButton_clicked();
    void showCurrentPlanDetail();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::TreatmentWindow *ui;
    QPointer<LoginDialog> loginDialog;
    QPointer<QDialog> m_credentialDialog;
    bool m_loginTransitionPending = false;
    bool m_closing = false;
    StartupState m_startupState;
    LogoutState m_logoutState;
    QTimer *m_logoutTimer = nullptr;
    bool m_closeAfterLogout = false;
    bool m_logoutConfirming = false;
    void setupLogoutFlow();
    QString logoutBlockReason() const;
    bool logoutBeamSafe() const;
    bool handleLogoutReply(const QJsonObject &message);
    void completeLogout(bool backendConfirmed);
    QLabel *m_initializationLabel = nullptr;
    bool sessionReady() const;
    void updateSessionUi();
    void handleInitializationNotification(const QJsonObject &message);
    ClientModeSwitchHandler m_clientModeSwitchHandler;
    QSettings *m_settings;
    WebSocketClient *m_webSocketClient;
    QString m_currentUsername;
    QString m_loginPassword;
    QString m_authToken;
    QJsonObject m_userInfo;
    void updateUserInfoUi();
    QString m_middlewareUrl;
    QString m_wsMiddlewareUrl;
    QString m_roomId;

    QTimer *m_clockTimer = nullptr;
    void initClock();
    void updateTimeLabel();


    BedController *m_bedController;
    QQuickWidget *m_bedView = nullptr;
    QQuickWidget *m_stepView = nullptr;
    QPointer<TodaySchedule> m_todayScheduleDialog;
    void updateStepBar();
    void updateTreatmentActionDisplay();
    void connectWebSocketSignals();
    void showLoginDialog();
    void loadMiddlewareSettings();
    void connectToMiddlewareWebSocket();
    bool showReportConfirmDialog(const QString &reportText);
    void initUiLayoutTuning();
    void setupClinicalWorkspace();
    void updateClinicalWorkspace();
    void updateClinicalDeviceLayout();
    void queueClinicalFaults(const QStringList &issues);
    void showClinicalFaults();
    QWidget *m_clinicalBody = nullptr;
    QTabWidget *m_executionTabs = nullptr;
    QTreeWidget *m_fieldSummary = nullptr;
    QPointer<QDialog> m_clinicalFaultDialog;
    QTimer *m_clinicalFaultTimer = nullptr;
    QStringList m_clinicalFaults;
    QSet<QString> m_announcedClinicalFaults;
    QJsonObject m_clinicalDisplayedPlan;
    void initTables();
    void initStatusIndicators();
    void updateDeviceStatusIndicators(const QJsonObject &payload);
    void setupDoseInfoPanel();
    void updateBeamMonitorSummary();
    void updateDoseInfoPanel(const QJsonObject &payload = QJsonObject());
    void setupBeamRoomOverview();
    void updateBeamRoomOverview(const QString &activeRoomId = QString());
    void updateAuditContext() const;
    void auditTreatmentEvent(const QString &category,
                             const QString &action,
                             const QString &objectType = QString(),
                             const QString &objectId = QString(),
                             const QString &result = QStringLiteral("success"),
                             const QString &message = QString(),
                             const QJsonObject &payload = QJsonObject(),
                             const QJsonObject &beforeState = QJsonObject(),
                             const QJsonObject &afterState = QJsonObject()) const;
    QJsonObject currentAuditObjectPayload() const;
    void updateTodaySchedule(const QJsonArray &entries);
    void resetTreatmentSessionAfterStop();
    QJsonArray m_todayScheduleEntries;
    QJsonObject m_currentPlanPayload;
    QJsonObject m_latestTelemetryPayload;
    QString m_lastDoseTelemetrySnapshot;
    QVector<QLabel*> m_beamRoomLabels;
    QVector<QLabel*> m_doseInfoValueLabels;
    QString m_activeBeamRoomId;

    TreatmentState m_treatmentState = TreatmentState::Idle;
    TreatmentState m_previousTreatmentState = TreatmentState::Idle;
    QString m_currentDeviceActionText;
    QString m_lastGrpcCall;
    QStringList m_grpcCallHistory;
    void clearGrpcCallDisplay();
    QString m_backendWorkflowState;
    QString currentWorkflowActionText() const;
    QStringList m_currentDeviceIssues;
    QString m_workflowErrorMessage;
    bool m_workflowSyncRequired = true;
    bool m_planConfirmationPending = false;
    bool m_workflowConfirmationPending = false;
    quint64 m_workflowConfirmationRevision = 0;
    TreatmentState m_workflowConfirmationState = TreatmentState::Idle;
    QJsonObject m_lastWorkflowSnapshot;
    QJsonObject m_deferredPlanConfirmation;
    void scheduleWorkflowConfirmation();
    bool m_resetPending = false;
    bool m_workflowReauthPending = false;
    quint64 m_workflowRevision = 0;
    QTimer *m_workflowTimer = nullptr;
    QPushButton *m_queryWorkflowButton = nullptr;
    QPushButton *m_resetWorkflowButton = nullptr;
    void setupWorkflowRecovery();
    void queryWorkflowState();
    void restorePendingWorkflowMessages();
    void requestWorkflowReset();
    void handleWorkflowStateSync(const QJsonObject &snapshot);
    void handleWorkflowReset();
    bool m_waitingOisPatientCallLogged = false;

    bool setTreatmentState(TreatmentState newState, const QString &reason = QString());
    bool canTransitTo(TreatmentState from, TreatmentState to) const;
    void applyTreatmentStateToUi();
    int treatmentStateToStep(TreatmentState state) const;
    QString treatmentStateText(TreatmentState state) const;

    bool ensureConnected() const;
    bool ensureStateAllowed(const QList<TreatmentState> &allowedStates,
                            const QString &operationName) const;
    void sendTaskCommand(const QString &cmd);
    bool showCredentialConfirmDialog(const QString &windowTitle,
                                     const QString &prompt,
                                     const QString &okText,
                                     const QVector<QPair<QString, QString>> &summaryRows,
                                     QString *confirmedUsername,
                                     QString *confirmedPassword);
    bool validateCredentialConfirmation(const QString &confirmedUsername,
                                        const QString &confirmedPassword);
    bool sendCredentialTaskCommand(const QString &cmd,
                                   const QString &confirmedUsername,
                                   const QString &confirmedPassword,
                                   const QString &successMessage);
    bool showPlanConfirmationDialog(QString *confirmedUsername,
                                    QString *confirmedPassword);
    bool sendPlanConfirmedCommand(const QString &confirmedUsername,
                                  const QString &confirmedPassword);
    bool showBeamApplyConfirmationDialog(QString *confirmedUsername,
                                         QString *confirmedPassword);
    bool sendBeamApplyCommand(const QString &confirmedUsername,
                              const QString &confirmedPassword);

};
#endif // TREATMENTWINDOW_H

