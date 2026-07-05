#ifndef JIM_WELCOME_WIDGET_H
#define JIM_WELCOME_WIDGET_H

#include <QStringList>
#include <QWidget>

class QString;
class QVBoxLayout;

class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeWidget(QWidget *parent = nullptr);
    void setRecentFiles(const QStringList &files);

signals:
    void openFileRequested();
    void openFolderRequested();
    void recentFileClicked(const QString &filePath);

private:
    QVBoxLayout *recentFilesLayout;
    void setupUI();
};

#endif
