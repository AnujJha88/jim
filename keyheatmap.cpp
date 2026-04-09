#include "keyheatmap.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QShowEvent>
#include <algorithm>

// ─── Key layout ──────────────────────────────────────────────────────────────
// col / w are in "key units"; 1 unit = a standard square key.
// Positions mirror a physical ANSI QWERTY keyboard.
struct KeyDef {
    const char *label;   // text drawn on the key
    const char *track;   // key used for frequency lookup (empty = no tracking)
    float col;           // left edge in key units
    int   row;           // row index 0-4 (top to bottom)
    float w;             // width in key units
};

static const KeyDef s_keys[] = {
    // ── Row 0 — number row ─────────────────────────────────────────────────
    {"`",  "`",   0.0f, 0, 1.0f}, {"1","1", 1.0f,0,1.0f}, {"2","2", 2.0f,0,1.0f},
    {"3","3",3.0f,0,1.0f}, {"4","4",4.0f,0,1.0f}, {"5","5",5.0f,0,1.0f},
    {"6","6",6.0f,0,1.0f}, {"7","7",7.0f,0,1.0f}, {"8","8",8.0f,0,1.0f},
    {"9","9",9.0f,0,1.0f}, {"0","0",10.0f,0,1.0f}, {"-","-",11.0f,0,1.0f},
    {"=","=",12.0f,0,1.0f}, {"BS","bs",13.0f,0,1.5f},

    // ── Row 1 — QWERTY ─────────────────────────────────────────────────────
    {"Tab","tab",0.0f,1,1.5f},
    {"Q","q",1.5f,1,1.0f}, {"W","w",2.5f,1,1.0f}, {"E","e",3.5f,1,1.0f},
    {"R","r",4.5f,1,1.0f}, {"T","t",5.5f,1,1.0f}, {"Y","y",6.5f,1,1.0f},
    {"U","u",7.5f,1,1.0f}, {"I","i",8.5f,1,1.0f}, {"O","o",9.5f,1,1.0f},
    {"P","p",10.5f,1,1.0f}, {"[","[",11.5f,1,1.0f}, {"]","]",12.5f,1,1.0f},
    {"\\","\\",13.5f,1,1.0f},

    // ── Row 2 — ASDF ───────────────────────────────────────────────────────
    {"Caps","",0.0f,2,1.75f},
    {"A","a",1.75f,2,1.0f}, {"S","s",2.75f,2,1.0f}, {"D","d",3.75f,2,1.0f},
    {"F","f",4.75f,2,1.0f}, {"G","g",5.75f,2,1.0f}, {"H","h",6.75f,2,1.0f},
    {"J","j",7.75f,2,1.0f}, {"K","k",8.75f,2,1.0f}, {"L","l",9.75f,2,1.0f},
    {";",";",10.75f,2,1.0f}, {"'","'",11.75f,2,1.0f},
    {"Enter","enter",12.75f,2,1.75f},

    // ── Row 3 — ZXCV ───────────────────────────────────────────────────────
    {"Shift","",0.0f,3,2.25f},
    {"Z","z",2.25f,3,1.0f}, {"X","x",3.25f,3,1.0f}, {"C","c",4.25f,3,1.0f},
    {"V","v",5.25f,3,1.0f}, {"B","b",6.25f,3,1.0f}, {"N","n",7.25f,3,1.0f},
    {"M","m",8.25f,3,1.0f}, {",",",",9.25f,3,1.0f}, {".",  ".",10.25f,3,1.0f},
    {"/","/",11.25f,3,1.0f}, {"Shift","",12.25f,3,2.25f},

    // ── Row 4 — Space ──────────────────────────────────────────────────────
    {"Space"," ",3.75f,4,7.0f},
};

static constexpr int NUM_KEYS = static_cast<int>(sizeof(s_keys) / sizeof(s_keys[0]));

// ─── Widget ──────────────────────────────────────────────────────────────────
static constexpr int kPad    = 12;
static constexpr int kTitle  = 28;
static constexpr int kKeyH   = 36;
static constexpr int kGap    = 3;
// Total key-unit span of the widest row (number row ends at 14.5 units)
static constexpr float kTotalUnits = 14.5f;
// Widget logical size
static constexpr int kW = 580;
static constexpr int kH = kPad + kTitle + 5 * (kKeyH + kGap) + kPad;

KeyHeatmapOverlay::KeyHeatmapOverlay(QWidget *parent) : QWidget(parent) {
    setFixedSize(kW, kH);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow);
}

void KeyHeatmapOverlay::recordKey(const QString &text, int qtKey) {
    if (!text.isEmpty() && text[0].isPrint() && !text[0].isSpace()) {
        QString key = text.toLower();
        m_freq[key] = m_freq.value(key, 0) + 1;
    } else {
        switch (qtKey) {
            case Qt::Key_Return:
            case Qt::Key_Enter:   m_freq["enter"] = m_freq.value("enter", 0) + 1; break;
            case Qt::Key_Backspace: m_freq["bs"]  = m_freq.value("bs",    0) + 1; break;
            case Qt::Key_Tab:     m_freq["tab"]   = m_freq.value("tab",   0) + 1; break;
            case Qt::Key_Space:   m_freq[" "]     = m_freq.value(" ",     0) + 1; break;
            default: break;
        }
    }
    update();
}

// Heat colour: cold (low) → blue → teal → yellow → red (high)
QColor KeyHeatmapOverlay::heatColor(float r) const {
    if (r <= 0.0f) return QColor(45, 45, 52);  // unused key: dark slate

    struct Stop { float pos; int R, G, B; };
    static const Stop stops[] = {
        {0.0f, 30,  90, 200},   // blue
        {0.35f, 20, 160, 110},  // teal
        {0.65f, 185, 150, 20},  // yellow
        {1.0f, 210,  40,  30},  // red
    };
    for (int i = 0; i < 3; ++i) {
        if (r <= stops[i + 1].pos) {
            float t = (r - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            auto lerp = [](int a, int b, float t) { return qBound(0, static_cast<int>(a + t * (b - a)), 255); };
            return QColor(lerp(stops[i].R, stops[i+1].R, t),
                          lerp(stops[i].G, stops[i+1].G, t),
                          lerp(stops[i].B, stops[i+1].B, t));
        }
    }
    return QColor(210, 40, 30);
}

void KeyHeatmapOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background panel
    QPainterPath bg;
    bg.addRoundedRect(rect(), 10, 10);
    p.fillPath(bg, QColor(18, 18, 24, 230));
    p.setPen(QColor(60, 60, 70));
    p.drawPath(bg);

    // Title
    p.setFont(QFont("Consolas", 10, QFont::Bold));
    p.setPen(QColor(180, 220, 255, 200));
    p.drawText(kPad, 0, kW - kPad * 2, kTitle,
               Qt::AlignVCenter | Qt::AlignLeft, "KEY HEATMAP  — click anywhere to close");

    // Find max frequency for normalisation
    int maxFreq = 1;
    for (auto v : m_freq) maxFreq = qMax(maxFreq, v);

    // Key unit → pixel conversion
    float unitW = static_cast<float>(kW - kPad * 2) / kTotalUnits;

    for (int idx = 0; idx < NUM_KEYS; ++idx) {
        const KeyDef &k = s_keys[idx];
        int px = kPad + static_cast<int>(k.col * unitW);
        int py = kPad + kTitle + k.row * (kKeyH + kGap);
        int pw = static_cast<int>(k.w * unitW) - kGap;
        int ph = kKeyH;

        QString track = QString::fromLatin1(k.track);
        int freq = track.isEmpty() ? 0 : m_freq.value(track, 0);
        float ratio = static_cast<float>(freq) / maxFreq;

        QColor fill = heatColor(ratio);
        QPainterPath kp;
        kp.addRoundedRect(px, py, pw, ph, 4, 4);
        p.fillPath(kp, fill);

        // Key label
        p.setFont(QFont("Consolas", track.size() > 1 ? 7 : 9));
        p.setPen(QColor(220, 220, 220, 220));
        p.drawText(px, py, pw, ph, Qt::AlignCenter, QString::fromLatin1(k.label));

        // Frequency count (small, bottom-right of key)
        if (freq > 0) {
            p.setFont(QFont("Consolas", 6));
            p.setPen(QColor(255, 255, 255, 140));
            p.drawText(px, py, pw - 2, ph - 2, Qt::AlignRight | Qt::AlignBottom,
                       freq >= 1000 ? QString("%1k").arg(freq / 1000) : QString::number(freq));
        }
    }
}

void KeyHeatmapOverlay::showEvent(QShowEvent *e) {
    // Centre within parent widget
    if (parentWidget()) {
        QRect pr = parentWidget()->rect();
        move((pr.width()  - width())  / 2,
             (pr.height() - height()) / 2);
    }
    QWidget::showEvent(e);
}
