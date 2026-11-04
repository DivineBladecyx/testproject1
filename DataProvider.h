#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

#endif // DATAPROVIDER_H
#pragma once
#include <QObject>

class DataProvider : public QObject
{
    Q_OBJECT
public:
    static DataProvider& instance() {
        static DataProvider p;
        return p;
    }

    void updateValue(QString key, double value) {
        emit valueChanged(key, value);
    }

signals:
    void valueChanged(QString key, double value);
};
