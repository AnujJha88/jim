#ifndef CITYSCAPE_WIDGET_H
#define CITYSCAPE_WIDGET_H

#include <QWidget>
#include <QString>
#include <QVector>
#include <QColor>
#include <QMap>

class CityscapeWidget : public QWidget {
    Q_OBJECT
public:
    explicit CityscapeWidget(QWidget *parent = nullptr);
    void loadDirectory(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct Building {
        QString name;
        QString path;
        int linesOfCode;
        float height;
        int gridX;
        int gridY;
        QColor color;
    };
    QVector<Building> buildings;
    QString currentPath;
    float scale;
    float panX;
    float panY;
    QPoint lastMousePos;
    int hoveredIndex;

    void scanDir(const QString &dirPath, int &gx, int &gy, int maxGx);
    void drawIsometricCube(QPainter &p, float x, float y, float w, float h, float d, const QColor &color);
};

#endif // CITYSCAPE_WIDGET_H
