#pragma once
#include <QWidget>
#include <QString>

class CenterPanel : public QWidget {
    Q_OBJECT
public:
    QString  timeStr    = "00:00";
    QString  tempStr    = "-- °C";
    double   steerAngle = 0;
    double   coolant    = 0;
    bool     canOnline  = false;
    QString  canStatus  = "SEARCHING...";
    quint32  odometer   = 0;
    bool     warnOil    = false;
    bool     warnBat    = false;
    bool     warnABS    = false;
    bool     warnEng    = false;
    bool     warnFuel   = false;
    bool     lightLow   = false;
    bool     lightHigh  = false;
    bool     lightFog   = false;
    double   m_fuel     = 0;

    explicit CenterPanel(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
};