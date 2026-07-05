#ifndef JIM_ANIMATIONWIDGET_H
#define JIM_ANIMATIONWIDGET_H

#include <QString>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>

class AudioMonitor;
class QPainter;
class QPaintEvent;
class QTimerEvent;

class AnimationWidget : public QWidget {
    Q_OBJECT
public:
    enum AnimationType {
        None,
        Matrix,
        Particles,
        Waves,
        Pulse,
        Starfield,
        Rain,
        Snow,
        Fire,
        DJMode
    };

    explicit AnimationWidget(QWidget *parent = nullptr);
    void setAnimationType(AnimationType type);
    void cycleAnimation();
    AnimationType getCurrentType() const { return currentType; }
    void setAudioMonitor(AudioMonitor *monitor) { audioMonitor = monitor; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    AudioMonitor *audioMonitor = nullptr;
    AnimationType currentType;
    int timerId;
    int frame;

    struct Particle {
        float x, y, vx, vy;
        int life;
        float size;
    };
    QVector<Particle> particles;

    struct MatrixColumn {
        int x, y, speed;
        QString text;
    };
    QVector<MatrixColumn> matrixColumns;

    void initMatrix();
    void initParticles();
    void initStarfield();
    void initFire();

    void drawMatrix(QPainter &painter);
    void drawParticles(QPainter &painter);
    void drawWaves(QPainter &painter);
    void drawPulse(QPainter &painter);
    void drawStarfield(QPainter &painter);
    void drawRain(QPainter &painter);
    void drawSnow(QPainter &painter);
    void drawFire(QPainter &painter);
    void drawDJMode(QPainter &painter);
};

class DJVisualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit DJVisualizerWidget(QWidget *parent = nullptr);
    ~DJVisualizerWidget();

    void setAudioMonitor(AudioMonitor *monitor);
    AnimationWidget *visualizerWidget;

private:
    QVBoxLayout *layout;
};

#endif
