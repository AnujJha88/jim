#include "animationwidget.h"
#include "audiomonitor.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QTimer>
#include <QtMath>
#include <cmath>
#include <algorithm>

// ============================================================
// DJ Visualizer Widget Implementation
// ============================================================
DJVisualizerWidget::DJVisualizerWidget(QWidget *parent)
    : QWidget(parent) {
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    // Create the visualizer widget
    visualizerWidget = new AnimationWidget(this);
    visualizerWidget->setMinimumHeight(180);
    visualizerWidget->setMaximumHeight(250);
    layout->addWidget(visualizerWidget);

    // Style the panel
    setStyleSheet(R"(
        QWidget {
            background-color: #0a0a0f;
            color: #ffffff;
        }
    )");
}

DJVisualizerWidget::~DJVisualizerWidget() {
    // Widget cleanup is handled by Qt
}

void DJVisualizerWidget::setAudioMonitor(AudioMonitor* monitor) {
    visualizerWidget->setAudioMonitor(monitor);
    visualizerWidget->setAnimationType(AnimationWidget::DJMode);
    visualizerWidget->update(); // Force initial update
}

// ============================================================
// AnimationWidget Implementation
// ============================================================
AnimationWidget::AnimationWidget(QWidget *parent)
    : QWidget(parent), currentType(None), timerId(0), frame(0) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent;");
    initMatrix();
    initParticles();
}

void AnimationWidget::setAnimationType(AnimationType type) {
    if (timerId) {
        killTimer(timerId);
        timerId = 0;
    }

    currentType = type;
    frame = 0;

    if (type != None) {
        timerId = startTimer(33); // ~30 FPS for smoother animation
    }

    if (type == Matrix) initMatrix();
    else if (type == Particles || type == Waves || type == Pulse || type == Rain || type == Snow) initParticles();
    else if (type == Starfield) initStarfield();
    else if (type == Fire) initFire();

    update();
}

void AnimationWidget::cycleAnimation() {
    int next = (static_cast<int>(currentType) + 1) % 9;
    setAnimationType(static_cast<AnimationType>(next));
}

void AnimationWidget::initMatrix() {
    matrixColumns.clear();
    for (int i = 0; i < 30; i++) {
        MatrixColumn col;
        col.x = QRandomGenerator::global()->bounded(width() + 100);
        col.y = -(QRandomGenerator::global()->bounded(500));
        col.speed = 2 + QRandomGenerator::global()->bounded(5);
        col.text = QString("01").at(QRandomGenerator::global()->bounded(2));
        matrixColumns.append(col);
    }
}

void AnimationWidget::initParticles() {
    particles.clear();
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width());
        p.y = QRandomGenerator::global()->bounded(height());
        p.vx = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
        p.vy = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
        p.life = 255;
        p.size = 2.0f;
        particles.append(p);
    }
}

void AnimationWidget::initStarfield() {
    particles.clear();
    for (int i = 0; i < 100; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width()) - width() / 2;
        p.y = QRandomGenerator::global()->bounded(height()) - height() / 2;
        p.vx = p.x / 50.0f;
        p.vy = p.y / 50.0f;
        p.life = 255;
        p.size = 0.5f + (QRandomGenerator::global()->bounded(100) / 50.0f);
        particles.append(p);
    }
}

void AnimationWidget::initFire() {
    particles.clear();
    for (int i = 0; i < 60; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width());
        p.y = height() + QRandomGenerator::global()->bounded(50);
        p.vx = (QRandomGenerator::global()->bounded(40) - 20) / 10.0f;
        p.vy = -(2.0f + QRandomGenerator::global()->bounded(40) / 10.0f);
        p.life = 100 + QRandomGenerator::global()->bounded(155);
        p.size = 5.0f + QRandomGenerator::global()->bounded(10);
        particles.append(p);
    }
}

void AnimationWidget::timerEvent(QTimerEvent *) {
    frame++;

    // Update matrix
    for (auto &col : matrixColumns) {
        col.y += col.speed;
        if (col.y > height()) {
            col.y = -20;
            col.x = QRandomGenerator::global()->bounded(width());
        }
    }

    // Update particles
    for (auto &p : particles) {
        if (currentType == Starfield) {
            p.x += p.vx;
            p.y += p.vy;
            p.vx *= 1.05f;
            p.vy *= 1.05f;
            if (qAbs(p.x) > width() / 2 || qAbs(p.y) > height() / 2) {
                p.x = QRandomGenerator::global()->bounded(10) - 5;
                p.y = QRandomGenerator::global()->bounded(10) - 5;
                p.vx = p.x / 2.0f;
                p.vy = p.y / 2.0f;
            }
        } else if (currentType == Rain) {
            p.y += 15.0f;
            if (p.y > height()) {
                p.y = -20;
                p.x = QRandomGenerator::global()->bounded(width());
            }
        } else if (currentType == Snow) {
            p.y += 2.0f;
            p.x += qSin(frame * 0.1f + p.life) * 1.5f;
            if (p.y > height()) {
                p.y = -10;
                p.x = QRandomGenerator::global()->bounded(width());
            }
        } else if (currentType == Fire) {
            p.x += p.vx;
            p.y += p.vy;
            p.life -= 5;
            if (p.life <= 0) {
                p.x = QRandomGenerator::global()->bounded(width());
                p.y = height() + 10;
                p.vx = (QRandomGenerator::global()->bounded(40) - 20) / 10.0f;
                p.vy = -(2.0f + QRandomGenerator::global()->bounded(40) / 10.0f);
                p.life = 255;
            }
        } else {
            p.x += p.vx;
            p.y += p.vy;
            p.life--;

            if (p.life <= 0 || p.x < 0 || p.x > width() || p.y < 0 || p.y > height()) {
                p.x = QRandomGenerator::global()->bounded(width());
                p.y = QRandomGenerator::global()->bounded(height());
                p.vx = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
                p.vy = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
                p.life = 255;
            }
        }
    }

    update();
}

void AnimationWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (currentType) {
        case Matrix:
            drawMatrix(painter);
            break;
        case Particles:
            drawParticles(painter);
            break;
        case Waves:
            drawWaves(painter);
            break;
        case Pulse:
            drawPulse(painter);
            break;
        case Starfield:
            drawStarfield(painter);
            break;
        case Rain:
            drawRain(painter);
            break;
        case Snow:
            drawSnow(painter);
            break;
        case Fire:
            drawFire(painter);
            break;
        case DJMode:
            drawDJMode(painter);
            break;
        default:
            break;
    }
}

void AnimationWidget::drawStarfield(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    for (const auto &p : particles) {
        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(p.x + width()/2, p.y + height()/2), p.size, p.size);
    }
}

void AnimationWidget::drawRain(QPainter &painter) {
    painter.setPen(QPen(QColor(100, 149, 237, 150), 2));
    for (const auto &p : particles) {
        painter.drawLine(QPointF(p.x, p.y), QPointF(p.x, p.y + 10));
    }
}

void AnimationWidget::drawSnow(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 200));
    for (const auto &p : particles) {
        painter.drawEllipse(QPointF(p.x, p.y), 3, 3);
    }
}

void AnimationWidget::drawFire(QPainter &painter) {
    for (const auto &p : particles) {
        int r = 255;
        int g = qMax(0, 255 - (255 - p.life) * 2);
        int b = 0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(r, g, b, p.life));
        painter.drawEllipse(QPointF(p.x, p.y), p.size * p.life / 255.0f, p.size * p.life / 255.0f);
    }
}

void AnimationWidget::drawDJMode(QPainter &painter) {
    if (!audioMonitor) {
        // Draw a placeholder when no audio monitor is available
        painter.fillRect(rect(), QColor(10, 10, 15));
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(rect(), Qt::AlignCenter, "DJ Mode - No Audio Monitor");
        return;
    }

    QVector<float> levels = audioMonitor->getLevels();
    if (levels.isEmpty()) {
        painter.fillRect(rect(), QColor(10, 10, 15));
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(rect(), Qt::AlignCenter, "DJ Mode - No Audio Levels");
        return;
    }

    int numBars = levels.size();
    float barWidth = static_cast<float>(width()) / numBars;

    // Dark background
    painter.fillRect(rect(), QColor(10, 10, 15));

    // Draw frequency bars with smoothing
    for (int i = 0; i < numBars; ++i) {
        float level = levels[i];
        if (level < 0.01f) level = 0.01f; // Higher minimum visibility

        float h = level * height() * 0.9f; // Increased to 90% of widget height for better visibility
        if (h < 5.0f) h = 5.0f; // Minimum 5 pixels height so bars are always visible

        // Make bars wider by reducing gaps
        float barGap = 0.5f; // Smaller gap between bars
        QRectF barRect(i * barWidth + barGap/2, height() - h, barWidth - barGap, h);

        // Create smoother gradient from bottom to top
        QLinearGradient grad(barRect.bottomLeft(), barRect.topLeft());

        // More subtle color range - blue to purple
        float hue = 200.0f + (i / float(numBars)) * 60.0f; // Blue to purple range
        grad.setColorAt(0, QColor::fromHsvF(hue / 360.0f, 0.7f, 0.6f)); // Less saturated
        grad.setColorAt(0.7f, QColor::fromHsvF(hue / 360.0f, 0.8f, 0.8f));
        grad.setColorAt(1.0f, QColor::fromHsvF((hue + 30) / 360.0f, 0.9f, 0.9f));

        painter.fillRect(barRect, grad);

        // Subtle glow only for very high levels
        if (level > 0.8f) {
            painter.setPen(QPen(QColor::fromHsvF(hue / 360.0f, 0.3f, 1.0f, 0.3f), 1)); // Less intense glow
            painter.drawRect(barRect);
        }
    }

    // Gentle reflection at bottom
    painter.setOpacity(0.2f); // Reduced opacity for subtler reflection
    for (int i = 0; i < numBars; ++i) {
        float level = levels[i];
        if (level < 0.01f) continue;

        float h = level * height() * 0.2f; // Smaller reflection
        float barGap = 0.5f; // Same gap as main bars
        QRectF barRect(i * barWidth + barGap/2, 0, barWidth - barGap, h);

        float hue = 200.0f + (i / float(numBars)) * 60.0f;
        painter.fillRect(barRect, QColor::fromHsvF(hue / 360.0f, 0.6f, 0.5f));
    }
    painter.setOpacity(1.0f);
}

void AnimationWidget::drawMatrix(QPainter &painter) {
    painter.setFont(QFont("Consolas", 12));
    for (const auto &col : matrixColumns) {
        int alpha = 255;
        for (int i = 0; i < 10; i++) {
            painter.setPen(QColor(0, 255, 0, alpha));
            painter.drawText(col.x, col.y - i * 20, col.text);
            alpha = qMax(0, alpha - 30);
        }
    }
}

void AnimationWidget::drawParticles(QPainter &painter) {
    for (const auto &p : particles) {
        int alpha = qMin(255, p.life);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(100, 149, 237, alpha));
        painter.drawEllipse(QPointF(p.x, p.y), 2, 2);
    }
}

void AnimationWidget::drawWaves(QPainter &painter) {
    painter.setPen(QPen(QColor(0, 120, 215, 100), 2));

    for (int wave = 0; wave < 3; wave++) {
        QPainterPath path;
        bool first = true;
        for (int x = 0; x < width(); x += 5) {
            float y = height() / 2 + 50 * qSin((x + frame * 2 + wave * 100) * 0.02);
            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    }
}

void AnimationWidget::drawPulse(QPainter &painter) {
    int centerX = width() / 2;
    int centerY = height() / 2;

    for (int i = 0; i < 5; i++) {
        int radius = ((frame + i * 20) % 200);
        int alpha = 255 - (radius * 255 / 200);
        painter.setPen(QPen(QColor(100, 149, 237, alpha), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
    }
}
