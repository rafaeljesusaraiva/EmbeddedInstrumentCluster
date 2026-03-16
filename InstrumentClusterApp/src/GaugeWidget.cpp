#include "GaugeWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QTimer>
#include "AppFont.h"

static const QColor C_BG     = QColor(8,   5,   2);
static const QColor C_CARD   = QColor(20,  12,  4);
static const QColor C_BORDER = QColor(50,  28,  8);
static const QColor C_WHITE  = QColor(240, 235, 225);
static const QColor C_DIM    = QColor(130,  90, 40);

static QPainterPath buildPerim(QRectF r, double cr) {
    QPainterPath path;
    path.moveTo(r.left() + cr, r.top());
    path.lineTo(r.right() - cr, r.top());
    path.arcTo(QRectF(r.right()-2*cr, r.top(),        2*cr, 2*cr),  90, -90);
    path.lineTo(r.right(), r.bottom() - cr);
    path.arcTo(QRectF(r.right()-2*cr, r.bottom()-2*cr, 2*cr, 2*cr),  0, -90);
    path.lineTo(r.left() + cr, r.bottom());
    path.arcTo(QRectF(r.left(),       r.bottom()-2*cr, 2*cr, 2*cr), 270, -90);
    path.lineTo(r.left(), r.top() + cr);
    path.arcTo(QRectF(r.left(),       r.top(),         2*cr, 2*cr), 180, -90);
    path.closeSubpath();
    return path;
}

static QVector<QPointF> samplePath(const QPainterPath &src,
                                    double startPct, double endPct,
                                    double step = 1.5)
{
    QVector<QPointF> pts;
    double total = src.length();
    double s = qBound(0.0, startPct, 1.0) * total;
    double e = qBound(0.0, endPct,   1.0) * total;
    if (e <= s) return pts;
    for (double d = s; d <= e + step*0.5; d += step)
        pts << src.pointAtPercent(qMin(d, e) / total);
    return pts;
}

static QPainterPath bandPath(const QPainterPath &outer,
                              const QPainterPath &inner,
                              double startPct, double endPct)
{
    startPct = qBound(0.0, startPct, 1.0);
    endPct   = qBound(0.0, endPct,   1.0);
    if (endPct <= startPct) return QPainterPath();
    auto op = samplePath(outer, startPct, endPct);
    auto ip = samplePath(inner, startPct, endPct);
    if (op.isEmpty() || ip.isEmpty()) return QPainterPath();
    QPainterPath res;
    res.moveTo(op.first());
    for (int i = 1; i < op.size(); i++) res.lineTo(op[i]);
    res.lineTo(ip.last());
    for (int i = ip.size()-2; i >= 0; i--) res.lineTo(ip[i]);
    res.closeSubpath();
    return res;
}

static void drawBandSolid(QPainter &p,
                           const QPainterPath &outer,
                           const QPainterPath &inner,
                           double startPct, double spanPct,
                           QColor color)
{
    double endPct = startPct + spanPct;
    p.setPen(Qt::NoPen);
    if (endPct <= 1.0) {
        p.fillPath(bandPath(outer, inner, startPct, endPct), color);
    } else {
        p.fillPath(bandPath(outer, inner, startPct, 1.0),         color);
        p.fillPath(bandPath(outer, inner, 0.0, endPct - 1.0),     color);
    }
}

GaugeWidget::GaugeWidget(Config cfg, QWidget *parent)
    : QWidget(parent), m_cfg(cfg), m_value(0), m_displayValue(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);

    // Smooth animation timer — interpolates display value toward target
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        double diff = m_value - m_displayValue;
        if (qAbs(diff) < 0.5) {
            m_displayValue = m_value;
            m_animTimer->stop();
        } else {
            m_displayValue += diff * 0.18;  // ease factor
        }
        update();
    });
}

void GaugeWidget::setValue(double v) {
    m_value = qBound(m_cfg.minVal, v, m_cfg.maxVal);
    if (!m_animTimer->isActive())
        m_animTimer->start(16);  // ~60fps
}

void GaugeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double W  = width();
    const double H  = height();
    const double cx = W / 2.0;
    const double cy = H / 2.0;
    const double S  = qMin(W, H);

    p.fillRect(rect(), C_BG);

    // ── Geometry ──────────────────────────────────────────────────────────
    double cardPad    = S * 0.04;
    double cardRadius = S * 0.13;
    QRectF cardRect(cx - S/2 + cardPad, cy - S/2 + cardPad,
                    S - cardPad*2,       S - cardPad*2);

    double outerPad = S * 0.045;
    double innerPad = S * 0.145;
    double bandCr   = cardRadius * 0.85;

    QRectF outerRect(cx - S/2 + outerPad, cy - S/2 + outerPad,
                     S - outerPad*2,       S - outerPad*2);
    QRectF innerRect(cx - S/2 + innerPad, cy - S/2 + innerPad,
                     S - innerPad*2,       S - innerPad*2);

    QPainterPath outerPerim = buildPerim(outerRect, bandCr);
    QPainterPath innerPerim = buildPerim(innerRect, bandCr);

    double startPct = m_cfg.arcStartPct;
    double spanPct  = m_cfg.arcSpanPct;

    // ── 1. Outer card ─────────────────────────────────────────────────────
    QPainterPath cardPath;
    cardPath.addRoundedRect(cardRect, cardRadius, cardRadius);
    p.fillPath(cardPath, C_CARD);
    p.setPen(QPen(C_BORDER, 1.2));
    p.drawPath(cardPath);

    // ── 2. Track band ─────────────────────────────────────────────────────
    drawBandSolid(p, outerPerim, innerPerim,
                  startPct, spanPct, QColor(25, 15, 5));

    // ── 3. Active band — single filled shape + gradient overlay ──────────
    {
        double valuePct  = (m_displayValue - m_cfg.minVal)
                        / (m_cfg.maxVal   - m_cfg.minVal);
        double activePct = spanPct * qBound(0.0, valuePct, 1.0);

        if (activePct > 0.001) {
            double endPct = qMin(startPct + activePct, 0.9999);

            QPainterPath activeBand;
            if (endPct <= 1.0) {
                activeBand = bandPath(outerPerim, innerPerim,
                                    startPct, endPct);
            } else {
                QPainterPath seg1 = bandPath(outerPerim, innerPerim,
                                            startPct, 1.0);
                QPainterPath seg2 = bandPath(outerPerim, innerPerim,
                                            0.0, endPct - 1.0);
                activeBand = seg1;
                activeBand.addPath(seg2);
            }

            QPointF ptStart = outerPerim.pointAtPercent(
                                qBound(0.0, startPct, 1.0));
            QPointF ptEnd   = outerPerim.pointAtPercent(
                                qBound(0.0, endPct, 1.0));

            QLinearGradient grad(ptStart, ptEnd);
            grad.setColorAt(0.0, QColor(30,  14,  3, 255));
            grad.setColorAt(0.3, QColor(80,  35,  5, 255));
            grad.setColorAt(0.7, QColor(150, 68,  8, 255));
            grad.setColorAt(1.0, QColor(190, 95, 12, 255));

            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawPath(activeBand);
        }
    }

    // ── 4. Inner card — fully covers band interior ────────────────────────
    double innerCardPad    = S * 0.150;
    double innerCardRadius = cardRadius * 0.80;
    QRectF innerCardRect(cx - S/2 + innerCardPad, cy - S/2 + innerCardPad,
                         S - innerCardPad*2,       S - innerCardPad*2);
    QPainterPath innerCardPath;
    innerCardPath.addRoundedRect(innerCardRect, innerCardRadius,
                                  innerCardRadius);
    p.fillPath(innerCardPath, QColor(16, 9, 3));
    p.setPen(QPen(C_BORDER, 0.8));
    p.drawPath(innerCardPath);

    // ── 5. Tick labels with smooth radial dark glow ───────────────────────
    double step     = m_cfg.tickStep > 0
                    ? m_cfg.tickStep
                    : (m_cfg.maxVal - m_cfg.minVal) / 4.0;
    int    numTicks = qRound((m_cfg.maxVal - m_cfg.minVal) / step);
    int    lblFs    = qBound(6, qRound(S * 0.028), 12);

    double valuePct = (m_displayValue - m_cfg.minVal)
                    / (m_cfg.maxVal   - m_cfg.minVal);

    for (int i = 0; i <= numTicks; i++) {
        double pct      = (double)i / (double)numTicks;
        double perimPct = startPct + pct * spanPct;
        while (perimPct > 1.0) perimPct -= 1.0;
        perimPct = qBound(0.0, perimPct, 1.0);

        QPointF onArc    = outerPerim.pointAtPercent(perimPct);
        QPointF toCenter(cx - onArc.x(), cy - onArc.y());
        double  dist     = qSqrt(toCenter.x()*toCenter.x()
                                + toCenter.y()*toCenter.y());
        if (dist < 1.0) continue;

        QPointF unit(toCenter.x()/dist, toCenter.y()/dist);
        double  inset = (innerPad - outerPad) * 0.52 + lblFs;
        double  lx    = onArc.x() + unit.x() * inset;
        double  ly    = onArc.y() + unit.y() * inset;

        double  val = m_cfg.minVal + pct * (m_cfg.maxVal - m_cfg.minVal);
        QString txt = QString::number(qRound(val));
        bool    overActive = (pct <= valuePct);

        QRectF labelRect(lx - lblFs*3, ly - lblFs,
                         lblFs*6,      lblFs*2);

        if (overActive) {
            // Shadow pass — direct, no offscreen buffer
            double brightness = qBound(0.0,
                                  pct / qMax(valuePct, 0.001), 1.0);
            // Draw shadow with offset
            p.setPen(QColor(0, 0, 0, 160));
            p.setFont(QFont(appFont(), lblFs, QFont::Bold));
            p.drawText(labelRect.translated(1, 1),
                       Qt::AlignCenter, txt);

            // Text — color scales with brightness
            int cr = qRound(190 + brightness * 65);
            int cg = qRound(140 + brightness * 95);
            int cb = qRound( 80 + brightness * 145);
            p.setPen(QColor(cr, cg, cb, 255));
            p.drawText(labelRect, Qt::AlignCenter, txt);

        } else {
            p.setPen(C_DIM);
            p.setFont(QFont(appFont(), lblFs));
            p.drawText(labelRect, Qt::AlignCenter, txt);
        }
    }

    // ── 6. Main value ─────────────────────────────────────────────────────
    double icW   = S - innerCardPad * 2;
    int    valFs = qBound(16, qRound(icW * 0.22), 72);

    // Shadow
    p.setPen(QColor(0, 0, 0, 160));
    p.setFont(QFont(appFont(), valFs, QFont::Bold));
    p.drawText(QRectF(cx - icW/2.0 + 2, cy - icW*0.24 + 2,
                      icW, icW*0.48),
               Qt::AlignCenter,
               QString::number(qRound(m_displayValue)));

    // Value
    p.setPen(C_WHITE);
    p.drawText(QRectF(cx - icW/2.0, cy - icW*0.24,
                      icW,           icW*0.48),
               Qt::AlignCenter,
               QString::number(qRound(m_displayValue)));

    // ── 7. Unit label ─────────────────────────────────────────────────────
    int unitFs = qBound(8, qRound(icW * 0.075), 20);
    p.setPen(C_DIM);
    p.setFont(QFont(appFont(), unitFs));
    p.drawText(QRectF(cx - icW/2.0, cy + icW*0.18,
                      icW,           icW*0.16),
               Qt::AlignCenter, m_cfg.unit);
}