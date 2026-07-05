#include "storageslotvisualizer.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QToolTip>
#include <QColor>
#include <QRect>
#include <QPoint>
#include <QSize>

// ─── Palette ──────────────────────────────────────────────────────────────────
namespace Pal {
    static const QColor Background  { 20,  20,  30 };
    static const QColor RowEven     { 25,  25,  38 };
    static const QColor RowOdd      { 22,  22,  33 };
    static const QColor WastedBytes { 60,  20,  20 };
    static const QColor Packed      { 30,  80,  50 };   // tightly-packed small type
    static const QColor LargeSlot   { 60,  40,  20 };   // full-slot / large non-dynamic type
    static const QColor Dynamic     { 30,  40,  70 };   // string / bytes / mapping
    static const QColor HoverOverlay{ 200, 200, 255, 40 };
    static const QColor HoverBorder { 200, 200, 255 };
    static const QColor GridLine    { 40,  40,  55 };
    static const QColor LabelText   { 150, 150, 170 };
    static const QColor VarText     { Qt::white };
    static const QColor HeaderBg    { 18,  18,  28 };
}

// ─── Constructor ──────────────────────────────────────────────────────────────

StorageSlotVisualizerWidget::StorageSlotVisualizerWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMinimumWidth(kLabelW + 32 * kByteW + 20);
}

// ─── Public API ───────────────────────────────────────────────────────────────

void StorageSlotVisualizerWidget::analyzeCode(const QString &solidityCode)
{
    m_vars = SolidityAnalyzer::parseStorageSlots(solidityCode);
    m_slotCount  = 0;
    m_hoveredVar = -1;
    for (const SolidityAnalyzer::StorageVar &v : m_vars)
        m_slotCount = qMax(m_slotCount, v.slot + 1);
    updateGeometry();
    update();
}

void StorageSlotVisualizerWidget::clear()
{
    m_vars.clear();
    m_slotCount  = 0;
    m_hoveredVar = -1;
    updateGeometry();
    update();
}

// ─── Size hints ───────────────────────────────────────────────────────────────

QSize StorageSlotVisualizerWidget::sizeHint() const
{
    int h = kHeaderH + m_slotCount * kSlotH + kLegendH + 20;
    return QSize(kLabelW + 32 * kByteW + 20, h);
}

QSize StorageSlotVisualizerWidget::minimumSizeHint() const
{
    return QSize(kLabelW + 32 * kByteW + 20, kHeaderH + 2 * kSlotH + kLegendH);
}

// ─── Geometry helpers ─────────────────────────────────────────────────────────

// Solidity stores variables right-to-left within a slot (little-endian layout).
// byteOffset 0 = lowest byte = rightmost byte in our display.
// Display column 0 is the leftmost = byte 31.
// Display column (31 - byteOffset) is where byteOffset starts (rightmost extent).
// For a variable spanning bytes [byteOffset .. byteOffset+byteSize-1]:
//   rightmost column = 31 - byteOffset
//   leftmost  column = 31 - (byteOffset + byteSize - 1)
QRect StorageSlotVisualizerWidget::varRect(int varIdx) const
{
    const SolidityAnalyzer::StorageVar &v = m_vars[varIdx];
    int leftCol = 31 - (v.byteOffset + v.byteSize - 1);
    int x = kLabelW + leftCol * kByteW;
    int y = kHeaderH + v.slot * kSlotH;
    int w = v.byteSize * kByteW;
    return QRect(x, y, w, kSlotH);
}

int StorageSlotVisualizerWidget::varIndexAtPoint(const QPoint &pt) const
{
    for (int i = 0; i < m_vars.size(); ++i) {
        if (varRect(i).contains(pt)) return i;
    }
    return -1;
}

// ─── Paint ────────────────────────────────────────────────────────────────────

void StorageSlotVisualizerWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);   // crisp grid lines

    // ── Background ──────────────────────────────────────────────────────────
    p.fillRect(rect(), Pal::Background);

    if (m_vars.isEmpty() && m_slotCount == 0) {
        p.setPen(Pal::LabelText);
        QFont f = font();
        f.setItalic(true);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   QStringLiteral("No storage variables found.\n"
                                  "Open a .sol file to visualize slot layout."));
        return;
    }

    QFont smallFont = font();
    smallFont.setPointSize(qMax(6, smallFont.pointSize() - 2));
    QFont labelFont = font();
    labelFont.setPointSize(qMax(7, font().pointSize() - 1));

    QFontMetrics smallFm(smallFont);
    QFontMetrics labelFm(labelFont);

    // ── Header row ──────────────────────────────────────────────────────────
    p.fillRect(0, 0, width(), kHeaderH, Pal::HeaderBg);

    p.setFont(labelFont);
    p.setPen(Pal::LabelText);
    p.drawText(QRect(0, 0, kLabelW - 4, kHeaderH),
               Qt::AlignVCenter | Qt::AlignRight,
               QStringLiteral("Slot / Byte"));

    // Byte position labels: column 0 = byte 31, column 31 = byte 0.
    // Only draw a number every 4 columns to avoid clutter.
    p.setFont(smallFont);
    for (int col = 0; col < 32; ++col) {
        int byteNum = 31 - col;
        int x = kLabelW + col * kByteW;
        if (byteNum % 4 == 3 || byteNum == 0) {
            QRect cell(x, 0, kByteW * (byteNum % 4 == 3 ? 4 : 1), kHeaderH);
            p.setPen(Pal::LabelText);
            p.drawText(cell, Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral(" %1").arg(byteNum));
        }
    }

    // ── Vertical divider between label area and slot grid ────────────────────
    p.setPen(QPen(Pal::GridLine, 1));
    p.drawLine(kLabelW, 0, kLabelW, kHeaderH + m_slotCount * kSlotH);

    // ── Per-slot rows ────────────────────────────────────────────────────────
    for (int slot = 0; slot < m_slotCount; ++slot) {
        int rowY = kHeaderH + slot * kSlotH;

        // Row background
        p.fillRect(0, rowY, width(), kSlotH,
                   (slot % 2 == 0) ? Pal::RowEven : Pal::RowOdd);

        // Entire grid area of this row filled with "wasted" colour first;
        // variables will be painted over it.
        p.fillRect(kLabelW + 1, rowY + 2, 32 * kByteW - 2, kSlotH - 4,
                   Pal::WastedBytes);

        // Horizontal separator
        p.setPen(QPen(Pal::GridLine, 1));
        p.drawLine(0, rowY + kSlotH - 1, width(), rowY + kSlotH - 1);

        // Slot label
        p.setFont(labelFont);
        p.setPen(Pal::LabelText);
        p.drawText(QRect(4, rowY, kLabelW - 8, kSlotH),
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString(QStringLiteral("Slot %1")).arg(slot));

        // Every-4-byte tick marks inside the grid row
        p.setPen(QPen(Pal::GridLine, 1));
        for (int col = 0; col <= 32; col += 4) {
            int x = kLabelW + col * kByteW;
            p.drawLine(x, rowY, x, rowY + kSlotH);
        }
    }

    // ── Variable rectangles ───────────────────────────────────────────────────
    for (int i = 0; i < m_vars.size(); ++i) {
        const SolidityAnalyzer::StorageVar &v = m_vars[i];
        QRect r = varRect(i);
        QRect inner = r.adjusted(1, 2, -1, -2);

        // Pick a colour based on type category.
        QColor varColor;
        if (v.typeName == QLatin1String("string")
                || v.typeName == QLatin1String("bytes")
                || v.typeName.startsWith(QLatin1String("mapping"))
                || v.typeName.startsWith(QLatin1String("struct"))) {
            varColor = Pal::Dynamic;
        } else if (v.isPacked) {
            varColor = Pal::Packed;
        } else {
            varColor = Pal::LargeSlot;
        }

        p.fillRect(inner, varColor);

        // Slightly lighter border so the rect is clearly outlined.
        p.setPen(QPen(varColor.lighter(160), 1));
        p.drawRect(inner);

        // Variable name label (elided if too narrow).
        if (inner.width() >= 10) {
            p.setFont(smallFont);
            p.setPen(Pal::VarText);
            QString label = v.name;
            label = smallFm.elidedText(label, Qt::ElideRight, inner.width() - 4);
            p.drawText(inner, Qt::AlignCenter, label);
        }
    }

    // ── Hover highlight ───────────────────────────────────────────────────────
    if (m_hoveredVar >= 0 && m_hoveredVar < m_vars.size()) {
        QRect r = varRect(m_hoveredVar).adjusted(1, 2, -1, -2);
        p.fillRect(r, Pal::HoverOverlay);
        p.setPen(QPen(Pal::HoverBorder, 2));
        p.drawRect(r);
    }

    // ── Column ticks over header ──────────────────────────────────────────────
    p.setPen(QPen(Pal::GridLine, 1));
    for (int col = 0; col <= 32; col += 4) {
        int x = kLabelW + col * kByteW;
        p.drawLine(x, 0, x, kHeaderH);
    }

    // ── Legend ────────────────────────────────────────────────────────────────
    struct LegendItem { QColor color; const char *label; };
    static const LegendItem legend[] = {
        { Pal::Packed,      "Packed (efficient)" },
        { Pal::LargeSlot,   "Full slot / large"  },
        { Pal::Dynamic,     "Dynamic type"        },
        { Pal::WastedBytes, "Wasted space"        },
    };

    int legendY = kHeaderH + m_slotCount * kSlotH + 6;
    int lx = kLabelW;
    p.setFont(smallFont);
    for (const LegendItem &item : legend) {
        QRect swatch(lx, legendY + 3, 12, 12);
        p.fillRect(swatch, item.color);
        p.setPen(QPen(item.color.lighter(160), 1));
        p.drawRect(swatch);
        p.setPen(Pal::LabelText);
        p.drawText(QRect(lx + 15, legendY, 118, kLegendH),
                   Qt::AlignVCenter, QString::fromLatin1(item.label));
        lx += 138;
        if (lx + 138 > width()) break;  // don't draw off-screen
    }
}

// ─── Mouse events ─────────────────────────────────────────────────────────────

void StorageSlotVisualizerWidget::mouseMoveEvent(QMouseEvent *event)
{
    int prev = m_hoveredVar;
    m_hoveredVar = varIndexAtPoint(event->pos());

    if (m_hoveredVar != prev) {
        if (m_hoveredVar >= 0) {
            const SolidityAnalyzer::StorageVar &v = m_vars[m_hoveredVar];
            QString tip = QString(
                QStringLiteral(
                    "<b>%1</b><br/>"
                    "Type: <code>%2</code><br/>"
                    "Slot&nbsp;%3,&nbsp;byte&nbsp;offset&nbsp;%4<br/>"
                    "Size: %5 byte%6%7"
                ))
                .arg(v.name, v.typeName)
                .arg(v.slot)
                .arg(v.byteOffset)
                .arg(v.byteSize)
                .arg(v.byteSize == 1 ? QStringLiteral("") : QStringLiteral("s"))
                .arg(v.isPacked ? QStringLiteral("&nbsp;<i>(packed)</i>")
                                : QStringLiteral(""));
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        } else {
            QToolTip::hideText();
        }
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void StorageSlotVisualizerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = varIndexAtPoint(event->pos());
        if (idx >= 0 && idx < m_vars.size())
            emit jumpToVariable(m_vars[idx].name);
    }
    QWidget::mousePressEvent(event);
}
