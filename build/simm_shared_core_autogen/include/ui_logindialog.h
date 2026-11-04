/********************************************************************************
** Form generated from reading UI file 'logindialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QVBoxLayout *verticalLayout_main;
    QLabel *label_title;
    QLabel *label_subtitle;
    QFormLayout *formLayout;
    QLabel *label_username;
    QLineEdit *lineEdit_username;
    QLabel *label_password;
    QHBoxLayout *horizontalLayout_password;
    QLineEdit *lineEdit_password;
    QPushButton *pb_show_passwd;
    QWidget *modeSelectorWidget;
    QVBoxLayout *verticalLayout_modeSelector;
    QLabel *label_mode;
    QHBoxLayout *horizontalLayout_modes;
    QToolButton *treatmentModeButton;
    QToolButton *qaModeButton;
    QHBoxLayout *horizontalLayout_options;
    QSpacerItem *horizontalSpacer_optionsLeft;
    QCheckBox *checkBox_remember;
    QSpacerItem *horizontalSpacer_options;
    QHBoxLayout *horizontalLayout_theme;
    QSpacerItem *horizontalSpacer_themeLeft;
    QRadioButton *radioLight;
    QRadioButton *radioDark;
    QSpacerItem *horizontalSpacer_theme;
    QSpacerItem *verticalSpacer_beforeButtons;
    QHBoxLayout *horizontalLayout_buttons;
    QSpacerItem *horizontalSpacer_buttons_left;
    QPushButton *pushButton_login;
    QPushButton *pushButton_cancel;
    QSpacerItem *horizontalSpacer_buttons_right;

    void setupUi(QDialog *LoginDialog)
    {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName("LoginDialog");
        LoginDialog->resize(520, 520);
        LoginDialog->setMinimumSize(QSize(500, 500));
        verticalLayout_main = new QVBoxLayout(LoginDialog);
        verticalLayout_main->setSpacing(16);
        verticalLayout_main->setObjectName("verticalLayout_main");
        verticalLayout_main->setContentsMargins(28, 24, 28, 24);
        label_title = new QLabel(LoginDialog);
        label_title->setObjectName("label_title");
        label_title->setMinimumSize(QSize(0, 34));
        QFont font;
        font.setPointSize(18);
        font.setBold(true);
        label_title->setFont(font);
        label_title->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_main->addWidget(label_title);

        label_subtitle = new QLabel(LoginDialog);
        label_subtitle->setObjectName("label_subtitle");
        label_subtitle->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_main->addWidget(label_subtitle);

        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setLabelAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter);
        formLayout->setFormAlignment(Qt::AlignmentFlag::AlignHCenter|Qt::AlignmentFlag::AlignTop);
        formLayout->setHorizontalSpacing(14);
        formLayout->setVerticalSpacing(14);
        label_username = new QLabel(LoginDialog);
        label_username->setObjectName("label_username");
        label_username->setMinimumSize(QSize(78, 40));

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, label_username);

        lineEdit_username = new QLineEdit(LoginDialog);
        lineEdit_username->setObjectName("lineEdit_username");
        lineEdit_username->setMinimumSize(QSize(260, 40));
        lineEdit_username->setClearButtonEnabled(true);

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, lineEdit_username);

        label_password = new QLabel(LoginDialog);
        label_password->setObjectName("label_password");
        label_password->setMinimumSize(QSize(78, 40));

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, label_password);

        horizontalLayout_password = new QHBoxLayout();
        horizontalLayout_password->setSpacing(8);
        horizontalLayout_password->setObjectName("horizontalLayout_password");
        lineEdit_password = new QLineEdit(LoginDialog);
        lineEdit_password->setObjectName("lineEdit_password");
        lineEdit_password->setMinimumSize(QSize(216, 40));
        lineEdit_password->setEchoMode(QLineEdit::EchoMode::Password);
        lineEdit_password->setClearButtonEnabled(true);

        horizontalLayout_password->addWidget(lineEdit_password);

        pb_show_passwd = new QPushButton(LoginDialog);
        pb_show_passwd->setObjectName("pb_show_passwd");
        pb_show_passwd->setMinimumSize(QSize(40, 40));
        pb_show_passwd->setMaximumSize(QSize(40, 40));
        pb_show_passwd->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/icons/eye-slash.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        pb_show_passwd->setIcon(icon);
        pb_show_passwd->setFlat(true);

        horizontalLayout_password->addWidget(pb_show_passwd);


        formLayout->setLayout(1, QFormLayout::ItemRole::FieldRole, horizontalLayout_password);


        verticalLayout_main->addLayout(formLayout);

        modeSelectorWidget = new QWidget(LoginDialog);
        modeSelectorWidget->setObjectName("modeSelectorWidget");
        verticalLayout_modeSelector = new QVBoxLayout(modeSelectorWidget);
        verticalLayout_modeSelector->setSpacing(8);
        verticalLayout_modeSelector->setObjectName("verticalLayout_modeSelector");
        verticalLayout_modeSelector->setContentsMargins(0, 0, 0, 0);
        label_mode = new QLabel(modeSelectorWidget);
        label_mode->setObjectName("label_mode");
        label_mode->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_modeSelector->addWidget(label_mode);

        horizontalLayout_modes = new QHBoxLayout();
        horizontalLayout_modes->setSpacing(10);
        horizontalLayout_modes->setObjectName("horizontalLayout_modes");
        treatmentModeButton = new QToolButton(modeSelectorWidget);
        treatmentModeButton->setObjectName("treatmentModeButton");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(treatmentModeButton->sizePolicy().hasHeightForWidth());
        treatmentModeButton->setSizePolicy(sizePolicy);
        treatmentModeButton->setMinimumSize(QSize(130, 88));
        treatmentModeButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        treatmentModeButton->setStyleSheet(QString::fromUtf8("QToolButton { background: #FFFFFF; border: 1px solid #D5E1EC; border-radius: 6px; padding: 8px; color: #526579; font-weight: 600; } QToolButton:hover { border-color: #66BFA8; background: #F3FBF8; } QToolButton:checked { border: 2px solid #19A982; background: #EAF8F3; color: #087A5D; }"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/icons/therapist.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        treatmentModeButton->setIcon(icon1);
        treatmentModeButton->setIconSize(QSize(34, 34));
        treatmentModeButton->setCheckable(true);
        treatmentModeButton->setChecked(true);
        treatmentModeButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);

        horizontalLayout_modes->addWidget(treatmentModeButton);

        qaModeButton = new QToolButton(modeSelectorWidget);
        qaModeButton->setObjectName("qaModeButton");
        sizePolicy.setHeightForWidth(qaModeButton->sizePolicy().hasHeightForWidth());
        qaModeButton->setSizePolicy(sizePolicy);
        qaModeButton->setMinimumSize(QSize(130, 88));
        qaModeButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        qaModeButton->setStyleSheet(QString::fromUtf8("QToolButton { background: #FFFFFF; border: 1px solid #D5E1EC; border-radius: 6px; padding: 8px; color: #526579; font-weight: 600; } QToolButton:hover { border-color: #66BFA8; background: #F3FBF8; } QToolButton:checked { border: 2px solid #19A982; background: #EAF8F3; color: #087A5D; }"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/icons/calculator-solid.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        qaModeButton->setIcon(icon2);
        qaModeButton->setIconSize(QSize(34, 34));
        qaModeButton->setCheckable(true);
        qaModeButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);

        horizontalLayout_modes->addWidget(qaModeButton);


        verticalLayout_modeSelector->addLayout(horizontalLayout_modes);


        verticalLayout_main->addWidget(modeSelectorWidget);

        horizontalLayout_options = new QHBoxLayout();
        horizontalLayout_options->setSpacing(14);
        horizontalLayout_options->setObjectName("horizontalLayout_options");
        horizontalSpacer_optionsLeft = new QSpacerItem(92, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_options->addItem(horizontalSpacer_optionsLeft);

        checkBox_remember = new QCheckBox(LoginDialog);
        checkBox_remember->setObjectName("checkBox_remember");

        horizontalLayout_options->addWidget(checkBox_remember);

        horizontalSpacer_options = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_options->addItem(horizontalSpacer_options);


        verticalLayout_main->addLayout(horizontalLayout_options);

        horizontalLayout_theme = new QHBoxLayout();
        horizontalLayout_theme->setSpacing(18);
        horizontalLayout_theme->setObjectName("horizontalLayout_theme");
        horizontalSpacer_themeLeft = new QSpacerItem(20, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_theme->addItem(horizontalSpacer_themeLeft);

        radioLight = new QRadioButton(LoginDialog);
        radioLight->setObjectName("radioLight");
        radioLight->setChecked(true);

        horizontalLayout_theme->addWidget(radioLight);

        radioDark = new QRadioButton(LoginDialog);
        radioDark->setObjectName("radioDark");

        horizontalLayout_theme->addWidget(radioDark);

        horizontalSpacer_theme = new QSpacerItem(20, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_theme->addItem(horizontalSpacer_theme);


        verticalLayout_main->addLayout(horizontalLayout_theme);

        verticalSpacer_beforeButtons = new QSpacerItem(20, 8, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_main->addItem(verticalSpacer_beforeButtons);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setSpacing(14);
        horizontalLayout_buttons->setObjectName("horizontalLayout_buttons");
        horizontalSpacer_buttons_left = new QSpacerItem(92, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_buttons_left);

        pushButton_login = new QPushButton(LoginDialog);
        pushButton_login->setObjectName("pushButton_login");
        pushButton_login->setMinimumSize(QSize(126, 42));
        pushButton_login->setMaximumSize(QSize(126, 42));

        horizontalLayout_buttons->addWidget(pushButton_login);

        pushButton_cancel = new QPushButton(LoginDialog);
        pushButton_cancel->setObjectName("pushButton_cancel");
        pushButton_cancel->setMinimumSize(QSize(126, 42));
        pushButton_cancel->setMaximumSize(QSize(126, 42));

        horizontalLayout_buttons->addWidget(pushButton_cancel);

        horizontalSpacer_buttons_right = new QSpacerItem(0, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_buttons_right);


        verticalLayout_main->addLayout(horizontalLayout_buttons);

        QWidget::setTabOrder(lineEdit_username, lineEdit_password);
        QWidget::setTabOrder(lineEdit_password, pb_show_passwd);
        QWidget::setTabOrder(pb_show_passwd, checkBox_remember);
        QWidget::setTabOrder(checkBox_remember, radioLight);
        QWidget::setTabOrder(radioLight, radioDark);
        QWidget::setTabOrder(radioDark, pushButton_login);
        QWidget::setTabOrder(pushButton_login, pushButton_cancel);

        retranslateUi(LoginDialog);

        pushButton_login->setDefault(true);


        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog)
    {
        LoginDialog->setWindowTitle(QCoreApplication::translate("LoginDialog", "\347\224\250\346\210\267\347\231\273\345\275\225", nullptr));
        label_title->setText(QCoreApplication::translate("LoginDialog", "SIMM \347\263\273\347\273\237\347\231\273\345\275\225", nullptr));
        label_subtitle->setText(QCoreApplication::translate("LoginDialog", "\350\257\267\351\200\211\346\213\251\345\267\245\344\275\234\346\250\241\345\274\217\345\271\266\350\276\223\345\205\245\350\264\246\345\217\267\344\277\241\346\201\257", nullptr));
        label_username->setText(QCoreApplication::translate("LoginDialog", "\347\224\250\346\210\267\345\220\215", nullptr));
        lineEdit_username->setPlaceholderText(QCoreApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\347\224\250\346\210\267\345\220\215", nullptr));
        label_password->setText(QCoreApplication::translate("LoginDialog", "\345\257\206\347\240\201", nullptr));
        lineEdit_password->setPlaceholderText(QCoreApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\345\257\206\347\240\201", nullptr));
#if QT_CONFIG(tooltip)
        pb_show_passwd->setToolTip(QCoreApplication::translate("LoginDialog", "\346\230\276\347\244\272\346\210\226\351\232\220\350\227\217\345\257\206\347\240\201", nullptr));
#endif // QT_CONFIG(tooltip)
        label_mode->setText(QCoreApplication::translate("LoginDialog", "\345\267\245\344\275\234\346\250\241\345\274\217", nullptr));
        treatmentModeButton->setText(QCoreApplication::translate("LoginDialog", "\346\262\273\347\226\227\346\250\241\345\274\217", nullptr));
        qaModeButton->setText(QCoreApplication::translate("LoginDialog", "QA\346\250\241\345\274\217", nullptr));
        checkBox_remember->setText(QCoreApplication::translate("LoginDialog", "\350\256\260\344\275\217\347\224\250\346\210\267\345\220\215", nullptr));
        radioLight->setText(QCoreApplication::translate("LoginDialog", "\346\265\205\350\211\262\346\250\241\345\274\217", nullptr));
        radioDark->setText(QCoreApplication::translate("LoginDialog", "\346\267\261\350\211\262\346\250\241\345\274\217", nullptr));
        pushButton_login->setText(QCoreApplication::translate("LoginDialog", "\347\231\273\345\275\225", nullptr));
        pushButton_cancel->setText(QCoreApplication::translate("LoginDialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoginDialog: public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINDIALOG_H
