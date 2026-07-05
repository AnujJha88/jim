#ifndef JIM_COMMAND_PALETTE_H
#define JIM_COMMAND_PALETTE_H

#include <QDialog>
#include <QList>

class QAction;
class QEvent;
class QKeyEvent;
class QLineEdit;
class QListWidget;
class QWidget;

class CommandPalette : public QDialog {
    Q_OBJECT
public:
    explicit CommandPalette(QWidget *parent = nullptr);
    void populate(const QList<QAction*> &actions);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLineEdit *searchBox;
    QListWidget *resultList;
    QList<QAction*> allActions;

    void filter(const QString &text);
    void runSelected();
};

#endif
