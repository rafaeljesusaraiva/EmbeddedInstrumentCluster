#include "ClusterWindow.h"
#include "GaugeWidget.h"
#include "CenterPanel.h"
#include "PillBadge.h"
#include "AppFont.h"

#include <QPainter>
#include <QDateTime>
#include <QProcess>
#include <QtMath>
#include <QGuiApplication>
#include <QScreen>

#ifndef DESKTOP_MOCK
#include <QCanBus>
#include <QCanBusFrame>
#endif

ClusterWindow::ClusterWindow(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setStyleSheet("background-color: #08050 2;");

#ifdef DESKTOP_MOCK
    m_winW = 1920;
    m_winH = 720;
    setFixedSize(m_winW, m_winH);
#else
    // On Pi: use actual screen resolution — no hardcoding
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->geometry();
        m_winW = geo.width();
        m_winH = geo.height();
    } else {
        m_winW = 1920;
        m_winH = 720;
    }
    // Fullscreen — size is set by showFullScreen(), not setFixedSize
#endif

    // ── Layout — all values derived from m_winW / m_winH ─────────────────
    const int gaugeSize = m_winH - 60;
    const int gaugeTop  = 10;
    const int pillW     = qRound(gaugeSize * 0.56);
    const int pillH     = qRound(m_winH * 0.042);
    const int pillY     = gaugeTop + gaugeSize + 8;
    const int gaugeL    = qRound(m_winW * 0.010);
    const int gaugeR    = m_winW - gaugeL - gaugeSize;
    const int centerX   = gaugeL + gaugeSize + qRound(m_winW * 0.010);
    const int centerW   = gaugeR - centerX - qRound(m_winW * 0.010);

    // ── RPM gauge ─────────────────────────────────────────────────────────
    GaugeWidget::Config rpmCfg;
    rpmCfg.unit              = "RPM";
    rpmCfg.minVal            = 0;
    rpmCfg.maxVal            = 7000;
    rpmCfg.tickStep          = 1000;
    rpmCfg.arcStartPct       = 0.62;
    rpmCfg.arcSpanPct        = 0.76;
    rpmCfg.arcActiveColor    = QColor(180, 95, 15);

    m_rpmGauge = new GaugeWidget(rpmCfg, this);
    m_rpmGauge->setGeometry(gaugeL, gaugeTop, gaugeSize, gaugeSize);

    m_rpmPill = new PillBadge(this);
    m_rpmPill->setGeometry(gaugeL + (gaugeSize - pillW) / 2,
                            pillY, pillW, pillH);
    m_rpmPill->leftLabel  = "FUEL";
    m_rpmPill->midValue   = "--%";
    m_rpmPill->rightValue = "---KM";

    // ── Speed gauge ───────────────────────────────────────────────────────
    GaugeWidget::Config spdCfg;
    spdCfg.unit              = "km/h";
    spdCfg.minVal            = 0;
    spdCfg.maxVal            = 200;
    spdCfg.tickStep          = 10;
    spdCfg.arcStartPct       = 0.62;
    spdCfg.arcSpanPct        = 0.76;
    spdCfg.arcActiveColor    = QColor(180, 95, 15);

    m_spdGauge = new GaugeWidget(spdCfg, this);
    m_spdGauge->setGeometry(gaugeR, gaugeTop, gaugeSize, gaugeSize);

    m_spdPill = new PillBadge(this);
    m_spdPill->setGeometry(gaugeR + (gaugeSize - pillW) / 2,
                            pillY, pillW, pillH);
    m_spdPill->leftLabel  = "BATTERY";
    m_spdPill->midValue   = "--V";
    m_spdPill->rightValue = "0V";

    // ── Center panel ──────────────────────────────────────────────────────
    m_center = new CenterPanel(this);
    m_center->setGeometry(centerX, 0, centerW, m_winH);

    // ── Timers ────────────────────────────────────────────────────────────
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout,
            this, &ClusterWindow::updateClock);
    m_clockTimer->start(1000);
    updateClock();

    m_detectTimer = new QTimer(this);
    connect(m_detectTimer, &QTimer::timeout,
            this, &ClusterWindow::checkCan);
    m_detectTimer->start(1500);

#ifdef DESKTOP_MOCK
    QTimer *mockTimer = new QTimer(this);
    connect(mockTimer, &QTimer::timeout, this, [this]() {
        static double t = 0;
        t += 0.04;
        m_rpm   = 1000 + 2500 * (0.5 + 0.5 * qSin(t));
        m_speed = 60   + 60   * (0.5 + 0.5 * qSin(t * 0.7));
        m_fuel  = 0.3  + 0.3  * (0.5 + 0.5 * qSin(t * 0.13));
        m_rpmGauge->setValue(m_rpm);
        m_spdGauge->setValue(m_speed);
        m_center->steerAngle = 80.0 * qSin(t * 0.3);
        m_center->coolant    = 80.0 + 20.0 * qSin(t * 0.1);
        m_center->odometer   = 138021;
        m_center->canOnline  = true;
        m_center->canStatus  = "MOCK MODE";
        m_center->warnOil    = qSin(t * 0.2) > 0.5;
        m_center->lightLow   = qSin(t * 0.15) > 0;
        m_center->lightHigh  = qSin(t * 0.1) > 0.7;
        m_center->m_fuel     = m_fuel;
        m_center->update();
        updateFuelPill();
    });
    mockTimer->start(50);
#endif
#ifndef DESKTOP_MOCK
    // Initialize all displays to zero — wait for CAN to update
    m_rpmGauge->setValue(0);
    m_spdGauge->setValue(0);

    m_center->coolant    = 0;
    m_center->m_fuel     = 0;
    m_center->steerAngle = 0;
    m_center->odometer   = 0;
    m_center->canOnline  = false;
    m_center->canStatus  = "WAITING FOR CAN...";
    m_center->warnOil    = false;
    m_center->warnBat    = false;
    m_center->warnABS    = false;
    m_center->warnEng    = false;
    m_center->warnFuel   = false;
    m_center->lightLow   = false;
    m_center->lightHigh  = false;
    m_center->lightFog   = false;
    m_center->tempStr    = "-- °C";

    m_rpmPill->midValue   = "0%";
    m_rpmPill->rightValue = "---KM";
    m_spdPill->midValue   = "--V";
    m_spdPill->rightValue = "0V";
#endif
}

void ClusterWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(12, 8, 4));
}

void ClusterWindow::updateClock() {
    m_center->timeStr = QDateTime::currentDateTime().toString("HH:mm");
    m_center->update();
}

void ClusterWindow::updateFuelPill() {
    int pct = qRound(m_fuel * 100);
    m_rpmPill->midValue = QString("%1%").arg(pct);
    m_rpmPill->update();
}

void ClusterWindow::updateGear() {
    int gear = 0;
    if (m_speed < 3) {
        gear = 0;
    } else if (m_rpm > 100) {
        double ratio = m_speed / (m_rpm / 1000.0);
        if      (ratio < 6)  gear = 1;
        else if (ratio < 11) gear = 2;
        else if (ratio < 16) gear = 3;
        else if (ratio < 22) gear = 4;
        else if (ratio < 30) gear = 5;
        else                 gear = 6;
    }
    // If CenterPanel has a gear field, set it here
    // m_center->gear = gear;
    // m_center->update();
    Q_UNUSED(gear)
}

void ClusterWindow::checkCan() {
#ifdef DESKTOP_MOCK
    m_detectTimer->stop();
    return;
#else
    if (m_canFound) return;

    // Show user what stage we're at
    static int attempt = 0;
    attempt++;
    m_center->canStatus = QString("CAN INIT... (%1)").arg(attempt);
    m_center->update();

    // Bring up the CAN interface
    QProcess::execute("ip", {"link", "set", "can0",
                             "type", "can", "bitrate", "500000"});
    QProcess::execute("ip", {"link", "set", "can0", "up"});

    QString errStr;
    m_canDev = QCanBus::instance()->createDevice(
                   "socketcan", "can0", &errStr);

    if (!m_canDev) {
        m_center->canStatus = "NO CAN DEVICE";
        m_center->update();
        return;
    }

    connect(m_canDev, &QCanBusDevice::framesReceived,
            this, &ClusterWindow::onCanFrames);

    if (!m_canDev->connectDevice()) {
        m_center->canStatus = "CAN CONNECT FAILED";
        m_center->update();
        delete m_canDev;
        m_canDev = nullptr;
        return;
    }

    m_canFound          = true;
    m_center->canOnline = true;
    m_center->canStatus = "CAN ONLINE";
    m_center->update();
    m_detectTimer->stop();
#endif
}

#ifndef DESKTOP_MOCK
void ClusterWindow::onCanFrames() {
    if (!m_canDev) return;

    while (m_canDev->framesAvailable()) {
        QCanBusFrame frame = m_canDev->readFrame();
        if (frame.frameType() != QCanBusFrame::DataFrame) continue;

        quint32    id   = frame.frameId();
        QByteArray data = frame.payload();

        if (id == 0x7E8 && data.size() >= 3) {
            quint8 pid = static_cast<quint8>(data[1]);
            switch (pid) {

            case 0x0D:  // Speed km/h
                m_speed = static_cast<quint8>(data[2]);
                m_spdGauge->setValue(m_speed);
                updateGear();
                break;

            case 0x0C:  // RPM
                m_rpm = ((static_cast<quint8>(data[2]) * 256)
                          + static_cast<quint8>(data[3])) / 4.0;
                m_rpmGauge->setValue(m_rpm);
                break;

            case 0x05:  // Coolant °C
                m_coolant         = static_cast<quint8>(data[2]) - 40.0;
                m_center->coolant = m_coolant;
                m_center->tempStr = QString("%1 °C").arg(qRound(m_coolant));
                m_center->update();
                break;

            case 0x2F:  // Fuel level 0-100%
                m_fuel           = static_cast<quint8>(data[2]) / 255.0;
                m_center->m_fuel = m_fuel;
                updateFuelPill();
                m_center->update();
                break;

            case 0x0E:  // Throttle — used as steering proxy
                m_center->steerAngle =
                    (static_cast<quint8>(data[2]) - 128) * 1.25;
                m_center->update();
                break;

            case 0x5C:  // Oil pressure warning
               m_center->warnOil = (static_cast<quint8>(data[2]) != 0);
                m_center->update();
                break;

            case 0x61:  // Battery voltage (0x61 = generic voltage PID)
                {
                    double volts = static_cast<quint8>(data[2]) * 0.1;
                    m_spdPill->midValue   = QString("%1V").arg(volts, 0, 'f', 1);
                    m_spdPill->update();
                }
                break;
            }
        }
    }
}
#endif