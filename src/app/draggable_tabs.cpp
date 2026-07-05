#include "draggable_tabs.h"

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>

DraggableTabBar::DraggableTabBar(QWidget *parent) : QTabBar(parent) {
    setMovable(true);
}

void DraggableTabBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->pos();
    }
    QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        QTabBar::mouseMoveEvent(event);
        return;
    }
    if ((event->pos() - dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if (!rect().contains(event->pos())) {
        int tabIdx = tabAt(dragStartPos);
        if (tabIdx >= 0) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();

            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream << reinterpret_cast<quintptr>(parentWidget()) << tabIdx;
            mimeData->setData("application/x-jim-tab", data);
            drag->setMimeData(mimeData);

            QPixmap pixmap = grab(tabRect(tabIdx));
            drag->setPixmap(pixmap);
            drag->setHotSpot(event->pos() - tabRect(tabIdx).topLeft());

            drag->exec(Qt::MoveAction);
            return;
        }
    }
    QTabBar::mouseMoveEvent(event);
}

DraggableTabWidget::DraggableTabWidget(QWidget *parent) : QTabWidget(parent) {
    setTabBar(new DraggableTabBar(this));
    setAcceptDrops(true);
}

void DraggableTabWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-jim-tab")) {
        QByteArray data = event->mimeData()->data("application/x-jim-tab");
        QDataStream stream(&data, QIODevice::ReadOnly);
        quintptr sourcePtr;
        stream >> sourcePtr;
        if (reinterpret_cast<QWidget*>(sourcePtr) != this) {
            event->acceptProposedAction();
            return;
        }
    }
    QTabWidget::dragEnterEvent(event);
}

void DraggableTabWidget::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-jim-tab")) {
        QByteArray data = event->mimeData()->data("application/x-jim-tab");
        QDataStream stream(&data, QIODevice::ReadOnly);
        quintptr sourcePtr;
        int tabIdx;
        stream >> sourcePtr >> tabIdx;

        QTabWidget *sourceTabWidget = reinterpret_cast<QTabWidget*>(sourcePtr);
        if (sourceTabWidget && sourceTabWidget != this) {
            QWidget *page = sourceTabWidget->widget(tabIdx);
            QString text = sourceTabWidget->tabText(tabIdx);
            sourceTabWidget->removeTab(tabIdx);

            int insertIndex = tabBar()->tabAt(event->position().toPoint());
            if (insertIndex < 0) {
                insertIndex = count();
            }
            insertTab(insertIndex, page, text);
            setCurrentIndex(insertIndex);

            event->acceptProposedAction();
            return;
        }
    }
    QTabWidget::dropEvent(event);
}
