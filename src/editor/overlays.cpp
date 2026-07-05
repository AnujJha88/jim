#include "overlays.h"
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QtMath>
#include <QVBoxLayout>
#include <cmath>

// ============================================================
// GraveyardWidget Implementation — The Code Graveyard
// ============================================================
GraveyardWidget::GraveyardWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *header = new QWidget();
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(10, 5, 8, 5);
    QLabel *lbl = new QLabel("Deleted Code");
    lbl->setStyleSheet("color:#cccccc; font-size:11px; font-weight:600; letter-spacing:0.5px;");
    hl->addWidget(lbl);
    hl->addStretch();
    QPushButton *clearBtn = new QPushButton("Clear");
    clearBtn->setStyleSheet(
        "QPushButton{background:transparent;color:#969696;border:none;"
        "padding:2px 6px;font-size:11px;}"
        "QPushButton:hover{color:#cccccc;}");
    hl->addWidget(clearBtn);
    header->setStyleSheet("background:#252526; border-bottom:1px solid #3e3e42;");
    layout->addWidget(header);

    listWidget = new QListWidget();
    listWidget->setStyleSheet(
        "QListWidget{background:#1e1e1e;color:#cccccc;border:none;"
        "font-family:Consolas,monospace;font-size:11px;}"
        "QListWidget::item{padding:5px 8px;border-bottom:1px solid #2d2d30;}"
        "QListWidget::item:selected{background:#094771;color:#ffffff;}"
        "QListWidget::item:hover{background:#2a2d2e;}");
    listWidget->setWordWrap(true);
    layout->addWidget(listWidget, 1);

    QPushButton *resurrectBtn = new QPushButton("Paste Selected");
    resurrectBtn->setStyleSheet(
        "QPushButton{background:#0e639c;color:#ffffff;border:none;"
        "padding:7px;font-size:12px;border-radius:0;}"
        "QPushButton:hover{background:#1177bb;}"
        "QPushButton:pressed{background:#094771;}");
    layout->addWidget(resurrectBtn);

    setStyleSheet("background:#1e1e1e;");

    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        listWidget->clear();
        snippets.clear();
    });

    connect(resurrectBtn, &QPushButton::clicked, this, [this]() {
        int row = listWidget->currentRow();
        if (row >= 0 && row < snippets.size())
            emit resurrectRequested(snippets[row]);
    });

    connect(listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        int row = listWidget->row(item);
        if (row >= 0 && row < snippets.size())
            emit resurrectRequested(snippets[row]);
    });
}

void GraveyardWidget::addSnippet(const QString &code, const QString &source) {
    snippets.prepend(code);
    QString preview = code.trimmed();
    if (preview.length() > 120)
        preview = preview.left(117) + "...";
    preview.replace(QChar::ParagraphSeparator, '\n');
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QListWidgetItem *item = new QListWidgetItem(
        QString("%1  %2\n%3").arg(timestamp).arg(source).arg(preview));
    item->setToolTip(code.left(1000));
    listWidget->insertItem(0, item);
    // Cap at 50 entries
    while (snippets.size() > 50) {
        snippets.removeLast();
        delete listWidget->takeItem(listWidget->count() - 1);
    }
}

// ============================================================
// CRTOverlay Implementation — CRT & Cyberpunk Post-Processing
// ============================================================
CRTOverlay::CRTOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background:transparent;");
}

void CRTOverlay::setEnabled(bool enabled) {
    m_enabled = enabled;
    setVisible(enabled);
    update();
}

void CRTOverlay::paintEvent(QPaintEvent *) {
    if (!m_enabled) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Scanlines — thin horizontal lines every 2px
    for (int y = 0; y < height(); y += 2) {
        p.fillRect(0, y, width(), 1, QColor(0, 0, 0, 28));
    }

    // Vignette — dark gradient from edges toward center
    QRadialGradient vignette(width() / 2.0, height() / 2.0,
                             qMax(width(), height()) * 0.75);
    vignette.setColorAt(0.0, QColor(0, 0, 0,  0));
    vignette.setColorAt(0.7, QColor(0, 0, 0,  0));
    vignette.setColorAt(1.0, QColor(0, 0, 0, 90));
    p.fillRect(rect(), vignette);

    // Chromatic aberration — coloured edge fringe
    int fringe = 6;
    // Left red fringe
    QLinearGradient lgR(0, 0, fringe, 0);
    lgR.setColorAt(0, QColor(255, 0, 0, 18));
    lgR.setColorAt(1, QColor(255, 0, 0,  0));
    p.fillRect(0, 0, fringe, height(), lgR);
    // Right cyan fringe
    QLinearGradient lgC(width() - fringe, 0, width(), 0);
    lgC.setColorAt(0, QColor(0, 255, 255, 0));
    lgC.setColorAt(1, QColor(0, 255, 255, 18));
    p.fillRect(width() - fringe, 0, fringe, height(), lgC);
    // Top/bottom green fringe
    QLinearGradient lgT(0, 0, 0, fringe);
    lgT.setColorAt(0, QColor(0, 255, 0, 12));
    lgT.setColorAt(1, QColor(0, 255, 0,  0));
    p.fillRect(0, 0, width(), fringe, lgT);

    // Phosphor bloom — subtle green horizontal glow every 8th line
    for (int y = 0; y < height(); y += 8) {
        p.fillRect(0, y, width(), 1, QColor(0, 255, 120, 6));
    }
}

// ============================================================
// LaserParticleOverlay Implementation — Kinetic/Laser Editing
// ============================================================
LaserParticleOverlay::LaserParticleOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background:transparent;");
}

void LaserParticleOverlay::spawnSlash(int yPos) {
    slashY = yPos;
    slashAlpha = 1.0f;

    // Spawn sparks along the slash line
    int count = 18 + QRandomGenerator::global()->bounded(12);
    for (int i = 0; i < count; i++) {
        Spark s;
        s.x = QRandomGenerator::global()->bounded(width());
        s.y = static_cast<float>(yPos) + QRandomGenerator::global()->bounded(16) - 8;
        float angle = QRandomGenerator::global()->bounded(360) * M_PI / 180.f;
        float speed = 1.5f + QRandomGenerator::global()->bounded(40) / 10.f;
        s.vx = qCos(angle) * speed;
        s.vy = qSin(angle) * speed - 1.5f; // upward bias
        s.life = 0.8f + QRandomGenerator::global()->bounded(100) / 200.f;
        sparks.append(s);
    }

    if (!timerId)
        timerId = startTimer(16); // 60fps
    update();
}

void LaserParticleOverlay::timerEvent(QTimerEvent *) {
    slashAlpha = qMax(0.f, slashAlpha - 0.08f);

    for (auto &s : sparks) {
        s.x += s.vx;
        s.y += s.vy;
        s.vy += 0.15f; // gravity
        s.life -= 0.04f;
    }
    sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
                                [](const Spark &s) { return s.life <= 0; }),
                 sparks.end());

    if (slashAlpha <= 0 && sparks.isEmpty()) {
        killTimer(timerId);
        timerId = 0;
    }
    update();
}

void LaserParticleOverlay::paintEvent(QPaintEvent *) {
    if (slashAlpha <= 0 && sparks.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Laser slash line
    if (slashAlpha > 0) {
        int alpha = static_cast<int>(slashAlpha * 200);
        // Outer glow
        QPen glowPen(QColor(255, 0, 60, alpha / 4), 6);
        p.setPen(glowPen);
        p.drawLine(0, slashY, width(), slashY);
        // Core
        QPen corePen(QColor(255, 60, 100, alpha), 1);
        p.setPen(corePen);
        p.drawLine(0, slashY, width(), slashY);
    }

    // Sparks
    for (const auto &s : sparks) {
        int alpha = static_cast<int>(s.life * 255);
        QColor c = QColor::fromHsv(
            350 + static_cast<int>(s.life * 30), 220, 255, alpha);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        float r = 1.5f + s.life * 2.f;
        p.drawEllipse(QPointF(s.x, s.y), r, r);
    }
}

// ============================================================
// HUDWidget Implementation — Cybernetic HUD Status Bar
// ============================================================
HUDWidget::HUDWidget(QWidget *parent) : QWidget(parent) {
    setFixedWidth(345);
    setFixedHeight(22);
    cpuHistory.fill(0.f, 40);
    memHistory.fill(0.f, 40);
    startTimer(600); // update every 600ms
}

float HUDWidget::readCpuUsage() {
#ifdef Q_OS_LINUX
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0.f;
    QString line = f.readLine();
    f.close();
    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5) return 0.f;
    quint64 user = parts[1].toULongLong();
    quint64 nice = parts[2].toULongLong();
    quint64 sys  = parts[3].toULongLong();
    quint64 idle = parts[4].toULongLong();
    quint64 iowait = parts.size() > 5 ? parts[5].toULongLong() : 0;
    quint64 totalIdle = idle + iowait;
    quint64 total = user + nice + sys + idle + iowait +
                    (parts.size() > 6 ? parts[6].toULongLong() : 0) +
                    (parts.size() > 7 ? parts[7].toULongLong() : 0);
    float cpu = 0.f;
    if (prevTotal > 0 && total > prevTotal) {
        quint64 dIdle  = totalIdle - prevIdle;
        quint64 dTotal = total     - prevTotal;
        cpu = 1.f - static_cast<float>(dIdle) / static_cast<float>(dTotal);
    }
    prevIdle  = totalIdle;
    prevTotal = total;
    return qBound(0.f, cpu, 1.f);
#else
    return 0.f;
#endif
}

float HUDWidget::readMemUsage() {
#ifdef Q_OS_LINUX
    QFile f("/proc/meminfo");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0.f;
    quint64 total = 0, available = 0;
    while (!f.atEnd()) {
        QString line = f.readLine();
        if (line.startsWith("MemTotal:"))
            total = line.split(' ', Qt::SkipEmptyParts)[1].toULongLong();
        else if (line.startsWith("MemAvailable:"))
            available = line.split(' ', Qt::SkipEmptyParts)[1].toULongLong();
    }
    f.close();
    if (total == 0) return 0.f;
    return qBound(0.f, 1.f - static_cast<float>(available) / static_cast<float>(total), 1.f);
#else
    return 0.f;
#endif
}

void HUDWidget::addKeystroke() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    keystrokeTimestamps.append(now);
    // Trim entries older than 60 s to keep memory bounded
    while (!keystrokeTimestamps.isEmpty() && now - keystrokeTimestamps.first() > 60000)
        keystrokeTimestamps.removeFirst();
}

void HUDWidget::timerEvent(QTimerEvent *) {
    cpuHistory.removeFirst();
    cpuHistory.append(readCpuUsage());
    memHistory.removeFirst();
    memHistory.append(readMemUsage());
    hexCounter += 0x7F3A1B + QRandomGenerator::global()->bounded(0x200);

    // Compute WPM: keystrokes in last 60 s / 5 chars-per-word
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    while (!keystrokeTimestamps.isEmpty() && now - keystrokeTimestamps.first() > 60000)
        keystrokeTimestamps.removeFirst();
    currentWPM = keystrokeTimestamps.size() / 5;
    update();
}

void HUDWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), Qt::transparent);

    // Rolling hex address
    QString hexStr = QString("0x%1").arg(hexCounter, 16, 16, QChar('0')).toUpper();
    p.setFont(QFont("Consolas", 8));
    p.setPen(QColor(0, 255, 180, 180));
    p.drawText(0, 0, 100, height(), Qt::AlignVCenter | Qt::AlignLeft, hexStr);

    // CPU sparkline
    int sparkX = 108, sparkW = 70, sparkH = height() - 4;
    p.setPen(QColor(80, 80, 80, 120));
    p.drawRect(sparkX, 2, sparkW, sparkH);
    p.setPen(QColor(0, 255, 120, 200));
    p.setFont(QFont("Consolas", 7));
    p.setPen(QColor(0, 200, 100, 160));
    p.drawText(sparkX, 0, sparkW, height(), Qt::AlignVCenter | Qt::AlignLeft,
               QString("CPU %1%").arg(static_cast<int>(cpuHistory.last() * 100)));
    // draw sparkline
    p.setPen(QPen(QColor(0, 255, 120, 200), 1));
    for (int i = 1; i < cpuHistory.size(); ++i) {
        int x1 = sparkX + (i - 1) * sparkW / cpuHistory.size();
        int x2 = sparkX + i       * sparkW / cpuHistory.size();
        int y1 = 2 + sparkH - static_cast<int>(cpuHistory[i - 1] * sparkH);
        int y2 = 2 + sparkH - static_cast<int>(cpuHistory[i]     * sparkH);
        p.drawLine(x1, y1, x2, y2);
    }

    // Mem sparkline
    sparkX = 188;
    p.setPen(QColor(80, 80, 80, 120));
    p.drawRect(sparkX, 2, sparkW, sparkH);
    p.setPen(QColor(255, 50, 120, 160));
    p.drawText(sparkX, 0, sparkW, height(), Qt::AlignVCenter | Qt::AlignLeft,
               QString("MEM %1%").arg(static_cast<int>(memHistory.last() * 100)));
    p.setPen(QPen(QColor(255, 60, 150, 200), 1));
    for (int i = 1; i < memHistory.size(); ++i) {
        int x1 = sparkX + (i - 1) * sparkW / memHistory.size();
        int x2 = sparkX + i       * sparkW / memHistory.size();
        int y1 = 2 + sparkH - static_cast<int>(memHistory[i - 1] * sparkH);
        int y2 = 2 + sparkH - static_cast<int>(memHistory[i]     * sparkH);
        p.drawLine(x1, y1, x2, y2);
    }

    // WPM counter
    p.setFont(QFont("Consolas", 8));
    p.setPen(QColor(180, 220, 255, 180));
    p.drawText(268, 0, 70, height(), Qt::AlignVCenter | Qt::AlignLeft,
               QString("%1 WPM").arg(currentWPM));
}

// ============================================================
// GlitchOverlay Implementation — Glitch Art Undo
// ============================================================
GlitchOverlay::GlitchOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
}

void GlitchOverlay::trigger(const QPixmap &snapshot) {
    originalSnapshot = snapshot;
    glitchTime = 1.0f;
    if (!timerId) timerId = startTimer(30);
    show();
    update();
}

void GlitchOverlay::timerEvent(QTimerEvent *) {
    glitchTime -= 0.15f;
    if (glitchTime <= 0) {
        killTimer(timerId);
        timerId = 0;
        hide();
    }
    update();
}

void GlitchOverlay::paintEvent(QPaintEvent *) {
    if (glitchTime <= 0 || originalSnapshot.isNull()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Randomize offsets and RGB split
    int yOffset = QRandomGenerator::global()->bounded(10) - 5;
    int xOffsetR = QRandomGenerator::global()->bounded(20) - 10;
    int xOffsetB = QRandomGenerator::global()->bounded(20) - 10;

    // Draw base distorted
    p.drawPixmap(0, yOffset, originalSnapshot);

    // RGB Split effect
    p.setCompositionMode(QPainter::CompositionMode_Screen);
    p.setOpacity(glitchTime * 0.7);
    
    // Red shift
    p.fillRect(rect(), QColor(255, 0, 0, 30));
    p.drawPixmap(xOffsetR, 0, originalSnapshot);
    
    // Blue shift
    p.fillRect(rect(), QColor(0, 0, 255, 30));
    p.drawPixmap(xOffsetB, 0, originalSnapshot);
    
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // Draw horizontal noise bars
    int numBars = QRandomGenerator::global()->bounded(3, 8);
    for (int i = 0; i < numBars; i++) {
        int y = QRandomGenerator::global()->bounded(height());
        int h = QRandomGenerator::global()->bounded(2, 10);
        int offset = QRandomGenerator::global()->bounded(40) - 20;
        p.drawPixmap(offset, y, originalSnapshot, 0, y, width(), h);
        p.fillRect(0, y, width(), h, QColor(255, 255, 255, 50));
    }
}
