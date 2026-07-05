#ifndef JIM_DRAGGABLE_TABS_H
#define JIM_DRAGGABLE_TABS_H

#include <QPoint>
#include <QTabBar>
#include <QTabWidget>

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;

class DraggableTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit DraggableTabBar(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint dragStartPos;
};

class DraggableTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit DraggableTabWidget(QWidget *parent = nullptr);

signals:
    void tabDroppedOutside();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif
