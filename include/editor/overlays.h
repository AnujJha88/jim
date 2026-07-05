#ifndef JIM_OVERLAYS_H
#define JIM_OVERLAYS_H

#include <QString>
#include <QPixmap>
#include <QVector>
#include <QWidget>

class QListWidget;
class QPaintEvent;
class QTimerEvent;

class GraveyardWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraveyardWidget(QWidget *parent = nullptr);
    void addSnippet(const QString &code, const QString &source);

signals:
    void resurrectRequested(const QString &code);

private:
    QListWidget *listWidget;
    QVector<QString> snippets;
};

class CRTOverlay : public QWidget {
public:
    explicit CRTOverlay(QWidget *parent = nullptr);
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_enabled = false;
};

class LaserParticleOverlay : public QWidget {
    Q_OBJECT
public:
    explicit LaserParticleOverlay(QWidget *parent = nullptr);
    void spawnSlash(int yPos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    struct Spark { float x, y, vx, vy, life; };
    QVector<Spark> sparks;
    float slashAlpha = 0.f;
    int slashY = 0;
    int timerId = 0;
};

class GlitchOverlay : public QWidget {
    Q_OBJECT
public:
    explicit GlitchOverlay(QWidget *parent = nullptr);
    void trigger(const QPixmap &snapshot);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    QPixmap originalSnapshot;
    float glitchTime = 0.f;
    int timerId = 0;
};

class HUDWidget : public QWidget {
    Q_OBJECT
public:
    explicit HUDWidget(QWidget *parent = nullptr);
    void addKeystroke();

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    QVector<float> cpuHistory;
    QVector<float> memHistory;
    quint64 hexCounter = 0xDEADBEEF00000000ULL;
    float readCpuUsage();
    float readMemUsage();
    quint64 prevIdle = 0;
    quint64 prevTotal = 0;
    QVector<qint64> keystrokeTimestamps;
    int currentWPM = 0;
};

#endif
