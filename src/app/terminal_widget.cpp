#include "terminal_widget.h"

#include <QDir>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QVBoxLayout>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent), process(nullptr) {
    setupUI();
}

TerminalWidget::~TerminalWidget() {
    if (process) {
        process->kill();
        process->waitForFinished(1000);
    }
}

void TerminalWidget::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *header = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 6, 12, 6);

    QLabel *termLabel = new QLabel("TERMINAL");
    termLabel->setStyleSheet("color: #cccccc; font-size: 11px; font-weight: 600; "
                             "letter-spacing: 1px;");
    headerLayout->addWidget(termLabel);
    headerLayout->addStretch();

    header->setStyleSheet("background-color: #252526; border-bottom: 1px solid "
                          "#3e3e42;");
    layout->addWidget(header);

    output = new QPlainTextEdit();
    output->setReadOnly(true);
    output->setFont(QFont("Consolas", 10));
    output->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #cccccc; border: "
        "none; padding: 8px; selection-background-color: #264f78; }");
    output->setMaximumBlockCount(5000);
    layout->addWidget(output);

    input = new QLineEdit();
    input->setFont(QFont("Consolas", 10));
    input->setStyleSheet(
        "QLineEdit { background-color: #1e1e1e; color: #cccccc; border: none; "
        "border-top: 1px solid #3e3e42; padding: 8px; } "
        "QLineEdit:focus { border-top: 1px solid #007acc; }");
    input->setPlaceholderText("Type command and press Enter...");
    connect(input, &QLineEdit::returnPressed, this,
            &TerminalWidget::executeCommand);
    layout->addWidget(input);

    currentDir = QDir::homePath();
    setStyleSheet("background-color: #1e1e1e;");
}

void TerminalWidget::setWorkingDirectory(const QString &dir) {
    QDir d(dir);
    if (d.exists()) {
        currentDir = d.absolutePath();
    }
}

void TerminalWidget::startShell() {}

void TerminalWidget::executeCommand() {
    QString cmd = input->text().trimmed();
    if (cmd.isEmpty()) {
        return;
    }

    output->appendPlainText("> " + cmd);
    input->clear();

    if (cmd == "clear" || cmd == "cls") {
        output->clear();
        return;
    }

    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(currentDir);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        output->appendPlainText(
            QString::fromLocal8Bit(proc->readAllStandardOutput()));
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc]() { proc->deleteLater(); });

#ifdef Q_OS_WIN
    proc->start("cmd.exe", QStringList() << "/c" << cmd);
#else
    proc->start("/bin/sh", QStringList() << "-c" << cmd);
#endif
}

void TerminalWidget::appendOutput(const QString &text) {
    output->appendPlainText(text);
}
