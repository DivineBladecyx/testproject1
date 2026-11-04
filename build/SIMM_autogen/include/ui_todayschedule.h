/********************************************************************************
** Form generated from reading UI file 'todayschedule.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TODAYSCHEDULE_H
#define UI_TODAYSCHEDULE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TodaySchedule
{
public:
    QTableWidget *tableWidget_t;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_p;
    QLabel *label_room;

    void setupUi(QWidget *TodaySchedule)
    {
        if (TodaySchedule->objectName().isEmpty())
            TodaySchedule->setObjectName("TodaySchedule");
        TodaySchedule->setWindowModality(Qt::WindowModality::NonModal);
        TodaySchedule->resize(653, 661);
        TodaySchedule->setStyleSheet(QString::fromUtf8(""));
        tableWidget_t = new QTableWidget(TodaySchedule);
        if (tableWidget_t->rowCount() < 2)
            tableWidget_t->setRowCount(2);
        tableWidget_t->setObjectName("tableWidget_t");
        tableWidget_t->setGeometry(QRect(30, 110, 591, 451));
        label = new QLabel(TodaySchedule);
        label->setObjectName("label");
        label->setGeometry(QRect(220, 0, 181, 41));
        QFont font;
        font.setPointSize(20);
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8(""));
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_2 = new QLabel(TodaySchedule);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(60, 50, 101, 41));
        QFont font1;
        font1.setPointSize(16);
        label_2->setFont(font1);
        label_2->setStyleSheet(QString::fromUtf8(""));
        label_3 = new QLabel(TodaySchedule);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(240, 50, 81, 41));
        label_3->setFont(font1);
        label_3->setStyleSheet(QString::fromUtf8(""));
        label_p = new QLabel(TodaySchedule);
        label_p->setObjectName("label_p");
        label_p->setGeometry(QRect(330, 50, 171, 41));
        label_p->setFont(font1);
        label_p->setStyleSheet(QString::fromUtf8(""));
        label_room = new QLabel(TodaySchedule);
        label_room->setObjectName("label_room");
        label_room->setGeometry(QRect(160, 50, 71, 41));
        label_room->setFont(font1);
        label_room->setStyleSheet(QString::fromUtf8(""));

        retranslateUi(TodaySchedule);

        QMetaObject::connectSlotsByName(TodaySchedule);
    } // setupUi

    void retranslateUi(QWidget *TodaySchedule)
    {
        TodaySchedule->setWindowTitle(QCoreApplication::translate("TodaySchedule", "Form", nullptr));
        label->setText(QCoreApplication::translate("TodaySchedule", "\344\273\212\346\227\245\346\216\222\347\250\213", nullptr));
        label_2->setText(QCoreApplication::translate("TodaySchedule", "\345\275\223\345\211\215\350\257\212\345\256\244\357\274\232", nullptr));
        label_3->setText(QCoreApplication::translate("TodaySchedule", "\347\211\251\347\220\206\345\270\210\357\274\232", nullptr));
        label_p->setText(QString());
        label_room->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class TodaySchedule: public Ui_TodaySchedule {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TODAYSCHEDULE_H
