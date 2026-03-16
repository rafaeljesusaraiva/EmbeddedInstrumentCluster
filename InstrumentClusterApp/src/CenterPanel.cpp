#include "CenterPanel.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include "AppFont.h"

static const QColor C_BG     = QColor(8,   5,   2);
static const QColor C_TRACK  = QColor(28,  16,  4);
static const QColor C_TEAL   = QColor(180, 95,  15);  // orange replaces teal
static const QColor C_WHITE  = QColor(240, 235, 225);
static const QColor C_DIM    = QColor(130, 90,  40);
static const QColor C_MUTED  = QColor(45,  28,  8);
static const QColor C_GREEN  = QColor(32,  200,  80);  // keep green for CAN online
static const QColor C_RED    = QColor(220,  50,  50);
static const QColor C_AMBER  = QColor(180, 140, 15);
static const QColor C_ORANGE = QColor(180, 85,  15);


CenterPanel::CenterPanel(QWidget *parent) : QWidget(parent) {}

void CenterPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double W  = width();
    const double H  = height();
    const double cx = W / 2.0;

    p.fillRect(rect(), C_BG);

    // ─────────────────────────────────────────────────────────────────────
    // Rows  (% of H, must sum to 100)
    //   0%  – 13%   time + temp
    //   13% – 27%   steering bar
    //   27% – 57%   TEMP + FUEL bars
    //   57% – 80%   icon row (single merged row)
    //   80% – 100%  CAN pill + odometer
    // ─────────────────────────────────────────────────────────────────────

    // ── Row 0: Time | Temp ────────────────────────────────────────────────
    {
        double rY = H * 0.01;
        double rH = H * 0.12;
        int    fs = qBound(13, qRound(rH * 0.60), 32);

        p.setPen(C_WHITE);
        p.setFont(QFont(appFont(), fs, QFont::Bold));
        p.drawText(QRectF(0, rY, W * 0.52, rH),
                   Qt::AlignLeft | Qt::AlignVCenter, timeStr);

        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), fs));
        p.drawText(QRectF(W * 0.48, rY, W * 0.52, rH),
                   Qt::AlignRight | Qt::AlignVCenter, tempStr);
    }

    // ── Row 1: Steering bar ───────────────────────────────────────────────
    {
        double rY   = H * 0.14;
        double bH   = qBound(5.0, H * 0.018, 12.0);
        double bW   = W * 0.80;
        double bX   = cx - bW / 2.0;
        int    lblFs = qBound(7, qRound(bH * 1.3), 13);

        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), lblFs));
        p.drawText(QRectF(bX - 28, rY, 26, bH + 2),
                   Qt::AlignRight | Qt::AlignVCenter, "0");
        p.drawText(QRectF(bX + bW + 2, rY, 32, bH + 2),
                   Qt::AlignLeft  | Qt::AlignVCenter, "160");

        p.setBrush(C_TRACK);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(bX, rY, bW, bH), 3, 3);

        double norm  = (steerAngle + 160.0) / 320.0;
        double fillW = qAbs(norm - 0.5) * bW;
        double fillX = norm < 0.5 ? (cx - fillW) : cx;
        p.setBrush(QColor(220, 120, 20)); 
        p.drawRoundedRect(QRectF(fillX, rY, fillW, bH), 3, 3);

        p.setBrush(C_WHITE);
        p.drawEllipse(QPointF(cx, rY + bH / 2.0), 3, 3);

        int angFs = qBound(7, qRound(bH * 1.1), 12);
        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), angFs));
        p.drawText(QRectF(cx - 26, rY + bH + 3, 52, angFs + 4),
                   Qt::AlignCenter,
                   QString("%1°").arg(qRound(steerAngle)));
    }

    // ── Row 2: TEMP + FUEL bars ───────────────────────────────────────────
    {
        double rY   = H * 0.27;
        double rH   = H * 0.28;
        double bW   = W * 0.36;
        double bH   = qBound(8.0, rH * 0.14, 16.0);
        double lX   = cx - W * 0.02 - bW;
        double rX   = cx + W * 0.02;

        int lblFs = qBound(9,  qRound(rH * 0.15), 17);
        int valFs = qBound(13, qRound(rH * 0.26), 30);

        auto drawBar = [&](double x, double pct,
                           QColor fill, QString label, QString valStr) {
            p.setPen(C_DIM);
            p.setFont(QFont(appFont(), lblFs));
            double lblH = lblFs + 6;
            p.drawText(QRectF(x, rY, bW, lblH),
                       Qt::AlignCenter, label);

            double barY = rY + lblH + 2;
            p.setBrush(C_TRACK);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(QRectF(x, barY, bW, bH), bH/2, bH/2);

            double fw = qBound(0.0, pct, 1.0) * bW;
            p.setBrush(fill);
            p.drawRoundedRect(QRectF(x, barY, fw, bH), bH/2, bH/2);

            double valY = barY + bH + 6;
            p.setPen(C_WHITE);
            p.setFont(QFont(appFont(), valFs, QFont::Bold));
            p.drawText(QRectF(x, valY, bW, valFs + 6),
                       Qt::AlignCenter, valStr);
        };

        double tempPct = qBound(0.0, coolant / 120.0, 1.0);
        QColor tempCol = coolant > 100 ? C_RED : QColor(160, 85, 12);
        double fuelPct = qBound(0.0, m_fuel, 1.0);
        QColor fuelCol = fuelPct < 0.2 ? C_RED : QColor(140, 70, 8);

        drawBar(lX, tempPct, tempCol,
                "TEMP", QString("%1°C").arg(qRound(coolant)));
        drawBar(rX, fuelPct, fuelCol,
                "FUEL", QString("%1%").arg(qRound(fuelPct * 100)));
    }

    // ── Row 3: ALL icons (single row, 9 icons evenly spaced) ─────────────
    {
        double rY   = H * 0.57;
        double rH   = H * 0.22;
        double icoD = qBound(26.0, rH * 0.62, 52.0);
        double icoR = icoD / 2.0;
        double icoY = rY + rH * 0.42;
        int    symFs = qBound(10, qRound(icoD * 0.44), 20);
        int    capFs = qBound(6,  qRound(icoD * 0.24), 10);

        struct Icon {
            QString sym;
            QString cap;
            bool    on;
            QColor  col;
        };

        QVector<Icon> icons = {
            { "⚙", "SYS",  false,      C_MUTED  },
            { "⚠", "OIL",  warnOil,   C_AMBER  },
            { "⚠", "ABS",  warnABS,   C_AMBER  },
            { "▣", "ENG",  warnEng,   C_ORANGE },
            { "⛽", "FUEL", warnFuel,  C_ORANGE },
            { "⚠", "BAT",  warnBat,   C_AMBER  },
            { "◉", "LOW",  lightLow,  C_TEAL   },
            { "◑", "HIGH", lightHigh, C_TEAL   },
            { "◎", "FOG",  lightFog,  C_TEAL   },
        };

        double spacing = W / (double)(icons.size() + 1);

        for (int i = 0; i < icons.size(); i++) {
            double ix  = spacing * (i + 1);
            bool   on  = icons[i].on;
            QColor col = on ? icons[i].col : C_MUTED;

            // circle background
            if (on) {
                p.setBrush(icons[i].col.darker(260));
                p.setPen(QPen(icons[i].col, 1.5));
            } else {
                p.setBrush(QColor(18, 20, 28));
                p.setPen(QPen(C_MUTED, 1.0));
            }
            p.drawEllipse(QPointF(ix, icoY), icoR, icoR);

            // symbol (top 65% of circle)
            p.setPen(col);
            p.setFont(QFont(appFont(), symFs));
            p.drawText(QRectF(ix - icoR,
                              icoY - icoR,
                              icoD,
                              icoD * 0.65),
                       Qt::AlignCenter, icons[i].sym);

            // caption (bottom 35% of circle)
            p.setFont(QFont(appFont(), capFs, QFont::Bold));
            p.drawText(QRectF(ix - icoR,
                              icoY + icoR * 0.05,
                              icoD,
                              icoR * 0.90),
                       Qt::AlignCenter, icons[i].cap);
        }
    }

    // ── Row 4: CAN pill + odometer ────────────────────────────────────────
    {
        double rY   = H * 0.81;
        double rH   = H * 0.19;

        double pillH = qBound(16.0, rH * 0.36, 26.0);
        double pillW = W * 0.52;
        int    pFs   = qBound(8, qRound(pillH * 0.50), 13);

        QColor canFg = canOnline ? C_GREEN : C_RED;
        QColor canBg = canOnline ? QColor(8,24,8) : QColor(24,8,8);

        QRectF canBox(cx - pillW/2.0, rY, pillW, pillH);
        QPainterPath pill;
        pill.addRoundedRect(canBox, 5, 5);
        p.fillPath(pill, canBg);
        p.setPen(QPen(canFg, 1));
        p.drawPath(pill);
        p.setPen(canFg);
        p.setFont(QFont(appFont(), pFs));
        p.drawText(canBox, Qt::AlignCenter, canStatus);

        int odoFs = qBound(11, qRound(rH * 0.36), 20);
        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), odoFs, QFont::Bold));
        p.drawText(QRectF(cx - 110, rY + pillH + 4,
                          220, odoFs + 6),
                   Qt::AlignCenter,
                   QString("%1 km").arg(odometer));
    }
}