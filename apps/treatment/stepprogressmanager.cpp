#include "stepprogressmanager.h"

#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QtSvg/QSvgRenderer>

namespace {
constexpr int kPending = 0;
constexpr int kActive = 1;
constexpr int kDone = 2;

const QColor kBlue("#2F7DEB");
const QColor kGray("#A9ADB6");
const QColor kText("#5B6675");
const QColor kWhite("#FFFFFF");
}

StepProgressManager::StepProgressManager() {}

void StepProgressManager::addStep(QLabel *label)
{
    if (!label) {
        return;
    }

    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(150, 105);
    label->setScaledContents(false);
    m_labels.append(label);
}

void StepProgressManager::setCurrentStep(int step)
{
    m_currentStep = step;
    updateUI();
}

void StepProgressManager::updateUI()
{
    for (int i = 0; i < m_labels.size(); ++i) {
        updateLabel(m_labels[i], i);
    }
}

void StepProgressManager::updateLabel(QLabel *label, int index)
{
    if (!label) {
        return;
    }

    int status = kPending;
    if (index < m_currentStep) {
        status = kDone;
    } else if (index == m_currentStep) {
        status = kActive;
    }

    QPixmap pix = buildStepPixmap(index, status, label->size());
    label->setPixmap(pix);
}

QPixmap StepProgressManager::buildStepPixmap(int index, int status, const QSize &size)
{
    QPixmap pix(size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int w = size.width();
    const int h = size.height();
    Q_UNUSED(h);

    const int numberRadius = 14;
    const int iconRadius = 24;
    const int cx = w / 2;

    const int numberCy = 8;   // 数字圆点上移
    const int iconCy = 40;    // 大圆图标上移
    const int lineY = iconCy; // 连线跟着图标上移
    const int textTop = 68;

    drawConnector(p, index, status, m_labels.size(), lineY, cx, iconRadius);
    drawNumberBadge(p, cx, numberCy, index, status);
    drawIconBadge(p, cx, iconCy, iconRadius, status);

    QRect iconRect(cx - 15, iconCy - 15, 30, 30);
    drawStepIconSvg(p, index, iconRect);

    QFont font = p.font();
    font.setPointSize(11);
    font.setBold(status == kActive);
    p.setFont(font);
    p.setPen(status == kActive ? kBlue : kText);
    p.drawText(QRect(0, textTop, w, 24), Qt::AlignHCenter | Qt::AlignVCenter, stepText(index));

    return pix;
}

void StepProgressManager::drawConnector(QPainter &p, int index, int status, int count, int y, int cx, int iconRadius)
{
    if (count <= 1) {
        return;
    }

    const int leftX1 = 6;
    const int leftX2 = cx - iconRadius - 10;
    const int rightX1 = cx + iconRadius + 10;
    const int rightX2 = p.device()->width() - 6;

    auto drawLine = [&](int x1, int x2, const QColor &color, bool dashed) {
        if (x2 <= x1) {
            return;
        }
        QPen pen(color, dashed ? 3.0 : 2.5, dashed ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(x1, y, x2, y);
    };

    if (index > 0) {
        drawLine(leftX1, leftX2, kGray, false);
    }

    if (index < count - 1) {
        if (status == kActive) {
            drawLine(rightX1, rightX2, kBlue, true);
        } else {
            drawLine(rightX1, rightX2, kGray, false);
        }
    }
}

void StepProgressManager::drawNumberBadge(QPainter &p, int cx, int cy, int index, int status)
{
    QColor fill = (status == kActive) ? kBlue : kGray;

    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawEllipse(QPoint(cx, cy), 14, 14);

    QFont font = p.font();
    font.setPointSize(11);
    font.setBold(true);
    p.setFont(font);
    p.setPen(kWhite);
    p.drawText(QRect(cx - 14, cy - 14, 28, 28), Qt::AlignCenter, QString::number(index + 1));
}

void StepProgressManager::drawIconBadge(QPainter &p, int cx, int cy, int radius, int status)
{
    QColor fill = (status == kActive) ? kBlue : kGray;

    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawEllipse(QPoint(cx, cy), radius, radius);
}

void StepProgressManager::drawStepIconSvg(QPainter &p, int index, const QRect &rect)
{
    const QString path = getStepIconPath(index);
    if (path.isEmpty()) {
        return;
    }

    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        qDebug() << "SVG加载失败:" << path;
        return;
    }

    renderer.render(&p, rect);
}

QString StepProgressManager::getStepIconPath(int index)
{
    switch (index) {
    case 0: return QStringLiteral(":/icons/step_preview.svg");
    case 1: return QStringLiteral(":/icons/step_prepare.svg");
    case 2: return QStringLiteral(":/icons/step_ready.svg");
    case 3: return QStringLiteral(":/icons/step_treating.svg");
    case 4: return QStringLiteral(":/icons/step_record.svg");
    default: return QString();
    }
}

QString StepProgressManager::stepText(int index) const
{
    static const QStringList texts = {
        QStringLiteral("预览"),
        QStringLiteral("准备"),
        QStringLiteral("就绪"),
        QStringLiteral("治疗中"),
        QStringLiteral("记录")
    };

    if (index >= 0 && index < texts.size()) {
        return texts[index];
    }
    return QStringLiteral("步骤");
}
