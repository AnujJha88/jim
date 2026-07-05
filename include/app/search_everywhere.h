#ifndef JIM_SEARCH_EVERYWHERE_H
#define JIM_SEARCH_EVERYWHERE_H

#include <QDialog>
#include <QStringList>

class QAction;
class QEvent;
class QKeyEvent;
class QLineEdit;
class QListWidget;

class SearchEverywhere : public QDialog {
    Q_OBJECT
public:
    explicit SearchEverywhere(QWidget *parent = nullptr);
    void populate(const QList<QAction*> &actions,
                  const QStringList &recentFiles,
                  const QStringList &openFiles);

signals:
    void fileRequested(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLineEdit *searchBox;
    QListWidget *resultList;
    QList<QAction*> allActions;
    QStringList allFiles;
    QStringList allRecent;

    void filter(const QString &text);
    void runSelected();
};

#endif
