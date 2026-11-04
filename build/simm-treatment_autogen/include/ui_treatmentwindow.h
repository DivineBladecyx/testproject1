/********************************************************************************
** Form generated from reading UI file 'treatmentwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TREATMENTWINDOW_H
#define UI_TREATMENTWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "spotmapwidget.h"

QT_BEGIN_NAMESPACE

class Ui_TreatmentWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_root;
    QHBoxLayout *horizontalLayout_topArea;
    QGroupBox *groupBoxPatientInfo;
    QVBoxLayout *verticalLayout_patient;
    QHBoxLayout *horizontalLayout_title_patient;
    QLabel *label_icon_patient;
    QLabel *label_title_patient;
    QSpacerItem *horizontalSpacer_title_patient;
    QLabel *patient_name;
    QLabel *patient_id;
    QSpacerItem *verticalSpacer_patient;
    QGroupBox *groupBoxPrescriptionDose;
    QVBoxLayout *verticalLayout_prescriptionDose;
    QHBoxLayout *horizontalLayout_title_prescriptionDose;
    QLabel *label_icon_prescriptionDose;
    QLabel *label_title_prescriptionDose;
    QSpacerItem *horizontalSpacer_title_prescriptionDose;
    QLabel *labelTopPrescriptionDose;
    QLabel *labelTopPrescriptionDetail;
    QSpacerItem *verticalSpacer_prescriptionDose;
    QGroupBox *groupBoxTherapistInfo;
    QVBoxLayout *verticalLayout_doctor;
    QHBoxLayout *horizontalLayout_title_doctor;
    QLabel *label_icon_doctor;
    QLabel *label_title_doctor;
    QSpacerItem *horizontalSpacer_title_doctor;
    QLabel *label_6;
    QLabel *label_user;
    QSpacerItem *verticalSpacer_doctor;
    QGroupBox *groupBoxRoomInfo;
    QVBoxLayout *verticalLayout_room;
    QHBoxLayout *horizontalLayout_title_room;
    QLabel *label_icon_room;
    QLabel *label_title_room;
    QSpacerItem *horizontalSpacer_title_room;
    QLabel *label_7;
    QLabel *label_room;
    QSpacerItem *verticalSpacer_room;
    QGroupBox *groupBoxCurrentTime;
    QVBoxLayout *verticalLayout_time;
    QHBoxLayout *horizontalLayout_title_time;
    QLabel *label_icon_time;
    QLabel *label_title_time;
    QSpacerItem *horizontalSpacer_title_time;
    QLabel *label_time;
    QLabel *label_time_week;
    QGroupBox *groupBoxTopActions;
    QHBoxLayout *horizontalLayout;
    QPushButton *todays_pushButton;
    QPushButton *theme_pushButton;
    QPushButton *logout_pushButton;
    QGridLayout *mainGrid;
    QGroupBox *groupBoxTreatmentPlan;
    QVBoxLayout *verticalLayout_plan;
    QHBoxLayout *horizontalLayout_title_plan;
    QLabel *label_icon_plan;
    QLabel *label_title_plan;
    QSpacerItem *horizontalSpacer_title_plan;
    QWidget *widgetPlanSummary;
    QGridLayout *gridLayout_planSummary;
    QLabel *labelPlanIdTitle;
    QLabel *labelPlanIdValue;
    QLabel *labelPlanFractionIdTitle;
    QLabel *labelPlanFractionIdValue;
    QLabel *labelPlanSiteTargetTitle;
    QLabel *labelPlanSiteTargetValue;
    QLabel *labelPlanIsocenterTitle;
    QLabel *labelPlanIsocenterValue;
    QLabel *labelPlanQaReleaseTitle;
    QLabel *labelPlanQaReleaseValue;
    QHBoxLayout *horizontalLayout_7;
    QPushButton *confirmPlan_pushButton;
    QPushButton *confirmPlan_cancel_pushButton;
    QPushButton *planDetail_pushButton;
    QSpacerItem *horizontalSpacer_planButtons;
    SpotMapWidget *spotMapWidget;
    QGroupBox *groupBoxTreatmentFlow;
    QVBoxLayout *verticalLayout_flowBox;
    QHBoxLayout *horizontalLayout_title_flow;
    QLabel *label_icon_flow;
    QLabel *label_title_flow;
    QSpacerItem *horizontalSpacer_title_flow;
    QHBoxLayout *horizontalLayout_6;
    QWidget *qmlStepContainer;
    QHBoxLayout *horizontalLayout_4;
    QLabel *labelCurrentTreatmentAction;
    QGroupBox *groupBoxOperationQueue;
    QVBoxLayout *verticalLayout_ops;
    QHBoxLayout *horizontalLayout_title_ops;
    QLabel *label_icon_ops;
    QLabel *label_title_ops;
    QSpacerItem *horizontalSpacer_title_ops;
    QLabel *label_10;
    QLabel *label_11;
    QLabel *beamRoomStatusTitle;
    QWidget *beamRoomOverviewPanel;
    QGridLayout *gridLayoutBeamRooms;
    QLabel *beamRoomLabel1;
    QLabel *beamRoomLabel2;
    QLabel *beamRoomLabel9;
    QSpacerItem *verticalSpacer_ops;
    QHBoxLayout *horizontalLayoutBeamWait;
    QSpacerItem *horizontalSpacerBeamWaitLeft;
    QLabel *labelBeamWaitTitle;
    QLabel *label_30;
    QSpacerItem *horizontalSpacerBeamWaitRight;
    QHBoxLayout *horizontalLayout_opsButtons;
    QSpacerItem *horizontalSpacer_opsButtonsLeft;
    QPushButton *beamApply_pushButton;
    QPushButton *beamApply_cancel_pushButton;
    QSpacerItem *horizontalSpacer_opsButtonsRight;
    QGroupBox *groupBoxSystemStatus;
    QVBoxLayout *verticalLayout_statusBox;
    QHBoxLayout *horizontalLayout_title_status;
    QLabel *label_icon_status;
    QLabel *label_title_status;
    QSpacerItem *horizontalSpacer_title_status;
    QGridLayout *horizontalLayout_3;
    QLabel *label_34;
    QLabel *label_9;
    QLabel *label_36;
    QLabel *label_35;
    QLabel *label_37;
    QLabel *label_46;
    QLabel *label_48;
    QLabel *label_47;
    QSpacerItem *horizontalSpacer_status;
    QGridLayout *gridLayout_deviceStatus;
    QLabel *labelDeviceStatusCaption;
    QLabel *labelCouchDeviceStatus;
    QLabel *labelDrDeviceStatus;
    QLabel *labelIgrtDeviceStatus;
    QLabel *labelFilterDeviceStatus;
    QLabel *labelPlcDeviceStatus;
    QSpacerItem *horizontalSpacer_deviceStatus;
    QGroupBox *groupBoxBedPosition;
    QVBoxLayout *verticalLayout_bedBox;
    QLabel *labelDosePanelTitle;
    QGridLayout *gridLayoutDoseInfo;
    QWidget *dosePrescriptionCard;
    QVBoxLayout *verticalLayoutDosePrescription;
    QLabel *labelDosePrescriptionName;
    QLabel *labelPrescriptionDose;
    QWidget *doseFractionCard;
    QVBoxLayout *verticalLayoutDoseFraction;
    QLabel *labelDoseFractionName;
    QLabel *labelFractionDose;
    QWidget *doseCurrentFractionCard;
    QVBoxLayout *verticalLayoutDoseCurrentFraction;
    QLabel *labelDoseCurrentFractionName;
    QLabel *labelCurrentFractionDose;
    QWidget *doseCompletedCard;
    QVBoxLayout *verticalLayoutDoseCompleted;
    QLabel *labelDoseCompletedName;
    QLabel *labelCompletedDose;
    QWidget *doseBeamMuCard;
    QVBoxLayout *verticalLayoutDoseBeamMu;
    QLabel *labelDoseBeamMuName;
    QProgressBar *progressBarBeamMeterset;
    QLabel *labelBeamMuDose;
    QWidget *doseDeviationCard;
    QVBoxLayout *verticalLayoutDoseDeviation;
    QLabel *labelDoseDeviationName;
    QLabel *labelDoseDeviation;
    QWidget *doseLayerProgressCard;
    QVBoxLayout *verticalLayoutDoseLayerProgress;
    QLabel *labelLayerProgressName;
    QHBoxLayout *horizontalLayoutLayerProgress;
    QProgressBar *progressBarLayerProgress;
    QLabel *labelLayerProgressValue;
    QWidget *doseBeamInfoCard;
    QVBoxLayout *verticalLayoutDoseBeamInfo;
    QLabel *labelBeamDoseInfoName;
    QLabel *labelBeamDoseInfo;
    QWidget *bedPositionLegacyContainer;
    QVBoxLayout *verticalLayoutBedPositionLegacy;
    QHBoxLayout *horizontalLayout_title_bed;
    QLabel *label_icon_bed;
    QLabel *label_title_bed;
    QGridLayout *gridLayout_2;
    QLabel *label_38;
    QLabel *label_39;
    QLabel *label_40;
    QLabel *label_41;
    QLabel *label_42;
    QLabel *label_43;
    QLabel *label_44;
    QLabel *label_45;
    QLabel *label_posa;
    QLabel *label_posb;
    QLabel *label_posc;
    QLabel *label_posx;
    QLabel *label_posy;
    QLabel *label_posz;
    QLineEdit *lineEdit_a;
    QLineEdit *lineEdit_b;
    QLineEdit *lineEdit_c;
    QLineEdit *lineEdit_x;
    QLineEdit *lineEdit_y;
    QLineEdit *lineEdit_z;
    QVBoxLayout *verticalLayout_preview;
    QGroupBox *groupBoxBedPreview;
    QVBoxLayout *verticalLayout_previewPanel;
    QLabel *label_preview_title;
    QHBoxLayout *horizontalLayout_bed;
    QVBoxLayout *verticalLayout_compat;
    QTabWidget *tabWidget;
    QWidget *tab;
    QWidget *tab_2;
    QGridLayout *gridLayout_6;
    QLabel *label_20;
    QLabel *label_21;
    QLabel *label_22;
    QTableWidget *tableWidget;
    QTableWidget *tableWidget_2;
    QTableWidget *tableWidget_3;
    QLabel *label_23;
    QHBoxLayout *horizontalLayout_mu;
    QLabel *label_24;
    QSpacerItem *horizontalSpacer_mu;
    QLabel *label_25;
    QWidget *doseDeliveryProgressCard;
    QVBoxLayout *verticalLayoutDoseDeliveryProgress;
    QLabel *labelDoseDeliveryProgressName;
    QHBoxLayout *horizontalLayoutDoseDeliveryProgress;
    QProgressBar *progressBarDoseDelivery;
    QLabel *labelDoseDeliveryProgressValue;
    QProgressBar *progressBar_6;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *TreatmentWindow)
    {
        if (TreatmentWindow->objectName().isEmpty())
            TreatmentWindow->setObjectName("TreatmentWindow");
        TreatmentWindow->resize(1652, 1000);
        QFont font;
        font.setPointSize(15);
        TreatmentWindow->setFont(font);
        centralwidget = new QWidget(TreatmentWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout_root = new QVBoxLayout(centralwidget);
        verticalLayout_root->setSpacing(12);
        verticalLayout_root->setObjectName("verticalLayout_root");
        verticalLayout_root->setContentsMargins(12, 12, 12, 12);
        horizontalLayout_topArea = new QHBoxLayout();
        horizontalLayout_topArea->setSpacing(12);
        horizontalLayout_topArea->setObjectName("horizontalLayout_topArea");
        horizontalLayout_topArea->setContentsMargins(0, 0, 0, 0);
        groupBoxPatientInfo = new QGroupBox(centralwidget);
        groupBoxPatientInfo->setObjectName("groupBoxPatientInfo");
        groupBoxPatientInfo->setMinimumSize(QSize(220, 112));
        verticalLayout_patient = new QVBoxLayout(groupBoxPatientInfo);
        verticalLayout_patient->setSpacing(8);
        verticalLayout_patient->setObjectName("verticalLayout_patient");
        horizontalLayout_title_patient = new QHBoxLayout();
        horizontalLayout_title_patient->setObjectName("horizontalLayout_title_patient");
        horizontalLayout_title_patient->setContentsMargins(0, 0, 0, 0);
        label_icon_patient = new QLabel(groupBoxPatientInfo);
        label_icon_patient->setObjectName("label_icon_patient");
        label_icon_patient->setMinimumSize(QSize(16, 16));
        label_icon_patient->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_patient->addWidget(label_icon_patient);

        label_title_patient = new QLabel(groupBoxPatientInfo);
        label_title_patient->setObjectName("label_title_patient");

        horizontalLayout_title_patient->addWidget(label_title_patient);

        horizontalSpacer_title_patient = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_patient->addItem(horizontalSpacer_title_patient);


        verticalLayout_patient->addLayout(horizontalLayout_title_patient);

        patient_name = new QLabel(groupBoxPatientInfo);
        patient_name->setObjectName("patient_name");

        verticalLayout_patient->addWidget(patient_name);

        patient_id = new QLabel(groupBoxPatientInfo);
        patient_id->setObjectName("patient_id");

        verticalLayout_patient->addWidget(patient_id);

        verticalSpacer_patient = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_patient->addItem(verticalSpacer_patient);


        horizontalLayout_topArea->addWidget(groupBoxPatientInfo);

        groupBoxPrescriptionDose = new QGroupBox(centralwidget);
        groupBoxPrescriptionDose->setObjectName("groupBoxPrescriptionDose");
        groupBoxPrescriptionDose->setMinimumSize(QSize(190, 112));
        verticalLayout_prescriptionDose = new QVBoxLayout(groupBoxPrescriptionDose);
        verticalLayout_prescriptionDose->setSpacing(6);
        verticalLayout_prescriptionDose->setObjectName("verticalLayout_prescriptionDose");
        horizontalLayout_title_prescriptionDose = new QHBoxLayout();
        horizontalLayout_title_prescriptionDose->setObjectName("horizontalLayout_title_prescriptionDose");
        horizontalLayout_title_prescriptionDose->setContentsMargins(0, 0, 0, 0);
        label_icon_prescriptionDose = new QLabel(groupBoxPrescriptionDose);
        label_icon_prescriptionDose->setObjectName("label_icon_prescriptionDose");
        label_icon_prescriptionDose->setMinimumSize(QSize(24, 18));
        label_icon_prescriptionDose->setMaximumSize(QSize(24, 18));
        label_icon_prescriptionDose->setAlignment(Qt::AlignmentFlag::AlignCenter);

        horizontalLayout_title_prescriptionDose->addWidget(label_icon_prescriptionDose);

        label_title_prescriptionDose = new QLabel(groupBoxPrescriptionDose);
        label_title_prescriptionDose->setObjectName("label_title_prescriptionDose");

        horizontalLayout_title_prescriptionDose->addWidget(label_title_prescriptionDose);

        horizontalSpacer_title_prescriptionDose = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_prescriptionDose->addItem(horizontalSpacer_title_prescriptionDose);


        verticalLayout_prescriptionDose->addLayout(horizontalLayout_title_prescriptionDose);

        labelTopPrescriptionDose = new QLabel(groupBoxPrescriptionDose);
        labelTopPrescriptionDose->setObjectName("labelTopPrescriptionDose");
        QFont font1;
        font1.setPointSize(16);
        font1.setBold(true);
        labelTopPrescriptionDose->setFont(font1);
        labelTopPrescriptionDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout_prescriptionDose->addWidget(labelTopPrescriptionDose);

        labelTopPrescriptionDetail = new QLabel(groupBoxPrescriptionDose);
        labelTopPrescriptionDetail->setObjectName("labelTopPrescriptionDetail");
        labelTopPrescriptionDetail->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout_prescriptionDose->addWidget(labelTopPrescriptionDetail);

        verticalSpacer_prescriptionDose = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_prescriptionDose->addItem(verticalSpacer_prescriptionDose);


        horizontalLayout_topArea->addWidget(groupBoxPrescriptionDose);

        groupBoxTherapistInfo = new QGroupBox(centralwidget);
        groupBoxTherapistInfo->setObjectName("groupBoxTherapistInfo");
        groupBoxTherapistInfo->setMinimumSize(QSize(210, 112));
        verticalLayout_doctor = new QVBoxLayout(groupBoxTherapistInfo);
        verticalLayout_doctor->setSpacing(8);
        verticalLayout_doctor->setObjectName("verticalLayout_doctor");
        horizontalLayout_title_doctor = new QHBoxLayout();
        horizontalLayout_title_doctor->setObjectName("horizontalLayout_title_doctor");
        horizontalLayout_title_doctor->setContentsMargins(0, 0, 0, 0);
        label_icon_doctor = new QLabel(groupBoxTherapistInfo);
        label_icon_doctor->setObjectName("label_icon_doctor");
        label_icon_doctor->setMinimumSize(QSize(16, 16));
        label_icon_doctor->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_doctor->addWidget(label_icon_doctor);

        label_title_doctor = new QLabel(groupBoxTherapistInfo);
        label_title_doctor->setObjectName("label_title_doctor");

        horizontalLayout_title_doctor->addWidget(label_title_doctor);

        horizontalSpacer_title_doctor = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_doctor->addItem(horizontalSpacer_title_doctor);


        verticalLayout_doctor->addLayout(horizontalLayout_title_doctor);

        label_6 = new QLabel(groupBoxTherapistInfo);
        label_6->setObjectName("label_6");

        verticalLayout_doctor->addWidget(label_6);

        label_user = new QLabel(groupBoxTherapistInfo);
        label_user->setObjectName("label_user");

        verticalLayout_doctor->addWidget(label_user);

        verticalSpacer_doctor = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_doctor->addItem(verticalSpacer_doctor);


        horizontalLayout_topArea->addWidget(groupBoxTherapistInfo);

        groupBoxRoomInfo = new QGroupBox(centralwidget);
        groupBoxRoomInfo->setObjectName("groupBoxRoomInfo");
        groupBoxRoomInfo->setMinimumSize(QSize(190, 112));
        verticalLayout_room = new QVBoxLayout(groupBoxRoomInfo);
        verticalLayout_room->setSpacing(8);
        verticalLayout_room->setObjectName("verticalLayout_room");
        horizontalLayout_title_room = new QHBoxLayout();
        horizontalLayout_title_room->setObjectName("horizontalLayout_title_room");
        horizontalLayout_title_room->setContentsMargins(0, 0, 0, 0);
        label_icon_room = new QLabel(groupBoxRoomInfo);
        label_icon_room->setObjectName("label_icon_room");
        label_icon_room->setMinimumSize(QSize(16, 16));
        label_icon_room->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_room->addWidget(label_icon_room);

        label_title_room = new QLabel(groupBoxRoomInfo);
        label_title_room->setObjectName("label_title_room");

        horizontalLayout_title_room->addWidget(label_title_room);

        horizontalSpacer_title_room = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_room->addItem(horizontalSpacer_title_room);


        verticalLayout_room->addLayout(horizontalLayout_title_room);

        label_7 = new QLabel(groupBoxRoomInfo);
        label_7->setObjectName("label_7");

        verticalLayout_room->addWidget(label_7);

        label_room = new QLabel(groupBoxRoomInfo);
        label_room->setObjectName("label_room");

        verticalLayout_room->addWidget(label_room);

        verticalSpacer_room = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_room->addItem(verticalSpacer_room);


        horizontalLayout_topArea->addWidget(groupBoxRoomInfo);

        groupBoxCurrentTime = new QGroupBox(centralwidget);
        groupBoxCurrentTime->setObjectName("groupBoxCurrentTime");
        groupBoxCurrentTime->setMinimumSize(QSize(260, 112));
        verticalLayout_time = new QVBoxLayout(groupBoxCurrentTime);
        verticalLayout_time->setSpacing(6);
        verticalLayout_time->setObjectName("verticalLayout_time");
        horizontalLayout_title_time = new QHBoxLayout();
        horizontalLayout_title_time->setObjectName("horizontalLayout_title_time");
        horizontalLayout_title_time->setContentsMargins(0, 0, 0, 0);
        label_icon_time = new QLabel(groupBoxCurrentTime);
        label_icon_time->setObjectName("label_icon_time");
        label_icon_time->setMinimumSize(QSize(16, 16));
        label_icon_time->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_time->addWidget(label_icon_time);

        label_title_time = new QLabel(groupBoxCurrentTime);
        label_title_time->setObjectName("label_title_time");

        horizontalLayout_title_time->addWidget(label_title_time);

        horizontalSpacer_title_time = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_time->addItem(horizontalSpacer_title_time);


        verticalLayout_time->addLayout(horizontalLayout_title_time);

        label_time = new QLabel(groupBoxCurrentTime);
        label_time->setObjectName("label_time");
        label_time->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_time->addWidget(label_time);

        label_time_week = new QLabel(groupBoxCurrentTime);
        label_time_week->setObjectName("label_time_week");
        label_time_week->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_time->addWidget(label_time_week);


        horizontalLayout_topArea->addWidget(groupBoxCurrentTime);

        groupBoxTopActions = new QGroupBox(centralwidget);
        groupBoxTopActions->setObjectName("groupBoxTopActions");
        groupBoxTopActions->setMinimumSize(QSize(360, 112));
        horizontalLayout = new QHBoxLayout(groupBoxTopActions);
        horizontalLayout->setSpacing(12);
        horizontalLayout->setObjectName("horizontalLayout");
        todays_pushButton = new QPushButton(groupBoxTopActions);
        todays_pushButton->setObjectName("todays_pushButton");
        todays_pushButton->setMinimumSize(QSize(140, 42));

        horizontalLayout->addWidget(todays_pushButton);

        theme_pushButton = new QPushButton(groupBoxTopActions);
        theme_pushButton->setObjectName("theme_pushButton");
        theme_pushButton->setMinimumSize(QSize(140, 42));

        horizontalLayout->addWidget(theme_pushButton);

        logout_pushButton = new QPushButton(groupBoxTopActions);
        logout_pushButton->setObjectName("logout_pushButton");
        logout_pushButton->setMinimumSize(QSize(110, 42));

        horizontalLayout->addWidget(logout_pushButton);


        horizontalLayout_topArea->addWidget(groupBoxTopActions);


        verticalLayout_root->addLayout(horizontalLayout_topArea);

        mainGrid = new QGridLayout();
        mainGrid->setSpacing(12);
        mainGrid->setObjectName("mainGrid");
        mainGrid->setContentsMargins(0, 0, 0, 0);
        groupBoxTreatmentPlan = new QGroupBox(centralwidget);
        groupBoxTreatmentPlan->setObjectName("groupBoxTreatmentPlan");
        verticalLayout_plan = new QVBoxLayout(groupBoxTreatmentPlan);
        verticalLayout_plan->setSpacing(10);
        verticalLayout_plan->setObjectName("verticalLayout_plan");
        horizontalLayout_title_plan = new QHBoxLayout();
        horizontalLayout_title_plan->setObjectName("horizontalLayout_title_plan");
        horizontalLayout_title_plan->setContentsMargins(0, 0, 0, 0);
        label_icon_plan = new QLabel(groupBoxTreatmentPlan);
        label_icon_plan->setObjectName("label_icon_plan");
        label_icon_plan->setMinimumSize(QSize(16, 16));
        label_icon_plan->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_plan->addWidget(label_icon_plan);

        label_title_plan = new QLabel(groupBoxTreatmentPlan);
        label_title_plan->setObjectName("label_title_plan");

        horizontalLayout_title_plan->addWidget(label_title_plan);

        horizontalSpacer_title_plan = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_plan->addItem(horizontalSpacer_title_plan);


        verticalLayout_plan->addLayout(horizontalLayout_title_plan);

        widgetPlanSummary = new QWidget(groupBoxTreatmentPlan);
        widgetPlanSummary->setObjectName("widgetPlanSummary");
        gridLayout_planSummary = new QGridLayout(widgetPlanSummary);
        gridLayout_planSummary->setObjectName("gridLayout_planSummary");
        gridLayout_planSummary->setHorizontalSpacing(10);
        gridLayout_planSummary->setVerticalSpacing(8);
        gridLayout_planSummary->setContentsMargins(0, 0, 0, 0);
        labelPlanIdTitle = new QLabel(widgetPlanSummary);
        labelPlanIdTitle->setObjectName("labelPlanIdTitle");

        gridLayout_planSummary->addWidget(labelPlanIdTitle, 0, 0, 1, 1);

        labelPlanIdValue = new QLabel(widgetPlanSummary);
        labelPlanIdValue->setObjectName("labelPlanIdValue");

        gridLayout_planSummary->addWidget(labelPlanIdValue, 0, 1, 1, 1);

        labelPlanFractionIdTitle = new QLabel(widgetPlanSummary);
        labelPlanFractionIdTitle->setObjectName("labelPlanFractionIdTitle");

        gridLayout_planSummary->addWidget(labelPlanFractionIdTitle, 1, 0, 1, 1);

        labelPlanFractionIdValue = new QLabel(widgetPlanSummary);
        labelPlanFractionIdValue->setObjectName("labelPlanFractionIdValue");

        gridLayout_planSummary->addWidget(labelPlanFractionIdValue, 1, 1, 1, 1);

        labelPlanSiteTargetTitle = new QLabel(widgetPlanSummary);
        labelPlanSiteTargetTitle->setObjectName("labelPlanSiteTargetTitle");

        gridLayout_planSummary->addWidget(labelPlanSiteTargetTitle, 2, 0, 1, 1);

        labelPlanSiteTargetValue = new QLabel(widgetPlanSummary);
        labelPlanSiteTargetValue->setObjectName("labelPlanSiteTargetValue");

        gridLayout_planSummary->addWidget(labelPlanSiteTargetValue, 2, 1, 1, 1);

        labelPlanIsocenterTitle = new QLabel(widgetPlanSummary);
        labelPlanIsocenterTitle->setObjectName("labelPlanIsocenterTitle");

        gridLayout_planSummary->addWidget(labelPlanIsocenterTitle, 3, 0, 1, 1);

        labelPlanIsocenterValue = new QLabel(widgetPlanSummary);
        labelPlanIsocenterValue->setObjectName("labelPlanIsocenterValue");

        gridLayout_planSummary->addWidget(labelPlanIsocenterValue, 3, 1, 1, 1);

        labelPlanQaReleaseTitle = new QLabel(widgetPlanSummary);
        labelPlanQaReleaseTitle->setObjectName("labelPlanQaReleaseTitle");

        gridLayout_planSummary->addWidget(labelPlanQaReleaseTitle, 4, 0, 1, 1);

        labelPlanQaReleaseValue = new QLabel(widgetPlanSummary);
        labelPlanQaReleaseValue->setObjectName("labelPlanQaReleaseValue");

        gridLayout_planSummary->addWidget(labelPlanQaReleaseValue, 4, 1, 1, 1);


        verticalLayout_plan->addWidget(widgetPlanSummary);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        confirmPlan_pushButton = new QPushButton(groupBoxTreatmentPlan);
        confirmPlan_pushButton->setObjectName("confirmPlan_pushButton");
        confirmPlan_pushButton->setMinimumSize(QSize(140, 42));

        horizontalLayout_7->addWidget(confirmPlan_pushButton);

        confirmPlan_cancel_pushButton = new QPushButton(groupBoxTreatmentPlan);
        confirmPlan_cancel_pushButton->setObjectName("confirmPlan_cancel_pushButton");
        confirmPlan_cancel_pushButton->setMinimumSize(QSize(140, 42));

        horizontalLayout_7->addWidget(confirmPlan_cancel_pushButton);

        planDetail_pushButton = new QPushButton(groupBoxTreatmentPlan);
        planDetail_pushButton->setObjectName("planDetail_pushButton");
        planDetail_pushButton->setMinimumSize(QSize(140, 42));

        horizontalLayout_7->addWidget(planDetail_pushButton);

        horizontalSpacer_planButtons = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_planButtons);


        verticalLayout_plan->addLayout(horizontalLayout_7);

        spotMapWidget = new SpotMapWidget(groupBoxTreatmentPlan);
        spotMapWidget->setObjectName("spotMapWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(spotMapWidget->sizePolicy().hasHeightForWidth());
        spotMapWidget->setSizePolicy(sizePolicy);
        spotMapWidget->setMinimumSize(QSize(0, 250));

        verticalLayout_plan->addWidget(spotMapWidget);


        mainGrid->addWidget(groupBoxTreatmentPlan, 0, 0, 3, 1);

        groupBoxTreatmentFlow = new QGroupBox(centralwidget);
        groupBoxTreatmentFlow->setObjectName("groupBoxTreatmentFlow");
        verticalLayout_flowBox = new QVBoxLayout(groupBoxTreatmentFlow);
        verticalLayout_flowBox->setSpacing(6);
        verticalLayout_flowBox->setObjectName("verticalLayout_flowBox");
        horizontalLayout_title_flow = new QHBoxLayout();
        horizontalLayout_title_flow->setObjectName("horizontalLayout_title_flow");
        horizontalLayout_title_flow->setContentsMargins(0, 0, 0, 0);
        label_icon_flow = new QLabel(groupBoxTreatmentFlow);
        label_icon_flow->setObjectName("label_icon_flow");
        label_icon_flow->setMinimumSize(QSize(16, 16));
        label_icon_flow->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_flow->addWidget(label_icon_flow);

        label_title_flow = new QLabel(groupBoxTreatmentFlow);
        label_title_flow->setObjectName("label_title_flow");

        horizontalLayout_title_flow->addWidget(label_title_flow);

        horizontalSpacer_title_flow = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_flow->addItem(horizontalSpacer_title_flow);


        verticalLayout_flowBox->addLayout(horizontalLayout_title_flow);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        horizontalLayout_6->setContentsMargins(8, 8, 8, 8);
        qmlStepContainer = new QWidget(groupBoxTreatmentFlow);
        qmlStepContainer->setObjectName("qmlStepContainer");
        horizontalLayout_4 = new QHBoxLayout(qmlStepContainer);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);

        horizontalLayout_6->addWidget(qmlStepContainer);


        verticalLayout_flowBox->addLayout(horizontalLayout_6);

        labelCurrentTreatmentAction = new QLabel(groupBoxTreatmentFlow);
        labelCurrentTreatmentAction->setObjectName("labelCurrentTreatmentAction");
        labelCurrentTreatmentAction->setMinimumSize(QSize(0, 32));
        labelCurrentTreatmentAction->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelCurrentTreatmentAction->setWordWrap(true);

        verticalLayout_flowBox->addWidget(labelCurrentTreatmentAction);


        mainGrid->addWidget(groupBoxTreatmentFlow, 0, 1, 1, 3);

        groupBoxOperationQueue = new QGroupBox(centralwidget);
        groupBoxOperationQueue->setObjectName("groupBoxOperationQueue");
        verticalLayout_ops = new QVBoxLayout(groupBoxOperationQueue);
        verticalLayout_ops->setSpacing(12);
        verticalLayout_ops->setObjectName("verticalLayout_ops");
        horizontalLayout_title_ops = new QHBoxLayout();
        horizontalLayout_title_ops->setObjectName("horizontalLayout_title_ops");
        horizontalLayout_title_ops->setContentsMargins(0, 0, 0, 0);
        label_icon_ops = new QLabel(groupBoxOperationQueue);
        label_icon_ops->setObjectName("label_icon_ops");
        label_icon_ops->setMinimumSize(QSize(16, 16));
        label_icon_ops->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_ops->addWidget(label_icon_ops);

        label_title_ops = new QLabel(groupBoxOperationQueue);
        label_title_ops->setObjectName("label_title_ops");

        horizontalLayout_title_ops->addWidget(label_title_ops);

        horizontalSpacer_title_ops = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_ops->addItem(horizontalSpacer_title_ops);


        verticalLayout_ops->addLayout(horizontalLayout_title_ops);

        label_10 = new QLabel(groupBoxOperationQueue);
        label_10->setObjectName("label_10");

        verticalLayout_ops->addWidget(label_10);

        label_11 = new QLabel(groupBoxOperationQueue);
        label_11->setObjectName("label_11");
        label_11->setMinimumSize(QSize(0, 54));
        label_11->setTextFormat(Qt::TextFormat::RichText);
        label_11->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout_ops->addWidget(label_11);

        beamRoomStatusTitle = new QLabel(groupBoxOperationQueue);
        beamRoomStatusTitle->setObjectName("beamRoomStatusTitle");
        beamRoomStatusTitle->setMinimumSize(QSize(0, 22));
        beamRoomStatusTitle->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout_ops->addWidget(beamRoomStatusTitle);

        beamRoomOverviewPanel = new QWidget(groupBoxOperationQueue);
        beamRoomOverviewPanel->setObjectName("beamRoomOverviewPanel");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(beamRoomOverviewPanel->sizePolicy().hasHeightForWidth());
        beamRoomOverviewPanel->setSizePolicy(sizePolicy1);
        gridLayoutBeamRooms = new QGridLayout(beamRoomOverviewPanel);
        gridLayoutBeamRooms->setObjectName("gridLayoutBeamRooms");
        gridLayoutBeamRooms->setHorizontalSpacing(6);
        gridLayoutBeamRooms->setVerticalSpacing(6);
        gridLayoutBeamRooms->setContentsMargins(0, 0, 0, 0);
        beamRoomLabel1 = new QLabel(beamRoomOverviewPanel);
        beamRoomLabel1->setObjectName("beamRoomLabel1");
        beamRoomLabel1->setMinimumSize(QSize(0, 44));
        beamRoomLabel1->setTextFormat(Qt::TextFormat::RichText);
        beamRoomLabel1->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayoutBeamRooms->addWidget(beamRoomLabel1, 0, 0, 1, 1);

        beamRoomLabel2 = new QLabel(beamRoomOverviewPanel);
        beamRoomLabel2->setObjectName("beamRoomLabel2");
        beamRoomLabel2->setMinimumSize(QSize(0, 44));
        beamRoomLabel2->setTextFormat(Qt::TextFormat::RichText);
        beamRoomLabel2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayoutBeamRooms->addWidget(beamRoomLabel2, 0, 1, 1, 1);

        beamRoomLabel9 = new QLabel(beamRoomOverviewPanel);
        beamRoomLabel9->setObjectName("beamRoomLabel9");
        beamRoomLabel9->setMinimumSize(QSize(0, 44));
        beamRoomLabel9->setTextFormat(Qt::TextFormat::RichText);
        beamRoomLabel9->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayoutBeamRooms->addWidget(beamRoomLabel9, 0, 2, 1, 1);


        verticalLayout_ops->addWidget(beamRoomOverviewPanel);

        verticalSpacer_ops = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_ops->addItem(verticalSpacer_ops);

        horizontalLayoutBeamWait = new QHBoxLayout();
        horizontalLayoutBeamWait->setSpacing(8);
        horizontalLayoutBeamWait->setObjectName("horizontalLayoutBeamWait");
        horizontalSpacerBeamWaitLeft = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutBeamWait->addItem(horizontalSpacerBeamWaitLeft);

        labelBeamWaitTitle = new QLabel(groupBoxOperationQueue);
        labelBeamWaitTitle->setObjectName("labelBeamWaitTitle");
        labelBeamWaitTitle->setAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayoutBeamWait->addWidget(labelBeamWaitTitle);

        label_30 = new QLabel(groupBoxOperationQueue);
        label_30->setObjectName("label_30");
        label_30->setMinimumSize(QSize(72, 34));
        label_30->setAlignment(Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayoutBeamWait->addWidget(label_30);

        horizontalSpacerBeamWaitRight = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutBeamWait->addItem(horizontalSpacerBeamWaitRight);


        verticalLayout_ops->addLayout(horizontalLayoutBeamWait);

        horizontalLayout_opsButtons = new QHBoxLayout();
        horizontalLayout_opsButtons->setSpacing(10);
        horizontalLayout_opsButtons->setObjectName("horizontalLayout_opsButtons");
        horizontalSpacer_opsButtonsLeft = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_opsButtons->addItem(horizontalSpacer_opsButtonsLeft);

        beamApply_pushButton = new QPushButton(groupBoxOperationQueue);
        beamApply_pushButton->setObjectName("beamApply_pushButton");
        beamApply_pushButton->setMinimumSize(QSize(118, 42));
        beamApply_pushButton->setMaximumSize(QSize(118, 42));

        horizontalLayout_opsButtons->addWidget(beamApply_pushButton);

        beamApply_cancel_pushButton = new QPushButton(groupBoxOperationQueue);
        beamApply_cancel_pushButton->setObjectName("beamApply_cancel_pushButton");
        beamApply_cancel_pushButton->setMinimumSize(QSize(118, 42));
        beamApply_cancel_pushButton->setMaximumSize(QSize(118, 42));

        horizontalLayout_opsButtons->addWidget(beamApply_cancel_pushButton);

        horizontalSpacer_opsButtonsRight = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_opsButtons->addItem(horizontalSpacer_opsButtonsRight);


        verticalLayout_ops->addLayout(horizontalLayout_opsButtons);


        mainGrid->addWidget(groupBoxOperationQueue, 0, 4, 2, 1);

        groupBoxSystemStatus = new QGroupBox(centralwidget);
        groupBoxSystemStatus->setObjectName("groupBoxSystemStatus");
        verticalLayout_statusBox = new QVBoxLayout(groupBoxSystemStatus);
        verticalLayout_statusBox->setSpacing(6);
        verticalLayout_statusBox->setObjectName("verticalLayout_statusBox");
        horizontalLayout_title_status = new QHBoxLayout();
        horizontalLayout_title_status->setObjectName("horizontalLayout_title_status");
        horizontalLayout_title_status->setContentsMargins(0, 0, 0, 0);
        label_icon_status = new QLabel(groupBoxSystemStatus);
        label_icon_status->setObjectName("label_icon_status");
        label_icon_status->setMinimumSize(QSize(16, 16));
        label_icon_status->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_status->addWidget(label_icon_status);

        label_title_status = new QLabel(groupBoxSystemStatus);
        label_title_status->setObjectName("label_title_status");

        horizontalLayout_title_status->addWidget(label_title_status);

        horizontalSpacer_title_status = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title_status->addItem(horizontalSpacer_title_status);


        verticalLayout_statusBox->addLayout(horizontalLayout_title_status);

        horizontalLayout_3 = new QGridLayout();
        horizontalLayout_3->setSpacing(24);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_34 = new QLabel(groupBoxSystemStatus);
        label_34->setObjectName("label_34");

        horizontalLayout_3->addWidget(label_34, 0, 0, 1, 1);

        label_9 = new QLabel(groupBoxSystemStatus);
        label_9->setObjectName("label_9");
        label_9->setMinimumSize(QSize(24, 24));
        label_9->setMaximumSize(QSize(24, 24));

        horizontalLayout_3->addWidget(label_9, 0, 1, 1, 1);

        label_36 = new QLabel(groupBoxSystemStatus);
        label_36->setObjectName("label_36");

        horizontalLayout_3->addWidget(label_36, 0, 2, 1, 1);

        label_35 = new QLabel(groupBoxSystemStatus);
        label_35->setObjectName("label_35");
        label_35->setMinimumSize(QSize(24, 24));
        label_35->setMaximumSize(QSize(24, 24));

        horizontalLayout_3->addWidget(label_35, 0, 3, 1, 1);

        label_37 = new QLabel(groupBoxSystemStatus);
        label_37->setObjectName("label_37");
        label_37->setVisible(true);

        horizontalLayout_3->addWidget(label_37, 0, 4, 1, 1);

        label_46 = new QLabel(groupBoxSystemStatus);
        label_46->setObjectName("label_46");
        label_46->setVisible(true);
        label_46->setMinimumSize(QSize(24, 24));
        label_46->setMaximumSize(QSize(24, 24));

        horizontalLayout_3->addWidget(label_46, 0, 5, 1, 1);

        label_48 = new QLabel(groupBoxSystemStatus);
        label_48->setObjectName("label_48");
        label_48->setVisible(true);

        horizontalLayout_3->addWidget(label_48, 0, 6, 1, 1);

        label_47 = new QLabel(groupBoxSystemStatus);
        label_47->setObjectName("label_47");
        label_47->setVisible(true);
        label_47->setMinimumSize(QSize(24, 24));
        label_47->setMaximumSize(QSize(24, 24));

        horizontalLayout_3->addWidget(label_47, 0, 7, 1, 1);

        horizontalSpacer_status = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_status, 1, 0, 1, 8);


        verticalLayout_statusBox->addLayout(horizontalLayout_3);

        gridLayout_deviceStatus = new QGridLayout();
        gridLayout_deviceStatus->setObjectName("gridLayout_deviceStatus");
        gridLayout_deviceStatus->setHorizontalSpacing(8);
        gridLayout_deviceStatus->setVerticalSpacing(6);
        labelDeviceStatusCaption = new QLabel(groupBoxSystemStatus);
        labelDeviceStatusCaption->setObjectName("labelDeviceStatusCaption");

        gridLayout_deviceStatus->addWidget(labelDeviceStatusCaption, 0, 0, 1, 3);

        labelCouchDeviceStatus = new QLabel(groupBoxSystemStatus);
        labelCouchDeviceStatus->setObjectName("labelCouchDeviceStatus");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(labelCouchDeviceStatus->sizePolicy().hasHeightForWidth());
        labelCouchDeviceStatus->setSizePolicy(sizePolicy2);
        labelCouchDeviceStatus->setMinimumSize(QSize(0, 28));
        labelCouchDeviceStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_deviceStatus->addWidget(labelCouchDeviceStatus, 1, 0, 1, 1);

        labelDrDeviceStatus = new QLabel(groupBoxSystemStatus);
        labelDrDeviceStatus->setObjectName("labelDrDeviceStatus");
        sizePolicy2.setHeightForWidth(labelDrDeviceStatus->sizePolicy().hasHeightForWidth());
        labelDrDeviceStatus->setSizePolicy(sizePolicy2);
        labelDrDeviceStatus->setMinimumSize(QSize(0, 28));
        labelDrDeviceStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_deviceStatus->addWidget(labelDrDeviceStatus, 1, 1, 1, 1);

        labelIgrtDeviceStatus = new QLabel(groupBoxSystemStatus);
        labelIgrtDeviceStatus->setObjectName("labelIgrtDeviceStatus");
        sizePolicy2.setHeightForWidth(labelIgrtDeviceStatus->sizePolicy().hasHeightForWidth());
        labelIgrtDeviceStatus->setSizePolicy(sizePolicy2);
        labelIgrtDeviceStatus->setMinimumSize(QSize(0, 28));
        labelIgrtDeviceStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_deviceStatus->addWidget(labelIgrtDeviceStatus, 1, 2, 1, 1);

        labelFilterDeviceStatus = new QLabel(groupBoxSystemStatus);
        labelFilterDeviceStatus->setObjectName("labelFilterDeviceStatus");
        sizePolicy2.setHeightForWidth(labelFilterDeviceStatus->sizePolicy().hasHeightForWidth());
        labelFilterDeviceStatus->setSizePolicy(sizePolicy2);
        labelFilterDeviceStatus->setMinimumSize(QSize(0, 28));
        labelFilterDeviceStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_deviceStatus->addWidget(labelFilterDeviceStatus, 2, 0, 1, 1);

        labelPlcDeviceStatus = new QLabel(groupBoxSystemStatus);
        labelPlcDeviceStatus->setObjectName("labelPlcDeviceStatus");
        sizePolicy2.setHeightForWidth(labelPlcDeviceStatus->sizePolicy().hasHeightForWidth());
        labelPlcDeviceStatus->setSizePolicy(sizePolicy2);
        labelPlcDeviceStatus->setMinimumSize(QSize(0, 28));
        labelPlcDeviceStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_deviceStatus->addWidget(labelPlcDeviceStatus, 2, 1, 1, 1);

        horizontalSpacer_deviceStatus = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_deviceStatus->addItem(horizontalSpacer_deviceStatus, 2, 2, 1, 1);


        verticalLayout_statusBox->addLayout(gridLayout_deviceStatus);


        mainGrid->addWidget(groupBoxSystemStatus, 1, 1, 1, 3);

        groupBoxBedPosition = new QGroupBox(centralwidget);
        groupBoxBedPosition->setObjectName("groupBoxBedPosition");
        verticalLayout_bedBox = new QVBoxLayout(groupBoxBedPosition);
        verticalLayout_bedBox->setSpacing(8);
        verticalLayout_bedBox->setObjectName("verticalLayout_bedBox");
        verticalLayout_bedBox->setContentsMargins(12, 10, 12, 12);
        labelDosePanelTitle = new QLabel(groupBoxBedPosition);
        labelDosePanelTitle->setObjectName("labelDosePanelTitle");

        verticalLayout_bedBox->addWidget(labelDosePanelTitle);

        gridLayoutDoseInfo = new QGridLayout();
        gridLayoutDoseInfo->setObjectName("gridLayoutDoseInfo");
        gridLayoutDoseInfo->setHorizontalSpacing(8);
        gridLayoutDoseInfo->setVerticalSpacing(8);
        dosePrescriptionCard = new QWidget(groupBoxBedPosition);
        dosePrescriptionCard->setObjectName("dosePrescriptionCard");
        verticalLayoutDosePrescription = new QVBoxLayout(dosePrescriptionCard);
        verticalLayoutDosePrescription->setSpacing(3);
        verticalLayoutDosePrescription->setObjectName("verticalLayoutDosePrescription");
        verticalLayoutDosePrescription->setContentsMargins(0, 0, 0, 0);
        labelDosePrescriptionName = new QLabel(dosePrescriptionCard);
        labelDosePrescriptionName->setObjectName("labelDosePrescriptionName");

        verticalLayoutDosePrescription->addWidget(labelDosePrescriptionName);

        labelPrescriptionDose = new QLabel(dosePrescriptionCard);
        labelPrescriptionDose->setObjectName("labelPrescriptionDose");
        labelPrescriptionDose->setMinimumSize(QSize(0, 36));
        labelPrescriptionDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelPrescriptionDose->setWordWrap(true);

        verticalLayoutDosePrescription->addWidget(labelPrescriptionDose);


        gridLayoutDoseInfo->addWidget(dosePrescriptionCard, 0, 0, 1, 1);

        doseFractionCard = new QWidget(groupBoxBedPosition);
        doseFractionCard->setObjectName("doseFractionCard");
        verticalLayoutDoseFraction = new QVBoxLayout(doseFractionCard);
        verticalLayoutDoseFraction->setSpacing(3);
        verticalLayoutDoseFraction->setObjectName("verticalLayoutDoseFraction");
        verticalLayoutDoseFraction->setContentsMargins(0, 0, 0, 0);
        labelDoseFractionName = new QLabel(doseFractionCard);
        labelDoseFractionName->setObjectName("labelDoseFractionName");

        verticalLayoutDoseFraction->addWidget(labelDoseFractionName);

        labelFractionDose = new QLabel(doseFractionCard);
        labelFractionDose->setObjectName("labelFractionDose");
        labelFractionDose->setMinimumSize(QSize(0, 36));
        labelFractionDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelFractionDose->setWordWrap(true);

        verticalLayoutDoseFraction->addWidget(labelFractionDose);


        gridLayoutDoseInfo->addWidget(doseFractionCard, 0, 1, 1, 1);

        doseCurrentFractionCard = new QWidget(groupBoxBedPosition);
        doseCurrentFractionCard->setObjectName("doseCurrentFractionCard");
        verticalLayoutDoseCurrentFraction = new QVBoxLayout(doseCurrentFractionCard);
        verticalLayoutDoseCurrentFraction->setSpacing(3);
        verticalLayoutDoseCurrentFraction->setObjectName("verticalLayoutDoseCurrentFraction");
        verticalLayoutDoseCurrentFraction->setContentsMargins(0, 0, 0, 0);
        labelDoseCurrentFractionName = new QLabel(doseCurrentFractionCard);
        labelDoseCurrentFractionName->setObjectName("labelDoseCurrentFractionName");

        verticalLayoutDoseCurrentFraction->addWidget(labelDoseCurrentFractionName);

        labelCurrentFractionDose = new QLabel(doseCurrentFractionCard);
        labelCurrentFractionDose->setObjectName("labelCurrentFractionDose");
        labelCurrentFractionDose->setMinimumSize(QSize(0, 36));
        labelCurrentFractionDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelCurrentFractionDose->setWordWrap(true);

        verticalLayoutDoseCurrentFraction->addWidget(labelCurrentFractionDose);


        gridLayoutDoseInfo->addWidget(doseCurrentFractionCard, 1, 0, 1, 1);

        doseCompletedCard = new QWidget(groupBoxBedPosition);
        doseCompletedCard->setObjectName("doseCompletedCard");
        verticalLayoutDoseCompleted = new QVBoxLayout(doseCompletedCard);
        verticalLayoutDoseCompleted->setSpacing(3);
        verticalLayoutDoseCompleted->setObjectName("verticalLayoutDoseCompleted");
        verticalLayoutDoseCompleted->setContentsMargins(0, 0, 0, 0);
        labelDoseCompletedName = new QLabel(doseCompletedCard);
        labelDoseCompletedName->setObjectName("labelDoseCompletedName");

        verticalLayoutDoseCompleted->addWidget(labelDoseCompletedName);

        labelCompletedDose = new QLabel(doseCompletedCard);
        labelCompletedDose->setObjectName("labelCompletedDose");
        labelCompletedDose->setMinimumSize(QSize(0, 36));
        labelCompletedDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelCompletedDose->setWordWrap(true);

        verticalLayoutDoseCompleted->addWidget(labelCompletedDose);


        gridLayoutDoseInfo->addWidget(doseCompletedCard, 1, 1, 1, 1);

        doseBeamMuCard = new QWidget(groupBoxBedPosition);
        doseBeamMuCard->setObjectName("doseBeamMuCard");
        verticalLayoutDoseBeamMu = new QVBoxLayout(doseBeamMuCard);
        verticalLayoutDoseBeamMu->setSpacing(3);
        verticalLayoutDoseBeamMu->setObjectName("verticalLayoutDoseBeamMu");
        verticalLayoutDoseBeamMu->setContentsMargins(0, 0, 0, 0);
        labelDoseBeamMuName = new QLabel(doseBeamMuCard);
        labelDoseBeamMuName->setObjectName("labelDoseBeamMuName");

        verticalLayoutDoseBeamMu->addWidget(labelDoseBeamMuName);

        progressBarBeamMeterset = new QProgressBar(doseBeamMuCard);
        progressBarBeamMeterset->setObjectName("progressBarBeamMeterset");
        progressBarBeamMeterset->setMinimumSize(QSize(0, 30));
        progressBarBeamMeterset->setMaximumSize(QSize(16777215, 30));
        progressBarBeamMeterset->setMinimum(0);
        progressBarBeamMeterset->setMaximum(1000);
        progressBarBeamMeterset->setValue(0);

        verticalLayoutDoseBeamMu->addWidget(progressBarBeamMeterset);

        labelBeamMuDose = new QLabel(doseBeamMuCard);
        labelBeamMuDose->setObjectName("labelBeamMuDose");
        labelBeamMuDose->setMinimumSize(QSize(0, 36));
        labelBeamMuDose->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelBeamMuDose->setWordWrap(true);

        verticalLayoutDoseBeamMu->addWidget(labelBeamMuDose);


        gridLayoutDoseInfo->addWidget(doseBeamMuCard, 2, 0, 1, 1);

        doseDeviationCard = new QWidget(groupBoxBedPosition);
        doseDeviationCard->setObjectName("doseDeviationCard");
        verticalLayoutDoseDeviation = new QVBoxLayout(doseDeviationCard);
        verticalLayoutDoseDeviation->setSpacing(3);
        verticalLayoutDoseDeviation->setObjectName("verticalLayoutDoseDeviation");
        verticalLayoutDoseDeviation->setContentsMargins(0, 0, 0, 0);
        labelDoseDeviationName = new QLabel(doseDeviationCard);
        labelDoseDeviationName->setObjectName("labelDoseDeviationName");

        verticalLayoutDoseDeviation->addWidget(labelDoseDeviationName);

        labelDoseDeviation = new QLabel(doseDeviationCard);
        labelDoseDeviation->setObjectName("labelDoseDeviation");
        labelDoseDeviation->setMinimumSize(QSize(0, 36));
        labelDoseDeviation->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelDoseDeviation->setWordWrap(true);

        verticalLayoutDoseDeviation->addWidget(labelDoseDeviation);


        gridLayoutDoseInfo->addWidget(doseDeviationCard, 2, 1, 1, 1);

        doseLayerProgressCard = new QWidget(groupBoxBedPosition);
        doseLayerProgressCard->setObjectName("doseLayerProgressCard");
        verticalLayoutDoseLayerProgress = new QVBoxLayout(doseLayerProgressCard);
        verticalLayoutDoseLayerProgress->setSpacing(3);
        verticalLayoutDoseLayerProgress->setObjectName("verticalLayoutDoseLayerProgress");
        verticalLayoutDoseLayerProgress->setContentsMargins(0, 0, 0, 0);
        labelLayerProgressName = new QLabel(doseLayerProgressCard);
        labelLayerProgressName->setObjectName("labelLayerProgressName");

        verticalLayoutDoseLayerProgress->addWidget(labelLayerProgressName);

        horizontalLayoutLayerProgress = new QHBoxLayout();
        horizontalLayoutLayerProgress->setSpacing(8);
        horizontalLayoutLayerProgress->setObjectName("horizontalLayoutLayerProgress");
        progressBarLayerProgress = new QProgressBar(doseLayerProgressCard);
        progressBarLayerProgress->setObjectName("progressBarLayerProgress");
        progressBarLayerProgress->setMinimumSize(QSize(0, 30));
        progressBarLayerProgress->setMaximumSize(QSize(16777215, 30));
        progressBarLayerProgress->setMinimum(0);
        progressBarLayerProgress->setMaximum(100);
        progressBarLayerProgress->setValue(0);

        horizontalLayoutLayerProgress->addWidget(progressBarLayerProgress);

        labelLayerProgressValue = new QLabel(doseLayerProgressCard);
        labelLayerProgressValue->setObjectName("labelLayerProgressValue");
        labelLayerProgressValue->setMinimumSize(QSize(92, 30));
        labelLayerProgressValue->setMaximumSize(QSize(112, 30));
        labelLayerProgressValue->setAlignment(Qt::AlignmentFlag::AlignCenter);

        horizontalLayoutLayerProgress->addWidget(labelLayerProgressValue);


        verticalLayoutDoseLayerProgress->addLayout(horizontalLayoutLayerProgress);


        gridLayoutDoseInfo->addWidget(doseLayerProgressCard, 3, 0, 1, 2);

        doseBeamInfoCard = new QWidget(groupBoxBedPosition);
        doseBeamInfoCard->setObjectName("doseBeamInfoCard");
        verticalLayoutDoseBeamInfo = new QVBoxLayout(doseBeamInfoCard);
        verticalLayoutDoseBeamInfo->setSpacing(3);
        verticalLayoutDoseBeamInfo->setObjectName("verticalLayoutDoseBeamInfo");
        verticalLayoutDoseBeamInfo->setContentsMargins(0, 0, 0, 0);
        labelBeamDoseInfoName = new QLabel(doseBeamInfoCard);
        labelBeamDoseInfoName->setObjectName("labelBeamDoseInfoName");

        verticalLayoutDoseBeamInfo->addWidget(labelBeamDoseInfoName);

        labelBeamDoseInfo = new QLabel(doseBeamInfoCard);
        labelBeamDoseInfo->setObjectName("labelBeamDoseInfo");
        labelBeamDoseInfo->setMinimumSize(QSize(0, 48));
        labelBeamDoseInfo->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignVCenter);
        labelBeamDoseInfo->setWordWrap(true);

        verticalLayoutDoseBeamInfo->addWidget(labelBeamDoseInfo);


        gridLayoutDoseInfo->addWidget(doseBeamInfoCard, 4, 0, 1, 2);


        verticalLayout_bedBox->addLayout(gridLayoutDoseInfo);

        bedPositionLegacyContainer = new QWidget(groupBoxBedPosition);
        bedPositionLegacyContainer->setObjectName("bedPositionLegacyContainer");
        bedPositionLegacyContainer->setVisible(false);
        verticalLayoutBedPositionLegacy = new QVBoxLayout(bedPositionLegacyContainer);
        verticalLayoutBedPositionLegacy->setObjectName("verticalLayoutBedPositionLegacy");
        horizontalLayout_title_bed = new QHBoxLayout();
        horizontalLayout_title_bed->setObjectName("horizontalLayout_title_bed");
        label_icon_bed = new QLabel(bedPositionLegacyContainer);
        label_icon_bed->setObjectName("label_icon_bed");
        label_icon_bed->setMinimumSize(QSize(16, 16));
        label_icon_bed->setMaximumSize(QSize(16, 16));

        horizontalLayout_title_bed->addWidget(label_icon_bed);

        label_title_bed = new QLabel(bedPositionLegacyContainer);
        label_title_bed->setObjectName("label_title_bed");

        horizontalLayout_title_bed->addWidget(label_title_bed);


        verticalLayoutBedPositionLegacy->addLayout(horizontalLayout_title_bed);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setSpacing(12);
        gridLayout_2->setObjectName("gridLayout_2");
        label_38 = new QLabel(bedPositionLegacyContainer);
        label_38->setObjectName("label_38");

        gridLayout_2->addWidget(label_38, 0, 1, 1, 1);

        label_39 = new QLabel(bedPositionLegacyContainer);
        label_39->setObjectName("label_39");

        gridLayout_2->addWidget(label_39, 0, 2, 1, 1);

        label_40 = new QLabel(bedPositionLegacyContainer);
        label_40->setObjectName("label_40");

        gridLayout_2->addWidget(label_40, 1, 0, 1, 1);

        label_41 = new QLabel(bedPositionLegacyContainer);
        label_41->setObjectName("label_41");

        gridLayout_2->addWidget(label_41, 2, 0, 1, 1);

        label_42 = new QLabel(bedPositionLegacyContainer);
        label_42->setObjectName("label_42");

        gridLayout_2->addWidget(label_42, 3, 0, 1, 1);

        label_43 = new QLabel(bedPositionLegacyContainer);
        label_43->setObjectName("label_43");

        gridLayout_2->addWidget(label_43, 4, 0, 1, 1);

        label_44 = new QLabel(bedPositionLegacyContainer);
        label_44->setObjectName("label_44");

        gridLayout_2->addWidget(label_44, 5, 0, 1, 1);

        label_45 = new QLabel(bedPositionLegacyContainer);
        label_45->setObjectName("label_45");

        gridLayout_2->addWidget(label_45, 6, 0, 1, 1);

        label_posa = new QLabel(bedPositionLegacyContainer);
        label_posa->setObjectName("label_posa");
        label_posa->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posa, 1, 1, 1, 1);

        label_posb = new QLabel(bedPositionLegacyContainer);
        label_posb->setObjectName("label_posb");
        label_posb->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posb, 2, 1, 1, 1);

        label_posc = new QLabel(bedPositionLegacyContainer);
        label_posc->setObjectName("label_posc");
        label_posc->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posc, 3, 1, 1, 1);

        label_posx = new QLabel(bedPositionLegacyContainer);
        label_posx->setObjectName("label_posx");
        label_posx->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posx, 4, 1, 1, 1);

        label_posy = new QLabel(bedPositionLegacyContainer);
        label_posy->setObjectName("label_posy");
        label_posy->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posy, 5, 1, 1, 1);

        label_posz = new QLabel(bedPositionLegacyContainer);
        label_posz->setObjectName("label_posz");
        label_posz->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_posz, 6, 1, 1, 1);

        lineEdit_a = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_a->setObjectName("lineEdit_a");

        gridLayout_2->addWidget(lineEdit_a, 1, 2, 1, 1);

        lineEdit_b = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_b->setObjectName("lineEdit_b");

        gridLayout_2->addWidget(lineEdit_b, 2, 2, 1, 1);

        lineEdit_c = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_c->setObjectName("lineEdit_c");

        gridLayout_2->addWidget(lineEdit_c, 3, 2, 1, 1);

        lineEdit_x = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_x->setObjectName("lineEdit_x");

        gridLayout_2->addWidget(lineEdit_x, 4, 2, 1, 1);

        lineEdit_y = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_y->setObjectName("lineEdit_y");

        gridLayout_2->addWidget(lineEdit_y, 5, 2, 1, 1);

        lineEdit_z = new QLineEdit(bedPositionLegacyContainer);
        lineEdit_z->setObjectName("lineEdit_z");

        gridLayout_2->addWidget(lineEdit_z, 6, 2, 1, 1);


        verticalLayoutBedPositionLegacy->addLayout(gridLayout_2);


        verticalLayout_bedBox->addWidget(bedPositionLegacyContainer);


        mainGrid->addWidget(groupBoxBedPosition, 2, 3, 1, 2);

        verticalLayout_preview = new QVBoxLayout();
        verticalLayout_preview->setObjectName("verticalLayout_preview");
        verticalLayout_preview->setContentsMargins(0, 0, 0, 0);
        groupBoxBedPreview = new QGroupBox(centralwidget);
        groupBoxBedPreview->setObjectName("groupBoxBedPreview");
        verticalLayout_previewPanel = new QVBoxLayout(groupBoxBedPreview);
        verticalLayout_previewPanel->setObjectName("verticalLayout_previewPanel");
        verticalLayout_previewPanel->setContentsMargins(10, 10, 10, 10);
        label_preview_title = new QLabel(groupBoxBedPreview);
        label_preview_title->setObjectName("label_preview_title");

        verticalLayout_previewPanel->addWidget(label_preview_title);

        horizontalLayout_bed = new QHBoxLayout();
        horizontalLayout_bed->setObjectName("horizontalLayout_bed");
        horizontalLayout_bed->setContentsMargins(0, 0, 0, 0);

        verticalLayout_previewPanel->addLayout(horizontalLayout_bed);


        verticalLayout_preview->addWidget(groupBoxBedPreview);


        mainGrid->addLayout(verticalLayout_preview, 2, 1, 1, 2);


        verticalLayout_root->addLayout(mainGrid);

        verticalLayout_compat = new QVBoxLayout();
        verticalLayout_compat->setObjectName("verticalLayout_compat");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        tabWidget->setVisible(false);
        tab = new QWidget();
        tab->setObjectName("tab");
        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        gridLayout_6 = new QGridLayout(tab_2);
        gridLayout_6->setObjectName("gridLayout_6");
        label_20 = new QLabel(tab_2);
        label_20->setObjectName("label_20");

        gridLayout_6->addWidget(label_20, 0, 0, 1, 1);

        label_21 = new QLabel(tab_2);
        label_21->setObjectName("label_21");

        gridLayout_6->addWidget(label_21, 0, 1, 1, 1);

        label_22 = new QLabel(tab_2);
        label_22->setObjectName("label_22");

        gridLayout_6->addWidget(label_22, 0, 2, 1, 1);

        tableWidget = new QTableWidget(tab_2);
        tableWidget->setObjectName("tableWidget");

        gridLayout_6->addWidget(tableWidget, 1, 0, 3, 1);

        tableWidget_2 = new QTableWidget(tab_2);
        tableWidget_2->setObjectName("tableWidget_2");

        gridLayout_6->addWidget(tableWidget_2, 1, 1, 1, 1);

        tableWidget_3 = new QTableWidget(tab_2);
        tableWidget_3->setObjectName("tableWidget_3");

        gridLayout_6->addWidget(tableWidget_3, 1, 2, 3, 1);

        label_23 = new QLabel(tab_2);
        label_23->setObjectName("label_23");

        gridLayout_6->addWidget(label_23, 2, 1, 1, 1);

        horizontalLayout_mu = new QHBoxLayout();
        horizontalLayout_mu->setObjectName("horizontalLayout_mu");
        label_24 = new QLabel(tab_2);
        label_24->setObjectName("label_24");

        horizontalLayout_mu->addWidget(label_24);

        horizontalSpacer_mu = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_mu->addItem(horizontalSpacer_mu);

        label_25 = new QLabel(tab_2);
        label_25->setObjectName("label_25");

        horizontalLayout_mu->addWidget(label_25);

        doseDeliveryProgressCard = new QWidget(tab_2);
        doseDeliveryProgressCard->setObjectName("doseDeliveryProgressCard");
        verticalLayoutDoseDeliveryProgress = new QVBoxLayout(doseDeliveryProgressCard);
        verticalLayoutDoseDeliveryProgress->setSpacing(3);
        verticalLayoutDoseDeliveryProgress->setObjectName("verticalLayoutDoseDeliveryProgress");
        verticalLayoutDoseDeliveryProgress->setContentsMargins(0, 0, 0, 0);
        labelDoseDeliveryProgressName = new QLabel(doseDeliveryProgressCard);
        labelDoseDeliveryProgressName->setObjectName("labelDoseDeliveryProgressName");

        verticalLayoutDoseDeliveryProgress->addWidget(labelDoseDeliveryProgressName);

        horizontalLayoutDoseDeliveryProgress = new QHBoxLayout();
        horizontalLayoutDoseDeliveryProgress->setSpacing(8);
        horizontalLayoutDoseDeliveryProgress->setObjectName("horizontalLayoutDoseDeliveryProgress");
        progressBarDoseDelivery = new QProgressBar(doseDeliveryProgressCard);
        progressBarDoseDelivery->setObjectName("progressBarDoseDelivery");
        progressBarDoseDelivery->setMinimumSize(QSize(0, 30));
        progressBarDoseDelivery->setMaximumSize(QSize(16777215, 30));
        progressBarDoseDelivery->setMinimum(0);
        progressBarDoseDelivery->setMaximum(1000);
        progressBarDoseDelivery->setValue(0);

        horizontalLayoutDoseDeliveryProgress->addWidget(progressBarDoseDelivery);

        labelDoseDeliveryProgressValue = new QLabel(doseDeliveryProgressCard);
        labelDoseDeliveryProgressValue->setObjectName("labelDoseDeliveryProgressValue");
        labelDoseDeliveryProgressValue->setMinimumSize(QSize(132, 30));
        labelDoseDeliveryProgressValue->setMaximumSize(QSize(172, 30));
        labelDoseDeliveryProgressValue->setAlignment(Qt::AlignmentFlag::AlignCenter);

        horizontalLayoutDoseDeliveryProgress->addWidget(labelDoseDeliveryProgressValue);


        verticalLayoutDoseDeliveryProgress->addLayout(horizontalLayoutDoseDeliveryProgress);


        horizontalLayout_mu->addWidget(doseDeliveryProgressCard);


        gridLayout_6->addLayout(horizontalLayout_mu, 3, 1, 1, 1);

        progressBar_6 = new QProgressBar(tab_2);
        progressBar_6->setObjectName("progressBar_6");
        progressBar_6->setValue(24);

        gridLayout_6->addWidget(progressBar_6, 4, 0, 1, 3);

        tabWidget->addTab(tab_2, QString());

        verticalLayout_compat->addWidget(tabWidget);


        verticalLayout_root->addLayout(verticalLayout_compat);

        TreatmentWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(TreatmentWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1652, 31));
        TreatmentWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(TreatmentWindow);
        statusbar->setObjectName("statusbar");
        TreatmentWindow->setStatusBar(statusbar);

        retranslateUi(TreatmentWindow);

        QMetaObject::connectSlotsByName(TreatmentWindow);
    } // setupUi

    void retranslateUi(QMainWindow *TreatmentWindow)
    {
        TreatmentWindow->setWindowTitle(QCoreApplication::translate("TreatmentWindow", "TreatmentWindow \346\262\273\347\226\227\346\216\247\345\210\266\347\263\273\347\273\237", nullptr));
        groupBoxPatientInfo->setTitle(QString());
        label_icon_patient->setText(QString());
        label_title_patient->setText(QCoreApplication::translate("TreatmentWindow", "\346\202\243\350\200\205\344\277\241\346\201\257", nullptr));
        patient_name->setText(QCoreApplication::translate("TreatmentWindow", "\345\247\223\345\220\215\357\274\232-", nullptr));
        patient_id->setText(QCoreApplication::translate("TreatmentWindow", "ID\357\274\232-", nullptr));
        groupBoxPrescriptionDose->setTitle(QString());
        label_icon_prescriptionDose->setText(QCoreApplication::translate("TreatmentWindow", "Rx", nullptr));
        label_title_prescriptionDose->setText(QCoreApplication::translate("TreatmentWindow", "\345\244\204\346\226\271\345\211\202\351\207\217", nullptr));
        labelTopPrescriptionDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelTopPrescriptionDetail->setText(QCoreApplication::translate("TreatmentWindow", "\347\255\211\345\276\205\350\256\241\345\210\222", nullptr));
        groupBoxTherapistInfo->setTitle(QString());
        label_icon_doctor->setText(QString());
        label_title_doctor->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\270\210", nullptr));
        label_6->setText(QCoreApplication::translate("TreatmentWindow", "\345\247\223\345\220\215\357\274\232\345\274\240\344\270\211", nullptr));
        label_user->setText(QCoreApplication::translate("TreatmentWindow", "\350\247\222\350\211\262\357\274\232\347\211\251\347\220\206\345\270\210", nullptr));
        groupBoxRoomInfo->setTitle(QString());
        label_icon_room->setText(QString());
        label_title_room->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\244", nullptr));
        label_7->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\2441", nullptr));
        label_room->setText(QCoreApplication::translate("TreatmentWindow", "\347\212\266\346\200\201\357\274\232\345\217\257\347\224\250", nullptr));
        groupBoxCurrentTime->setTitle(QString());
        label_icon_time->setText(QString());
        label_title_time->setText(QCoreApplication::translate("TreatmentWindow", "\345\275\223\345\211\215\346\227\266\351\227\264", nullptr));
        label_time->setText(QCoreApplication::translate("TreatmentWindow", "2026-04-22 17:01:49", nullptr));
        label_time_week->setText(QCoreApplication::translate("TreatmentWindow", "\346\230\237\346\234\237\344\272\214", nullptr));
        groupBoxTopActions->setTitle(QString());
        todays_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\344\273\212\346\227\245\346\216\222\347\250\213", nullptr));
        theme_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\344\270\273\351\242\230\345\210\207\346\215\242", nullptr));
        logout_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\346\263\250\351\224\200", nullptr));
        groupBoxTreatmentPlan->setTitle(QString());
        label_icon_plan->setText(QString());
        label_title_plan->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\350\256\241\345\210\222", nullptr));
        labelPlanIdTitle->setText(QCoreApplication::translate("TreatmentWindow", "\350\256\241\345\210\222ID", nullptr));
        labelPlanIdValue->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelPlanFractionIdTitle->setText(QCoreApplication::translate("TreatmentWindow", "\345\210\206\346\254\241ID", nullptr));
        labelPlanFractionIdValue->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelPlanSiteTargetTitle->setText(QCoreApplication::translate("TreatmentWindow", "\351\235\266\345\214\272", nullptr));
        labelPlanSiteTargetValue->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelPlanIsocenterTitle->setText(QCoreApplication::translate("TreatmentWindow", "\347\255\211\344\270\255\345\277\203\347\202\271\345\235\220\346\240\207", nullptr));
        labelPlanIsocenterValue->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelPlanQaReleaseTitle->setText(QCoreApplication::translate("TreatmentWindow", "QA / \346\224\276\350\241\214", nullptr));
        labelPlanQaReleaseValue->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        confirmPlan_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\347\241\256\350\256\244", nullptr));
        confirmPlan_cancel_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\345\217\226\346\266\210", nullptr));
        planDetail_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\350\257\246\346\203\205", nullptr));
        groupBoxTreatmentFlow->setTitle(QString());
        label_icon_flow->setText(QString());
        label_title_flow->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\346\265\201\347\250\213", nullptr));
        labelCurrentTreatmentAction->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\347\212\266\346\200\201\357\274\232\351\242\204\350\247\210\343\200\200|\343\200\200\345\275\223\345\211\215\345\212\250\344\275\234\357\274\232\347\255\211\345\276\205\350\256\276\345\244\207\347\212\266\346\200\201", nullptr));
        groupBoxOperationQueue->setTitle(QString());
        label_icon_ops->setText(QString());
        label_title_ops->setText(QCoreApplication::translate("TreatmentWindow", "\346\235\237\346\265\201\350\260\203\345\272\246", nullptr));
        label_10->setText(QCoreApplication::translate("TreatmentWindow", "\346\235\237\346\265\201\345\210\206\351\205\215", nullptr));
        label_11->setText(QCoreApplication::translate("TreatmentWindow", "<b>\345\275\223\345\211\215\346\235\237\346\265\201</b><br><span style=\"font-size:12px;\">\346\234\252\345\210\206\351\205\215 \302\267 \347\255\211\345\276\205\346\262\273\347\226\227\345\256\244\347\224\263\350\257\267</span>", nullptr));
        beamRoomStatusTitle->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\244\346\235\237\346\265\201\347\212\266\346\200\201", nullptr));
        beamRoomLabel1->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\2441<br>\347\251\272\351\227\262", nullptr));
        beamRoomLabel2->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\2442<br>\347\251\272\351\227\262", nullptr));
        beamRoomLabel9->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\2449<br>\347\251\272\351\227\262", nullptr));
        labelBeamWaitTitle->setText(QCoreApplication::translate("TreatmentWindow", "\351\242\204\350\256\241\347\255\211\345\276\205\346\227\266\351\227\264", nullptr));
        label_30->setText(QCoreApplication::translate("TreatmentWindow", "04:30", nullptr));
        beamApply_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\347\224\263\350\257\267\346\235\237\346\265\201", nullptr));
        beamApply_cancel_pushButton->setText(QCoreApplication::translate("TreatmentWindow", "\345\217\226\346\266\210", nullptr));
        groupBoxSystemStatus->setTitle(QString());
        label_icon_status->setText(QString());
        label_title_status->setText(QCoreApplication::translate("TreatmentWindow", "\347\263\273\347\273\237\350\201\224\351\224\201\347\212\266\346\200\201", nullptr));
        label_34->setText(QCoreApplication::translate("TreatmentWindow", "\345\205\250\345\261\200\345\256\211\345\205\250", nullptr));
        label_9->setText(QString());
        label_36->setText(QCoreApplication::translate("TreatmentWindow", "\351\200\232\344\277\241\351\223\276\350\267\257", nullptr));
        label_35->setText(QString());
        label_37->setText(QCoreApplication::translate("TreatmentWindow", "\346\235\237\346\265\201\350\201\224\351\224\201", nullptr));
        label_46->setText(QString());
        label_48->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\272\212\350\201\224\351\224\201", nullptr));
        label_47->setText(QString());
        labelDeviceStatusCaption->setText(QCoreApplication::translate("TreatmentWindow", "\350\256\276\345\244\207\347\212\266\346\200\201", nullptr));
        labelCouchDeviceStatus->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\272\212\357\274\232\347\255\211\345\276\205\346\225\260\346\215\256", nullptr));
        labelDrDeviceStatus->setText(QCoreApplication::translate("TreatmentWindow", "DR\357\274\232\347\255\211\345\276\205\346\225\260\346\215\256", nullptr));
        labelIgrtDeviceStatus->setText(QCoreApplication::translate("TreatmentWindow", "IGRT\357\274\232\347\255\211\345\276\205\346\225\260\346\215\256", nullptr));
        labelFilterDeviceStatus->setText(QCoreApplication::translate("TreatmentWindow", "\350\204\212\345\275\242\350\277\207\346\273\244\345\231\250\357\274\232\347\255\211\345\276\205\346\225\260\346\215\256", nullptr));
        labelPlcDeviceStatus->setText(QCoreApplication::translate("TreatmentWindow", "PLC\357\274\232\347\255\211\345\276\205\346\225\260\346\215\256", nullptr));
        groupBoxBedPosition->setTitle(QString());
        labelDosePanelTitle->setText(QCoreApplication::translate("TreatmentWindow", "\346\235\237\346\265\201 / \345\260\204\351\207\216\344\277\241\346\201\257", nullptr));
        labelDosePrescriptionName->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\256\244 / \350\256\276\345\244\207", nullptr));
        labelPrescriptionDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelDoseFractionName->setText(QCoreApplication::translate("TreatmentWindow", "\346\211\253\346\217\217\346\250\241\345\274\217 / \347\262\222\345\255\220", nullptr));
        labelFractionDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelDoseCurrentFractionName->setText(QCoreApplication::translate("TreatmentWindow", "Beam \346\225\260\351\207\217", nullptr));
        labelCurrentFractionDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelDoseCompletedName->setText(QCoreApplication::translate("TreatmentWindow", "\350\203\275\345\261\202 / Spot", nullptr));
        labelCompletedDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelDoseBeamMuName->setText(QCoreApplication::translate("TreatmentWindow", "\346\235\237\346\265\201\350\256\241\351\207\217\345\233\236\350\257\273", nullptr));
        progressBarBeamMeterset->setFormat(QCoreApplication::translate("TreatmentWindow", "\346\232\202\346\227\240\350\256\241\345\210\222\346\235\237\346\265\201\350\256\241\351\207\217", nullptr));
        labelBeamMuDose->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelDoseDeviationName->setText(QCoreApplication::translate("TreatmentWindow", "\345\201\217\345\267\256 / \344\270\255\346\226\255\350\241\245\345\201\277", nullptr));
        labelDoseDeviation->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        labelLayerProgressName->setText(QCoreApplication::translate("TreatmentWindow", "\345\261\202\350\277\233\345\272\246", nullptr));
        labelLayerProgressValue->setText(QCoreApplication::translate("TreatmentWindow", "- / -", nullptr));
        labelBeamDoseInfoName->setText(QCoreApplication::translate("TreatmentWindow", "Beam \346\230\216\347\273\206", nullptr));
        labelBeamDoseInfo->setText(QCoreApplication::translate("TreatmentWindow", "-", nullptr));
        label_icon_bed->setText(QString());
        label_title_bed->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\272\212\344\275\215\347\275\256\350\256\276\347\275\256", nullptr));
        label_38->setText(QCoreApplication::translate("TreatmentWindow", "\345\256\236\346\227\266\344\275\215\347\275\256\357\274\210mm\357\274\211", nullptr));
        label_39->setText(QCoreApplication::translate("TreatmentWindow", "\350\256\276\347\275\256\344\275\215\347\275\256\357\274\210mm\357\274\211", nullptr));
        label_40->setText(QCoreApplication::translate("TreatmentWindow", "A", nullptr));
        label_41->setText(QCoreApplication::translate("TreatmentWindow", "B", nullptr));
        label_42->setText(QCoreApplication::translate("TreatmentWindow", "C", nullptr));
        label_43->setText(QCoreApplication::translate("TreatmentWindow", "X", nullptr));
        label_44->setText(QCoreApplication::translate("TreatmentWindow", "Y", nullptr));
        label_45->setText(QCoreApplication::translate("TreatmentWindow", "Z", nullptr));
        label_posa->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        label_posb->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        label_posc->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        label_posx->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        label_posy->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        label_posz->setText(QCoreApplication::translate("TreatmentWindow", "0.0", nullptr));
        groupBoxBedPreview->setTitle(QString());
        label_preview_title->setText(QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227\345\272\212 / \346\234\272\346\236\266 3D \351\242\204\350\247\210", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("TreatmentWindow", "\350\256\276\347\275\256", nullptr));
        label_20->setText(QCoreApplication::translate("TreatmentWindow", "\345\260\204\351\207\216 MU", nullptr));
        label_21->setText(QCoreApplication::translate("TreatmentWindow", "\345\234\272\345\256\236\347\216\260", nullptr));
        label_22->setText(QCoreApplication::translate("TreatmentWindow", "\345\234\272\345\256\236\347\216\260", nullptr));
        label_23->setText(QCoreApplication::translate("TreatmentWindow", "MU\351\200\237\347\216\207", nullptr));
        label_24->setText(QCoreApplication::translate("TreatmentWindow", "MU/min", nullptr));
        label_25->setText(QCoreApplication::translate("TreatmentWindow", "MU/min", nullptr));
        labelDoseDeliveryProgressName->setText(QCoreApplication::translate("TreatmentWindow", "\345\211\202\351\207\217\350\277\233\345\272\246", nullptr));
        progressBarDoseDelivery->setFormat(QCoreApplication::translate("TreatmentWindow", "\346\232\202\346\227\240\350\256\241\345\210\222\345\211\202\351\207\217", nullptr));
        labelDoseDeliveryProgressValue->setText(QCoreApplication::translate("TreatmentWindow", "- / - Gy", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("TreatmentWindow", "\346\262\273\347\226\227", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TreatmentWindow: public Ui_TreatmentWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TREATMENTWINDOW_H
