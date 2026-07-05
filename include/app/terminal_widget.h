#ifndef JIM_TERMINAL_WIDGET_H
#define JIM_TERMINAL_WIDGET_H

#include <QWidget>

class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QString;

class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget();
    void setWorkingDirectory(const QString &dir);

private slots:
    void executeCommand();

private:
    QPlainTextEdit *output;
    QLineEdit *input;
    QProcess *process;
    QString currentDir;

    void setupUI();
    void startShell();
    void appendOutput(const QString &text);
};

#endif
