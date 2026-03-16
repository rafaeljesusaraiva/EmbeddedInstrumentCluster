#pragma once
#include <QWidget>
#include <QColor>
#include <QString>
#include <QTimer>

class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    struct Config {
        QString unit;
        double  minVal       = 0;
        double  maxVal       = 7000;
        double  tickStep     = 1000;
        double  arcStartPct  = 0.62;
        double  arcSpanPct   = 0.76;
        QColor  arcActiveColor;
    };

    explicit GaugeWidget(Config cfg, QWidget *parent = nullptr);
    void   setValue(double v);
    double value() const { return m_value; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Config  m_cfg;
    double  m_value        = 0;
    double  m_displayValue = 0;  // animated toward m_value
    QTimer *m_animTimer    = nullptr;
};