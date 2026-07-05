#ifndef JIM_TODO_PANEL_H
#define JIM_TODO_PANEL_H

#include <QWidget>

class QTabWidget;
class QTreeWidget;

class TodoPanel : public QWidget {
    Q_OBJECT
public:
    explicit TodoPanel(QWidget *parent = nullptr);
    void scan(QTabWidget *tabs);

signals:
    void jumpRequested(const QString &filePath, int line);

private:
    QTreeWidget *tree;
};

#endif
