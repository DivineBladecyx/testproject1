#pragma once

#include <QWidget>
#include <QPainter>
#include <QMap>
#include <QStringList>
#include <QColor>
#include <QPen>
#include <QFont>
#include <QSizePolicy>
#include <QSize>
#include <QtGlobal>
#include <cmath>

#include "thememanager.h"

// A read-only workflow view. Highlight only the confirmed current state.
class WorkflowOverview : public QWidget
{
public:
    explicit WorkflowOverview(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("workflowOverview");
        // Enlarge the requested space. The ring still scales down when the
        // clinical workspace is too narrow to fit its preferred dimensions.
        setMinimumSize(540, 540);
        setMaximumSize(980, 900);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // The application uses ThemeManager/QSS, not necessarily QPalette.
        // Redraw this custom-painted widget whenever the theme changes.
        QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                         this, [this](Theme) { update(); });
    }

    QSize sizeHint() const override { return QSize(800, 760); }

    void setState(const QString &state, const QString &action) {
        if (state_ == state && action_ == action) return;
        state_ = state;
        action_ = action;
        setAccessibleName(action);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Read the authoritative application theme. QPalette may remain dark
        // after a QSS-driven switch to the light theme.
        const bool dark = ThemeManager::instance().currentTheme() == Theme::Dark;
        const QColor ink = dark ? QColor("#E3EBF4") : QColor("#38516A");
        const QColor line = dark ? QColor("#536477") : QColor("#D8E3ED");
        const QColor surface = dark ? QColor("#202C38") : QColor("#FFFFFF");
        const QColor blue("#2563EB");

        const QStringList names = {
            tr("系统初始化"), tr("操作员登录"), tr("排程叫号"),
            tr("计划确认"), tr("设备确认"), tr("影像摆位"),
            tr("申请出束"), tr("束流准备"), tr("束流照射"),
            tr("出束暂停"), tr("治疗结束"), tr("报告归档")
        };

        static const QMap<QString, int> stages = {
            {"INITIALIZING", 0},
            {"WAITING_LOGIN", 1},
            {"WAITING_OIS_PATIENT_CALL", 2},
            {"WAITING_OIS_SCHEDULE_CONFIRM", 2},
            {"WAITING_FRONTEND_PLAN_CONFIRM", 3},
            {"WAITING_DEVICE_PLAN_ACK", 4},
            {"WAITING_DEVICE_AUTOPOS_DONE", 5},
            {"WAITING_FRONTEND_BEAM_APPLY", 6},
            {"WAITING_DEVICE_BEAM_READY", 7},
            {"BEAM_READY", 7},
            {"TREATMENT_IN_PROGRESS", 8},
            {"TREATMENT_PAUSED", 9},
            {"BEAM_FINISHED", 10},
            {"WAITING_CHART_GENERATION", 11},
            {"COMPLETED", 11}
        };

        const int current = stages.value(state_, -1);
        const bool fault = state_ == "ERROR_STOPPED";
        const QPointF center(width() / 2.0, height() / 2.0);
        // All geometry scales together: ring, 12 nodes, labels and center.
        // Fit the entire node rectangles inside the widget (no clipping).
        const double scale = qMin(width(), height()) / 760.0;
        const double nodeWidth = qBound(102.0, 142.0 * scale, 168.0);
        const double nodeHeight = qBound(44.0, 58.0 * scale, 70.0);
        const double edge = 14.0;
        const double radius = qMax(120.0, qMin(305.0,
            qMin((width() - nodeWidth) / 2.0 - edge,
                 (height() - nodeHeight) / 2.0 - edge)));

        p.setPen(QPen(line, qBound(6.0, 9.0 * scale, 11.0)));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, radius, radius);

        p.setPen(QPen(dark ? QColor("#6B839A") : QColor("#B9D4ED"), 1, Qt::DashLine));
        p.drawEllipse(center, radius - 13.0 * scale, radius - 13.0 * scale);

        QFont font = this->font();
        font.setPixelSize(qBound(12, qRound(16 * scale), 19));
        for (int i = 0; i < names.size(); ++i) {
            const double angle = (-90 + i * 30) * 3.14159265358979323846 / 180.0;
            const QPointF pos = center + QPointF(radius * std::cos(angle), radius * std::sin(angle));
            const QRectF rect(pos.x() - nodeWidth / 2.0, pos.y() - nodeHeight / 2.0,
                              nodeWidth, nodeHeight);
            const bool active = i == current;

            p.setPen(QPen(active ? blue : line, active ? 2 : 1));
            p.setBrush(active ? (dark ? QColor("#223D63") : QColor("#EAF2FF")) : surface);
            p.drawRoundedRect(rect, 5, 5);
            p.setPen(active ? blue : ink);
            font.setBold(active);
            p.setFont(font);
            p.drawText(rect.adjusted(3, 2, -3, -2), Qt::AlignCenter,
                       QStringLiteral("%1\n%2")
                           .arg(i + 1, 2, 10, QLatin1Char('0'))
                           .arg(names[i]));
        }

        const double inner = qMin(155.0 * scale, radius * 0.53);
        p.setBrush(surface);
        p.setPen(QPen(fault ? QColor("#D92D20") : line, 2));
        p.drawEllipse(center, inner, inner);

        font.setPixelSize(qBound(16, qRound(22 * scale), 26));
        font.setBold(true);
        p.setFont(font);
        p.setPen(fault ? QColor("#D92D20") : ink);
        const double textMargin = qMax(12.0, 18.0 * scale);
        p.drawText(QRectF(center.x() - inner + textMargin,
                          center.y() - inner + textMargin,
                          2.0 * (inner - textMargin), 2.0 * (inner - textMargin)),
                   Qt::AlignCenter | Qt::TextWordWrap, action_);
    }

private:
    QString state_;
    QString action_;
};
