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
#include <QTreeWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTabWidget>

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

// ============================================================
// Command Palette
// ============================================================
static const QString kPaletteStyle =
    "QDialog { background:#1e1e2e; border:1px solid #45475a; border-radius:8px; }"
    "QLineEdit { background:#313244; color:#cdd6f4; border:none; border-radius:4px;"
    "            padding:8px 12px; font-family:Consolas,monospace; font-size:13px; }"
    "QListWidget { background:#1e1e2e; color:#cdd6f4; border:none;"
    "              font-family:Consolas,monospace; font-size:12px; outline:none; }"
    "QListWidget::item { padding:6px 12px; border-radius:4px; }"
    "QListWidget::item:selected { background:#313244; color:#89b4fa; }"
    "QListWidget::item:hover { background:#2a2a3e; }";

CommandPalette::CommandPalette(QWidget *parent) : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup) {
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(560);
    setStyleSheet(kPaletteStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Type a command...");
    searchBox->installEventFilter(this);
    root->addWidget(searchBox);

    resultList = new QListWidget(this);
    resultList->setMaximumHeight(340);
    resultList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    resultList->setFocusProxy(searchBox);
    root->addWidget(resultList);

    connect(searchBox, &QLineEdit::textChanged, this, &CommandPalette::filter);
    connect(resultList, &QListWidget::itemActivated, this, [this](QListWidgetItem *) {
        runSelected();
    });
}

void CommandPalette::populate(const QList<QAction*> &actions) {
    allActions = actions;
    filter(QString());
    if (resultList->count() > 0)
        resultList->setCurrentRow(0);
}

void CommandPalette::filter(const QString &text) {
    resultList->clear();
    for (QAction *act : allActions) {
        QString label = act->text().remove('&').trimmed();
        if (label.isEmpty() || !act->isEnabled()) continue;
        if (text.isEmpty() || label.contains(text, Qt::CaseInsensitive)) {
            auto *item = new QListWidgetItem(resultList);
            // Show shortcut on the right if available
            QString sc = act->shortcut().toString(QKeySequence::NativeText);
            item->setText(label + (sc.isEmpty() ? "" : "   " + sc));
            item->setData(Qt::UserRole, QVariant::fromValue(act));
            resultList->addItem(item);
        }
    }
    if (resultList->count() > 0)
        resultList->setCurrentRow(0);
    // Resize height to content
    int rows = qMin(resultList->count(), 12);
    resultList->setMaximumHeight(rows * 30 + 8);
    adjustSize();
}

void CommandPalette::runSelected() {
    QListWidgetItem *item = resultList->currentItem();
    if (!item) return;
    auto *act = item->data(Qt::UserRole).value<QAction*>();
    if (act && act->isEnabled()) {
        accept();
        act->trigger();
    }
}

void CommandPalette::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) { reject(); return; }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) { runSelected(); return; }
    if (event->key() == Qt::Key_Down) {
        int next = resultList->currentRow() + 1;
        if (next < resultList->count()) resultList->setCurrentRow(next);
        return;
    }
    if (event->key() == Qt::Key_Up) {
        int prev = resultList->currentRow() - 1;
        if (prev >= 0) resultList->setCurrentRow(prev);
        return;
    }
    QDialog::keyPressEvent(event);
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event) {
    if (obj == searchBox && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Up ||
            ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
            ke->key() == Qt::Key_Escape) {
            keyPressEvent(ke);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

// ============================================================
// TODO/FIXME Panel
// ============================================================
static const QString kTagColors[] = {
    "#f38ba8", // TODO  — red
    "#fab387", // FIXME — peach
    "#f9e2af", // HACK  — yellow
    "#89b4fa", // NOTE  — blue
    "#ff5555", // BUG   — bright red
};
static const QString kTags[] = { "TODO", "FIXME", "HACK", "NOTE", "BUG" };

TodoPanel::TodoPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar
    auto *header = new QWidget(this);
    header->setFixedHeight(32);
    header->setStyleSheet("background:#252526; border-bottom:1px solid #3c3c3c;");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(8, 0, 8, 0);
    auto *title = new QLabel("TODO / FIXME", header);
    title->setStyleSheet("color:#cccccc; font-family:Consolas; font-size:11px; font-weight:bold;");
    hbox->addWidget(title);
    hbox->addStretch();
    auto *refreshBtn = new QPushButton("↻", header);
    refreshBtn->setFixedSize(24, 24);
    refreshBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#cccccc; border:none; font-size:14px; }"
        "QPushButton:hover { color:#ffffff; }");
    hbox->addWidget(refreshBtn);
    layout->addWidget(header);

    tree = new QTreeWidget(this);
    tree->setColumnCount(4);
    tree->setHeaderLabels({"Tag", "File", "Line", "Text"});
    tree->setStyleSheet(
        "QTreeWidget { background:#1e1e1e; color:#cccccc; border:none;"
        "              font-family:Consolas,monospace; font-size:11px; outline:none; }"
        "QTreeWidget::item { padding:3px 4px; }"
        "QTreeWidget::item:selected { background:#094771; }"
        "QTreeWidget::item:hover { background:#2a2d2e; }"
        "QHeaderView::section { background:#252526; color:#888; border:none;"
        "                       border-bottom:1px solid #3c3c3c; padding:4px; }");
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    tree->setRootIsDecorated(false);
    tree->setSortingEnabled(true);
    tree->sortByColumn(1, Qt::AscendingOrder);
    layout->addWidget(tree);

    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        // Trigger a re-scan from outside — parent will call scan() again
        // We emit a dummy signal by re-emitting the last item click to force parent rescan
        emit jumpRequested(QString(), -1);
    });

    connect(tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        int line = item->data(1, Qt::UserRole).toInt();
        if (!filePath.isEmpty() && line >= 0)
            emit jumpRequested(filePath, line);
    });

    setStyleSheet("background:#1e1e1e;");
}

void TodoPanel::scan(QTabWidget *tabs) {
    tree->clear();
    static QRegularExpression tagRe(
        R"(\b(TODO|FIXME|HACK|NOTE|BUG)\b[:\s]?\s*(.*))",
        QRegularExpression::CaseInsensitiveOption);

    for (int i = 0; i < tabs->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabs->widget(i));
        if (!ed) continue;
        QString filePath = ed->getFileName();
        QString displayName = filePath.isEmpty() ? tabs->tabText(i) : QFileInfo(filePath).fileName();
        QTextDocument *doc = ed->document();
        for (QTextBlock blk = doc->begin(); blk != doc->end(); blk = blk.next()) {
            QRegularExpressionMatch m = tagRe.match(blk.text());
            if (!m.hasMatch()) continue;
            QString tag  = m.captured(1).toUpper();
            QString text = m.captured(2).trimmed();
            int lineNum  = blk.blockNumber() + 1;

            auto *item = new QTreeWidgetItem(tree);
            item->setText(0, tag);
            item->setText(1, displayName);
            item->setText(2, QString::number(lineNum));
            item->setText(3, text);
            item->setData(0, Qt::UserRole, filePath);
            item->setData(1, Qt::UserRole, lineNum - 1); // 0-based for block navigation

            // Colour the tag column
            for (int t = 0; t < 5; ++t) {
                if (kTags[t] == tag) {
                    item->setForeground(0, QColor(kTagColors[t]));
                    break;
                }
            }
            item->setForeground(2, QColor("#888888"));
            item->setForeground(3, QColor("#a0a0a0"));
        }
    }
}
