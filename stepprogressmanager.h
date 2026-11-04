#pragma once

#include <QLabel>
#include <QList>
#include <QPixmap>
#include <QSize>
#include <QString>

class QPainter;

class StepProgressManager
{
public:
    StepProgressManager();

    void addStep(QLabel *label);
    void setCurrentStep(int step);

private:
    void updateUI();
    void updateLabel(QLabel *label, int index);

    QPixmap buildStepPixmap(int index, int status, const QSize &size);
    void drawConnector(QPainter &p, int index, int status, int count, int y, int cx, int iconRadius);
    void drawNumberBadge(QPainter &p, int cx, int cy, int index, int status);
    void drawIconBadge(QPainter &p, int cx, int cy, int radius, int status);
    void drawStepIconSvg(QPainter &p, int index, const QRect &rect);
    QString getStepIconPath(int index);
    QString stepText(int index) const;

private:
    QList<QLabel *> m_labels;
    int m_currentStep = 0;
};
