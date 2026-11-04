/********************************************************************************
** Form generated from reading UI file 'qawindow.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QAWINDOW_H
#define UI_QAWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QaWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *mainLayout;
    QWidget *header;
    QGridLayout *headerLayout;
    QLabel *titleLabel;
    QLabel *roomLabel;
    QLabel *identityLabel;
    QLabel *connectionLabel;
    QLabel *modeLabel;
    QPushButton *themeButton;
    QWidget *commandBar;
    QHBoxLayout *commandLayout;
    QPushButton *enterModeButton;
    QPushButton *queryStatusButton;
    QPushButton *saveSnapshotButton;
    QPushButton *resultButton;
    QPushButton *abortButton;
    QLabel *blockingLabel;
    QLabel *workflowStateLabel;
    QLabel *currentActionLabel;
    QScrollArea *workspaceScroll;
    QWidget *workspaceContent;
    QVBoxLayout *workspaceLayout;
    QWidget *workArea;
    QGridLayout *workLayout;
    QWidget *protocolParameters;
    QVBoxLayout *parametersLayout;
    QLabel *paramsOriginLabel;
    QTabWidget *protocolTabs;
    QWidget *mechanicalPage;
    QGridLayout *mechanicalPageLayout;
    QLabel *mechanicalNotice;
    QLabel *xTitle;
    QDoubleSpinBox *target_x;
    QLabel *yTitle;
    QDoubleSpinBox *target_y;
    QLabel *zTitle;
    QDoubleSpinBox *target_z;
    QWidget *dosimetryPage;
    QGridLayout *dosimetryPageLayout;
    QLabel *dosimetryNotice;
    QLabel *energyTitle;
    QDoubleSpinBox *energyInput;
    QLabel *beamlineTitle;
    QSpinBox *beamlineInput;
    QLabel *target_muTitle;
    QDoubleSpinBox *target_muInput;
    QLabel *doseLimitLabel;
    QWidget *verificationPage;
    QGridLayout *verificationPageLayout;
    QLabel *verificationNotice;
    QLabel *planIdTitle;
    QLineEdit *planIdInput;
    QLabel *protocolDescription;
    QPushButton *executeButton;
    QWidget *executionMonitor;
    QVBoxLayout *monitorLayout;
    QLabel *monitorTitle;
    QLabel *activeProtocolLabel;
    QLabel *stageLabel;
    QLabel *beamTitle;
    QLabel *beamValue;
    QLabel *resultTitle;
    QLabel *resultValue;
    QLabel *muLabel;
    QProgressBar *doseProgress;
    QLabel *qaEnergyLabel;
    QLabel *resultSummary;
    QTableWidget *resultTable;
    QLabel *deviceSummaryTitle;
    QWidget *lampsWidget;
    QGridLayout *devicesGrid;
    QLabel *couchName;
    QLabel *couchLamp;
    QLabel *drName;
    QLabel *drLamp;
    QLabel *igrtName;
    QLabel *igrtLamp;
    QLabel *filterName;
    QLabel *filterLamp;
    QLabel *plcName;
    QLabel *plcLamp;
    QLabel *g01Name;
    QLabel *g01Lamp;
    QLabel *g02Name;
    QLabel *g02Lamp;
    QLabel *acceleratorName;
    QLabel *acceleratorLamp;
    QLabel *ddsName;
    QLabel *ddsLamp;
    QLabel *faultName;
    QLabel *faultLamp;
    QToolButton *detailsButton;
    QTabWidget *detailsTabs;
    QWidget *telemetryPage;
    QVBoxLayout *telemetryPageLayout;
    QWidget *telemetryValues;
    QGridLayout *telemetryGrid;
    QLabel *sum_doseTitle;
    QLabel *sum_doseValue;
    QLabel *dose_rateTitle;
    QLabel *dose_rateValue;
    QLabel *current_layerTitle;
    QLabel *current_layerValue;
    QLabel *current_pointTitle;
    QLabel *current_pointValue;
    QLabel *current_doseTitle;
    QLabel *current_doseValue;
    QLabel *layer_sum_doseTitle;
    QLabel *layer_sum_doseValue;
    QLabel *next_layerTitle;
    QLabel *next_layerValue;
    QLabel *therapy_idTitle;
    QLabel *therapy_idValue;
    QLabel *timestampTitle;
    QLabel *timestampValue;
    QLabel *positionTitle;
    QTableWidget *positionTable;
    QLabel *telemetryNotice;
    QWidget *communicationPage;
    QVBoxLayout *communicationPageLayout;
    QPlainTextEdit *communicationLog;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *QaWindow)
    {
        if (QaWindow->objectName().isEmpty())
            QaWindow->setObjectName("QaWindow");
        QaWindow->setMinimumSize(QSize(1080, 720));
        centralwidget = new QWidget(QaWindow);
        centralwidget->setObjectName("centralwidget");
        mainLayout = new QVBoxLayout(centralwidget);
        mainLayout->setObjectName("mainLayout");
        header = new QWidget(centralwidget);
        header->setObjectName("header");
        headerLayout = new QGridLayout(header);
        headerLayout->setObjectName("headerLayout");
        headerLayout->setContentsMargins(0, 0, 0, 0);
        titleLabel = new QLabel(header);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setWordWrap(true);
        titleLabel->setTextFormat(Qt::TextFormat::PlainText);

        headerLayout->addWidget(titleLabel, 0, 0, 1, 1);

        roomLabel = new QLabel(header);
        roomLabel->setObjectName("roomLabel");
        roomLabel->setWordWrap(true);
        roomLabel->setTextFormat(Qt::TextFormat::PlainText);

        headerLayout->addWidget(roomLabel, 0, 1, 1, 1);

        identityLabel = new QLabel(header);
        identityLabel->setObjectName("identityLabel");
        identityLabel->setWordWrap(true);
        identityLabel->setTextFormat(Qt::TextFormat::PlainText);

        headerLayout->addWidget(identityLabel, 0, 2, 1, 1);

        connectionLabel = new QLabel(header);
        connectionLabel->setObjectName("connectionLabel");
        connectionLabel->setWordWrap(true);
        connectionLabel->setTextFormat(Qt::TextFormat::PlainText);

        headerLayout->addWidget(connectionLabel, 1, 0, 1, 1);

        modeLabel = new QLabel(header);
        modeLabel->setObjectName("modeLabel");
        modeLabel->setWordWrap(true);
        modeLabel->setTextFormat(Qt::TextFormat::PlainText);

        headerLayout->addWidget(modeLabel, 1, 1, 1, 1);

        themeButton = new QPushButton(header);
        themeButton->setObjectName("themeButton");

        headerLayout->addWidget(themeButton, 1, 2, 1, 1);


        mainLayout->addWidget(header);

        commandBar = new QWidget(centralwidget);
        commandBar->setObjectName("commandBar");
        commandLayout = new QHBoxLayout(commandBar);
        commandLayout->setObjectName("commandLayout");
        commandLayout->setContentsMargins(0, 0, 0, 0);
        enterModeButton = new QPushButton(commandBar);
        enterModeButton->setObjectName("enterModeButton");

        commandLayout->addWidget(enterModeButton);

        queryStatusButton = new QPushButton(commandBar);
        queryStatusButton->setObjectName("queryStatusButton");

        commandLayout->addWidget(queryStatusButton);

        saveSnapshotButton = new QPushButton(commandBar);
        saveSnapshotButton->setObjectName("saveSnapshotButton");

        commandLayout->addWidget(saveSnapshotButton);

        resultButton = new QPushButton(commandBar);
        resultButton->setObjectName("resultButton");

        commandLayout->addWidget(resultButton);

        abortButton = new QPushButton(commandBar);
        abortButton->setObjectName("abortButton");

        commandLayout->addWidget(abortButton);


        mainLayout->addWidget(commandBar);

        blockingLabel = new QLabel(centralwidget);
        blockingLabel->setObjectName("blockingLabel");
        blockingLabel->setWordWrap(true);
        blockingLabel->setTextFormat(Qt::TextFormat::PlainText);

        mainLayout->addWidget(blockingLabel);

        workflowStateLabel = new QLabel(centralwidget);
        workflowStateLabel->setObjectName("workflowStateLabel");
        workflowStateLabel->setWordWrap(true);
        workflowStateLabel->setTextFormat(Qt::TextFormat::PlainText);

        mainLayout->addWidget(workflowStateLabel);

        currentActionLabel = new QLabel(centralwidget);
        currentActionLabel->setObjectName("currentActionLabel");
        currentActionLabel->setWordWrap(true);
        currentActionLabel->setTextFormat(Qt::TextFormat::PlainText);

        mainLayout->addWidget(currentActionLabel);

        workspaceScroll = new QScrollArea(centralwidget);
        workspaceScroll->setObjectName("workspaceScroll");
        workspaceScroll->setWidgetResizable(true);
        workspaceContent = new QWidget();
        workspaceContent->setObjectName("workspaceContent");
        workspaceLayout = new QVBoxLayout(workspaceContent);
        workspaceLayout->setObjectName("workspaceLayout");
        workArea = new QWidget(workspaceContent);
        workArea->setObjectName("workArea");
        workLayout = new QGridLayout(workArea);
        workLayout->setObjectName("workLayout");
        workLayout->setContentsMargins(0, 0, 0, 0);
        protocolParameters = new QWidget(workArea);
        protocolParameters->setObjectName("protocolParameters");
        protocolParameters->setMaximumSize(QSize(680, 16777215));
        parametersLayout = new QVBoxLayout(protocolParameters);
        parametersLayout->setObjectName("parametersLayout");
        parametersLayout->setContentsMargins(0, 0, 0, 0);
        paramsOriginLabel = new QLabel(protocolParameters);
        paramsOriginLabel->setObjectName("paramsOriginLabel");
        paramsOriginLabel->setWordWrap(true);
        paramsOriginLabel->setTextFormat(Qt::TextFormat::PlainText);

        parametersLayout->addWidget(paramsOriginLabel);

        protocolTabs = new QTabWidget(protocolParameters);
        protocolTabs->setObjectName("protocolTabs");
        mechanicalPage = new QWidget();
        mechanicalPage->setObjectName("mechanicalPage");
        mechanicalPageLayout = new QGridLayout(mechanicalPage);
        mechanicalPageLayout->setObjectName("mechanicalPageLayout");
        mechanicalNotice = new QLabel(mechanicalPage);
        mechanicalNotice->setObjectName("mechanicalNotice");
        mechanicalNotice->setWordWrap(true);
        mechanicalNotice->setTextFormat(Qt::TextFormat::PlainText);

        mechanicalPageLayout->addWidget(mechanicalNotice, 0, 0, 1, 2);

        xTitle = new QLabel(mechanicalPage);
        xTitle->setObjectName("xTitle");
        xTitle->setWordWrap(true);
        xTitle->setTextFormat(Qt::TextFormat::PlainText);

        mechanicalPageLayout->addWidget(xTitle, 1, 0, 1, 1);

        target_x = new QDoubleSpinBox(mechanicalPage);
        target_x->setObjectName("target_x");
        target_x->setMinimum(-1000000.000000000000000);
        target_x->setMaximum(1000000.000000000000000);
        target_x->setDecimals(3);

        mechanicalPageLayout->addWidget(target_x, 1, 1, 1, 1);

        yTitle = new QLabel(mechanicalPage);
        yTitle->setObjectName("yTitle");
        yTitle->setWordWrap(true);
        yTitle->setTextFormat(Qt::TextFormat::PlainText);

        mechanicalPageLayout->addWidget(yTitle, 2, 0, 1, 1);

        target_y = new QDoubleSpinBox(mechanicalPage);
        target_y->setObjectName("target_y");
        target_y->setMinimum(-1000000.000000000000000);
        target_y->setMaximum(1000000.000000000000000);
        target_y->setDecimals(3);

        mechanicalPageLayout->addWidget(target_y, 2, 1, 1, 1);

        zTitle = new QLabel(mechanicalPage);
        zTitle->setObjectName("zTitle");
        zTitle->setWordWrap(true);
        zTitle->setTextFormat(Qt::TextFormat::PlainText);

        mechanicalPageLayout->addWidget(zTitle, 3, 0, 1, 1);

        target_z = new QDoubleSpinBox(mechanicalPage);
        target_z->setObjectName("target_z");
        target_z->setMinimum(-1000000.000000000000000);
        target_z->setMaximum(1000000.000000000000000);
        target_z->setDecimals(3);

        mechanicalPageLayout->addWidget(target_z, 3, 1, 1, 1);

        protocolTabs->addTab(mechanicalPage, QString());
        dosimetryPage = new QWidget();
        dosimetryPage->setObjectName("dosimetryPage");
        dosimetryPageLayout = new QGridLayout(dosimetryPage);
        dosimetryPageLayout->setObjectName("dosimetryPageLayout");
        dosimetryNotice = new QLabel(dosimetryPage);
        dosimetryNotice->setObjectName("dosimetryNotice");
        dosimetryNotice->setWordWrap(true);
        dosimetryNotice->setTextFormat(Qt::TextFormat::PlainText);

        dosimetryPageLayout->addWidget(dosimetryNotice, 0, 0, 1, 2);

        energyTitle = new QLabel(dosimetryPage);
        energyTitle->setObjectName("energyTitle");
        energyTitle->setWordWrap(true);
        energyTitle->setTextFormat(Qt::TextFormat::PlainText);

        dosimetryPageLayout->addWidget(energyTitle, 1, 0, 1, 1);

        energyInput = new QDoubleSpinBox(dosimetryPage);
        energyInput->setObjectName("energyInput");
        energyInput->setDecimals(3);
        energyInput->setMaximum(1000000.000000000000000);

        dosimetryPageLayout->addWidget(energyInput, 1, 1, 1, 1);

        beamlineTitle = new QLabel(dosimetryPage);
        beamlineTitle->setObjectName("beamlineTitle");
        beamlineTitle->setWordWrap(true);
        beamlineTitle->setTextFormat(Qt::TextFormat::PlainText);

        dosimetryPageLayout->addWidget(beamlineTitle, 2, 0, 1, 1);

        beamlineInput = new QSpinBox(dosimetryPage);
        beamlineInput->setObjectName("beamlineInput");
        beamlineInput->setMaximum(2147483647);

        dosimetryPageLayout->addWidget(beamlineInput, 2, 1, 1, 1);

        target_muTitle = new QLabel(dosimetryPage);
        target_muTitle->setObjectName("target_muTitle");
        target_muTitle->setWordWrap(true);
        target_muTitle->setTextFormat(Qt::TextFormat::PlainText);

        dosimetryPageLayout->addWidget(target_muTitle, 3, 0, 1, 1);

        target_muInput = new QDoubleSpinBox(dosimetryPage);
        target_muInput->setObjectName("target_muInput");
        target_muInput->setDecimals(3);
        target_muInput->setMaximum(300.000000000000000);

        dosimetryPageLayout->addWidget(target_muInput, 3, 1, 1, 1);

        doseLimitLabel = new QLabel(dosimetryPage);
        doseLimitLabel->setObjectName("doseLimitLabel");
        doseLimitLabel->setWordWrap(true);
        doseLimitLabel->setTextFormat(Qt::TextFormat::PlainText);

        dosimetryPageLayout->addWidget(doseLimitLabel, 4, 0, 1, 2);

        protocolTabs->addTab(dosimetryPage, QString());
        verificationPage = new QWidget();
        verificationPage->setObjectName("verificationPage");
        verificationPageLayout = new QGridLayout(verificationPage);
        verificationPageLayout->setObjectName("verificationPageLayout");
        verificationNotice = new QLabel(verificationPage);
        verificationNotice->setObjectName("verificationNotice");
        verificationNotice->setWordWrap(true);
        verificationNotice->setTextFormat(Qt::TextFormat::PlainText);

        verificationPageLayout->addWidget(verificationNotice, 0, 0, 1, 2);

        planIdTitle = new QLabel(verificationPage);
        planIdTitle->setObjectName("planIdTitle");
        planIdTitle->setWordWrap(true);
        planIdTitle->setTextFormat(Qt::TextFormat::PlainText);

        verificationPageLayout->addWidget(planIdTitle, 1, 0, 1, 1);

        planIdInput = new QLineEdit(verificationPage);
        planIdInput->setObjectName("planIdInput");

        verificationPageLayout->addWidget(planIdInput, 1, 1, 1, 1);

        protocolTabs->addTab(verificationPage, QString());

        parametersLayout->addWidget(protocolTabs);

        protocolDescription = new QLabel(protocolParameters);
        protocolDescription->setObjectName("protocolDescription");
        protocolDescription->setWordWrap(true);
        protocolDescription->setTextFormat(Qt::TextFormat::PlainText);

        parametersLayout->addWidget(protocolDescription);

        executeButton = new QPushButton(protocolParameters);
        executeButton->setObjectName("executeButton");

        parametersLayout->addWidget(executeButton);


        workLayout->addWidget(protocolParameters, 0, 0, 1, 1);

        executionMonitor = new QWidget(workArea);
        executionMonitor->setObjectName("executionMonitor");
        monitorLayout = new QVBoxLayout(executionMonitor);
        monitorLayout->setObjectName("monitorLayout");
        monitorLayout->setContentsMargins(0, 0, 0, 0);
        monitorTitle = new QLabel(executionMonitor);
        monitorTitle->setObjectName("monitorTitle");
        monitorTitle->setWordWrap(true);
        monitorTitle->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(monitorTitle);

        activeProtocolLabel = new QLabel(executionMonitor);
        activeProtocolLabel->setObjectName("activeProtocolLabel");
        activeProtocolLabel->setWordWrap(true);
        activeProtocolLabel->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(activeProtocolLabel);

        stageLabel = new QLabel(executionMonitor);
        stageLabel->setObjectName("stageLabel");
        stageLabel->setWordWrap(true);
        stageLabel->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(stageLabel);

        beamTitle = new QLabel(executionMonitor);
        beamTitle->setObjectName("beamTitle");
        beamTitle->setWordWrap(true);
        beamTitle->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(beamTitle);

        beamValue = new QLabel(executionMonitor);
        beamValue->setObjectName("beamValue");
        beamValue->setWordWrap(true);
        beamValue->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(beamValue);

        resultTitle = new QLabel(executionMonitor);
        resultTitle->setObjectName("resultTitle");
        resultTitle->setWordWrap(true);
        resultTitle->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(resultTitle);

        resultValue = new QLabel(executionMonitor);
        resultValue->setObjectName("resultValue");
        resultValue->setWordWrap(true);
        resultValue->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(resultValue);

        muLabel = new QLabel(executionMonitor);
        muLabel->setObjectName("muLabel");
        muLabel->setWordWrap(true);
        muLabel->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(muLabel);

        doseProgress = new QProgressBar(executionMonitor);
        doseProgress->setObjectName("doseProgress");
        doseProgress->setValue(0);

        monitorLayout->addWidget(doseProgress);

        qaEnergyLabel = new QLabel(executionMonitor);
        qaEnergyLabel->setObjectName("qaEnergyLabel");
        qaEnergyLabel->setWordWrap(true);
        qaEnergyLabel->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(qaEnergyLabel);

        resultSummary = new QLabel(executionMonitor);
        resultSummary->setObjectName("resultSummary");
        resultSummary->setWordWrap(true);
        resultSummary->setTextFormat(Qt::TextFormat::PlainText);

        monitorLayout->addWidget(resultSummary);

        resultTable = new QTableWidget(executionMonitor);
        if (resultTable->columnCount() < 2)
            resultTable->setColumnCount(2);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        resultTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        resultTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        resultTable->setObjectName("resultTable");
        resultTable->setColumnCount(2);
        resultTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        resultTable->setMinimumSize(QSize(0, 170));

        monitorLayout->addWidget(resultTable);


        workLayout->addWidget(executionMonitor, 0, 1, 1, 1);

        workLayout->setColumnStretch(0, 1);
        workLayout->setColumnStretch(1, 2);

        workspaceLayout->addWidget(workArea);

        deviceSummaryTitle = new QLabel(workspaceContent);
        deviceSummaryTitle->setObjectName("deviceSummaryTitle");
        deviceSummaryTitle->setWordWrap(true);
        deviceSummaryTitle->setTextFormat(Qt::TextFormat::PlainText);

        workspaceLayout->addWidget(deviceSummaryTitle);

        lampsWidget = new QWidget(workspaceContent);
        lampsWidget->setObjectName("lampsWidget");
        devicesGrid = new QGridLayout(lampsWidget);
        devicesGrid->setObjectName("devicesGrid");
        devicesGrid->setContentsMargins(0, 0, 0, 0);
        couchName = new QLabel(lampsWidget);
        couchName->setObjectName("couchName");
        couchName->setWordWrap(true);
        couchName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(couchName, 0, 0, 1, 1);

        couchLamp = new QLabel(lampsWidget);
        couchLamp->setObjectName("couchLamp");
        couchLamp->setWordWrap(true);
        couchLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(couchLamp, 1, 0, 1, 1);

        drName = new QLabel(lampsWidget);
        drName->setObjectName("drName");
        drName->setWordWrap(true);
        drName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(drName, 0, 1, 1, 1);

        drLamp = new QLabel(lampsWidget);
        drLamp->setObjectName("drLamp");
        drLamp->setWordWrap(true);
        drLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(drLamp, 1, 1, 1, 1);

        igrtName = new QLabel(lampsWidget);
        igrtName->setObjectName("igrtName");
        igrtName->setWordWrap(true);
        igrtName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(igrtName, 0, 2, 1, 1);

        igrtLamp = new QLabel(lampsWidget);
        igrtLamp->setObjectName("igrtLamp");
        igrtLamp->setWordWrap(true);
        igrtLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(igrtLamp, 1, 2, 1, 1);

        filterName = new QLabel(lampsWidget);
        filterName->setObjectName("filterName");
        filterName->setWordWrap(true);
        filterName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(filterName, 0, 3, 1, 1);

        filterLamp = new QLabel(lampsWidget);
        filterLamp->setObjectName("filterLamp");
        filterLamp->setWordWrap(true);
        filterLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(filterLamp, 1, 3, 1, 1);

        plcName = new QLabel(lampsWidget);
        plcName->setObjectName("plcName");
        plcName->setWordWrap(true);
        plcName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(plcName, 0, 4, 1, 1);

        plcLamp = new QLabel(lampsWidget);
        plcLamp->setObjectName("plcLamp");
        plcLamp->setWordWrap(true);
        plcLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(plcLamp, 1, 4, 1, 1);

        g01Name = new QLabel(lampsWidget);
        g01Name->setObjectName("g01Name");
        g01Name->setWordWrap(true);
        g01Name->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(g01Name, 2, 0, 1, 1);

        g01Lamp = new QLabel(lampsWidget);
        g01Lamp->setObjectName("g01Lamp");
        g01Lamp->setWordWrap(true);
        g01Lamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(g01Lamp, 3, 0, 1, 1);

        g02Name = new QLabel(lampsWidget);
        g02Name->setObjectName("g02Name");
        g02Name->setWordWrap(true);
        g02Name->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(g02Name, 2, 1, 1, 1);

        g02Lamp = new QLabel(lampsWidget);
        g02Lamp->setObjectName("g02Lamp");
        g02Lamp->setWordWrap(true);
        g02Lamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(g02Lamp, 3, 1, 1, 1);

        acceleratorName = new QLabel(lampsWidget);
        acceleratorName->setObjectName("acceleratorName");
        acceleratorName->setWordWrap(true);
        acceleratorName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(acceleratorName, 2, 2, 1, 1);

        acceleratorLamp = new QLabel(lampsWidget);
        acceleratorLamp->setObjectName("acceleratorLamp");
        acceleratorLamp->setWordWrap(true);
        acceleratorLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(acceleratorLamp, 3, 2, 1, 1);

        ddsName = new QLabel(lampsWidget);
        ddsName->setObjectName("ddsName");
        ddsName->setWordWrap(true);
        ddsName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(ddsName, 2, 3, 1, 1);

        ddsLamp = new QLabel(lampsWidget);
        ddsLamp->setObjectName("ddsLamp");
        ddsLamp->setWordWrap(true);
        ddsLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(ddsLamp, 3, 3, 1, 1);

        faultName = new QLabel(lampsWidget);
        faultName->setObjectName("faultName");
        faultName->setWordWrap(true);
        faultName->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(faultName, 2, 4, 1, 1);

        faultLamp = new QLabel(lampsWidget);
        faultLamp->setObjectName("faultLamp");
        faultLamp->setWordWrap(true);
        faultLamp->setTextFormat(Qt::TextFormat::PlainText);

        devicesGrid->addWidget(faultLamp, 3, 4, 1, 1);


        workspaceLayout->addWidget(lampsWidget);

        detailsButton = new QToolButton(workspaceContent);
        detailsButton->setObjectName("detailsButton");
        detailsButton->setCheckable(true);
        detailsButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        detailsButton->setArrowType(Qt::ArrowType::RightArrow);

        workspaceLayout->addWidget(detailsButton);

        detailsTabs = new QTabWidget(workspaceContent);
        detailsTabs->setObjectName("detailsTabs");
        telemetryPage = new QWidget();
        telemetryPage->setObjectName("telemetryPage");
        telemetryPageLayout = new QVBoxLayout(telemetryPage);
        telemetryPageLayout->setObjectName("telemetryPageLayout");
        telemetryValues = new QWidget(telemetryPage);
        telemetryValues->setObjectName("telemetryValues");
        telemetryGrid = new QGridLayout(telemetryValues);
        telemetryGrid->setObjectName("telemetryGrid");
        telemetryGrid->setContentsMargins(0, 0, 0, 0);
        sum_doseTitle = new QLabel(telemetryValues);
        sum_doseTitle->setObjectName("sum_doseTitle");
        sum_doseTitle->setWordWrap(true);
        sum_doseTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(sum_doseTitle, 0, 0, 1, 1);

        sum_doseValue = new QLabel(telemetryValues);
        sum_doseValue->setObjectName("sum_doseValue");
        sum_doseValue->setWordWrap(true);
        sum_doseValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(sum_doseValue, 1, 0, 1, 1);

        dose_rateTitle = new QLabel(telemetryValues);
        dose_rateTitle->setObjectName("dose_rateTitle");
        dose_rateTitle->setWordWrap(true);
        dose_rateTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(dose_rateTitle, 0, 1, 1, 1);

        dose_rateValue = new QLabel(telemetryValues);
        dose_rateValue->setObjectName("dose_rateValue");
        dose_rateValue->setWordWrap(true);
        dose_rateValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(dose_rateValue, 1, 1, 1, 1);

        current_layerTitle = new QLabel(telemetryValues);
        current_layerTitle->setObjectName("current_layerTitle");
        current_layerTitle->setWordWrap(true);
        current_layerTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_layerTitle, 0, 2, 1, 1);

        current_layerValue = new QLabel(telemetryValues);
        current_layerValue->setObjectName("current_layerValue");
        current_layerValue->setWordWrap(true);
        current_layerValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_layerValue, 1, 2, 1, 1);

        current_pointTitle = new QLabel(telemetryValues);
        current_pointTitle->setObjectName("current_pointTitle");
        current_pointTitle->setWordWrap(true);
        current_pointTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_pointTitle, 2, 0, 1, 1);

        current_pointValue = new QLabel(telemetryValues);
        current_pointValue->setObjectName("current_pointValue");
        current_pointValue->setWordWrap(true);
        current_pointValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_pointValue, 3, 0, 1, 1);

        current_doseTitle = new QLabel(telemetryValues);
        current_doseTitle->setObjectName("current_doseTitle");
        current_doseTitle->setWordWrap(true);
        current_doseTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_doseTitle, 2, 1, 1, 1);

        current_doseValue = new QLabel(telemetryValues);
        current_doseValue->setObjectName("current_doseValue");
        current_doseValue->setWordWrap(true);
        current_doseValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(current_doseValue, 3, 1, 1, 1);

        layer_sum_doseTitle = new QLabel(telemetryValues);
        layer_sum_doseTitle->setObjectName("layer_sum_doseTitle");
        layer_sum_doseTitle->setWordWrap(true);
        layer_sum_doseTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(layer_sum_doseTitle, 2, 2, 1, 1);

        layer_sum_doseValue = new QLabel(telemetryValues);
        layer_sum_doseValue->setObjectName("layer_sum_doseValue");
        layer_sum_doseValue->setWordWrap(true);
        layer_sum_doseValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(layer_sum_doseValue, 3, 2, 1, 1);

        next_layerTitle = new QLabel(telemetryValues);
        next_layerTitle->setObjectName("next_layerTitle");
        next_layerTitle->setWordWrap(true);
        next_layerTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(next_layerTitle, 4, 0, 1, 1);

        next_layerValue = new QLabel(telemetryValues);
        next_layerValue->setObjectName("next_layerValue");
        next_layerValue->setWordWrap(true);
        next_layerValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(next_layerValue, 5, 0, 1, 1);

        therapy_idTitle = new QLabel(telemetryValues);
        therapy_idTitle->setObjectName("therapy_idTitle");
        therapy_idTitle->setWordWrap(true);
        therapy_idTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(therapy_idTitle, 4, 1, 1, 1);

        therapy_idValue = new QLabel(telemetryValues);
        therapy_idValue->setObjectName("therapy_idValue");
        therapy_idValue->setWordWrap(true);
        therapy_idValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(therapy_idValue, 5, 1, 1, 1);

        timestampTitle = new QLabel(telemetryValues);
        timestampTitle->setObjectName("timestampTitle");
        timestampTitle->setWordWrap(true);
        timestampTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(timestampTitle, 4, 2, 1, 1);

        timestampValue = new QLabel(telemetryValues);
        timestampValue->setObjectName("timestampValue");
        timestampValue->setWordWrap(true);
        timestampValue->setTextFormat(Qt::TextFormat::PlainText);

        telemetryGrid->addWidget(timestampValue, 5, 2, 1, 1);


        telemetryPageLayout->addWidget(telemetryValues);

        positionTitle = new QLabel(telemetryPage);
        positionTitle->setObjectName("positionTitle");
        positionTitle->setWordWrap(true);
        positionTitle->setTextFormat(Qt::TextFormat::PlainText);

        telemetryPageLayout->addWidget(positionTitle);

        positionTable = new QTableWidget(telemetryPage);
        if (positionTable->columnCount() < 6)
            positionTable->setColumnCount(6);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(0, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(1, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(2, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(3, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(4, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        positionTable->setHorizontalHeaderItem(5, __qtablewidgetitem7);
        if (positionTable->rowCount() < 1)
            positionTable->setRowCount(1);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        positionTable->setVerticalHeaderItem(0, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        positionTable->setItem(0, 0, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        positionTable->setItem(0, 1, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        positionTable->setItem(0, 2, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        positionTable->setItem(0, 3, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        positionTable->setItem(0, 4, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        positionTable->setItem(0, 5, __qtablewidgetitem14);
        positionTable->setObjectName("positionTable");
        positionTable->setRowCount(1);
        positionTable->setColumnCount(6);
        positionTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        positionTable->setMaximumSize(QSize(16777215, 100));

        telemetryPageLayout->addWidget(positionTable);

        telemetryNotice = new QLabel(telemetryPage);
        telemetryNotice->setObjectName("telemetryNotice");
        telemetryNotice->setWordWrap(true);
        telemetryNotice->setTextFormat(Qt::TextFormat::PlainText);

        telemetryPageLayout->addWidget(telemetryNotice);

        detailsTabs->addTab(telemetryPage, QString());
        communicationPage = new QWidget();
        communicationPage->setObjectName("communicationPage");
        communicationPageLayout = new QVBoxLayout(communicationPage);
        communicationPageLayout->setObjectName("communicationPageLayout");
        communicationLog = new QPlainTextEdit(communicationPage);
        communicationLog->setObjectName("communicationLog");
        communicationLog->setReadOnly(true);
        communicationLog->setMinimumSize(QSize(0, 220));

        communicationPageLayout->addWidget(communicationLog);

        detailsTabs->addTab(communicationPage, QString());

        workspaceLayout->addWidget(detailsTabs);

        workspaceScroll->setWidget(workspaceContent);

        mainLayout->addWidget(workspaceScroll);

        QaWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(QaWindow);
        statusbar->setObjectName("statusbar");
        QaWindow->setStatusBar(statusbar);

        retranslateUi(QaWindow);

        QMetaObject::connectSlotsByName(QaWindow);
    } // setupUi

    void retranslateUi(QMainWindow *QaWindow)
    {
        QaWindow->setWindowTitle(QCoreApplication::translate("QaWindow", "SIMM \346\262\273\347\226\227\346\216\247\345\210\266\347\263\273\347\273\237 \302\267 QA \350\264\250\346\216\247", nullptr));
        titleLabel->setText(QCoreApplication::translate("QaWindow", "\350\264\250\346\216\247\344\270\216\347\211\251\347\220\206\345\270\210\345\267\245\344\275\234\345\217\260", nullptr));
        roomLabel->setText(QCoreApplication::translate("QaWindow", "\346\262\273\347\226\227\345\256\244\357\274\232\346\234\252\351\205\215\347\275\256", nullptr));
        identityLabel->setText(QCoreApplication::translate("QaWindow", "\346\211\247\350\241\214\344\272\272\357\274\232-", nullptr));
        connectionLabel->setText(QCoreApplication::translate("QaWindow", "\344\270\255\351\227\264\345\261\202\357\274\232\346\234\252\350\277\236\346\216\245", nullptr));
        modeLabel->setText(QCoreApplication::translate("QaWindow", "\345\220\216\347\253\257 QA \346\250\241\345\274\217\357\274\232\346\234\252\347\241\256\350\256\244", nullptr));
        themeButton->setText(QCoreApplication::translate("QaWindow", "\344\270\273\351\242\230", nullptr));
        enterModeButton->setText(QCoreApplication::translate("QaWindow", "\350\277\233\345\205\245 QA \346\250\241\345\274\217", nullptr));
        queryStatusButton->setText(QCoreApplication::translate("QaWindow", "\345\220\214\346\255\245 QA \347\212\266\346\200\201", nullptr));
        saveSnapshotButton->setText(QCoreApplication::translate("QaWindow", "\345\257\274\345\207\272\346\234\254\345\234\260\350\256\260\345\275\225", nullptr));
        resultButton->setText(QCoreApplication::translate("QaWindow", "\346\237\245\347\234\213\344\270\212\346\254\241\347\273\223\346\236\234", nullptr));
        abortButton->setText(QCoreApplication::translate("QaWindow", "\344\270\255\346\255\242 QA", nullptr));
        blockingLabel->setText(QCoreApplication::translate("QaWindow", "\347\255\211\345\276\205\345\220\216\347\253\257 QA \346\250\241\345\274\217\344\270\216\347\212\266\346\200\201\347\241\256\350\256\244", nullptr));
        workflowStateLabel->setText(QCoreApplication::translate("QaWindow", "QA \347\212\266\346\200\201\357\274\232\346\234\252\347\237\245", nullptr));
        currentActionLabel->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215\345\212\250\344\275\234\357\274\232\347\255\211\345\276\205\347\212\266\346\200\201\345\220\214\346\255\245", nullptr));
        paramsOriginLabel->setText(QCoreApplication::translate("QaWindow", "\346\234\254\345\234\260\345\276\205\346\217\220\344\272\244\345\217\202\346\225\260", nullptr));
        mechanicalNotice->setText(QCoreApplication::translate("QaWindow", "\346\234\272\346\242\260\344\270\216\345\275\261\345\203\217\346\221\206\344\275\215\350\264\250\346\216\247 \302\267 \344\270\245\346\240\274\344\270\215\345\207\272\346\235\237", nullptr));
        xTitle->setText(QCoreApplication::translate("QaWindow", "\347\233\256\346\240\207 X\357\274\210\346\216\245\345\217\243\345\215\225\344\275\215\357\274\211", nullptr));
        yTitle->setText(QCoreApplication::translate("QaWindow", "\347\233\256\346\240\207 Y\357\274\210\346\216\245\345\217\243\345\215\225\344\275\215\357\274\211", nullptr));
        zTitle->setText(QCoreApplication::translate("QaWindow", "\347\233\256\346\240\207 Z\357\274\210\346\216\245\345\217\243\345\215\225\344\275\215\357\274\211", nullptr));
        protocolTabs->setTabText(protocolTabs->indexOf(mechanicalPage), QCoreApplication::translate("QaWindow", "\346\234\272\346\242\260 / \345\275\261\345\203\217", nullptr));
        dosimetryNotice->setText(QCoreApplication::translate("QaWindow", "\347\234\237\345\256\236\345\207\272\346\235\237 \302\267 \345\215\225\346\254\241\347\233\256\346\240\207\345\277\205\351\241\273\345\244\247\344\272\216 0 \344\270\224\344\270\215\350\266\205\350\277\207\345\220\216\347\253\257\351\231\220\351\242\235\345\217\212 300 MU", nullptr));
        energyTitle->setText(QCoreApplication::translate("QaWindow", "\350\203\275\351\207\217\357\274\210\346\216\245\345\217\243\345\215\225\344\275\215\357\274\211", nullptr));
        beamlineTitle->setText(QCoreApplication::translate("QaWindow", "\346\235\237\347\272\277\347\274\226\345\217\267", nullptr));
        target_muTitle->setText(QCoreApplication::translate("QaWindow", "\347\233\256\346\240\207 MU", nullptr));
        doseLimitLabel->setText(QCoreApplication::translate("QaWindow", "\345\220\216\347\253\257\345\211\202\351\207\217\351\231\220\351\242\235\357\274\232\346\234\252\347\241\256\350\256\244", nullptr));
        protocolTabs->setTabText(protocolTabs->indexOf(dosimetryPage), QCoreApplication::translate("QaWindow", "\346\235\237\346\265\201 / \345\211\202\351\207\217", nullptr));
        verificationNotice->setText(QCoreApplication::translate("QaWindow", "\345\201\207\344\275\223\350\256\241\345\210\222\351\252\214\350\257\201 \302\267 \345\217\257\350\203\275\347\234\237\345\256\236\345\207\272\346\235\237 \302\267 QA \347\213\254\347\253\213\345\275\222\346\241\243", nullptr));
        planIdTitle->setText(QCoreApplication::translate("QaWindow", "QA \350\256\241\345\210\222 ID", nullptr));
        protocolTabs->setTabText(protocolTabs->indexOf(verificationPage), QCoreApplication::translate("QaWindow", "\350\256\241\345\210\222\351\242\204\346\274\224", nullptr));
        protocolDescription->setText(QCoreApplication::translate("QaWindow", "\351\242\204\346\241\210\350\203\275\345\212\233\357\274\232\347\255\211\345\276\205\345\220\216\347\253\257\346\217\220\344\276\233", nullptr));
        executeButton->setText(QCoreApplication::translate("QaWindow", "\346\211\247\350\241\214\346\211\200\351\200\211 QA \351\242\204\346\241\210", nullptr));
        monitorTitle->setText(QCoreApplication::translate("QaWindow", "\346\211\247\350\241\214\345\217\215\351\246\210", nullptr));
        activeProtocolLabel->setText(QCoreApplication::translate("QaWindow", "\346\211\247\350\241\214\351\242\204\346\241\210\357\274\232-", nullptr));
        stageLabel->setText(QCoreApplication::translate("QaWindow", "\351\230\266\346\256\265\357\274\232\346\234\252\347\237\245", nullptr));
        beamTitle->setText(QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\344\276\233\346\235\237\345\233\236\350\257\273", nullptr));
        beamValue->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        resultTitle->setText(QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\347\205\247\345\260\204\345\217\215\351\246\210", nullptr));
        resultValue->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        muLabel->setText(QCoreApplication::translate("QaWindow", "\347\264\257\350\256\241 MU / \347\233\256\346\240\207\357\274\232- / -", nullptr));
        doseProgress->setFormat(QCoreApplication::translate("QaWindow", "\347\255\211\345\276\205\345\211\202\351\207\217\345\217\215\351\246\210", nullptr));
        qaEnergyLabel->setText(QCoreApplication::translate("QaWindow", "\345\233\236\350\257\273\350\203\275\351\207\217\357\274\232-", nullptr));
        resultSummary->setText(QCoreApplication::translate("QaWindow", "\345\260\232\346\227\240 QA \345\256\214\346\210\220\345\233\236\346\211\247", nullptr));
        QTableWidgetItem *___qtablewidgetitem = resultTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("QaWindow", "\347\273\223\346\236\234\345\255\227\346\256\265", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = resultTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("QaWindow", "\345\220\216\347\253\257\345\233\236\350\257\273", nullptr));
        deviceSummaryTitle->setText(QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\350\277\236\346\216\245\344\270\216\345\256\211\345\205\250\345\217\215\351\246\210", nullptr));
        couchName->setText(QCoreApplication::translate("QaWindow", "\346\262\273\347\226\227\345\272\212", nullptr));
        couchLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        drName->setText(QCoreApplication::translate("QaWindow", "DR \350\265\267\350\220\275\346\236\266", nullptr));
        drLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        igrtName->setText(QCoreApplication::translate("QaWindow", "IGRT", nullptr));
        igrtLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        filterName->setText(QCoreApplication::translate("QaWindow", "\350\204\212\345\275\242\350\277\207\346\273\244\345\231\250", nullptr));
        filterLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        plcName->setText(QCoreApplication::translate("QaWindow", "PLC", nullptr));
        plcLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        g01Name->setText(QCoreApplication::translate("QaWindow", "\346\234\272\346\236\266 G01", nullptr));
        g01Lamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        g02Name->setText(QCoreApplication::translate("QaWindow", "\346\234\272\346\236\266 G02", nullptr));
        g02Lamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        acceleratorName->setText(QCoreApplication::translate("QaWindow", "\345\212\240\351\200\237\345\231\250", nullptr));
        acceleratorLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        ddsName->setText(QCoreApplication::translate("QaWindow", "DDS \350\201\224\351\224\201", nullptr));
        ddsLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        faultName->setText(QCoreApplication::translate("QaWindow", "\345\207\272\346\235\237\346\225\205\351\232\234", nullptr));
        faultLamp->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        detailsButton->setText(QCoreApplication::translate("QaWindow", "\350\257\212\346\226\255\346\230\216\347\273\206", nullptr));
        sum_doseTitle->setText(QCoreApplication::translate("QaWindow", "\347\264\257\350\256\241\345\216\237\345\247\213\350\256\241\346\225\260", nullptr));
        sum_doseValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        dose_rateTitle->setText(QCoreApplication::translate("QaWindow", "\345\216\237\345\247\213\345\211\202\351\207\217\347\216\207", nullptr));
        dose_rateValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        current_layerTitle->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215\345\261\202", nullptr));
        current_layerValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        current_pointTitle->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215\347\202\271", nullptr));
        current_pointValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        current_doseTitle->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215\347\202\271\350\256\241\346\225\260", nullptr));
        current_doseValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        layer_sum_doseTitle->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215\345\261\202\350\256\241\346\225\260", nullptr));
        layer_sum_doseValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        next_layerTitle->setText(QCoreApplication::translate("QaWindow", "\344\270\213\344\270\200\345\261\202", nullptr));
        next_layerValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        therapy_idTitle->setText(QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\344\273\273\345\212\241 ID", nullptr));
        therapy_idValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        timestampTitle->setText(QCoreApplication::translate("QaWindow", "\351\201\245\346\265\213\346\227\266\351\227\264", nullptr));
        timestampValue->setText(QCoreApplication::translate("QaWindow", "-", nullptr));
        positionTitle->setText(QCoreApplication::translate("QaWindow", "\346\262\273\347\226\227\345\272\212\344\275\215\347\275\256\357\274\210\345\216\237\346\216\245\345\217\243\345\215\225\344\275\215\357\274\211", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = positionTable->horizontalHeaderItem(0);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("QaWindow", "X", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = positionTable->horizontalHeaderItem(1);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("QaWindow", "Y", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = positionTable->horizontalHeaderItem(2);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("QaWindow", "Z", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = positionTable->horizontalHeaderItem(3);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("QaWindow", "A", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = positionTable->horizontalHeaderItem(4);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("QaWindow", "B", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = positionTable->horizontalHeaderItem(5);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("QaWindow", "C", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = positionTable->verticalHeaderItem(0);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("QaWindow", "\345\275\223\345\211\215", nullptr));

        const bool __sortingEnabled = positionTable->isSortingEnabled();
        positionTable->setSortingEnabled(false);
        QTableWidgetItem *___qtablewidgetitem9 = positionTable->item(0, 0);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = positionTable->item(0, 1);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = positionTable->item(0, 2);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = positionTable->item(0, 3);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = positionTable->item(0, 4);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = positionTable->item(0, 5);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("QaWindow", "\346\234\252\347\237\245", nullptr));
        positionTable->setSortingEnabled(__sortingEnabled);

        telemetryNotice->setText(QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\351\201\245\346\265\213\347\213\254\347\253\213\346\230\276\347\244\272\357\274\233\345\216\237\345\247\213\350\256\241\346\225\260\344\270\215\346\215\242\347\256\227\344\270\272 MU \346\210\226 Gy\357\274\214\344\276\233\346\235\237\345\217\215\351\246\210\344\270\215\344\273\243\350\241\250 QA \345\210\244\345\256\232\343\200\202", nullptr));
        detailsTabs->setTabText(detailsTabs->indexOf(telemetryPage), QCoreApplication::translate("QaWindow", "\350\256\276\345\244\207\351\201\245\346\265\213", nullptr));
        detailsTabs->setTabText(detailsTabs->indexOf(communicationPage), QCoreApplication::translate("QaWindow", "\351\200\232\344\277\241\350\256\260\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QaWindow: public Ui_QaWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QAWINDOW_H
