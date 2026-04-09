#include "texteditor.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScrollBar>
#include <QGuiApplication>
#include <QScreen>

// ============================================================
// TitleBar Implementation
// ============================================================
TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 0, 0, 0);
  layout->setSpacing(0);

  // Logo / Title
  QLabel *icon = new QLabel("J");
  icon->setStyleSheet("color: #569cd6; font-weight: bold; font-family: "
                      "Consolas; font-size: 14px;");
  layout->addWidget(icon);

  layout->addSpacing(10);

  titleLabel = new QLabel("Jim");
  titleLabel->setStyleSheet(
      "color: #cccccc; font-size: 12px; font-family: 'Segoe UI', sans-serif;");
  titleLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(titleLabel, 1); // stretch

  // Window controls
  QString btnStyle =
      "QPushButton {"
      "    background-color: transparent;"
      "    color: #cccccc;"
      "    border: none;"
      "    width: 45px;"
      "    height: 30px;"
      "    font-family: 'Segoe MDL2 Assets', 'Segoe UI Symbol', sans-serif;"
      "    font-size: 10px;"
      "}"
      "QPushButton:hover {"
      "    background-color: #3e3e42;"
      "}";

  QString closeBtnStyle =
      "QPushButton {"
      "    background-color: transparent;"
      "    color: #cccccc;"
      "    border: none;"
      "    width: 45px;"
      "    height: 30px;"
      "    font-family: 'Segoe MDL2 Assets', 'Segoe UI Symbol', sans-serif;"
      "    font-size: 10px;"
      "}"
      "QPushButton:hover {"
      "    background-color: #e81123;"
      "    color: white;"
      "}";

  QPushButton *minBtn =
      new QPushButton(QString::fromUtf8("\xE2\x80\x94")); // Em dash
  minBtn->setStyleSheet(btnStyle);
  connect(minBtn, &QPushButton::clicked, this, [this]() {
    if (window())
      window()->showMinimized();
  });
  layout->addWidget(minBtn);

  QPushButton *maxBtn =
      new QPushButton(QString::fromUtf8("\xE2\x96\xA1")); // White square
  maxBtn->setStyleSheet(btnStyle);
  connect(maxBtn, &QPushButton::clicked, this, &TitleBar::toggleMaximized);
  layout->addWidget(maxBtn);

  QPushButton *closeBtn =
      new QPushButton(QString::fromUtf8("\xE2\x95\xB3")); // Cross
  closeBtn->setStyleSheet(closeBtnStyle);
  connect(closeBtn, &QPushButton::clicked, this, [this]() {
    if (window())
      window()->close();
  });
  layout->addWidget(closeBtn);

  setFixedHeight(30);
  setStyleSheet("background-color: #323233;");
}

void TitleBar::setTitle(const QString &title) { titleLabel->setText(title); }

void TitleBar::toggleMaximized() {
  if (!window())
    return;
  if (window()->isMaximized()) {
    window()->showNormal();
    qobject_cast<QPushButton *>(sender())->setText(
        QString::fromUtf8("\xE2\x96\xA1"));
  } else {
    window()->showMaximized();
    qobject_cast<QPushButton *>(sender())->setText(
        QString::fromUtf8("\xE2\x9D\x90")); // Maximize icon
  }
}

void TitleBar::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragStartPos =
        event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
    event->accept();
  }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    if (window()->isMaximized()) {
      window()->showNormal();
      dragStartPos = QPoint(window()->width() / 2, height() / 2);
    }
    window()->move(event->globalPosition().toPoint() - dragStartPos);
    event->accept();
  }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    toggleMaximized();
    event->accept();
  }
}
// SyntaxHighlighter implementation moved to syntaxhighlighter.cpp
// ============================================================
// WelcomeWidget Implementation
// ============================================================
WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent) { setupUI(); }

void WelcomeWidget::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setAlignment(Qt::AlignCenter);
  mainLayout->setSpacing(20);

  QLabel *logo = new QLabel("Jim");
  logo->setStyleSheet(
      "font-size: 64px; font-weight: 300; color: #569cd6; letter-spacing: 8px; "
      "font-family: 'Segoe UI', 'Consolas', monospace;");
  logo->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(logo);

  QLabel *subtitle = new QLabel("Lightweight Code Editor");
  subtitle->setStyleSheet("font-size: 16px; color: #808080; font-weight: 300; "
                          "letter-spacing: 2px; margin-bottom: 30px;");
  subtitle->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(subtitle);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setAlignment(Qt::AlignCenter);
  buttonLayout->setSpacing(16);

  QString btnStyle =
      "QPushButton { background-color: #0e639c; color: #ffffff; border: none; "
      "padding: 12px 28px; border-radius: 6px; font-size: 14px; font-weight: "
      "500; min-width: 140px; } QPushButton:hover { background-color: #1177bb; "
      "} QPushButton:pressed { background-color: #094771; }";

  QPushButton *openFileBtn = new QPushButton("Open File");
  openFileBtn->setStyleSheet(btnStyle);
  connect(openFileBtn, &QPushButton::clicked, this,
          &WelcomeWidget::openFileRequested);
  buttonLayout->addWidget(openFileBtn);

  QPushButton *openFolderBtn = new QPushButton("Open Folder");
  openFolderBtn->setStyleSheet(btnStyle);
  connect(openFolderBtn, &QPushButton::clicked, this,
          &WelcomeWidget::openFolderRequested);
  buttonLayout->addWidget(openFolderBtn);

  mainLayout->addLayout(buttonLayout);

  QLabel *recentLabel = new QLabel("Recent Files");
  recentLabel->setStyleSheet("font-size: 13px; color: #cccccc; font-weight: "
                             "600; margin-top: 30px; letter-spacing: 1px;");
  recentLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(recentLabel);

  recentFilesLayout = new QVBoxLayout();
  recentFilesLayout->setAlignment(Qt::AlignCenter);
  recentFilesLayout->setSpacing(4);
  mainLayout->addLayout(recentFilesLayout);
  mainLayout->addStretch();
  setStyleSheet("QWidget { background-color: #1e1e1e; }");
}

void WelcomeWidget::setRecentFiles(const QStringList &files) {
  QLayoutItem *item;
  while ((item = recentFilesLayout->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  int count = 0;
  for (const QString &file : files) {
    if (count >= 8)
      break;
    QPushButton *btn = new QPushButton(QFileInfo(file).fileName());
    btn->setToolTip(file);
    btn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #3794ff; border: "
        "none; padding: 6px 16px; font-size: 13px; text-align: center; "
        "border-radius: 4px; min-width: 200px; } QPushButton:hover { "
        "background-color: #2a2d2e; color: #58b0ff; }");
    QString filePath = file;
    connect(btn, &QPushButton::clicked, this,
            [this, filePath]() { emit recentFileClicked(filePath); });
    recentFilesLayout->addWidget(btn);
    count++;
  }
  if (files.isEmpty()) {
    QLabel *noFiles = new QLabel("No recent files");
    noFiles->setStyleSheet("color: #555555; font-size: 12px; padding: 8px;");
    noFiles->setAlignment(Qt::AlignCenter);
    recentFilesLayout->addWidget(noFiles);
  }
}

// ============================================================
// BreadcrumbBar Implementation
// ============================================================
BreadcrumbBar::BreadcrumbBar(QWidget *parent) : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 0, 12, 0);
  layout->setSpacing(8);

  iconLabel = new QLabel("\xF0\x9F\x93\x84"); // File icon
  iconLabel->setStyleSheet("color: #999999; font-size: 14px;");
  layout->addWidget(iconLabel);

  pathLabel = new QLabel("");
  pathLabel->setStyleSheet(
      "color: #808080; font-size: 12px; font-family: 'Segoe UI', sans-serif;");
  layout->addWidget(pathLabel);

  fileLabel = new QLabel("");
  fileLabel->setStyleSheet(
      "color: #cccccc; font-size: 12px; font-family: 'Segoe UI', sans-serif; font-weight: 500;");
  layout->addWidget(fileLabel);

  symbolLabel = new QLabel("");
  symbolLabel->setStyleSheet(
      "color: #dcdcaa; font-size: 12px; font-family: 'Consolas', monospace;");
  layout->addWidget(symbolLabel);

  layout->addStretch();
  setFixedHeight(30);
  setStyleSheet("QWidget { background-color: #252526; border-bottom: 1px solid "
                "#3e3e42; }");
}

void BreadcrumbBar::updatePath(const QString &filePath, const QString &symbol) {
  if (filePath.isEmpty()) {
    pathLabel->setText("");
    fileLabel->setText("Untitled");
    symbolLabel->setText("");
    return;
  }
  QFileInfo info(filePath);
  pathLabel->setText(info.absolutePath() + " > ");
  fileLabel->setText(info.fileName());

  if (!symbol.isEmpty()) {
    symbolLabel->setText(" > " + symbol);
    symbolLabel->show();
  } else {
    symbolLabel->setText("");
    symbolLabel->hide();
  }
}

// ============================================================
// FindBar Implementation
// ============================================================
FindBar::FindBar(QWidget *parent) : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 2, 10, 2);
  layout->setSpacing(10);

  findInput = new QLineEdit(this);
  findInput->setPlaceholderText("Find...");
  findInput->setStyleSheet("QLineEdit { background-color: #3c3c3c; color: #cccccc; border: 1px solid #555555; padding: 2px 5px; border-radius: 2px; }");
  layout->addWidget(findInput);

  matchLabel = new QLabel("0/0", this);
  matchLabel->setStyleSheet("color: #999999; font-size: 11px;");
  layout->addWidget(matchLabel);

  prevBtn = new QPushButton("\xE2\x86\x91", this); // Up arrow
  nextBtn = new QPushButton("\xE2\x86\x93", this); // Down arrow
  closeBtn = new QPushButton("\xE2\x9C\x95", this); // Close icon

  QString btnStyle = "QPushButton { background-color: transparent; color: #cccccc; border: none; padding: 2px 8px; font-size: 14px; } "
                      "QPushButton:hover { background-color: #4a4a4a; border-radius: 2px; }";
  prevBtn->setStyleSheet(btnStyle);
  nextBtn->setStyleSheet(btnStyle);
  closeBtn->setStyleSheet(btnStyle + " QPushButton:hover { background-color: #e81123; color: white; }");

  layout->addWidget(prevBtn);
  layout->addWidget(nextBtn);
  layout->addWidget(closeBtn);

  setFixedHeight(34);
  setStyleSheet("QWidget { background-color: #2d2d2d; border-bottom: 1px solid #3e3e42; border-left: 1px solid #3e3e42; }");

  connect(findInput, &QLineEdit::textChanged, this, &FindBar::textChanged);
  connect(findInput, &QLineEdit::returnPressed, this, [this]() { emit findNextRequested(findInput->text()); });
  connect(prevBtn, &QPushButton::clicked, this, [this]() { emit findPreviousRequested(findInput->text()); });
  connect(nextBtn, &QPushButton::clicked, this, [this]() { emit findNextRequested(findInput->text()); });
  connect(closeBtn, &QPushButton::clicked, this, &FindBar::closeRequested);

  hide();
}

void FindBar::showAndFocus(const QString &text) {
  if (!text.isEmpty()) findInput->setText(text);
  show();
  findInput->setFocus();
  findInput->selectAll();
}

void FindBar::setMatchCount(int current, int total) {
  if (total == 0) matchLabel->setText("No results");
  else matchLabel->setText(QString("%1/%2").arg(current).arg(total));
}

QString FindBar::getSearchText() const {
  return findInput->text();
}

// ============================================================
// TerminalWidget Implementation
// ============================================================
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
  termLabel->setStyleSheet("color: #cccccc; font-size: 11px; font-weight: 600; letter-spacing: 1px;");
  headerLayout->addWidget(termLabel);
  headerLayout->addStretch();

  header->setStyleSheet("background-color: #252526; border-bottom: 1px solid #3e3e42;");
  layout->addWidget(header);

  output = new QPlainTextEdit();
  output->setReadOnly(true);
  output->setFont(QFont("Consolas", 10));
  output->setStyleSheet(
      "QPlainTextEdit { background-color: #1e1e1e; color: #cccccc; border: none; "
      "padding: 8px; selection-background-color: #264f78; }");
  output->setMaximumBlockCount(5000);
  layout->addWidget(output);

  input = new QLineEdit();
  input->setFont(QFont("Consolas", 10));
  input->setStyleSheet(
      "QLineEdit { background-color: #1e1e1e; color: #cccccc; border: none; "
      "border-top: 1px solid #3e3e42; padding: 8px; } "
      "QLineEdit:focus { border-top: 1px solid #007acc; }");
  input->setPlaceholderText("Type command and press Enter...");
  connect(input, &QLineEdit::returnPressed, this, &TerminalWidget::executeCommand);
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
  if (cmd.isEmpty()) return;

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
    output->appendPlainText(QString::fromLocal8Bit(proc->readAllStandardOutput()));
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
