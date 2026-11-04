#ifndef TODAYSCHEDULE_H
#define TODAYSCHEDULE_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QJsonArray>
#include <QJsonObject>

namespace Ui {
class TodaySchedule;
}

class TodaySchedule : public QWidget
{
    Q_OBJECT

public:
    explicit TodaySchedule(QWidget *parent = nullptr);
    ~TodaySchedule();

    void setWebSocketClient(class WebSocketClient *client);
    void fetchTodaySchedule();
    void updateScheduleEntries(const QJsonArray &entries);
    void setHeaderInfo(const QString &physicistName, const QString &roomId);
signals:
    void menssageReceived(const QString &message);
private slots:
    void onTextMessageRecevied(const QString &message);
    void onDataReceived(const QJsonObject &data);
    //void on_refreshButton_clicked(); // 可选刷新按钮


private:
    Ui::TodaySchedule *ui;
    WebSocketClient *m_webSocketClient = nullptr;
    void setupTable();

};

#endif // TODAYSCHEDULE_H
