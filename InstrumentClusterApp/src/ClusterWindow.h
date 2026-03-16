#pragma once
#include <QWidget>
#include <QTimer>

class GaugeWidget;
class CenterPanel;
class PillBadge;

#ifndef DESKTOP_MOCK
#include <QCanBusDevice>
#endif

class ClusterWindow : public QWidget {
    Q_OBJECT
public:
    explicit ClusterWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void updateClock();
    void checkCan();

#ifndef DESKTOP_MOCK
    void onCanFrames();
#endif

private:
    void updateFuelPill();
    void updateGear();

    GaugeWidget  *m_rpmGauge;
    GaugeWidget  *m_spdGauge;
    PillBadge    *m_rpmPill;
    PillBadge    *m_spdPill;
    CenterPanel  *m_center;

    QTimer *m_detectTimer;
    QTimer *m_clockTimer;

    double  m_speed   = 0;
    double  m_rpm     = 0;
    double  m_coolant = 0;
    double  m_fuel    = 0;
    bool    m_canFound = false;

    int m_winW = 1920;
    int m_winH = 720;

#ifndef DESKTOP_MOCK
    QCanBusDevice *m_canDev = nullptr;
#endif
};