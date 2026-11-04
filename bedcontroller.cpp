#include "BedController.h"
#include "DataProvider.h"

#include <algorithm>
#include <cmath>

BedController::BedController(QObject *parent)
    : QObject(parent)
{
    connect(&DataProvider::instance(),
            &DataProvider::valueChanged,
            this,
            [this](QString key, double value)
            {
                bool changed = false;

                if (key == "bed.a1") {
                    m_a1 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                else if (key == "bed.a2") {
                    m_a2 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                else if (key == "bed.a3") {
                    m_a3 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                else if (key == "bed.a4") {
                    m_a4 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                else if (key == "bed.a5") {
                    m_a5 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                else if (key == "bed.a6") {
                    m_a6 = std::clamp((float)value, -15.0f, 15.0f);
                    changed = true;
                }

                if (changed) {
                    emit anglesChanged();
                }

                // 报警逻辑
                bool newAlarm =
                    std::fabs(m_a1) > 12 ||
                    std::fabs(m_a2) > 12 ||
                    std::fabs(m_a3) > 12;

                if (newAlarm != m_alarm) {
                    m_alarm = newAlarm;
                    emit alarmChanged();
                }
            });
}
double BedController::a1() const
{
    return m_a1;
}

double BedController::a2() const
{
    return m_a2;
}

double BedController::a3() const
{
    return m_a3;
}

double BedController::a4() const
{
    return m_a4;
}

double BedController::a5() const
{
    return m_a5;
}

double BedController::a6() const
{
    return m_a6;
}
