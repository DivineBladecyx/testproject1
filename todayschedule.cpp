#include "todayschedule.h"
#include "ui_todayschedule.h"
#include "websocketclient.h"

#include <QAbstractItemView>
#include <QDate>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QTableWidgetItem>

namespace {

QString formatTimeText(const QString &isoText)
{
    const QDateTime dt = QDateTime::fromString(isoText, Qt::ISODate);
    if (!dt.isValid()) {
        return isoText;
    }

    return dt.toLocalTime().toString("yyyy-MM-dd HH:mm");
}
QTableWidgetItem *makeCenterItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    item->setToolTip(text);
    return item;
}

}

TodaySchedule::TodaySchedule(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TodaySchedule)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setWindowTitle(tr("今日排程"));
    setupTable();
}

TodaySchedule::~TodaySchedule()
{
    delete ui;
}

void TodaySchedule::setWebSocketClient(WebSocketClient *client)
{
    m_webSocketClient = client;
    if (!m_webSocketClient) {
        return;
    }

    connect(m_webSocketClient, &WebSocketClient::dataReceived,
            this, &TodaySchedule::onDataReceived, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::textMessageReceived,
            this, &TodaySchedule::onTextMessageRecevied, Qt::UniqueConnection);
}

void TodaySchedule::setupTable()
{
    ui->tableWidget_t->clearContents();
    ui->tableWidget_t->setColumnCount(5);
    ui->tableWidget_t->setHorizontalHeaderLabels(
        QStringList()
        << tr("序号")
        << tr("病历号")
        << tr("患者姓名")
        << tr("计划开始时间")
        << tr("治疗状态"));
    ui->tableWidget_t->setRowCount(0);
    ui->tableWidget_t->verticalHeader()->setVisible(false);

    ui->tableWidget_t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_t->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_t->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_t->setAlternatingRowColors(true);
    ui->tableWidget_t->setShowGrid(true);
    ui->tableWidget_t->setWordWrap(false);
    ui->tableWidget_t->setTextElideMode(Qt::ElideRight);


    ui->tableWidget_t->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableWidget_t->horizontalHeader()->setMinimumHeight(42);

    ui->tableWidget_t->horizontalHeader()->setStretchLastSection(false);

    ui->tableWidget_t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidget_t->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);

    ui->tableWidget_t->setColumnWidth(0, 55);
    ui->tableWidget_t->setColumnWidth(4, 90);

    ui->tableWidget_t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableWidget_t->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->tableWidget_t->verticalHeader()->setDefaultSectionSize(34);

    ui->tableWidget_t->setStyleSheet(
        "QTableWidget {"
        "  background: white;"
        "  alternate-background-color: #F6FAFF;"
        "  gridline-color: #D9E6F5;"
        "  border: 1px solid #CFE0F4;"
        "  border-radius: 8px;"
        "  color: #0E2847;"
        "}"
        "QHeaderView::section {"
        "  background: #EAF3FF;"
        "  color: #0B559F;"
        "  font-weight: 700;"
        "  border: none;"
        "  border-right: 1px solid #D9E6F5;"
        "  border-bottom: 1px solid #D9E6F5;"
        "  padding: 6px;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 6px;"
        "}"
        "QTableWidget::item:selected {"
        "  background: #DCEBFF;"
        "  color: #0E2847;"
        "}"
        );
}
void TodaySchedule::setHeaderInfo(const QString &physicistName, const QString &roomId)
{
    ui->label_p->setText(physicistName);
    ui->label_room->setText(roomId);
}
void TodaySchedule::updateScheduleEntries(const QJsonArray &entries)
{
    ui->tableWidget_t->setSortingEnabled(false);
    ui->tableWidget_t->clearContents();
    ui->tableWidget_t->setRowCount(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        const QJsonObject entry = entries.at(i).toObject();

       // ui->tableWidget_t->setItem(i, 0, new QTableWidgetItem(entry.value("schedule_item_id").toVariant().toString()));
       // ui->tableWidget_t->setItem(i, 1, new QTableWidgetItem(entry.value("fraction_id").toVariant().toString()));
        ui->tableWidget_t->setItem(i, 0, makeCenterItem(entry.value("priority").toVariant().toString()));
        ui->tableWidget_t->setItem(i, 1, makeCenterItem(entry.value("patient_mrn").toString()));
        ui->tableWidget_t->setItem(i, 2, makeCenterItem(entry.value("patient_name").toString()));
        ui->tableWidget_t->setItem(i, 3, makeCenterItem(entry.value("planned_start_time").toString()));
        ui->tableWidget_t->setItem(i, 4, makeCenterItem(entry.value("queue_state").toString()));
    }
     ui->tableWidget_t->setSortingEnabled(true);
}

void TodaySchedule::fetchTodaySchedule()
{
    if (!m_webSocketClient) {
        return;
    }

    QJsonObject request;
    request["type"] = "TASK";
    request["cmd"] = "GET_TODAY_SCHEDULE";
    request["date"] = QDate::currentDate().toString("yyyy-MM-dd");
    m_webSocketClient->sendData(request);
}

void TodaySchedule::onTextMessageRecevied(const QString &message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.value("type").toString() == "todayScheduleResponse") {
        updateScheduleEntries(obj.value("data").toArray());
    }
}

void TodaySchedule::onDataReceived(const QJsonObject &data)
{
    const QString type = data.value("type").toString();
    const QString cmd = data.value("cmd").toString();

    if (type == "TASK" && (cmd == "TASK_LOGIN_SUCCESS" )) {
        const QJsonObject payload = data.value("payload").toObject();
        const QJsonArray entries = payload.value("entries").toArray(
            payload.value("data").toArray());
        updateScheduleEntries(entries);
    }
}
