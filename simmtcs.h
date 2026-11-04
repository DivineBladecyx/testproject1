#ifndef SIMMTCS_H
#define SIMMTCS_H
#include "logindialog.h"
#include "websocketclient.h"
#include <QMainWindow>
#include <QSettings>
#include "thememanager.h"
#include "StepProgressManager.h"
#include <QShowEvent>   // 记得加这个
#include <QResizeEvent>
#include "bedcontroller.h"
#include <QQuickWidget>
#include <QPointer>
#include <QTimer>
#include <QHeaderView>
#include <QTableWidget>
#include <QTabBar>
#include <QSplitter>
#include <QJsonArray>

QT_BEGIN_NAMESPACE
namespace Ui {
class SimmTcs;
}
QT_END_NAMESPACE

class LoginDialog;
class TodaySchedule;
class WebSocketClient;
// simmtcs.h

enum class TreatmentState {
    Idle,              // 等待任务
    PlanConfirm,       // 计划确认
    Preparing,         // 准备 / 摆位
    ReadyToApplyBeam,  // 可申请束流
    BeamReady,         // 束流已就绪
    Treating,          // 治疗中
    BeamFinished,      // 出束结束
    ReportConfirm,     // 报告确认
    Completed,         // 完成
    ErrorStopped       // 异常中止
};


class SimmTcs : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimmTcs(WebSocketClient *webSocketClient,
                     const QString &username,
                     const QString &authToken,
                     const QJsonObject &userInfo,
                     const QJsonObject &loginResponse,
                     QWidget *parent = nullptr);
    ~SimmTcs();
private slots:
    void onThemeChanged(Theme newTheme);
    void onLoginSuccess(const QString &username,const QString &authToken, WebSocketClient *client,const QJsonObject &loginResponse);
    //void showLoginDialog();
    void onLoginFailed(const QString &error);
    //void onmiddlewareChanged(const QString &middlewareUrl);
    //void on_actionLogout_triggered();

    // WebSocket相关槽函数
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(const QString &error);
    void onWebSocketDataReceived(const QJsonObject &data);
    //void onWebSocketTextReceived(const QString &message);
    void onWebSocketDataSent(const QString &messageId);
    //void onWebSocketDataSent(const QString &messageId)
    //void connectWebSocketSignals();
    void onWebSocketReconnecting();

    void initTheme();
    void initWebSocket();
    void initBedView();
    void initStepView();
    void initConnectionState();

    void handleRspMessage(const QJsonObject &data);
    void handleTelemetryMessage(const QJsonObject &data);
    void handleTaskMessage(const QJsonObject &data);
    void handleTaskErrorStopped(const QJsonObject &data);

    // UI按钮槽函数
    //void onConnectButtonClicked();
    //void onDisconnectButtonClicked();
    //void onSendButtonClicked();
    //void onClearButtonClicked();

    // 定时器槽函数（用于定期发送数据）
    //void onSendTimerTimeout();





    void on_bed_pushButton_clicked();

    void on_todays_pushButton_clicked();

    void on_theme_pushButton_clicked();

    void on_logout_pushButton_clicked();

    void on_beamApply_pushButton_clicked();

    void on_beamApply_cancel_pushButton_clicked();

    void on_confirmPlan_pushButton_clicked();

    void on_confirmPlan_cancel_pushButton_clicked();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::SimmTcs *ui;
    LoginDialog *loginDialog;
    //QString m_currentUsername;
    QSettings *m_settings; // 可保留用于保存其他设置
    // WebSocket相关成员
    WebSocketClient *m_webSocketClient;
    QString m_currentUsername;
    QString m_authToken;
    QJsonObject m_userInfo;
    void updateUserInfoUi();
    QString m_middlewareUrl;
    QString m_wsMiddlewareUrl;
    StepProgressManager stepManager;
    int currentStep = 0;
    QString m_roomId;

    QTimer *m_clockTimer = nullptr;
    void initClock();
    void updateTimeLabel();


    BedController *m_bedController;
    QQuickWidget *m_bedView = nullptr;
    QQuickWidget *m_stepView = nullptr;
    QPointer<TodaySchedule> m_todayScheduleDialog;
    void updateStepBar();
    QTimer *m_sendTimer;  // 用于定期发送数据
    void connectWebSocketSignals();
    void showLoginDialog();
    QString convertToWebSocketUrl(const QString &middlewareUrl) const;
    void loadMiddlewareSettings();
    void saveMiddlewareSettings();
    // 初始化WebSocket UI
    void initWebSocketUI();
    void connectToMiddlewareWebSocket();
    bool showReportConfirmDialog(const QString &reportText);
    // 更新连接状态显示
    void updateConnectionStatus(bool connected);

    // 在界面上显示接收到的数据
    void displayReceivedData(const QJsonObject &data);
    void displayReceivedText(const QString &message);

    // 构建要发送的参数
    QJsonObject buildSendParameters() const;

    void setupMenu();
    void initUiLayoutTuning();
    void initTables();
    void initStatusIndicators();
    void updateTodaySchedule(const QJsonArray &entries);
    QJsonArray m_todayScheduleEntries;

    TreatmentState m_treatmentState = TreatmentState::Idle;
    TreatmentState m_previousTreatmentState = TreatmentState::Idle;

    void setTreatmentState(TreatmentState newState, const QString &reason = QString());
    bool canTransitTo(TreatmentState from, TreatmentState to) const;
    void applyTreatmentStateToUi();
    int treatmentStateToStep(TreatmentState state) const;
    QString treatmentStateText(TreatmentState state) const;

    bool ensureConnected() const;
    bool ensureStateAllowed(const QList<TreatmentState> &allowedStates,
                            const QString &operationName) const;
    void sendTaskCommand(const QString &cmd);

};
#endif // SIMMTCS_H
