#pragma once
#include <QObject>

class BedController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double a1 READ a1 NOTIFY anglesChanged)
    Q_PROPERTY(double a2 READ a2 NOTIFY anglesChanged)
    Q_PROPERTY(double a3 READ a3 NOTIFY anglesChanged)
    Q_PROPERTY(double a4 READ a4 NOTIFY anglesChanged)
    Q_PROPERTY(double a5 READ a5 NOTIFY anglesChanged)
    Q_PROPERTY(double a6 READ a6 NOTIFY anglesChanged)
    Q_PROPERTY(bool alarm READ alarm NOTIFY alarmChanged)

public:
explicit BedController(QObject *parent = nullptr);

double a1() const;
double a2() const;
double a3() const;
double a4() const;
double a5() const;
double a6() const;

bool alarm() const { return m_alarm; }

signals:
void anglesChanged();
void alarmChanged();

private:
float m_a1 = 0;
float m_a2  = 0;
float m_a3   = 0;
float m_a4 = 0;
float m_a5  = 0;
float m_a6   = 0;

bool m_alarm = false;
};


