#pragma once

#include <QMainWindow>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <memory>

class QLabel;
class QTimer;
class QTableWidget;
class QDialog;
class QCloseEvent;
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;
class WebSocketClient;
namespace Ui { class QaWindow; }

class QaWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit QaWindow(WebSocketClient *client, const QString &username,
                      const QString &authToken, const QJsonObject &userInfo,
                      const QJsonObject &loginResponse, QWidget *parent = nullptr);
    ~QaWindow() override;
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void receiveMessage(const QJsonObject &message);
    void receiveTelemetry(const QJsonObject &message);
    void refresh();
    void clearTelemetry();
    void updateControls();
    void updateSelection();
    void applyTheme();
    void enterQaMode();
    void queryStatus();
    void executeProtocol();
    void abortQa();
    void resetDdsFault();
    void connectionLost();
    void requestTimedOut();
    bool send(const QJsonObject &request);
    bool canExecute() const;
    QString selectedProtocol() const;
    QJsonObject selectedParams() const;
    QString validationError() const;
    void applyProtocols(const QJsonObject &payload);
    void updateProgress(const QJsonObject &payload);
    void finishProtocol(const QJsonObject &payload, bool aborted);
    void showResult();
    void saveSnapshot();
    void record(const QString &text);
    void indicator(const QString &key, const QString &text, const QString &tone,
                   const QString &details = QString());

    std::unique_ptr<Ui::QaWindow> m_ui;
    QPointer<WebSocketClient> m_client;
    QPointer<QDialog> m_resultDialog;
    QTimer *m_timeout = nullptr;
    QTimer *m_requestTimer = nullptr;
    QTimer *m_queryTimer = nullptr;
    QMap<QString, QJsonObject> m_protocols;
    QJsonObject m_result;
    QJsonObject m_progress;
    QJsonObject m_submittedParams;
    QJsonObject m_data;
    QJsonObject m_registration;
    QLabel *m_registrationLabel = nullptr;
    QLabel *m_ddsAlert = nullptr;
    QWidget *m_directPage = nullptr;
    QLineEdit *m_directPlan = nullptr;
    QLineEdit *m_directTreatment = nullptr;
    QDoubleSpinBox *m_directMu = nullptr;
    QPushButton *m_ddsReset = nullptr;
    QTimer *m_ddsResetTimer = nullptr;
    bool m_authenticated = false;
    QMap<QString, qint64> m_updated;
    QMap<QString, QLabel *> m_fields;
    QMap<QString, QLabel *> m_lamps;
    QTableWidget *m_positions = nullptr;
    QString m_roomId;
    QString m_receivedAt;
    QString m_activeProtocol;
    QString m_pending;
    QString m_problem;
    QString m_stageText;
    quint64 m_revision = 0;
    bool m_modeConfirmed = false;
    bool m_enabled = false;
    bool m_statusKnown = false;
    bool m_running = false;
    bool m_uncertain = false;
    bool m_confirmationOpen = false;
};
