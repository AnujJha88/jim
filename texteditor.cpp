#include "texteditor.h"
#include "syntaxhighlighter.h"
#include "keyheatmap.h"
#include "vimmode.h"
#include "hexeditor.h"
#include "disassembler.h"
#include "binaryinspector.h"
#include "markdownviewer.h"
#include "linenumberarea.h"
#include "aiautocomplete.h"
#include "aisettingsdialog.h"
#include "codegraph.h"
#include "solidityanalyzer.h"
#include "storageslotvisualizer.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QTableWidget>
#include <QNetworkRequest>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFontDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QSoundEffect>
#include "audiomonitor.h"
#include <QPainterPath>
#include <QProcessEnvironment>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextBlock>
#include <QDesktopServices>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVariantAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QTabBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDrag>
#include <QMimeData>
#include <QRadialGradient>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QFontDatabase>
#include <cmath>
#include <algorithm>
#include <functional>

// ============================================================
// Language Auto-Detection
// ============================================================
Language TextEditor::detectLanguage(const QString &fileName) {
  QString ext = QFileInfo(fileName).suffix().toLower();
  if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c" || ext == "h" ||
      ext == "hpp" || ext == "hxx")
    return Language::CPP;
  if (ext == "py" || ext == "pyw" || ext == "pyi")
    return Language::Python;
  if (ext == "js" || ext == "jsx" || ext == "mjs" || ext == "ts" ||
      ext == "tsx")
    return Language::JavaScript;
  if (ext == "html" || ext == "htm" || ext == "xml" || ext == "svg")
    return Language::HTML;
  if (ext == "css" || ext == "scss" || ext == "sass" || ext == "less")
    return Language::CSS;
  if (ext == "rs")
    return Language::Rust;
  if (ext == "go")
    return Language::Go;
  if (ext == "json" || ext == "jsonc")
    return Language::JSON;
  if (ext == "yaml" || ext == "yml")
    return Language::YAML;
  if (ext == "md" || ext == "markdown" || ext == "mkd")
    return Language::Markdown;
  if (ext == "sol")
    return Language::Solidity;
  if (ext == "yul")
    return Language::Yul;
  return Language::PlainText;
}

// DJVisualizerWidget implementation → animationwidget.cpp

// CodeEditor implementation → codeeditor.cpp

// TitleBar / WelcomeWidget / BreadcrumbBar / FindBar implementation → uiwidgets.cpp

// AnimationWidget implementation → animationwidget.cpp

// GraveyardWidget / CRTOverlay / LaserParticleOverlay / HUDWidget implementation → overlays.cpp


// ============================================================
// Drag and Drop Split Panes Implementation
// ============================================================

DraggableTabBar::DraggableTabBar(QWidget *parent) : QTabBar(parent) {
    setMovable(true); // Allow internal dragging by default
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
    
    // Check if the drag is moving outside the tab bar boundaries significantly
    if (!rect().contains(event->pos())) {
        int tabIdx = tabAt(dragStartPos);
        if (tabIdx >= 0) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();
            
            // Store the source widget pointer and tab index in mime data
            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream << reinterpret_cast<quintptr>(parentWidget()) << tabIdx;
            mimeData->setData("application/x-jim-tab", data);
            drag->setMimeData(mimeData);
            
            // Set drag pixmap
            QPixmap pixmap = grab(tabRect(tabIdx));
            drag->setPixmap(pixmap);
            drag->setHotSpot(event->pos() - tabRect(tabIdx).topLeft());
            
            Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
            if (dropAction == Qt::MoveAction) {
                // Drop was successful outside, we don't need to do anything here
                // as the receiving widget will handle it.
            }
            return; // We consumed the drag
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
        // Only accept if it comes from a different tab widget
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
            if (insertIndex < 0) insertIndex = count();
            insertTab(insertIndex, page, text);
            setCurrentIndex(insertIndex);
            
            event->acceptProposedAction();
            return;
        }
    }
    QTabWidget::dropEvent(event);
}

// TerminalWidget implementation → uiwidgets.cpp

// ============================================================
// TextEditor Implementation (Main Window)
// ============================================================
TextEditor::TextEditor(QWidget *parent)
    : QMainWindow(parent), wordWrapEnabled(false), splitViewEnabled(false),
      fontSize(11), currentThemeIndex(0), currentMatchIndex(-1) {
  fileWatcher = new QFileSystemWatcher(this);
  connect(fileWatcher, &QFileSystemWatcher::fileChanged, this,
          &TextEditor::onFileChangedExternally);

  // Frameless window with custom title bar
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                 Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint |
                 Qt::WindowCloseButtonHint);

  setupUI();
  initializeThemes();
  createActions();
  createMenus();
  createStatusBar();
  applyModernStyle();
  readSettings();
  setWindowTitle("Jim");
  resize(1200, 800);
  showWelcomeScreen();
  // Install event filter for double-Shift Search Everywhere detection
  qApp->installEventFilter(this);
}

TextEditor::~TextEditor() { writeSettings(); }

void TextEditor::setupUI() {
  // Main layout container (since QMainWindow needs a central widget)
  QWidget *mainContainer = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(1, 1, 1, 1); // 1px border for frameless
  mainLayout->setSpacing(0);

  titleBar = new TitleBar(this);
  mainLayout->addWidget(titleBar);

  // Explicitly place a custom menu bar inside our custom layout so it renders
  // below the title bar
  customMenuBar = new QMenuBar(mainContainer);
  customMenuBar->setStyleSheet(
      "QMenuBar { background-color: transparent; color: #cccccc; } "
      "QMenuBar::item:selected { background-color: #3e3e42; }");
  mainLayout->addWidget(customMenuBar);

  // Vertical splitter: top = editor area, bottom = terminal
  verticalSplitter = new QSplitter(Qt::Vertical, mainContainer);
  verticalSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  verticalSplitter->setOpaqueResize(false); // Show live preview while dragging

  // Container for breadcrumb + editor
  QWidget *editorContainer = new QWidget();
  editorContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QVBoxLayout *editorLayout = new QVBoxLayout(editorContainer);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);

  // Breadcrumb bar
  breadcrumbBar = new BreadcrumbBar();
  editorLayout->addWidget(breadcrumbBar);

  // Find bar (overlay/top of editor)
  findBar = new FindBar(this);
  editorLayout->addWidget(findBar);

  // Main horizontal splitter for editor tabs
  mainSplitter = new QSplitter(Qt::Horizontal);
  tabWidget = new DraggableTabWidget();
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);
  tabWidget->setDocumentMode(true);
  mainSplitter->addWidget(tabWidget);
  tabWidget2 = nullptr;

  connect(tabWidget, &QTabWidget::tabCloseRequested, this,
          &TextEditor::closeTab);
  connect(tabWidget, &QTabWidget::currentChanged, this,
          &TextEditor::tabChanged);
  
  aiAutocomplete = new AIAutocomplete(this);
  connect(aiAutocomplete, &AIAutocomplete::suggestionReady, this, &TextEditor::onAISuggestion);
  
  connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
      if (index >= 0) {
          CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(index));
          if (ed) {
              connect(ed, &QPlainTextEdit::textChanged, this, [this, ed]() {
                  aiAutocomplete->trigger(ed);
              }, Qt::UniqueConnection);
          }
      }
  });

  editorLayout->addWidget(mainSplitter);
  verticalSplitter->addWidget(editorContainer);

  // Welcome widget (stacked on top of editor)
  welcomeWidget = new WelcomeWidget();
  connect(welcomeWidget, &WelcomeWidget::openFileRequested, this,
          &TextEditor::openFile);
  connect(welcomeWidget, &WelcomeWidget::openFolderRequested, this,
          &TextEditor::openFolder);
  connect(welcomeWidget, &WelcomeWidget::recentFileClicked, this,
          [this](const QString &path) { loadFile(path); });

  // Terminal widget (hidden by default)
  terminalWidget = new TerminalWidget();
  terminalWidget->setMinimumHeight(100);
  terminalWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  terminalWidget->hide();
  verticalSplitter->addWidget(terminalWidget);
  verticalSplitter->setStretchFactor(0, 3);
  verticalSplitter->setStretchFactor(1, 1);
  verticalSplitter->setHandleWidth(6);
  verticalSplitter->setChildrenCollapsible(false);
  
  // Set initial sizes for the splitter (70% editor, 30% terminal)
  QList<int> sizes;
  sizes << 700 << 300;
  verticalSplitter->setSizes(sizes);

  mainLayout->addWidget(verticalSplitter, 1); // stretch to fill
  
  setCentralWidget(mainContainer);

  // Animation widget as a dockable pane
  animationDock = new QDockWidget("Animation", this);
  animationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  animationWidget = new AnimationWidget(animationDock);
  animationDock->setWidget(animationWidget);
  animationDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  animationDock->setMinimumWidth(300); // Increased initial width
  addDockWidget(Qt::RightDockWidgetArea, animationDock);
  animationDock->hide();

  // File tree dock
  fileTreeDock = new QDockWidget("Explorer", this);
  fileTreeDock->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetClosable);

  fileTreeContainer = new QStackedWidget();

  // 0: Empty state
  emptyTreeWidget = new QWidget();
  QVBoxLayout *emptyLayout = new QVBoxLayout(emptyTreeWidget);
  emptyLayout->setAlignment(Qt::AlignCenter);
  QLabel *emptyDesc = new QLabel("You have not yet opened a folder.");
  emptyDesc->setStyleSheet("color: #cccccc; font-size: 13px;");
  emptyDesc->setWordWrap(true);
  emptyDesc->setAlignment(Qt::AlignCenter);
  QPushButton *openFolderBtn = new QPushButton("Open Folder");
  openFolderBtn->setStyleSheet(
      "QPushButton { background-color: #0e639c; color: white; border: none; "
      "padding: 6px 12px; border-radius: 2px; } QPushButton:hover { "
      "background-color: #1177bb; }");
  connect(openFolderBtn, &QPushButton::clicked, this, &TextEditor::openFolder);
  emptyLayout->addWidget(emptyDesc);
  emptyLayout->addWidget(openFolderBtn);
  fileTreeContainer->addWidget(emptyTreeWidget);

  // 1: File tree state
  fileTree = new QTreeView();
  fileSystemModel = new QFileSystemModel();
  fileSystemModel->setFilter(QDir::AllDirs | QDir::Files |
                             QDir::NoDotAndDotDot);
  fileTree->setModel(fileSystemModel);
  fileTree->setColumnWidth(0, 250);
  fileTree->setHeaderHidden(false);
  fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  fileTree->setAnimated(true);
  fileTree->setIndentation(20);
  fileTree->setSortingEnabled(true);
  for (int i = 1; i < fileSystemModel->columnCount(); ++i)
    fileTree->hideColumn(i);
  fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(fileTree, &QTreeView::doubleClicked, this,
          &TextEditor::onFileTreeDoubleClicked);
  connect(fileTree, &QTreeView::customContextMenuRequested, this,
          &TextEditor::onFileTreeContextMenu);
  fileTreeContainer->addWidget(fileTree);

  // Set default state to Empty (0)
  fileTreeContainer->setCurrentIndex(0);

  fileTreeDock->setWidget(fileTreeContainer);
  addDockWidget(Qt::LeftDockWidgetArea, fileTreeDock);

  // Code Graveyard dock
  graveyardDock = new QDockWidget("Deleted Code", this);
  graveyardDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  graveyardWidget = new GraveyardWidget(graveyardDock);
  graveyardDock->setWidget(graveyardWidget);
  graveyardDock->setMinimumWidth(240);
  graveyardDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  addDockWidget(Qt::RightDockWidgetArea, graveyardDock);
  graveyardDock->hide();
  connect(graveyardWidget, &GraveyardWidget::resurrectRequested,
          this, [this](const QString &code) {
              CodeEditor *ed = currentEditor();
              if (ed) {
                  QTextCursor c = ed->textCursor();
                  c.insertText(code);
                  ed->setTextCursor(c);
                  flashStatusMessage("Pasted from deleted code history", QColor("#4ec9b0"), 2500);
              }
          });

  // Cybernetic HUD widget in status bar
  hudWidget = new HUDWidget(statusBar());
  statusBar()->addPermanentWidget(hudWidget);

  // Keystroke heatmap overlay (child of main window, centred on show)
  keyHeatmap = new KeyHeatmapOverlay(this);
  keyHeatmap->hide();

  // Ambient time-of-day theme tinting — fires immediately then every 10 min
  ambientTimer = new QTimer(this);
  ambientTimer->setInterval(10 * 60 * 1000);
  connect(ambientTimer, &QTimer::timeout, this, &TextEditor::updateAmbientTheme);
  ambientTimer->start();
  // Initial application after a short delay (editors not yet created)
  QTimer::singleShot(500, this, &TextEditor::updateAmbientTheme);
}

void TextEditor::showWelcomeScreen() {
  welcomeWidget->setRecentFiles(recentFiles);
  int idx = tabWidget->indexOf(welcomeWidget);
  if (idx == -1)
    idx = tabWidget->addTab(welcomeWidget, "Welcome");
  tabWidget->setCurrentIndex(idx);

  // Fade in
  if (!welcomeOpacity) {
    welcomeOpacity = new QGraphicsOpacityEffect(welcomeWidget);
    welcomeWidget->setGraphicsEffect(welcomeOpacity);
  }
  welcomeOpacity->setOpacity(0.0);
  auto *anim = new QPropertyAnimation(welcomeOpacity, "opacity", this);
  anim->setDuration(350);
  anim->setStartValue(0.0);
  anim->setEndValue(1.0);
  anim->setEasingCurve(QEasingCurve::OutCubic);
  anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void TextEditor::hideWelcomeScreen() {
  for (int i = 0; i < tabWidget->count(); ++i) {
    if (tabWidget->widget(i) == welcomeWidget) {
      // Quick fade out then remove
      if (!welcomeOpacity) {
        welcomeOpacity = new QGraphicsOpacityEffect(welcomeWidget);
        welcomeWidget->setGraphicsEffect(welcomeOpacity);
      }
      auto *anim = new QPropertyAnimation(welcomeOpacity, "opacity", this);
      anim->setDuration(180);
      anim->setStartValue(1.0);
      anim->setEndValue(0.0);
      anim->setEasingCurve(QEasingCurve::InCubic);
      int capturedIndex = i;
      connect(anim, &QPropertyAnimation::finished, this, [this, capturedIndex]() {
        // Re-check the index in case tabs changed during animation
        for (int j = 0; j < tabWidget->count(); ++j) {
          if (tabWidget->widget(j) == welcomeWidget) {
            tabWidget->removeTab(j);
            break;
          }
        }
        Q_UNUSED(capturedIndex)
      });
      anim->start(QAbstractAnimation::DeleteWhenStopped);
      return;
    }
  }
}

void TextEditor::watchFile(const QString &filePath) {
  if (paranoiaMode) return;
  if (!filePath.isEmpty() && QFileInfo::exists(filePath))
    fileWatcher->addPath(filePath);
}

void TextEditor::unwatchFile(const QString &filePath) {
  if (!filePath.isEmpty() && fileWatcher->files().contains(filePath))
    fileWatcher->removePath(filePath);
}

void TextEditor::onFileChangedExternally(const QString &path) {
  // Find the editor with this file
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor && editor->getFileName() == path) {
      QMessageBox::StandardButton reply = QMessageBox::question(
          this, "File Changed",
          QString("The file '%1' has been modified externally.\nDo you want to "
                  "reload it?")
              .arg(QFileInfo(path).fileName()),
          QMessageBox::Yes | QMessageBox::No);
      if (reply == QMessageBox::Yes) {
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
          int cursorPos = editor->textCursor().position();
          editor->setPlainText(QTextStream(&file).readAll());
          editor->document()->setModified(false);
          QTextCursor cursor = editor->textCursor();
          cursor.setPosition(
              qMin(cursorPos, editor->document()->characterCount() - 1));
          editor->setTextCursor(cursor);
        }
      }
      // Re-watch (Qt removes paths after change signal)
      watchFile(path);
      break;
    }
  }
}

QString TextEditor::detectCurrentSymbol(CodeEditor *editor) {
  if (!editor)
    return "";
  QTextBlock block = editor->textCursor().block();
  
  // For C++, walk up to find the closest scopes
  Language lang = editor->getLanguage();
  
  QRegularExpression funcRe;
  QRegularExpression classRe("(?:class|struct|enum|interface|trait|impl|namespace)\\s+([A-Za-z_][A-Za-z0-9_]*)");

  if (lang == Language::CPP) {
      // Improved C++ regex: captures return type (optional), scope (optional), and function name
      // Handles: void Class::Func(), int* ptr(), std::vector<int> some_func(), etc.
      // We make the trailing brace optional to handle "brace on next line"
      funcRe = QRegularExpression("(?xi)"
                                  "(?: [A-Za-z_][A-Za-z0-9_<>:\\*&\\s]* \\s+ )?" // Return type (optional)
                                  " ( [A-Za-z_][A-Za-z0-9_\\s]* :: )? "           // Class/Namespace scope (optional)
                                  " ( [A-Za-z_][A-Za-z0-9_]* ) "                  // Function name
                                  " \\s* \\( [^\\)]* \\) "                       // Parameters
                                  " \\s* (?: const )? \\s* (?: [{;]|$) ");        // End of signature (brace, semi, or EOL)
  } else if (lang == Language::Python) {
      funcRe = QRegularExpression("^\\s*def\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(");
      classRe = QRegularExpression("^\\s*class\\s+([A-Za-z_][A-Za-z0-9_]*)");
  } else {
      funcRe = QRegularExpression("(?:fn|func|def|function)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(");
  }

  // Keywords to exclude in C++
  static const QStringList cppKeywords = {"if", "while", "for", "switch", "catch", "else", "foreach"};

  while (block.isValid()) {
    QString text = block.text().trimmed();
    if (text.isEmpty()) {
        block = block.previous();
        continue;
    }

    QRegularExpressionMatch m;
    
    if (lang == Language::CPP) {
        m = funcRe.match(text);
        if (m.hasMatch()) {
            QString name = m.captured(2);
            // Verify it's not a control flow keyword
            if (!cppKeywords.contains(name)) {
                QString scope = m.captured(1);
                return (scope.isEmpty() ? "" : scope) + name + "()";
            }
        }
    } else {
        m = funcRe.match(text);
        if (m.hasMatch()) return m.captured(1) + "()";
    }
    
    m = classRe.match(text);
    if (m.hasMatch()) return m.captured(1);
    
    block = block.previous();
  }
  return "";
}

void TextEditor::updateBreadcrumb() {
  CodeEditor *editor = currentEditor();
  if (!editor) {
    breadcrumbBar->updatePath("", "");
    return;
  }
  QString symbol = detectCurrentSymbol(editor);
  breadcrumbBar->updatePath(editor->getFileName(), symbol);
}

void TextEditor::createActions() {
  newAct = new QAction("&New", this);
  newAct->setShortcuts(QKeySequence::New);
  connect(newAct, &QAction::triggered, this, &TextEditor::newFile);

  openAct = new QAction("&Open File...", this);
  openAct->setShortcuts(QKeySequence::Open);
  connect(openAct, &QAction::triggered, this, &TextEditor::openFile);

  openFolderAct = new QAction("Open &Folder...", this);
  openFolderAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
  connect(openFolderAct, &QAction::triggered, this, &TextEditor::openFolder);

  saveAct = new QAction("&Save", this);
  saveAct->setShortcuts(QKeySequence::Save);
  connect(saveAct, &QAction::triggered, this, &TextEditor::saveFile);

  saveAsAct = new QAction("Save &As...", this);
  saveAsAct->setShortcuts(QKeySequence::SaveAs);
  connect(saveAsAct, &QAction::triggered, this, &TextEditor::saveFileAs);

  closeTabAct = new QAction("&Close Tab", this);
  closeTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
  connect(closeTabAct, &QAction::triggered, this,
          [this]() { closeTab(tabWidget->currentIndex()); });

  exitAct = new QAction("E&xit", this);
  exitAct->setShortcuts(QKeySequence::Quit);
  connect(exitAct, &QAction::triggered, this, &QWidget::close);

  cutAct = new QAction("Cu&t", this);
  cutAct->setShortcuts(QKeySequence::Cut);
  connect(cutAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->cut();
  });

  copyAct = new QAction("&Copy", this);
  copyAct->setShortcuts(QKeySequence::Copy);
  connect(copyAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->copy();
  });

  pasteAct = new QAction("&Paste", this);
  pasteAct->setShortcuts(QKeySequence::Paste);
  connect(pasteAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->paste();
  });

  undoAct = new QAction("&Undo", this);
  undoAct->setShortcuts(QKeySequence::Undo);
  connect(undoAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->undo();
  });

  redoAct = new QAction("&Redo", this);
  redoAct->setShortcuts(QKeySequence::Redo);
  connect(redoAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->redo();
  });

  selectAllAct = new QAction("Select &All", this);
  selectAllAct->setShortcuts(QKeySequence::SelectAll);
  connect(selectAllAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->selectAll();
  });

  findAct = new QAction("&Find...", this);
  findAct->setShortcuts(QKeySequence::Find);
  connect(findAct, &QAction::triggered, this, &TextEditor::findText);

  findNextAct = new QAction("Find &Next", this);
  findNextAct->setShortcut(QKeySequence(Qt::Key_F3));
  connect(findNextAct, &QAction::triggered, this, &TextEditor::findNext);

  connect(findBar, &FindBar::textChanged, this, &TextEditor::onFindTextChanged);
  connect(findBar, &FindBar::findNextRequested, this, &TextEditor::findNext);
  connect(findBar, &FindBar::findPreviousRequested, this, &TextEditor::findPrevious);
  connect(findBar, &FindBar::closeRequested, this, &TextEditor::closeFindBar);

  replaceAct = new QAction("&Replace...", this);
  replaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
  connect(replaceAct, &QAction::triggered, this, &TextEditor::replaceText);

  goToLineAct = new QAction("&Go to Line...", this);
  goToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  connect(goToLineAct, &QAction::triggered, this, &TextEditor::goToLine);

  // Line editing actions
  duplicateLineAct = new QAction("Duplicate Line", this);
  duplicateLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
  connect(duplicateLineAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->duplicateLine();
  });

  QAction *selectNextAct = new QAction("Select Next Occurrence", this);
  selectNextAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  connect(selectNextAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->selectNextOccurrence();
  });

  moveLineUpAct = new QAction("Move Line Up", this);
  moveLineUpAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
  connect(moveLineUpAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->moveLineUp();
  });

  moveLineDownAct = new QAction("Move Line Down", this);
  moveLineDownAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
  connect(moveLineDownAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->moveLineDown();
  });

  deleteLineAct = new QAction("Delete Line", this);
  deleteLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
  connect(deleteLineAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->deleteLine();
  });

  toggleCommentAct = new QAction("Toggle Line Comment", this);
  toggleCommentAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
  connect(toggleCommentAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->toggleComment();
  });

  smartHomeAct = new QAction("Smart Home", this);
  // Home key handling is done in keyPressEvent directly so it auto-overrides
  // but we add it to the menu just in case.
  connect(smartHomeAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->smartHome();
  });

  increaseFontAct = new QAction("Increase Font Size", this);
  increaseFontAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
  connect(increaseFontAct, &QAction::triggered, this,
          &TextEditor::increaseFontSize);

  decreaseFontAct = new QAction("Decrease Font Size", this);
  decreaseFontAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
  connect(decreaseFontAct, &QAction::triggered, this,
          &TextEditor::decreaseFontSize);

  selectFontAct = new QAction("Select Font...", this);
  connect(selectFontAct, &QAction::triggered, this, &TextEditor::selectFont);

  wordWrapAct = new QAction("Word Wrap", this);
  wordWrapAct->setCheckable(true);
  wordWrapAct->setChecked(wordWrapEnabled);
  connect(wordWrapAct, &QAction::triggered, this, &TextEditor::toggleWordWrap);

  splitViewAct = new QAction("Split View", this);
  splitViewAct->setCheckable(true);
  splitViewAct->setChecked(splitViewEnabled);
  splitViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));
  connect(splitViewAct, &QAction::triggered, this,
          &TextEditor::toggleSplitView);

  fileTreeAct = new QAction("Explorer", this);
  fileTreeAct->setCheckable(true);
  fileTreeAct->setChecked(true);
  fileTreeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
  connect(fileTreeAct, &QAction::triggered, this, &TextEditor::toggleFileTree);

  miniMapAct = new QAction("Mini Map", this);
  miniMapAct->setCheckable(true);
  miniMapAct->setChecked(false);
  miniMapAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
  connect(miniMapAct, &QAction::triggered, this, &TextEditor::toggleMiniMap);

  terminalAct = new QAction("Terminal", this);
  terminalAct->setCheckable(true);
  terminalAct->setChecked(false);
  terminalAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
  connect(terminalAct, &QAction::triggered, this, &TextEditor::toggleTerminal);

  djModeAct = new QAction("DJ Mode", this);
  djModeAct->setIcon(QIcon::fromTheme("audio-card", QIcon::fromTheme("audio-volume-high")));
  djModeAct->setCheckable(true);
  djModeAct->setShortcut(QKeySequence("Ctrl+Shift+J"));
  connect(djModeAct, &QAction::triggered, this, &TextEditor::toggleDJMode);

  animationAct = new QAction("Cycle Animation", this);
  animationAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
  connect(animationAct, &QAction::triggered, this, &TextEditor::cycleAnimation);

  toggleAnimationDockAct = new QAction("Toggle Animation Dock", this);
  toggleAnimationDockAct->setCheckable(true);
  toggleAnimationDockAct->setChecked(false);
  connect(toggleAnimationDockAct, &QAction::triggered, this, &TextEditor::toggleAnimationDock);

  themeAct = new QAction("Toggle Theme", this);
  themeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
  connect(themeAct, &QAction::triggered, this, &TextEditor::changeTheme);

  zenModeAct = new QAction("\U0001F9D8 Zen Mode", this);
  zenModeAct->setShortcut(QKeySequence("Ctrl+Shift+Z"));
  zenModeAct->setCheckable(true);
  connect(zenModeAct, &QAction::triggered, this, &TextEditor::toggleZenMode);

  typingSoundAct = new QAction("Typing Sounds", this);
  typingSoundAct->setCheckable(true);
  typingSoundAct->setChecked(typingSoundEnabled);
  connect(typingSoundAct, &QAction::triggered, this, &TextEditor::toggleTypingSound);

  customizeColorsAct = new QAction("Customize Colors...", this);
  connect(customizeColorsAct, &QAction::triggered, this,
          &TextEditor::customizeColors);

  aboutAct = new QAction("&About", this);
  connect(aboutAct, &QAction::triggered, this, &TextEditor::showAbout);

  // ── Markdown preview action ───────────────────────────────────────────────
  markdownPreviewAct = new QAction("📄 Markdown Preview", this);
  markdownPreviewAct->setCheckable(true);
  markdownPreviewAct->setChecked(false);
  markdownPreviewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
  markdownPreviewAct->setStatusTip("Toggle split Markdown preview panel");
  connect(markdownPreviewAct, &QAction::triggered, this, &TextEditor::toggleMarkdownPreview);

  // ── Tools actions ────────────────────────────────────────────────────────
  disassembleAct = new QAction("&Disassemble File...", this);
  disassembleAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
  disassembleAct->setStatusTip("Disassemble a binary using objdump");
  disassembleAct->setIcon(QIcon::fromTheme("applications-engineering",
                          QIcon::fromTheme("utilities-terminal")));
  connect(disassembleAct, &QAction::triggered, this, &TextEditor::openDisassembler);

  binaryInspectAct = new QAction("&Binary Inspector...", this);
  binaryInspectAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
  binaryInspectAct->setStatusTip("Inspect ELF/PE headers, sections and imports");
  binaryInspectAct->setIcon(QIcon::fromTheme("system-search",
                             QIcon::fromTheme("document-properties")));
  connect(binaryInspectAct, &QAction::triggered, this, &TextEditor::openBinaryInspector);

  neuralGraphAct = new QAction("&Neural Code Graph...", this);
  neuralGraphAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  neuralGraphAct->setStatusTip("Visualize workspace includes as a 3D force-directed graph");
  neuralGraphAct->setIcon(QIcon::fromTheme("preferences-system-network",
                          QIcon::fromTheme("view-web-browser-dom")));
  connect(neuralGraphAct, &QAction::triggered, this, &TextEditor::openNeuralGraph);

  ghostReplayAct = new QAction("&Ghost Replay Mode", this);
  ghostReplayAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
  ghostReplayAct->setStatusTip("Watch a replay of the current session's code edits");
  ghostReplayAct->setIcon(QIcon::fromTheme("media-playback-start"));
  connect(ghostReplayAct, &QAction::triggered, this, &TextEditor::openGhostReplay);

  // v1.7 Cyberpunk actions
  graveyardAct = new QAction("Deleted Code History", this);
  graveyardAct->setCheckable(true);
  graveyardAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G));
  graveyardAct->setStatusTip("Toggle Code Graveyard panel (deleted code >3 lines)");
  connect(graveyardAct, &QAction::triggered, this, &TextEditor::toggleGraveyard);

  crtAct = new QAction("CRT &Post-Processing", this);
  crtAct->setCheckable(true);
  crtAct->setStatusTip("Toggle CRT scanlines, chromatic aberration, and phosphor bloom");
  connect(crtAct, &QAction::triggered, this, &TextEditor::toggleCRT);

  keyHeatmapAct = new QAction("Key &Heatmap", this);
  keyHeatmapAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
  keyHeatmapAct->setStatusTip("Show keystroke frequency heatmap overlay (click to dismiss)");
  connect(keyHeatmapAct, &QAction::triggered, this, &TextEditor::toggleKeyHeatmap);

  vimModeAct = new QAction("&Vim Mode", this);
  vimModeAct->setCheckable(true);
  vimModeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_V));
  vimModeAct->setStatusTip("Toggle Vim Normal/Insert modal editing");
  connect(vimModeAct, &QAction::triggered, this, &TextEditor::toggleVimMode);

  scratchpadAct = new QAction("&Scratchpad", this);
  scratchpadAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
  scratchpadAct->setStatusTip("Open persistent scratchpad for mid-session notes");
  scratchpadAct->setIcon(QIcon::fromTheme("accessories-text-editor",
                         QIcon::fromTheme("document-new")));
  connect(scratchpadAct, &QAction::triggered, this, &TextEditor::openScratchpad);

  commandPaletteAct = new QAction("&Command Palette", this);
  commandPaletteAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
  commandPaletteAct->setStatusTip("Fuzzy-search all editor commands");
  connect(commandPaletteAct, &QAction::triggered, this, &TextEditor::openCommandPalette);

  todoAct = new QAction("TODO/FIXME &Panel", this);
  todoAct->setCheckable(true);
  todoAct->setStatusTip("Show panel listing all TODO, FIXME, HACK, NOTE, BUG tags in open files");
  connect(todoAct, &QAction::triggered, this, &TextEditor::toggleTodoPanel);

  // Security Pack
  paranoiaModeAct = new QAction("☣ Paranoia Mode", this);
  paranoiaModeAct->setCheckable(true);
  paranoiaModeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));
  paranoiaModeAct->setStatusTip("Zero-trace mode: disables disk writes, history, and telemetry");
  connect(paranoiaModeAct, &QAction::triggered, this, &TextEditor::toggleParanoiaMode);

  vulnScanAct = new QAction("⚡ Vuln Scanner", this);
  vulnScanAct->setCheckable(true);
  vulnScanAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_X));
  vulnScanAct->setStatusTip("Enable native regex scanner for secrets and dangerous patterns");
  connect(vulnScanAct, &QAction::triggered, this, &TextEditor::toggleVulnScan);

  focusFadeAct = new QAction("Focus &Fade", this);
  focusFadeAct->setCheckable(true);
  focusFadeAct->setStatusTip("Dim all lines except the current line while editing");
  connect(focusFadeAct, &QAction::triggered, this, &TextEditor::toggleFocusFade);

  imagePreviewAct = new QAction("Image &Preview on Hover", this);
  imagePreviewAct->setCheckable(true);
  imagePreviewAct->setStatusTip("Show thumbnail tooltip when hovering over image paths in code");
  connect(imagePreviewAct, &QAction::triggered, this, &TextEditor::toggleImagePreview);

  sessionStatsAct = new QAction("Session &Statistics", this);
  sessionStatsAct->setStatusTip("View keystrokes, WPM, and activity stats for this session");
  connect(sessionStatsAct, &QAction::triggered, this, &TextEditor::showSessionStats);

  openHexAct = new QAction("Open in &Hex Editor", this);
  openHexAct->setIcon(QIcon::fromTheme("text-x-generic",
                      QIcon::fromTheme("document-open")));
  openHexAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
  openHexAct->setStatusTip("Re-open the current file in the built-in hex editor");
  connect(openHexAct, &QAction::triggered, this, [this]() {
      CodeEditor *ed = currentEditor();
      if (ed && !ed->getFileName().isEmpty()) {
          QFile f(ed->getFileName());
          if (f.open(QIODevice::ReadOnly)) {
              QByteArray bytes = f.readAll();
              HexEditor *hex = new HexEditor();
              hex->setData(bytes);
              hex->setProperty("fileName", ed->getFileName());
              connect(hex, &HexEditor::modificationChanged,
                      this, &TextEditor::documentWasModified);
              int idx = tabWidget->addTab(hex, "[HEX] " + strippedName(ed->getFileName()));
              tabWidget->setCurrentIndex(idx);
              flashTabLabel(idx);
          }
      }
  });

  // ── v0.8.0 QoL Actions ─────────────────────────────────────────────
  switchHeaderSourceAct = new QAction("Switch Header/Source", this);
  switchHeaderSourceAct->setShortcut(QKeySequence(Qt::ALT | Qt::Key_O));
  switchHeaderSourceAct->setStatusTip("Toggle between .cpp and .h counterpart (Alt+O)");
  connect(switchHeaderSourceAct, &QAction::triggered, this, &TextEditor::switchHeaderSource);

  locateInTreeAct = new QAction("Locate File in Tree", this);
  locateInTreeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
  locateInTreeAct->setStatusTip("Reveal and highlight the current file in the file explorer");
  connect(locateInTreeAct, &QAction::triggered, this, &TextEditor::locateCurrentFileInTree);

  searchEverywhereAct = new QAction("Search Everywhere", this);
  searchEverywhereAct->setStatusTip("Search files, actions, and history (double-Shift)");
  connect(searchEverywhereAct, &QAction::triggered, this, &TextEditor::openSearchEverywhere);

  stickyScrollAct = new QAction("Sticky Scroll", this);
  stickyScrollAct->setCheckable(true);
  stickyScrollAct->setStatusTip("Pin the enclosing scope definition to the top of the viewport");
  connect(stickyScrollAct, &QAction::triggered, this, &TextEditor::toggleStickyScroll);

  invisibleCharsAct = new QAction("Show Invisible Characters", this);
  invisibleCharsAct->setCheckable(true);
  invisibleCharsAct->setStatusTip("Render trailing spaces and tabs as visible markers");
  connect(invisibleCharsAct, &QAction::triggered, this, &TextEditor::toggleInvisibleChars);

  gitBlameAct = new QAction("Git Blame Annotations", this);
  gitBlameAct->setCheckable(true);
  gitBlameAct->setStatusTip("Show inline git blame (author, time) at end of each line");
  connect(gitBlameAct, &QAction::triggered, this, &TextEditor::toggleGitBlame);

  autoSaveFocusAct = new QAction("Auto-Save on Focus Lost", this);
  autoSaveFocusAct->setCheckable(true);
  autoSaveFocusAct->setStatusTip("Automatically save file when editor loses window focus");
  connect(autoSaveFocusAct, &QAction::triggered, this, &TextEditor::toggleAutoSaveOnFocusLost);

  sendToScratchpadAct = new QAction("Send Selection to Scratchpad", this);
  sendToScratchpadAct->setStatusTip("Append selected code to Scratchpad with a timestamp");
  connect(sendToScratchpadAct, &QAction::triggered, this, &TextEditor::sendSelectionToScratchpad);

  // ── v0.9.0 Web3Sec Actions ──────────────────────────────────────────────
  godViewAct = new QAction("\U0001F441 God View Contract", this);
  godViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G));
  godViewAct->setStatusTip("Collapse all function bodies to spec-mode (signatures + NatSpec only)");
  connect(godViewAct, &QAction::triggered, this, &TextEditor::triggerGodView);

  extractABIAct = new QAction("\U0001F4CB Extract ABI/Bytecode", this);
  extractABIAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
  extractABIAct->setStatusTip("Run solc on current file and copy ABI to clipboard (Ctrl+Alt+C)");
  connect(extractABIAct, &QAction::triggered, this, &TextEditor::extractABI);

  resolveFourByteAct = new QAction("\U0001F50D Resolve 4-Byte Signature", this);
  resolveFourByteAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F));
  resolveFourByteAct->setStatusTip("Look up selected 4-byte hex selector in 4byte.directory");
  connect(resolveFourByteAct, &QAction::triggered, this, &TextEditor::resolveFourByte);

  panicButtonAct = new QAction("\U0001F480 PANIC BUTTON", this);
  panicButtonAct->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_Delete));
  panicButtonAct->setStatusTip("Anti-forensic killswitch: close editor, shred scratchpad, wipe history");
  connect(panicButtonAct, &QAction::triggered, this, &TextEditor::triggerPanicButton);

  storageSlotVizAct = new QAction("\U0001F5C4 Storage Slot Visualizer", this);
  storageSlotVizAct->setStatusTip("Show Tetris-style 32-byte storage slot layout for .sol files");
  connect(storageSlotVizAct, &QAction::triggered, this, &TextEditor::showStorageSlotVisualizer);

  gasMiniMapAct = new QAction("\U0001F321 Gas Topographic Minimap", this);
  gasMiniMapAct->setCheckable(true);
  gasMiniMapAct->setStatusTip("Overlay gas consumption heatmap on minimap (red=expensive, blue=cheap)");
  connect(gasMiniMapAct, &QAction::triggered, this, &TextEditor::toggleGasMinimap);

  slitherOverlayAct = new QAction("\u2622 Slither/Foundry Overlays", this);
  slitherOverlayAct->setStatusTip("Run slither or forge test and overlay findings on editor");
  connect(slitherOverlayAct, &QAction::triggered, this, &TextEditor::toggleSlitherOverlay);

  onChainTracerAct = new QAction("\u26D3 On-Chain Trace Explorer", this);
  onChainTracerAct->setStatusTip("Paste a tx hash to replay execution trace against local .sol files");
  connect(onChainTracerAct, &QAction::triggered, this, &TextEditor::showOnChainTracer);

  proxyDiffAct = new QAction("\U0001F504 Proxy/Implementation Diff", this);
  proxyDiffAct->setStatusTip("Compare storage layouts of two .sol files for proxy collision detection");
  connect(proxyDiffAct, &QAction::triggered, this, &TextEditor::showProxyDiff);

  memTraceAct = new QAction("\U0001F9E0 Memory Pointer Tracer", this);
  memTraceAct->setCheckable(true);
  memTraceAct->setStatusTip("Enable: click on 0x40/memory offsets in Yul to highlight all mload/mstore ops");
  connect(memTraceAct, &QAction::triggered, this, [this](bool checked) {
      int i = 0;
      while (auto *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i++)))
          ed->setMemTraceEnabled(checked);
      if (tabWidget2) { int j = 0; while (auto *ed = qobject_cast<CodeEditor*>(tabWidget2->widget(j++))) ed->setMemTraceEnabled(checked); }
  });
}

void TextEditor::createMenus() {
  fileMenu = customMenuBar->addMenu("&File");
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(openFolderAct);
  recentFilesMenu = fileMenu->addMenu("Recent Files");
  fileMenu->addSeparator();
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addAction(closeTabAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  editMenu = customMenuBar->addMenu("&Edit");
  editMenu->addAction(undoAct);
  editMenu->addAction(redoAct);
  editMenu->addSeparator();
  editMenu->addAction(cutAct);
  editMenu->addAction(copyAct);
  editMenu->addAction(pasteAct);
  editMenu->addSeparator();
  editMenu->addAction(selectAllAct);

  searchMenu = customMenuBar->addMenu("&Search");
  searchMenu->addAction(findAct);
  searchMenu->addAction(findNextAct);
  searchMenu->addAction(replaceAct);
  searchMenu->addAction(goToLineAct);

  viewMenu = customMenuBar->addMenu("&View");
  viewMenu->addAction(fileTreeAct);
  viewMenu->addAction(miniMapAct);
  viewMenu->addAction(terminalAct);
  viewMenu->addAction(toggleAnimationDockAct);
  viewMenu->addAction(animationAct);
  viewMenu->addAction(splitViewAct);
  viewMenu->addSeparator();
  viewMenu->addAction(increaseFontAct);
  viewMenu->addAction(decreaseFontAct);
  viewMenu->addAction(selectFontAct);
  viewMenu->addSeparator();
  viewMenu->addAction(wordWrapAct);
  viewMenu->addSeparator();
  viewMenu->addAction(markdownPreviewAct);
  viewMenu->addAction(djModeAct);
  viewMenu->addSeparator();
  viewMenu->addAction(themeAct);
  viewMenu->addAction(customizeColorsAct);
  viewMenu->addSeparator();
  viewMenu->addAction(zenModeAct);
  viewMenu->addAction(typingSoundAct);
  viewMenu->addSeparator();
  viewMenu->addAction(crtAct);
  viewMenu->addAction(vimModeAct);
  viewMenu->addAction(keyHeatmapAct);
  viewMenu->addAction(graveyardAct);
  viewMenu->addSeparator();
  viewMenu->addAction(focusFadeAct);
  viewMenu->addAction(imagePreviewAct);
  viewMenu->addSeparator();
  viewMenu->addAction(stickyScrollAct);
  viewMenu->addAction(invisibleCharsAct);
  viewMenu->addAction(gitBlameAct);
  viewMenu->addAction(autoSaveFocusAct);

  toolsMenu = customMenuBar->addMenu("&Tools");
  toolsMenu->addAction(commandPaletteAct);
  toolsMenu->addAction(searchEverywhereAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(switchHeaderSourceAct);
  toolsMenu->addAction(locateInTreeAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(scratchpadAct);
  toolsMenu->addAction(sendToScratchpadAct);
  toolsMenu->addAction(sessionStatsAct);
  toolsMenu->addAction(todoAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(paranoiaModeAct);
  toolsMenu->addAction(vulnScanAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(openHexAct);
  toolsMenu->addAction(disassembleAct);
  toolsMenu->addAction(binaryInspectAct);
  toolsMenu->addAction(neuralGraphAct);
  toolsMenu->addAction(ghostReplayAct);
  toolsMenu->addSeparator();
  toolsMenu->setStyleSheet(
      "QMenu { background-color: #252526; color: #d4d4d4; border: 1px solid #3c3c3c; }"
      "QMenu::item:selected { background-color: #094771; }"
      "QMenu::separator { background: #3c3c3c; height: 1px; margin: 2px 8px; }");

  // ── v0.9.0 Web3Sec Menu ──────────────────────────────────────────────
  QMenu *web3Menu = customMenuBar->addMenu("&Web3Sec");
  web3Menu->addAction(godViewAct);
  web3Menu->addAction(extractABIAct);
  web3Menu->addAction(resolveFourByteAct);
  web3Menu->addSeparator();
  web3Menu->addAction(storageSlotVizAct);
  web3Menu->addAction(gasMiniMapAct);
  web3Menu->addAction(memTraceAct);
  web3Menu->addSeparator();
  web3Menu->addAction(slitherOverlayAct);
  web3Menu->addAction(onChainTracerAct);
  web3Menu->addAction(proxyDiffAct);
  web3Menu->addSeparator();
  web3Menu->addAction(panicButtonAct);
  web3Menu->setStyleSheet(
      "QMenu { background-color: #1a0a0a; color: #ff6666; border: 1px solid #ff3333; }"
      "QMenu::item:selected { background-color: #3a0000; }"
      "QMenu::separator { background: #ff3333; height: 1px; margin: 2px 8px; }");

  helpMenu = customMenuBar->addMenu("&Help");
  helpMenu->addAction(aboutAct);

  pluginsMenu = customMenuBar->addMenu("&Plugins");
  aiSettingsAct = new QAction("AI Autocomplete Settings...", this);
  connect(aiSettingsAct, &QAction::triggered, this, &TextEditor::showAISettings);
  pluginsMenu->addAction(aiSettingsAct);

  aiToggleAct = new QAction("Enable AI Autocomplete", this);
  aiToggleAct->setCheckable(true);
  connect(aiToggleAct, &QAction::toggled, this, &TextEditor::toggleAIAutocomplete);
  pluginsMenu->addAction(aiToggleAct);

  updateRecentFilesMenu();
}

void TextEditor::showAISettings() {
    AISettingsDialog dialog(this);
    QSettings settings;
    dialog.setSettings(settings.value("ai/baseUrl").toString(),
                       settings.value("ai/apiKey").toString(),
                       settings.value("ai/model").toString(),
                       aiAutocomplete->isEnabled());
    
    if (dialog.exec() == QDialog::Accepted) {
        settings.setValue("ai/baseUrl", dialog.getBaseUrl());
        settings.setValue("ai/apiKey", dialog.getApiKey());
        settings.setValue("ai/model", dialog.getModel());
        
        aiAutocomplete->setProvider(dialog.getBaseUrl(), dialog.getApiKey(), dialog.getModel());
        toggleAIAutocomplete(dialog.isEnabled());
        aiToggleAct->setChecked(dialog.isEnabled());
    }
}

void TextEditor::toggleAIAutocomplete(bool enabled) {
    aiAutocomplete->setEnabled(enabled);
}

void TextEditor::onAISuggestion(const QString &suggestion) {
    if (suggestion.isEmpty()) return;
    statusBar()->showMessage("AI Suggestion: " + suggestion, 5000);
    qDebug() << "AI Suggestion:" << suggestion;
}

void TextEditor::clampToScreen() {
    QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    QRect cur   = geometry();
    // Clamp size first so the window actually fits
    int w = qMin(cur.width(),  avail.width());
    int h = qMin(cur.height(), avail.height());
    // Then clamp position so the bottom/right edges don't escape
    int x = qBound(avail.left(), cur.left(), avail.right()  - w);
    int y = qBound(avail.top(),  cur.top(),  avail.bottom() - h);
    if (w != cur.width() || h != cur.height() || x != cur.left() || y != cur.top())
        setGeometry(x, y, w, h);
}

void TextEditor::createStatusBar() {
  static const char *kPill =
      "background: #252526; color: #cccccc; padding: 1px 10px; "
      "border-radius: 8px; font-size: 11px; margin: 2px 2px;";

  languageLabel = new QLabel("Plain Text");
  languageLabel->setStyleSheet(
      "background: #005f99; color: #e8f4fd; padding: 1px 10px; "
      "border-radius: 8px; font-size: 11px; margin: 2px 2px;");
  statusBar()->addPermanentWidget(languageLabel);

  sessionTimeLabel = new QLabel("⏱ 0m", this);
  sessionTimeLabel->setStyleSheet(
      "background: #252526; color: #777777; padding: 1px 10px; "
      "border-radius: 8px; font-size: 11px; margin: 2px 2px;");
  statusBar()->addPermanentWidget(sessionTimeLabel);

  statusLabel = new QLabel("Ln 1, Col 1");
  statusLabel->setStyleSheet(kPill);
  statusBar()->addPermanentWidget(statusLabel);

  vimModeLabel = new QLabel("  NORMAL  ");
  vimModeLabel->setStyleSheet(
      "color: #282c34; background-color: #c678dd; padding: 1px 8px; "
      "font-family: Consolas; font-weight: bold; font-size: 10px; "
      "border-radius: 8px; margin: 2px 2px;");
  vimModeLabel->setVisible(false);
  statusBar()->addPermanentWidget(vimModeLabel);

  paranoiaLabel = new QLabel("  ☣ PARANOIA  ");
  paranoiaLabel->setStyleSheet(
      "color: #ffffff; background-color: #e81123; padding: 1px 8px; "
      "font-family: Consolas; font-weight: bold; font-size: 10px; "
      "border-radius: 8px; margin: 2px 2px;");
  paranoiaLabel->setVisible(false);
  statusBar()->addPermanentWidget(paranoiaLabel);

  statusBar()->showMessage("Ready");

  // Lift the bar with a subtle upward shadow
  auto *shadow = new QGraphicsDropShadowEffect(statusBar());
  shadow->setBlurRadius(10);
  shadow->setColor(QColor(0, 0, 0, 130));
  shadow->setOffset(0, -3);
  statusBar()->setGraphicsEffect(shadow);
}

void TextEditor::newFile() {
  hideWelcomeScreen();
  CodeEditor *editor = new CodeEditor();
  SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
  highlighters[editor] = highlighter;
  QFont font(editorFontFamily, fontSize);
  font.setStyleStrategy(QFont::PreferDefault);
  editor->setFont(font);
  editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth
                                          : QPlainTextEdit::NoWrap);
  applyThemeToEditor(editor, highlighter);
  connect(editor->document(), &QTextDocument::modificationChanged, this,
          &TextEditor::documentWasModified);
  connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
          &TextEditor::updateStatusBar);
  connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
          &TextEditor::updateBreadcrumb);
  connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
      aiAutocomplete->trigger(editor);
  });
  if (typingSoundEnabled && typingSound) {
      connect(editor, &CodeEditor::characterTyped, typingSound, &QSoundEffect::play);
  }
  connect(editor, &CodeEditor::codeBlockDeleted, this, &TextEditor::onCodeBlockDeleted);
  if (hudWidget)
      connect(editor, &CodeEditor::characterTyped, hudWidget, &HUDWidget::addKeystroke);
  // Keystroke heatmap
  if (keyHeatmap)
      connect(editor, &CodeEditor::keyPressed, keyHeatmap, [this](int key, const QString &text) {
          keyHeatmap->recordKey(text, key);
      });
  // Vim mode status label
  connect(editor, &CodeEditor::vimModeChanged, this, [this](const QString &mode) {
      if (vimModeLabel) {
          if (mode.isEmpty()) { vimModeLabel->hide(); return; }
          vimModeLabel->setText("  " + mode + "  ");
          bool isNormal = (mode == "NORMAL");
          vimModeLabel->setStyleSheet(QString(
              "color: #282c34; background-color: %1; padding: 1px 8px; "
              "font-family: Consolas; font-weight: bold; font-size: 10px;")
              .arg(isNormal ? "#c678dd" : "#98c379"));
          vimModeLabel->show();
      }
  });
  if (crtAct && crtAct->isChecked())
      editor->setCRTEnabled(true);
  if (vimModeAct && vimModeAct->isChecked())
      editor->setVimEnabled(true);
  if (paranoiaModeAct && paranoiaModeAct->isChecked())
      editor->setParanoiaMode(true);
  if (vulnScanAct && vulnScanAct->isChecked())
      editor->setVulnScanEnabled(true);
  if (focusFadeAct && focusFadeAct->isChecked())
      editor->setFocusFadeEnabled(true);
  if (imagePreviewAct && imagePreviewAct->isChecked())
      editor->setImagePreviewEnabled(true);
  propagateV080Settings(editor);
  propagateV090Settings(editor);
  connect(editor, &CodeEditor::keyPressed, this, &TextEditor::trackKeystroke);
  int index = tabWidget->addTab(editor, "Untitled");
  tabWidget->setCurrentIndex(index);
  editor->setFocus();
  QTimer::singleShot(0, this, &TextEditor::clampToScreen);
}

void TextEditor::openFile() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open File", "",
      "All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files "
      "(*.py);;JavaScript (*.js *.ts);;Rust (*.rs);;Go (*.go)");
  if (!fileName.isEmpty()) {
    for (int i = 0; i < tabWidget->count(); ++i) {
      CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
      if (editor && editor->getFileName() == fileName) {
        tabWidget->setCurrentIndex(i);
        return;
      }
    }
    loadFile(fileName);
  }
}

void TextEditor::openRecentFile() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    loadFile(action->data().toString());
}

bool TextEditor::saveFile() {
  CodeEditor *editor = currentEditor();
  if (!editor) {
    HexEditor *hexEditor =
        qobject_cast<HexEditor *>(tabWidget->currentWidget());
    if (hexEditor) {
      QString fileName = hexEditor->property("fileName").toString();
      if (fileName.isEmpty())
        return saveFileAs();
      return saveFileToPath(fileName);
    }
    return false;
  }
  if (editor->getFileName().isEmpty())
    return saveFileAs();
  else
    return saveFileToPath(editor->getFileName());
}

bool TextEditor::saveFileAs() {
  CodeEditor *editor = currentEditor();
  HexEditor *hexEditor = qobject_cast<HexEditor *>(tabWidget->currentWidget());
  if (!editor && !hexEditor)
    return false;

  QString fileName =
      QFileDialog::getSaveFileName(this, "Save File", "",
                                   "All Files (*);;Text Files (*.txt);;C++ "
                                   "Files (*.cpp *.h);;Python Files (*.py)");
  if (fileName.isEmpty())
    return false;
  return saveFileToPath(fileName);
}

void TextEditor::closeTab(int index) {
  if (tabWidget->widget(index) == welcomeWidget) {
    tabWidget->removeTab(index);
    return;
  }
  if (maybeSave(index)) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(index));
    if (editor) {
      unwatchFile(editor->getFileName());
      highlighters.remove(editor);
    }
    tabWidget->removeTab(index);
    if (tabWidget->count() == 0)
      showWelcomeScreen();
  }
}

void TextEditor::tabChanged(int) {
  updateStatusBar();
  updateBreadcrumb();

  // Keep markdown preview in sync when switching tabs
  if (markdownPreview && markdownPreview->isVisible()) {
      CodeEditor *ed = currentEditor();
      if (ed && ed != markdownEditor)
          connectMarkdownPreview(ed);
      // Trigger a refresh (debounced)
      if (markdownTimer)
          markdownTimer->start();
  }

  CodeEditor *editor = currentEditor();
  HexEditor *hexEditor = qobject_cast<HexEditor *>(tabWidget->currentWidget());
  if (editor) {
    QString title = "Jim";
    if (!editor->getFileName().isEmpty())
      title = strippedName(editor->getFileName()) + " - " + title;
    if (editor->isModified())
      title = "*" + title;
    setWindowTitle(title);

    // Update tab text with asterisk if modified
    int currentIdx = tabWidget->currentIndex();
    QString tabText = strippedName(editor->getFileName());
    if (tabText.isEmpty())
      tabText = "Untitled";
    if (editor->isModified())
      tabText = "*" + tabText;
    tabWidget->setTabText(currentIdx, tabText);

    // Update language label
    Language lang = editor->getLanguage();
    QStringList langNames = {"Plain Text", "C++",  "Python",  "JavaScript",
                             "HTML",       "CSS",  "Rust",    "Go",
                             "JSON",       "YAML", "Markdown"};
    languageLabel->setText(langNames[static_cast<int>(lang)]);
  } else if (hexEditor) {
    QString fileName = hexEditor->property("fileName").toString();
    QString title = "Jim";
    if (!fileName.isEmpty())
      title = strippedName(fileName) + " - " + title;
    if (hexEditor->isModified())
      title = "*" + title;
    setWindowTitle(title);

    // Update tab text with asterisk
    int currentIdx = tabWidget->currentIndex();
    QString tabText = "[HEX] " + strippedName(fileName);
    if (hexEditor->isModified())
      tabText = "*" + tabText;
    tabWidget->setTabText(currentIdx, tabText);

    languageLabel->setText("Binary (Hex)");
  }
}

void TextEditor::findText() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    
    QString selected = editor->textCursor().selectedText();
    if (selected.isEmpty()) selected = lastSearchText;
    
    findBar->showAndFocus(selected);
    onFindTextChanged(findBar->getSearchText());
}

void TextEditor::onFindTextChanged(const QString &text) {
    lastSearchText = text;
    currentMatches.clear();
    currentMatchIndex = -1;
    
    CodeEditor *editor = currentEditor();
    if (!editor || text.isEmpty()) {
        updateSearchHighlights();
        findBar->setMatchCount(0, 0);
        return;
    }

    QString content = editor->toPlainText();
    QRegularExpression re(QRegularExpression::escape(text), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = re.globalMatch(content);
    
    int currentPos = editor->textCursor().position();
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QTextCursor cursor(editor->document());
        cursor.setPosition(match.capturedStart());
        cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
        currentMatches.append(cursor);
        
        if (currentMatchIndex == -1 && match.capturedStart() >= currentPos) {
            currentMatchIndex = currentMatches.size() - 1;
        }
    }

    if (currentMatchIndex == -1 && !currentMatches.isEmpty()) {
        currentMatchIndex = 0;
    }

    updateSearchHighlights();
    findBar->setMatchCount(currentMatchIndex + 1, currentMatches.size());
    
    if (currentMatchIndex != -1) {
        editor->setTextCursor(currentMatches[currentMatchIndex]);
        editor->ensureCursorVisible();
    }
}

void TextEditor::findNext() {
    if (currentMatches.isEmpty()) return;
    currentMatchIndex = (currentMatchIndex + 1) % currentMatches.size();
    CodeEditor *editor = currentEditor();
    if (editor) {
        editor->setTextCursor(currentMatches[currentMatchIndex]);
        editor->ensureCursorVisible();
        updateSearchHighlights();
        findBar->setMatchCount(currentMatchIndex + 1, currentMatches.size());
    }
}

void TextEditor::findPrevious() {
    if (currentMatches.isEmpty()) return;
    currentMatchIndex = (currentMatchIndex - 1 + currentMatches.size()) % currentMatches.size();
    CodeEditor *editor = currentEditor();
    if (editor) {
        editor->setTextCursor(currentMatches[currentMatchIndex]);
        editor->ensureCursorVisible();
        updateSearchHighlights();
        findBar->setMatchCount(currentMatchIndex + 1, currentMatches.size());
    }
}

void TextEditor::closeFindBar() {
    findBar->hide();
    currentMatches.clear();
    currentMatchIndex = -1;
    updateSearchHighlights();
    if (CodeEditor *editor = currentEditor()) {
        editor->setFocus();
    }
}

void TextEditor::updateSearchHighlights() {
    CodeEditor *editor = currentEditor();
    if (editor) {
        editor->setSearchSelections(currentMatches);
    }
}

void TextEditor::replaceText() {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return;
  bool ok;
  QString findStr = QInputDialog::getText(
      this, "Replace", "Find what:", QLineEdit::Normal, lastSearchText, &ok);
  if (!ok || findStr.isEmpty())
    return;
  QString replaceStr = QInputDialog::getText(
      this, "Replace", "Replace with:", QLineEdit::Normal, "", &ok);
  if (!ok)
    return;
  lastSearchText = findStr;
  QString content = editor->toPlainText();
  content.replace(findStr, replaceStr);
  editor->setPlainText(content);
}

void TextEditor::goToLine() {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return;
  bool ok;
  int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1,
                                  editor->document()->blockCount(), 1, &ok);
  if (ok) {
    QTextCursor cursor(editor->document()->findBlockByLineNumber(line - 1));
    editor->setTextCursor(cursor);
    editor->centerCursor();
  }
}

void TextEditor::documentWasModified() {
  tabChanged(tabWidget->currentIndex());
}

void TextEditor::updateStatusBar() {
  CodeEditor *editor = currentEditor();
  if (editor) {
    QTextCursor cursor = editor->textCursor();
    statusLabel->setText(QString("Ln %1, Col %2")
                             .arg(cursor.blockNumber() + 1)
                             .arg(cursor.columnNumber() + 1));
  }
}

void TextEditor::increaseFontSize() {
  fontSize++;
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor) {
      QFont font = editor->font();
      font.setPointSize(fontSize);
      editor->setFont(font);
    }
  }
}

void TextEditor::decreaseFontSize() {
  if (fontSize > 6) {
    fontSize--;
    for (int i = 0; i < tabWidget->count(); ++i) {
      CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
      if (editor) {
        QFont font = editor->font();
        font.setPointSize(fontSize);
        editor->setFont(font);
      }
    }
  }
}

void TextEditor::selectFont() {
  // Build font list: featured ligature fonts first, then remaining monospace
  static const QStringList kFeatured = {
      "JetBrains Mono", "Fira Code", "Cascadia Code", "Cascadia Mono",
      "Iosevka", "Iosevka Term", "Victor Mono", "Hack", "Inconsolata",
      "Source Code Pro", "Consolas", "Courier New"
  };

  QStringList allFamilies = QFontDatabase::families();
  QStringList fontList;
  for (const QString &f : kFeatured)
      if (allFamilies.contains(f))
          fontList << f;
  for (const QString &f : allFamilies)
      if (QFontDatabase::isFixedPitch(f) && !fontList.contains(f))
          fontList << f;

  QDialog dlg(this);
  dlg.setWindowTitle("Select Editor Font");
  dlg.setMinimumWidth(420);
  dlg.setStyleSheet(
      "QDialog { background: #1e1e1e; color: #cccccc; }"
      "QLabel  { color: #cccccc; }"
      "QComboBox { background: #252526; color: #cccccc; border: 1px solid #3e3e42; "
      "            border-radius: 4px; padding: 4px 8px; font-size: 12px; }"
      "QComboBox::drop-down { border: none; }"
      "QComboBox QAbstractItemView { background: #252526; color: #cccccc; "
      "                              selection-background-color: #094771; }"
      "QDialogButtonBox QPushButton { background: #313244; color: #cccccc; "
      "  border: 1px solid #45475a; border-radius: 6px; padding: 5px 18px; }"
      "QDialogButtonBox QPushButton:hover { background: #45475a; }");

  auto *layout = new QVBoxLayout(&dlg);
  layout->setSpacing(10);

  auto *descLabel = new QLabel("Choose a monospace font. Ligature fonts (e.g. Fira Code, JetBrains Mono)\nrender <code>=>  !=  >=  -></code> as single glyphs when installed.");
  descLabel->setTextFormat(Qt::RichText);
  descLabel->setStyleSheet("color: #9e9e9e; font-size: 11px;");
  layout->addWidget(descLabel);

  auto *combo = new QComboBox;
  combo->addItems(fontList);
  int cur = fontList.indexOf(editorFontFamily);
  if (cur >= 0) combo->setCurrentIndex(cur);
  layout->addWidget(combo);

  auto *previewLabel = new QLabel("fn => x != y && result >= 0 -> done");
  previewLabel->setFont(QFont(editorFontFamily, 13));
  previewLabel->setStyleSheet(
      "padding: 10px 12px; background: #0d0d0d; color: #9cdcfe; "
      "border-radius: 6px; font-size: 13px;");
  layout->addWidget(previewLabel);

  connect(combo, &QComboBox::currentTextChanged, previewLabel, [previewLabel](const QString &f) {
      QFont pf(f, 13);
      pf.setStyleStrategy(QFont::PreferDefault);
      previewLabel->setFont(pf);
  });

  auto *noteLabel = new QLabel("Tip: Install Fira Code or JetBrains Mono for ligature support.");
  noteLabel->setStyleSheet("color: #555; font-size: 10px;");
  layout->addWidget(noteLabel);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  if (dlg.exec() != QDialog::Accepted)
      return;

  editorFontFamily = combo->currentText();
  for (int i = 0; i < tabWidget->count(); ++i) {
      auto *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
      if (editor) {
          QFont f(editorFontFamily, fontSize);
          f.setStyleStrategy(QFont::PreferDefault);
          editor->setFont(f);
      }
  }
  flashStatusMessage(QString("Font: %1").arg(editorFontFamily), QColor("#98c379"));
}

void TextEditor::toggleWordWrap() {
  wordWrapEnabled = !wordWrapEnabled;
  wordWrapAct->setChecked(wordWrapEnabled);
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor)
      editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth
                                              : QPlainTextEdit::NoWrap);
  }
}

void TextEditor::animateTerminalShow() {
  terminalWidget->show();
  if (!terminalAnim) {
    terminalAnim = new QVariantAnimation(this);
    terminalAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(terminalAnim, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &val) {
              QList<int> sizes = verticalSplitter->sizes();
              int total = sizes[0] + sizes[1];
              int newBottom = val.toInt();
              sizes[1] = newBottom;
              sizes[0] = qMax(50, total - newBottom);
              verticalSplitter->setSizes(sizes);
            });
  }
  if (terminalAnim->state() == QAbstractAnimation::Running)
    terminalAnim->stop();

  QList<int> sizes = verticalSplitter->sizes();
  int current = sizes.value(1, 0);
  terminalAnim->setDuration(260);
  terminalAnim->setStartValue(current);
  terminalAnim->setEndValue(terminalTargetH);
  terminalAnim->start();
}

void TextEditor::animateTerminalHide() {
  if (!terminalAnim) {
    terminalAnim = new QVariantAnimation(this);
    terminalAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(terminalAnim, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &val) {
              QList<int> sizes = verticalSplitter->sizes();
              int total = sizes[0] + sizes[1];
              int newBottom = val.toInt();
              sizes[1] = newBottom;
              sizes[0] = qMax(50, total - newBottom);
              verticalSplitter->setSizes(sizes);
            });
  }
  if (terminalAnim->state() == QAbstractAnimation::Running)
    terminalAnim->stop();

  // Remember the current height before collapsing
  QList<int> sizes = verticalSplitter->sizes();
  if (sizes.value(1, 0) > 10)
    terminalTargetH = sizes[1];

  terminalAnim->setDuration(220);
  terminalAnim->setStartValue(sizes.value(1, 0));
  terminalAnim->setEndValue(0);
  connect(terminalAnim, &QVariantAnimation::finished, this, [this]() {
    terminalWidget->hide();
    disconnect(terminalAnim, &QVariantAnimation::finished, this, nullptr);
  });
  terminalAnim->start();
}

void TextEditor::toggleTerminal() {
  bool currentlyVisible = terminalWidget->isVisible() &&
                          verticalSplitter->sizes().value(1, 0) > 5;
  if (currentlyVisible) {
    animateTerminalHide();
    terminalAct->setChecked(false);
  } else {
    animateTerminalShow();
    terminalAct->setChecked(true);
  }
}

void TextEditor::cycleAnimation() {
  animationWidget->cycleAnimation();
  
  AnimationWidget::AnimationType currentType = animationWidget->getCurrentType();
  if (currentType == AnimationWidget::None) {
    animationDock->hide();
    toggleAnimationDockAct->setChecked(false);
  } else {
    animationDock->show();
    toggleAnimationDockAct->setChecked(true);
  }
  
  QString animName;
  switch (currentType) {
    case AnimationWidget::None: animName = "None"; break;
    case AnimationWidget::Matrix: animName = "Matrix"; break;
    case AnimationWidget::Particles: animName = "Particles"; break;
    case AnimationWidget::Waves: animName = "Waves"; break;
    case AnimationWidget::Pulse: animName = "Pulse"; break;
    case AnimationWidget::Starfield: animName = "Starfield"; break;
    case AnimationWidget::Rain: animName = "Rain"; break;
    case AnimationWidget::Snow: animName = "Snow"; break;
    case AnimationWidget::Fire: animName = "Fire"; break;
    case AnimationWidget::DJMode: animName = "DJ Mode"; break;
  }
  statusBar()->showMessage("Animation: " + animName, 2000);
}

void TextEditor::toggleAnimationDock() {
    if (animationDock->isVisible()) {
        animationDock->hide();
        animationWidget->setAnimationType(AnimationWidget::None);
    } else {
        animationDock->show();
        if (animationWidget->getCurrentType() == AnimationWidget::None) {
            animationWidget->cycleAnimation(); // Start with first animation
        }
    }
    toggleAnimationDockAct->setChecked(animationDock->isVisible());
}

void TextEditor::toggleZenMode() {
    zenModeActive = !zenModeActive;
    if (zenModeAct) zenModeAct->setChecked(zenModeActive);
    
    auto applyZen = [&](QWidget *w, bool hide) {
        if (w) w->setVisible(!hide);
    };

    applyZen(menuBar(), zenModeActive);
    applyZen(statusBar(), zenModeActive);
    applyZen(breadcrumbBar, zenModeActive);
    
    if (zenModeActive) {
        if (terminalWidget->isVisible()) terminalWidget->hide();
        if (animationDock->isVisible()) animationDock->hide();
        if (fileTreeDock->isVisible()) fileTreeDock->hide();
        // Don't hide DJ visualizer dock - keep it visible in fullscreen
        setWindowState(windowState() | Qt::WindowFullScreen);
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
}

void TextEditor::toggleTypingSound() {
    typingSoundEnabled = !typingSoundEnabled;
    if (typingSoundAct) typingSoundAct->setChecked(typingSoundEnabled);
    
    if (typingSoundEnabled && !typingSound) {
        typingSound = new QSoundEffect(this);
        QString tempPath = QDir::tempPath() + "/jim_click.wav";
        if (!QFile::exists(tempPath)) {
            QFile f(tempPath);
            if (f.open(QIODevice::WriteOnly)) {
                QByteArray wav = QByteArray::fromHex(
                    "524946463A00000057415645666D74201000000001000100112B0000112B0000010008006461746116000000809a80b380bf80b3809a807f8065804c8040804c8065807f809a80b380bf80b3809a807f8065804c804080"
                );
                f.write(wav);
                f.close();
            }
        }
        typingSound->setSource(QUrl::fromLocalFile(tempPath));
        typingSound->setVolume(0.5f);
        
        for (int i=0; i<tabWidget->count(); i++) {
            CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (ed) connect(ed, &CodeEditor::characterTyped, typingSound, &QSoundEffect::play);
        }
    } else if (!typingSoundEnabled && typingSound) {
        for (int i=0; i<tabWidget->count(); i++) {
            CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (ed) disconnect(ed, &CodeEditor::characterTyped, typingSound, &QSoundEffect::play);
        }
    }
}

void TextEditor::toggleDJMode() {
    bool active = djModeAct->isChecked();
    if (active) {
        if (!audioMonitor) {
            audioMonitor = new AudioMonitor(this);
        }
        audioMonitor->start();
        
        // Cap height before adding the dock so Qt can't grow past the screen
        setMaximumHeight(QGuiApplication::primaryScreen()->availableGeometry().height());

        // Create and show the DJ visualizer dock
        if (!djVisualizerWidget) {
            djVisualizerWidget = new DJVisualizerWidget(this);
            djVisualizerDock = new QDockWidget("🎵 DJ Mode Visualizer", this);
            djVisualizerDock->setWidget(djVisualizerWidget);
            djVisualizerDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
            djVisualizerDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
            // Make the dock more persistent in fullscreen
            djVisualizerDock->setProperty("fullscreen", true);
            addDockWidget(Qt::BottomDockWidgetArea, djVisualizerDock);
            
            // Connect audio updates to the visualizer
            connect(audioMonitor, &AudioMonitor::levelsUpdated,
                    djVisualizerWidget->visualizerWidget, qOverload<>(&QWidget::update));
        }
        djVisualizerWidget->setAudioMonitor(audioMonitor);
        djVisualizerDock->setVisible(true);
        djVisualizerDock->raise();
        QTimer::singleShot(0, this, &TextEditor::clampToScreen);

        // Make sure the audio monitor is running
        if (!audioMonitor->isRunning()) {
            audioMonitor->start();
        }

    } else {
        if (audioMonitor) {
            audioMonitor->stop();
        }
        if (djVisualizerDock) {
            djVisualizerDock->setVisible(false);
        }
        setMaximumHeight(QWIDGETSIZE_MAX); // release the screen-height cap
        // Reset the dock animation widget to Matrix mode
        animationWidget->setAnimationType(AnimationWidget::Matrix);
    }
}

void TextEditor::showAbout() {
  QMessageBox::about(this, "About Jim",
                     "Jim - Lightweight Code Editor\n\n"
                     "Features:\n"
                     "  Syntax highlighting (11 languages)\n"
                     "  Code folding & Breadcrumb navigation\n"
                     "  Integrated terminal\n"
                     "  Multiple tabs & Split view\n"
                     "  Find & Replace\n"
                     "  Auto-indentation & bracket pairing\n"
                     "  Theme switching & File watcher\n"
                     "  Mini map & Welcome screen");
}

void TextEditor::closeEvent(QCloseEvent *event) {
  for (int i = 0; i < tabWidget->count(); ++i) {
    if (tabWidget->widget(i) == welcomeWidget)
      continue;
    if (!maybeSave(i)) {
      event->ignore();
      return;
    }
  }
  
  // Save session time
  QSettings settings("Jim", "JimEditor");
  int secs = sessionSecondsAccumulated + sessionStart.secsTo(QDateTime::currentDateTime());
  settings.setValue("sessionDate", sessionDateString);
  settings.setValue("sessionSeconds", secs);
  
  writeSettings();
  event->accept();
}

void TextEditor::readSettings() {
  QSettings settings("TextEditor", "Settings");
  recentFiles = settings.value("recentFiles").toStringList();
  fontSize = settings.value("fontSize", 11).toInt();
  editorFontFamily = settings.value("editorFont", "Consolas").toString();
  wordWrapEnabled = settings.value("wordWrap", false).toBool();
  wordWrapAct->setChecked(wordWrapEnabled);
}

void TextEditor::writeSettings() {
  if (paranoiaMode) return;
  QSettings settings("TextEditor", "Settings");
  settings.setValue("recentFiles", recentFiles);
  settings.setValue("fontSize", fontSize);
  settings.setValue("editorFont", editorFontFamily);
  settings.setValue("wordWrap", wordWrapEnabled);
}

bool TextEditor::maybeSave(int tabIndex) {
  QWidget *widget = tabWidget->widget(tabIndex);
  CodeEditor *editor = qobject_cast<CodeEditor *>(widget);
  HexEditor *hexEditor = qobject_cast<HexEditor *>(widget);

  bool modified = false;
  if (editor)
    modified = editor->isModified();
  else if (hexEditor)
    modified = hexEditor->isModified();

  if (!modified)
    return true;

  tabWidget->setCurrentIndex(tabIndex);
  const QMessageBox::StandardButton ret = QMessageBox::warning(
      this, "Jim",
      "The document has been modified.\nDo you want to save your changes?",
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  switch (ret) {
  case QMessageBox::Save:
    return saveFile();
  case QMessageBox::Cancel:
    return false;
  default:
    break;
  }
  return true;
}

void TextEditor::loadFile(const QString &fileName) {
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, "Jim",
                         QString("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
    return;
  }

  // Check if file is binary
  QByteArray fileData = file.readAll();
  file.close();

  bool isBinary = false;
  int nullCount = 0;
  int sampleSize = qMin(512, fileData.size());
  for (int i = 0; i < sampleSize; ++i) {
    if (fileData[i] == 0) {
      nullCount++;
      if (nullCount > 1) {
        isBinary = true;
        break;
      }
    }
  }

  hideWelcomeScreen();
  QApplication::setOverrideCursor(Qt::WaitCursor);

  Language lang = Language::PlainText; // Default language

  if (isBinary) {
    // Open in hex editor
    HexEditor *hexEditor = new HexEditor();
    hexEditor->setData(fileData);
    hexEditor->setProperty("fileName", fileName);

    connect(hexEditor, &HexEditor::modificationChanged, this,
            &TextEditor::documentWasModified);

    int index = tabWidget->addTab(hexEditor, "[HEX] " + strippedName(fileName));
    tabWidget->setCurrentIndex(index);
  } else {
    // Open in text editor
    CodeEditor *editor = new CodeEditor();
    editor->setPlainText(QString::fromUtf8(fileData));
    editor->setFileName(fileName);
    editor->document()->setModified(false);

    // Auto-detect language
    lang = detectLanguage(fileName);
    editor->setLanguage(lang);

    SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
    highlighter->setLanguage(lang);
    highlighters[editor] = highlighter;

    QFont font(editorFontFamily, fontSize);
    font.setStyleStrategy(QFont::PreferDefault);
    editor->setFont(font);
    editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth
                                            : QPlainTextEdit::NoWrap);
    applyThemeToEditor(editor, highlighter);

    connect(editor->document(), &QTextDocument::modificationChanged, this,
            &TextEditor::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
            &TextEditor::updateStatusBar);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this,
            &TextEditor::updateBreadcrumb);
    connect(editor, &CodeEditor::codeBlockDeleted, this,
            &TextEditor::onCodeBlockDeleted);
    if (keyHeatmap)
        connect(editor, &CodeEditor::keyPressed, keyHeatmap, [this](int key, const QString &text) {
          keyHeatmap->recordKey(text, key);
      });
    connect(editor, &CodeEditor::vimModeChanged, this, [this](const QString &mode) {
        if (vimModeLabel) {
            if (mode.isEmpty()) { vimModeLabel->hide(); return; }
            vimModeLabel->setText("  " + mode + "  ");
            bool isNormal = (mode == "NORMAL");
            vimModeLabel->setStyleSheet(QString(
                "color: #282c34; background-color: %1; padding: 1px 8px; "
                "font-family: Consolas; font-weight: bold; font-size: 10px;")
                .arg(isNormal ? "#c678dd" : "#98c379"));
            vimModeLabel->show();
        }
    });
    // Apply CRT if currently enabled
    if (crtAct && crtAct->isChecked())
        editor->setCRTEnabled(true);
    if (vimModeAct && vimModeAct->isChecked())
        editor->setVimEnabled(true);
    if (paranoiaModeAct && paranoiaModeAct->isChecked())
        editor->setParanoiaMode(true);
    if (vulnScanAct && vulnScanAct->isChecked())
        editor->setVulnScanEnabled(true);
    if (focusFadeAct && focusFadeAct->isChecked())
        editor->setFocusFadeEnabled(true);
    if (imagePreviewAct && imagePreviewAct->isChecked())
        editor->setImagePreviewEnabled(true);
    propagateV080Settings(editor);
    propagateV090Settings(editor);
    connect(editor, &CodeEditor::keyPressed, this, &TextEditor::trackKeystroke);
    if (!paranoiaMode) ++sessionFilesOpened;

    // Apply ambient tint immediately so the new editor matches others
    updateAmbientTheme();

    int index = tabWidget->addTab(editor, strippedName(fileName));
    tabWidget->setCurrentIndex(index);

    watchFile(fileName);
  }

  QApplication::restoreOverrideCursor();
  updateRecentFiles(fileName);

  // Update language label
  if (isBinary) {
    languageLabel->setText("Binary (Hex)");
  } else {
    QStringList langNames = {"Plain Text", "C++",  "Python",  "JavaScript",
                             "HTML",       "CSS",  "Rust",    "Go",
                             "JSON",       "YAML", "Markdown", "Solidity", "Yul"};
    languageLabel->setText(langNames[static_cast<int>(lang)]);
  }

  statusBar()->showMessage("File loaded", 2000);
  QTimer::singleShot(0, this, &TextEditor::clampToScreen);
}

bool TextEditor::saveFileToPath(const QString &fileName) {
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  
  // Temporarily unwatch to prevent false "modified externally" alert
  unwatchFile(fileName);
  
  QSaveFile file(fileName);
  if (file.open(QFile::WriteOnly)) {
    CodeEditor *editor = currentEditor();
    HexEditor *hexEditor =
        qobject_cast<HexEditor *>(tabWidget->currentWidget());
    if (editor) {
      // Trim trailing whitespace
      QString text = editor->toPlainText();
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        while (lines[i].endsWith(' ') || lines[i].endsWith('\t')) {
          lines[i].chop(1);
        }
      }
      text = lines.join('\n');

      QTextStream out(&file);
      out << text;
      if (!file.commit()) {
        QGuiApplication::restoreOverrideCursor();
        watchFile(fileName); // Re-watch on failure
        QMessageBox::warning(this, "Jim",
                             QString("Cannot write file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
        return false;
      }
    } else if (hexEditor) {
      file.write(hexEditor->data());
      if (!file.commit()) {
        QGuiApplication::restoreOverrideCursor();
        watchFile(fileName); // Re-watch on failure
        QMessageBox::warning(this, "Jim",
                             QString("Cannot write file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
        return false;
      }
      hexEditor->setModified(false);
      hexEditor->setProperty("fileName", fileName);
    }
  } else {
    QGuiApplication::restoreOverrideCursor();
    watchFile(fileName); // Re-watch on failure
    QMessageBox::warning(this, "Jim",
                         QString("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
    return false;
  }
  QApplication::restoreOverrideCursor();
  
  // Re-watch after successful save
  watchFile(fileName);
  
  setCurrentFile(fileName);
  updateRecentFiles(fileName);
  statusBar()->showMessage("File saved", 2000);
  return true;
}

void TextEditor::setCurrentFile(const QString &fileName) {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return;
  unwatchFile(editor->getFileName());
  editor->setFileName(fileName);
  editor->document()->setModified(false);
  // Re-detect language
  Language lang = detectLanguage(fileName);
  editor->setLanguage(lang);
  SyntaxHighlighter *hl = highlighters.value(editor);
  if (hl) {
    hl->setLanguage(lang);
  }
  QString shownName = strippedName(fileName);
  tabWidget->setTabText(tabWidget->currentIndex(), shownName);
  setWindowTitle(shownName + " - Jim");
  watchFile(fileName);
}

QString TextEditor::strippedName(const QString &fullFileName) {
  return QFileInfo(fullFileName).fileName();
}

void TextEditor::updateRecentFiles(const QString &fileName) {
  if (paranoiaMode) return;
  recentFiles.removeAll(fileName);
  recentFiles.prepend(fileName);
  while (recentFiles.size() > 10)
    recentFiles.removeLast();
  updateRecentFilesMenu();
}

void TextEditor::updateRecentFilesMenu() {
  recentFilesMenu->clear();
  for (const QString &file : recentFiles) {
    QAction *action = new QAction(strippedName(file), this);
    action->setData(file);
    action->setStatusTip(file);
    connect(action, &QAction::triggered, this, &TextEditor::openRecentFile);
    recentFilesMenu->addAction(action);
  }
  if (recentFiles.isEmpty()) {
    QAction *noFilesAction = new QAction("No recent files", this);
    noFilesAction->setEnabled(false);
    recentFilesMenu->addAction(noFilesAction);
  }
}

CodeEditor *TextEditor::currentEditor() {
  return qobject_cast<CodeEditor *>(tabWidget->currentWidget());
}
SyntaxHighlighter *TextEditor::currentHighlighter() {
  CodeEditor *editor = currentEditor();
  return editor ? highlighters.value(editor) : nullptr;
}

void TextEditor::toggleSplitView() {
  splitViewEnabled = !splitViewEnabled;
  splitViewAct->setChecked(splitViewEnabled);
  if (splitViewEnabled) {
    if (!tabWidget2) {
      tabWidget2 = new DraggableTabWidget();
      tabWidget2->setTabsClosable(true);
      tabWidget2->setMovable(true);
      
      connect(tabWidget2, &QTabWidget::tabCloseRequested, this, [this](int index) {
          if (maybeSave(index)) {
              CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget2->widget(index));
              if (editor) {
                  unwatchFile(editor->getFileName());
                  highlighters.remove(editor);
              }
              tabWidget2->removeTab(index);
          }
      });
      connect(tabWidget2, &QTabWidget::currentChanged, this, &TextEditor::tabChanged);
      
      mainSplitter->addWidget(tabWidget2);
    }
    tabWidget2->show();
  } else {
    if (tabWidget2)
      tabWidget2->hide();
  }
}

void TextEditor::changeTheme() {
  currentThemeIndex = (currentThemeIndex + 1) % themes.size();
  applyThemeToAllEditors();
  statusBar()->showMessage(
      QString("Theme: %1").arg(themes[currentThemeIndex].name), 2000);
}

void TextEditor::initializeThemes() {
  ColorTheme light;
  light.name = "Light";
  light.background = QColor(255, 255, 255);
  light.foreground = QColor(0, 0, 0);
  light.lineNumberBg = QColor(240, 240, 240);
  light.lineNumberFg = QColor(128, 128, 128);
  light.currentLine = QColor(255, 255, 200);
  light.selection = QColor(0, 120, 215);
  light.keyword = QColor(0, 0, 255);
  light.string = QColor(0, 128, 0);
  light.comment = QColor(128, 128, 128);
  light.number = QColor(128, 0, 128);
  light.function = QColor(255, 140, 0);
  themes.append(light);

  ColorTheme dark;
  dark.name = "Dark";
  dark.background = QColor(30, 30, 30);
  dark.foreground = QColor(220, 220, 220);
  dark.lineNumberBg = QColor(40, 40, 40);
  dark.lineNumberFg = QColor(128, 128, 128);
  dark.currentLine = QColor(50, 50, 50);
  dark.selection = QColor(0, 120, 215);
  dark.keyword = QColor(86, 156, 214);
  dark.string = QColor(206, 145, 120);
  dark.comment = QColor(106, 153, 85);
  dark.number = QColor(181, 206, 168);
  dark.function = QColor(220, 220, 170);
  themes.append(dark);

  ColorTheme monokai;
  monokai.name = "Monokai";
  monokai.background = QColor(39, 40, 34);
  monokai.foreground = QColor(248, 248, 242);
  monokai.lineNumberBg = QColor(49, 50, 44);
  monokai.lineNumberFg = QColor(144, 144, 138);
  monokai.currentLine = QColor(62, 63, 55);
  monokai.selection = QColor(73, 72, 62);
  monokai.keyword = QColor(249, 38, 114);
  monokai.string = QColor(230, 219, 116);
  monokai.comment = QColor(117, 113, 94);
  monokai.number = QColor(174, 129, 255);
  monokai.function = QColor(166, 226, 46);
  themes.append(monokai);

  ColorTheme noir;
  noir.name = "Noir Edition";
  noir.background = QColor("#0a0a0a");
  noir.foreground = QColor("#c0c0c0");
  noir.selection = QColor("#333333");
  noir.lineNumberBg = QColor("#111111");
  noir.lineNumberFg = QColor("#555555");
  noir.currentLine = QColor("#1a1a1a");
  noir.keyword = QColor("#ff4444");
  noir.string = QColor("#999999");
  noir.comment = QColor("#666666");
  noir.number = QColor("#bbbbbb");
  noir.function = QColor("#e0e0e0");
  themes.append(noir);

  currentThemeIndex = 1; // Default to dark
}

void TextEditor::applyThemeToEditor(CodeEditor *editor,
                                    SyntaxHighlighter *highlighter) {
  if (!editor)
    return;
  const ColorTheme &theme = themes[currentThemeIndex];
  editor->applyTheme(theme);
  if (highlighter)
    highlighter->applyTheme(theme);
}

void TextEditor::applyThemeToAllEditors() {
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor)
      applyThemeToEditor(editor, highlighters.value(editor));
  }
}

void TextEditor::openFolder() {
  QString folder =
      QFileDialog::getExistingDirectory(this, "Open Folder", QDir::homePath());
  if (!folder.isEmpty()) {
    currentFolder = folder;
    fileSystemModel->setRootPath(folder);
    fileTree->setRootIndex(fileSystemModel->index(folder));

    // Show file tree, hide empty state wrapper
    if (fileTreeContainer)
      fileTreeContainer->setCurrentIndex(1);

    // Update terminal working directory
    if (terminalWidget)
      terminalWidget->setWorkingDirectory(folder);

    statusBar()->showMessage("Opened folder: " + folder, 2000);
  }
}

void TextEditor::toggleFileTree() {
  if (fileTreeDock->isVisible())
    fileTreeDock->hide();
  else
    fileTreeDock->show();
}

void TextEditor::onFileTreeDoubleClicked(const QModelIndex &index) {
  QString filePath = fileSystemModel->filePath(index);
  QFileInfo fileInfo(filePath);
  if (fileInfo.isFile()) {
    for (int i = 0; i < tabWidget->count(); ++i) {
      CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
      if (editor && editor->getFileName() == filePath) {
        tabWidget->setCurrentIndex(i);
        return;
      }
    }
    loadFile(filePath);
  }
}

void TextEditor::customizeColors() {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return;
  QColor bgColor =
      QColorDialog::getColor(Qt::white, this, "Choose Background Color");
  if (bgColor.isValid()) {
    QPalette p = editor->palette();
    p.setColor(QPalette::Base, bgColor);
    int brightness =
        (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) /
        1000;
    QColor textColor = brightness > 128 ? Qt::black : Qt::white;
    p.setColor(QPalette::Text, textColor);
    editor->setPalette(p);
    editor->update();
  }
}

void TextEditor::openFilePath(const QString &filePath) {
  QFileInfo fileInfo(filePath);
  if (fileInfo.exists() && fileInfo.isFile())
    loadFile(filePath);
}

void TextEditor::openFolderPath(const QString &folderPath) {
  QFileInfo fileInfo(folderPath);
  if (fileInfo.exists() && fileInfo.isDir()) {
    currentFolder = folderPath;
    fileTree->setRootIndex(fileSystemModel->index(folderPath));
    fileTreeDock->show();
    statusBar()->showMessage("Opened folder: " + folderPath, 2000);
  }
}

void TextEditor::toggleMiniMap() {
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor) {
      MiniMap *miniMap = editor->getMiniMap();
      if (miniMap) {
        if (miniMapAct->isChecked())
          miniMap->show();
        else
          miniMap->hide();
        QResizeEvent event(editor->size(), editor->size());
        QApplication::sendEvent(editor, &event);
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Markdown Preview
// ─────────────────────────────────────────────────────────────────────────────

void TextEditor::toggleMarkdownPreview()
{
    if (markdownPreview && markdownPreview->isVisible()) {
        // Hide: collapse and remove from splitter
        disconnectMarkdownPreview();
        markdownPreview->hide();
        markdownPreviewAct->setChecked(false);
        return;
    }

    // Only makes sense for text editors (Markdown or any file)
    CodeEditor *editor = currentEditor();
    if (!editor) {
        markdownPreviewAct->setChecked(false);
        return;
    }

    // Create preview widget on first use
    if (!markdownPreview) {
        markdownPreview = new MarkdownPreviewWidget();
        markdownPreview->setMinimumWidth(280);
    }

    // Set up debounce timer
    if (!markdownTimer) {
        markdownTimer = new QTimer(this);
        markdownTimer->setSingleShot(true);
        markdownTimer->setInterval(400);
        connect(markdownTimer, &QTimer::timeout, this, &TextEditor::updateMarkdownPreview);
    }

    // Add to the horizontal splitter alongside the tab widget
    mainSplitter->addWidget(markdownPreview);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(mainSplitter->count() - 1, 2);

    // Animate the preview in with a width animation
    markdownPreview->show();
    {
        auto *eff = new QGraphicsOpacityEffect(markdownPreview);
        markdownPreview->setGraphicsEffect(eff);
        auto *anim = new QPropertyAnimation(eff, "opacity", this);
        anim->setDuration(250);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    connectMarkdownPreview(editor);
    updateMarkdownPreview();
    markdownPreviewAct->setChecked(true);
}

void TextEditor::connectMarkdownPreview(CodeEditor *editor)
{
    // Disconnect previous editor if any
    disconnectMarkdownPreview();
    markdownEditor = editor;
    if (editor) {
        connect(editor->document(), &QTextDocument::contentsChanged,
                markdownTimer,       qOverload<>(&QTimer::start));
    }
}

void TextEditor::disconnectMarkdownPreview()
{
    if (markdownEditor) {
        disconnect(markdownEditor->document(), &QTextDocument::contentsChanged,
                   markdownTimer, qOverload<>(&QTimer::start));
        markdownEditor = nullptr;
    }
}

void TextEditor::updateMarkdownPreview()
{
    if (!markdownPreview || !markdownPreview->isVisible()) return;

    CodeEditor *editor = currentEditor();
    if (!editor) {
        markdownPreview->setContent("*No markdown file open.*");
        return;
    }

    // Re-connect if the tab changed
    if (editor != markdownEditor)
        connectMarkdownPreview(editor);

    QString baseDir;
    if (!editor->getFileName().isEmpty())
        baseDir = QFileInfo(editor->getFileName()).absolutePath();

    markdownPreview->setContent(editor->toPlainText(), baseDir);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Animation helpers
// ─────────────────────────────────────────────────────────────────────────────

void TextEditor::flashTabLabel(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= tabWidget->count()) return;
    // Briefly change the tab text colour to accent blue then fade back via timer
    QTabBar *bar = tabWidget->tabBar();
    bar->setTabTextColor(tabIndex, QColor("#569cd6"));
    QTimer::singleShot(600, this, [bar, tabIndex]() {
        bar->setTabTextColor(tabIndex, QColor()); // reset to stylesheet default
    });
}

void TextEditor::flashStatusMessage(const QString &msg, const QColor &color, int ms) {
    statusBar()->showMessage(msg, ms);
    QString prev = statusLabel->styleSheet();
    statusLabel->setStyleSheet(
        QString("color: %1; font-weight: bold; background: #252526; "
                "padding: 1px 10px; border-radius: 8px; font-size: 11px; margin: 2px 2px;")
        .arg(color.name()));
    QTimer::singleShot(ms, this, [this, prev]() {
        statusLabel->setStyleSheet(prev);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tools – open in disassembler / binary inspector
// ─────────────────────────────────────────────────────────────────────────────

void TextEditor::openInDisassembler(const QString &filePath) {
    if (filePath.isEmpty()) return;

    // Check if a disassembler tab for this file is already open
    for (int i = 0; i < tabWidget->count(); ++i) {
        DisassemblerWidget *w = qobject_cast<DisassemblerWidget *>(tabWidget->widget(i));
        if (w && w->getFilePath() == filePath) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }

    hideWelcomeScreen();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto *dw = new DisassemblerWidget();
    dw->loadFile(filePath);

    QString label = "[ASM] " + strippedName(filePath);
    int idx = tabWidget->addTab(dw, label);
    tabWidget->setCurrentIndex(idx);
    flashTabLabel(idx);

    QApplication::restoreOverrideCursor();
    flashStatusMessage("Disassembling " + strippedName(filePath) + "…",
                       QColor("#4ec9b0"), 3000);
}

void TextEditor::openInBinaryInspector(const QString &filePath) {
    if (filePath.isEmpty()) return;

    // Check if an inspector tab for this file is already open
    for (int i = 0; i < tabWidget->count(); ++i) {
        BinaryInspectorWidget *w =
            qobject_cast<BinaryInspectorWidget *>(tabWidget->widget(i));
        if (w && w->getFilePath() == filePath) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }

    hideWelcomeScreen();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto *bw = new BinaryInspectorWidget();
    bw->loadFile(filePath);

    QString label = "[BIN] " + strippedName(filePath);
    int idx = tabWidget->addTab(bw, label);
    tabWidget->setCurrentIndex(idx);
    flashTabLabel(idx);

    QApplication::restoreOverrideCursor();
    flashStatusMessage("Inspected " + strippedName(filePath),
                       QColor("#4ec9b0"), 2500);
}

// ── Public slots called by menu actions ──────────────────────────────────────

void TextEditor::openDisassembler() {
    // Try to use the current tab's file; otherwise show a file picker
    QString path;
    CodeEditor *ed = currentEditor();
    if (ed && !ed->getFileName().isEmpty()) {
        path = ed->getFileName();
    } else {
        HexEditor *hex = qobject_cast<HexEditor *>(tabWidget->currentWidget());
        if (hex)
            path = hex->property("fileName").toString();
    }

    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(
            this, "Select Binary to Disassemble", "",
            "Executables & Libraries (*.exe *.dll *.so *.o *.out *.elf);;"
            "All Files (*)");
    }
    if (!path.isEmpty())
        openInDisassembler(path);
}

void TextEditor::openNeuralGraph() {
    QString folder = currentFolder.isEmpty() ? QDir::currentPath() : currentFolder;
    QString activeFile;
    if (CodeEditor *ed = currentEditor())
        activeFile = ed->getFileName();
    // If no folder is set but a file is open, use its directory
    if (currentFolder.isEmpty() && !activeFile.isEmpty())
        folder = QFileInfo(activeFile).absolutePath();
    CodeGraph *graph = new CodeGraph(folder, activeFile, this);
    graph->exec();
}

void TextEditor::openGhostReplay() {
    CodeEditor *ed = currentEditor();
    if (!ed) return;
    
    QDialog *replayDialog = new QDialog(this);
    replayDialog->setWindowTitle("Ghost Replay Mode: " + ed->getFileName());
    replayDialog->resize(800, 600);
    QVBoxLayout *layout = new QVBoxLayout(replayDialog);
    QPlainTextEdit *replayEditor = new QPlainTextEdit(replayDialog);
    replayEditor->setReadOnly(true);
    replayEditor->setStyleSheet(ed->styleSheet());
    layout->addWidget(replayEditor);
    
    QTimer *playbackTimer = new QTimer(replayDialog);
    int eventIndex = 0;
    connect(playbackTimer, &QTimer::timeout, replayDialog, [=]() mutable {
        if (eventIndex >= ed->ghostLog.size()) {
            playbackTimer->stop();
            return;
        }
        GhostEvent ev = ed->ghostLog[eventIndex];
        QTextCursor c(replayEditor->document());
        c.setPosition(ev.position);
        if (ev.charsRemoved > 0) {
            c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, ev.charsRemoved);
            c.removeSelectedText();
        }
        if (!ev.textAdded.isEmpty()) {
            c.insertText(ev.textAdded);
        }
        eventIndex++;
    });
    playbackTimer->start(150); 
    
    replayDialog->exec();
}

void TextEditor::openScratchpad() {
    // If already open, just switch to it
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->tabText(i) == "📝 Scratchpad") {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }

    hideWelcomeScreen();

    if (!scratchpadEditor) {
        scratchpadEditor = new QPlainTextEdit();
        scratchpadEditor->setFont(QFont("Consolas", fontSize));
        scratchpadEditor->setStyleSheet(
            "QPlainTextEdit { background:#1a1a2e; color:#e0e0e0; "
            "border:none; font-family:Consolas,monospace; }");
        scratchpadEditor->setPlaceholderText(
            "Scratchpad — jot anything here. Saved automatically.");

        // Load persisted content
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + "/jim_scratchpad.txt";
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            scratchpadEditor->setPlainText(f.readAll());

        // Auto-save on every change
        connect(scratchpadEditor, &QPlainTextEdit::textChanged, this, [this]() {
            if (paranoiaMode) return;
            QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + "/jim_scratchpad.txt";
            QDir().mkpath(QFileInfo(path).absolutePath());
            QSaveFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(scratchpadEditor->toPlainText().toUtf8());
                f.commit();
            }
        });
    }

    int idx = tabWidget->addTab(scratchpadEditor, "📝 Scratchpad");
    tabWidget->setCurrentIndex(idx);
    scratchpadEditor->setFocus();
    QTimer::singleShot(0, this, &TextEditor::clampToScreen);
}

// ── v1.9 Feature Implementations ──────────────────────────────────────────────

void TextEditor::toggleFocusFade() {
    bool enabled = focusFadeAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setFocusFadeEnabled(enabled);
    }
    flashStatusMessage(enabled ? "Focus Fade: ON" : "Focus Fade: OFF",
                       QColor("#61afef"), 2000);
}

void TextEditor::toggleImagePreview() {
    bool enabled = imagePreviewAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setImagePreviewEnabled(enabled);
    }
    flashStatusMessage(enabled ? "Image Preview: ON (hover over image paths)" : "Image Preview: OFF",
                       QColor("#61afef"), 2000);
}

void TextEditor::trackKeystroke(int key, const QString &text) {
    if (paranoiaMode) return;
    Q_UNUSED(text)
    sessionKeystrokes++;
    if (key == Qt::Key_Return || key == Qt::Key_Enter)
        sessionLinesWritten++;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    statsKeystrokeTimestamps.append(now);
    // Prune timestamps older than 60 seconds
    qint64 cutoff = now - 60000;
    while (!statsKeystrokeTimestamps.isEmpty() && statsKeystrokeTimestamps.first() < cutoff)
        statsKeystrokeTimestamps.removeFirst();

    // WPM = (keystrokes in window / 5) / (window_minutes)
    int wpm = 0;
    if (statsKeystrokeTimestamps.size() >= 2) {
        double windowSec = (now - statsKeystrokeTimestamps.first()) / 1000.0;
        if (windowSec > 0)
            wpm = static_cast<int>((statsKeystrokeTimestamps.size() / 5.0) / (windowSec / 60.0));
    }
    sessionPeakWPM = qMax(sessionPeakWPM, wpm);
}

void TextEditor::showSessionStats() {
    // Compute active time from session timer
    int totalSecs = sessionSecondsAccumulated;
    if (sessionTimer && sessionTimer->isActive()) {
        QDateTime now = QDateTime::currentDateTime();
        totalSecs += static_cast<int>(sessionStart.secsTo(now));
    }
    int hours   = totalSecs / 3600;
    int minutes = (totalSecs % 3600) / 60;
    int secs    = totalSecs % 60;
    QString timeStr = QString("%1h %2m %3s").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));

    QDialog dlg(this);
    dlg.setWindowTitle("Session Statistics");
    dlg.setFixedSize(340, 280);
    dlg.setStyleSheet(
        "QDialog { background:#1e1e2e; color:#cdd6f4; }"
        "QLabel  { color:#cdd6f4; font-family:Consolas,monospace; }"
        "QPushButton { background:#313244; color:#cdd6f4; border:1px solid #45475a; "
        "              border-radius:6px; padding:6px 20px; font-family:Consolas; }"
        "QPushButton:hover { background:#45475a; }");

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(10);

    auto *title = new QLabel("  Session Stats", &dlg);
    title->setStyleSheet("font-size:15px; font-weight:bold; color:#89b4fa;");
    layout->addWidget(title);

    auto addRow = [&](const QString &icon, const QString &label, const QString &value) {
        auto *row = new QLabel(QString("%1  <span style='color:#a6e3a1'>%2</span>"
                                       "  <span style='color:#cdd6f4'>%3</span>").arg(icon, label, value), &dlg);
        row->setTextFormat(Qt::RichText);
        row->setStyleSheet("font-size:12px; padding:2px 0;");
        layout->addWidget(row);
    };

    addRow("⌨", "Keystrokes:",   QString::number(sessionKeystrokes));
    addRow("↵", "Lines written:", QString::number(sessionLinesWritten));
    addRow("📂", "Files opened:", QString::number(sessionFilesOpened));
    addRow("⏱", "Active time:",  timeStr);
    addRow("🚀", "Peak WPM:",    QString::number(sessionPeakWPM));

    layout->addStretch();

    auto *closeBtn = new QPushButton("Close", &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);

    dlg.exec();
}

void TextEditor::openCommandPalette() {
    if (!commandPalette)
        commandPalette = new CommandPalette(this);

    // Collect every QAction from all menus recursively
    QList<QAction*> actions;
    std::function<void(QMenu*)> collect = [&](QMenu *menu) {
        for (QAction *act : menu->actions()) {
            if (act->isSeparator()) continue;
            if (act->menu()) { collect(act->menu()); continue; }
            if (!act->text().isEmpty())
                actions.append(act);
        }
    };
    for (QAction *act : customMenuBar->actions()) {
        if (act->menu()) collect(act->menu());
    }

    commandPalette->populate(actions);

    // Centre it below the menu bar
    QRect geo = geometry();
    int cx = geo.left() + (geo.width() - commandPalette->width()) / 2;
    int cy = geo.top() + 60;
    commandPalette->move(cx, cy);
    commandPalette->exec();
}

void TextEditor::toggleTodoPanel() {
    if (!todoPanel) {
        todoPanel = new TodoPanel(this);
        todoDock = new QDockWidget("TODO / FIXME", this);
        todoDock->setObjectName("todoDock");
        todoDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
        todoDock->setWidget(todoPanel);
        todoDock->setStyleSheet(
            "QDockWidget { color:#cccccc; font-family:Consolas; font-size:11px; }"
            "QDockWidget::title { background:#252526; padding:4px 8px; "
            "                     border-bottom:1px solid #3c3c3c; }");
        addDockWidget(Qt::BottomDockWidgetArea, todoDock);

        connect(todoPanel, &TodoPanel::jumpRequested, this, &TextEditor::onTodoJump);
        connect(todoDock, &QDockWidget::visibilityChanged, this, [this](bool vis) {
            if (todoAct) todoAct->setChecked(vis);
            if (vis) todoPanel->scan(tabWidget);
        });
    }

    bool show = !todoDock->isVisible();
    todoDock->setVisible(show);
    if (todoAct) todoAct->setChecked(show);
    if (show) todoPanel->scan(tabWidget);
}

void TextEditor::onTodoJump(const QString &filePath, int line) {
    if (filePath.isEmpty()) {
        // Refresh button pressed
        if (todoPanel) todoPanel->scan(tabWidget);
        return;
    }
    // Find the tab with this file and jump to the line
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed && ed->getFileName() == filePath) {
            tabWidget->setCurrentIndex(i);
            QTextCursor cur = ed->textCursor();
            cur.movePosition(QTextCursor::Start);
            cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, line);
            ed->setTextCursor(cur);
            ed->centerCursor();
            ed->setFocus();
            return;
        }
    }
    // File not open — load it
    loadFile(filePath);
    QTimer::singleShot(100, this, [this, line]() {
        CodeEditor *ed = currentEditor();
        if (!ed) return;
        QTextCursor cur = ed->textCursor();
        cur.movePosition(QTextCursor::Start);
        cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, line);
        ed->setTextCursor(cur);
        ed->centerCursor();
    });
}

void TextEditor::toggleGraveyard() {
    bool visible = !graveyardDock->isVisible();
    graveyardDock->setVisible(visible);
    if (graveyardAct) graveyardAct->setChecked(visible);
}

// ── Security Pack Implementations ───────────────────────────────────────────

void TextEditor::toggleParanoiaMode() {
    paranoiaMode = paranoiaModeAct->isChecked();
    if (paranoiaLabel) {
        paranoiaLabel->setVisible(paranoiaMode);
    }
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setParanoiaMode(paranoiaMode);
    }
    if (paranoiaMode) {
        flashStatusMessage("PARANOIA MODE: ON — session leaves no trace", QColor("#e81123"), 3000);
    } else {
        flashStatusMessage("PARANOIA MODE: OFF", QColor("#569cd6"), 2000);
    }
}

void TextEditor::toggleVulnScan() {
    bool enabled = vulnScanAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setVulnScanEnabled(enabled);
    }
    if (enabled) {
        flashStatusMessage("⚡ Vuln Scanner: ON — laser underlines active", QColor("#ff2828"), 3000);
    } else {
        flashStatusMessage("Vuln Scanner: OFF", QColor("#569cd6"), 2000);
    }
}

void TextEditor::toggleCRT() {
    bool enabled = crtAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setCRTEnabled(enabled);
    }
    flashStatusMessage(enabled ? "CRT Effect: ON" : "CRT Effect: OFF",
                       QColor("#66fcf1"), 2000);
}

void TextEditor::onCodeBlockDeleted(const QString &code, const QString &source) {
    if (graveyardWidget)
        graveyardWidget->addSnippet(code, source);
    // Auto-show graveyard dock briefly
    if (graveyardDock && !graveyardDock->isVisible()) {
        graveyardDock->show();
        if (graveyardAct) graveyardAct->setChecked(true);
    }
}

void TextEditor::toggleKeyHeatmap() {
    if (!keyHeatmap) return;
    if (keyHeatmap->isVisible())
        keyHeatmap->hide();
    else
        keyHeatmap->show();
}

void TextEditor::toggleVimMode() {
    bool enabled = vimModeAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setVimEnabled(enabled);
    }
    if (vimModeLabel)
        vimModeLabel->setVisible(enabled);
    flashStatusMessage(enabled ? "Vim Mode: ON  (Esc = Normal)" : "Vim Mode: OFF",
                       QColor("#c678dd"), 2500);
}

void TextEditor::updateAmbientTheme() {
    int hour = QTime::currentTime().hour();
    QColor tint;
    if      (hour >=  5 && hour <  8) tint = QColor(255, 160,  80, 22); // dawn — warm amber
    else if (hour >= 17 && hour < 20) tint = QColor(255, 100,  40, 28); // dusk — deep orange
    else if (hour >= 20 || hour <  5) tint = QColor( 40,  70, 200, 20); // night — cool blue
    // else: daytime — no tint (default theme bg)

    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabWidget->widget(i));
        if (ed) ed->setAmbientBackground(tint);
    }
}

void TextEditor::openBinaryInspector() {
    QString path;
    CodeEditor *ed = currentEditor();
    if (ed && !ed->getFileName().isEmpty()) {
        path = ed->getFileName();
    } else {
        HexEditor *hex = qobject_cast<HexEditor *>(tabWidget->currentWidget());
        if (hex)
            path = hex->property("fileName").toString();
    }

    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(
            this, "Select Binary to Inspect", "",
            "Executables & Libraries (*.exe *.dll *.so *.o *.out *.elf);;"
            "All Files (*)");
    }
    if (!path.isEmpty())
        openInBinaryInspector(path);
}

// ── File-tree context menu ────────────────────────────────────────────────────

void TextEditor::onFileTreeContextMenu(const QPoint &pos) {
    QModelIndex idx = fileTree->indexAt(pos);
    if (!idx.isValid()) return;

    QString filePath = fileSystemModel->filePath(idx);
    QFileInfo fi(filePath);
    if (!fi.isFile()) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background:#2d2d30; color:#cccccc; border:1px solid #454545; "
        "        border-radius:6px; padding:4px; }"
        "QMenu::item { padding:6px 24px 6px 12px; border-radius:3px; }"
        "QMenu::item:selected { background:#094771; }"
        "QMenu::separator { height:1px; background:#3e3e42; margin:3px 8px; }");

    QAction *openAct       = menu.addAction("Open");
    QAction *openHexMenuAct   = menu.addAction("⬡  Open in Hex Editor");
    menu.addSeparator();
    QAction *disasmAct     = menu.addAction("⚙  Disassemble");
    QAction *inspectAct    = menu.addAction("🔍 Binary Inspector");
    menu.addSeparator();
    QAction *revealAct     = menu.addAction("Reveal in Explorer");

    QAction *chosen = menu.exec(fileTree->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == openAct) {
        loadFile(filePath);
    } else if (chosen == openHexMenuAct) {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray bytes = f.readAll();
            HexEditor *hex = new HexEditor();
            hex->setData(bytes);
            hex->setProperty("fileName", filePath);
            connect(hex, &HexEditor::modificationChanged,
                    this, &TextEditor::documentWasModified);
            hideWelcomeScreen();
            int tabIdx = tabWidget->addTab(hex, "[HEX] " + fi.fileName());
            tabWidget->setCurrentIndex(tabIdx);
            flashTabLabel(tabIdx);
        }
    } else if (chosen == disasmAct) {
        openInDisassembler(filePath);
    } else if (chosen == inspectAct) {
        openInBinaryInspector(filePath);
    } else if (chosen == revealAct) {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(fi.absolutePath()));
    }
}

void TextEditor::applyModernStyle() {
  QString style = R"(
        QMainWindow {
            background-color: #1e1e1e;
            border: none;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #cccccc;
            font-family: 'Segoe UI', 'Roboto', sans-serif;
        }
        QMenuBar {
            background-color: #323233;
            color: #cccccc;
            border: none;
            border-bottom: 1px solid #1e1e1e;
            padding: 0px;
            font-size: 13px;
        }
        QMenuBar::item {
            padding: 8px 14px;
            background: transparent;
            border-radius: 0px;
        }
        QMenuBar::item:selected {
            background-color: #505050;
        }
        QMenuBar::item:pressed {
            background-color: #094771;
        }
        QMenu {
            background-color: #2d2d30;
            color: #cccccc;
            border: 1px solid #454545;
            border-radius: 8px;
            padding: 6px;
            font-size: 13px;
        }
        QMenu::item {
            padding: 8px 28px 8px 16px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #094771;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3e3e42;
            margin: 4px 10px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #1e1e1e;
            top: -1px;
        }
        QTabBar {
            background-color: #252526;
        }
        QTabBar::tab {
            background-color: #2d2d30;
            color: #969696;
            padding: 10px 20px;
            border: none;
            border-right: 1px solid #252526;
            min-width: 100px;
            font-size: 13px;
        }
        QTabBar::tab:selected {
            background-color: #1e1e1e;
            color: #ffffff;
            border-top: 2px solid #007acc;
        }
        QTabBar::tab:hover:!selected {
            background-color: #2a2d2e;
            color: #cccccc;
        }
        QTabBar::close-button {
            subcontrol-position: right;
            margin: 4px;
            padding: 4px;
            border-radius: 3px;
            background-color: transparent;
            width: 16px;
            height: 16px;
        }
        QTabBar::close-button:hover {
            background-color: #e81123;
        }
        QStatusBar {
            background-color: #141414;
            color: #9e9e9e;
            border: none;
            border-top: 1px solid #2a2a2a;
            padding: 2px 4px;
            font-size: 11px;
        }
        QStatusBar QLabel {
            background-color: transparent;
            color: #cccccc;
            font-size: 11px;
        }
        QDockWidget {
            color: #cccccc;
            border: none;
            font-size: 12px;
        }
        QDockWidget::title {
            background-color: #252526;
            padding: 8px 12px;
            border: none;
            text-align: left;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 1px;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: transparent;
            border: none;
            padding: 2px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background-color: #3e3e42;
        }
        QTreeView {
            background-color: #252526;
            color: #cccccc;
            border: none;
            outline: none;
            show-decoration-selected: 1;
            font-size: 13px;
        }
        QTreeView::item {
            padding: 5px 4px;
            border: none;
        }
        QTreeView::item:hover {
            background-color: #2a2d2e;
        }
        QTreeView::item:selected {
            background-color: #094771;
            color: #ffffff;
        }
        QHeaderView::section {
            background-color: #252526;
            color: #cccccc;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #3e3e42;
            font-size: 12px;
        }
        QScrollBar:vertical {
            background-color: transparent;
            width: 14px;
            margin: 0;
            border: none;
        }
        QScrollBar::handle:vertical {
            background-color: rgba(121, 121, 121, 0.4);
            min-height: 30px;
            border-radius: 7px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: rgba(121, 121, 121, 0.7);
        }
        QScrollBar::handle:vertical:pressed {
            background-color: rgba(121, 121, 121, 0.9);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
        QScrollBar:horizontal {
            background-color: transparent;
            height: 14px;
            margin: 0;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background-color: rgba(121, 121, 121, 0.4);
            min-width: 30px;
            border-radius: 7px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: rgba(121, 121, 121, 0.7);
        }
        QScrollBar::handle:horizontal:pressed {
            background-color: rgba(121, 121, 121, 0.9);
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
        }
        QPlainTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: none;
            selection-background-color: #264f78;
            selection-color: #ffffff;
            font-family: 'Consolas', 'Fira Code', monospace;
        }
        QSplitter::handle {
            background-color: #3e3e42;
            width: 6px;
            height: 6px;
        }
        QSplitter::handle:hover {
            background-color: #007acc;
        }
        QSplitter::handle:vertical {
            height: 6px;
            margin: 0px;
        }
        QSplitter::handle:horizontal {
            width: 6px;
            margin: 0px;
        }
        QInputDialog {
            background-color: #2d2d30;
        }
        QMessageBox {
            background-color: #2d2d30;
        }
    )";
  setStyleSheet(style);
}

// ── v0.8.0 QoL Implementations ───────────────────────────────────────────────

void TextEditor::changeEvent(QEvent *e) {
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::ActivationChange && !isActiveWindow()) {
        // Auto-save on focus lost
        if (autoSaveFocusEnabled) {
            for (int i = 0; i < tabWidget->count(); ++i) {
                CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
                if (ed && ed->isModified() && !ed->getFileName().isEmpty())
                    saveFileToPath(ed->getFileName());
            }
        }
    }
}

bool TextEditor::eventFilter(QObject *obj, QEvent *event) {
    // Double-Shift detection for Search Everywhere
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Shift) {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - lastShiftPressMs < 400)
                openSearchEverywhere();
            lastShiftPressMs = now;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void TextEditor::switchHeaderSource() {
    CodeEditor *ed = currentEditor();
    if (!ed || ed->getFileName().isEmpty()) return;
    QFileInfo fi(ed->getFileName());
    QString ext = fi.suffix().toLower();
    QString companion;
    if (ext == "cpp" || ext == "cxx" || ext == "cc")
        companion = fi.absolutePath() + "/" + fi.completeBaseName() + ".h";
    else if (ext == "h" || ext == "hpp" || ext == "hxx")
        companion = fi.absolutePath() + "/" + fi.completeBaseName() + ".cpp";
    else {
        flashStatusMessage("No header/source counterpart for this file type", QColor("#ffb86c"), 2000);
        return;
    }
    if (QFileInfo::exists(companion)) {
        loadFile(companion);
    } else {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Create Counterpart",
            QString("'%1' doesn't exist. Create it?").arg(QFileInfo(companion).fileName()),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QFile f(companion);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream s(&f);
                if (companion.endsWith(".h")) {
                    QString guard = QFileInfo(companion).completeBaseName().toUpper() + "_H";
                    s << "#ifndef " << guard << "\n#define " << guard << "\n\n\n#endif // " << guard << "\n";
                } else {
                    s << "#include \"" << fi.completeBaseName() << ".h\"\n\n";
                }
            }
            loadFile(companion);
        }
    }
}

void TextEditor::locateCurrentFileInTree() {
    CodeEditor *ed = currentEditor();
    if (!ed || ed->getFileName().isEmpty()) return;
    // Ensure file tree is visible
    if (!fileTreeDock->isVisible()) {
        fileTreeDock->setVisible(true);
        if (fileTreeAct) fileTreeAct->setChecked(true);
    }
    QModelIndex idx = fileSystemModel->index(ed->getFileName());
    if (idx.isValid()) {
        fileTree->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        fileTree->setCurrentIndex(idx);
        flashStatusMessage("📁 Located: " + QFileInfo(ed->getFileName()).fileName(),
                           QColor("#569cd6"), 2000);
    }
}

void TextEditor::openSearchEverywhere() {
    if (!searchEverywhere) {
        searchEverywhere = new SearchEverywhere(this);
        connect(searchEverywhere, &SearchEverywhere::fileRequested,
                this, &TextEditor::loadFile);
    }
    // Gather all actions
    QList<QAction*> allActs;
    allActs << menuBar()->actions();
    // Gather open files
    QStringList openFiles;
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed && !ed->getFileName().isEmpty())
            openFiles << ed->getFileName();
    }
    searchEverywhere->populate(allActs, recentFiles, openFiles);
    searchEverywhere->exec();
}

void TextEditor::toggleStickyScroll() {
    stickyScrollEnabled = stickyScrollAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed) ed->setStickyScrollEnabled(stickyScrollEnabled);
    }
}

void TextEditor::toggleInvisibleChars() {
    invisibleCharsEnabled = invisibleCharsAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed) ed->setInvisibleCharsEnabled(invisibleCharsEnabled);
    }
}

void TextEditor::toggleGitBlame() {
    gitBlameEnabled = gitBlameAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed) ed->setGitBlameEnabled(gitBlameEnabled);
    }
}

void TextEditor::toggleAutoSaveOnFocusLost() {
    autoSaveFocusEnabled = autoSaveFocusAct->isChecked();
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (ed) ed->setAutoSaveOnFocusLost(autoSaveFocusEnabled);
    }
    if (autoSaveFocusEnabled)
        flashStatusMessage("Auto-Save on Focus Lost: ON", QColor("#4ec9b0"), 2000);
    else
        flashStatusMessage("Auto-Save on Focus Lost: OFF", QColor("#569cd6"), 2000);
}

void TextEditor::sendSelectionToScratchpad() {
    CodeEditor *ed = currentEditor();
    if (!ed || !ed->textCursor().hasSelection()) {
        flashStatusMessage("No selection to send", QColor("#ffb86c"), 1500);
        return;
    }
    QString sel = ed->textCursor().selectedText()
                      .replace(QChar::ParagraphSeparator, '\n');
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                   + "/jim_scratchpad.txt";
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << "\n\n--- " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
          << " [from " << (ed->getFileName().isEmpty() ? "untitled" : QFileInfo(ed->getFileName()).fileName())
          << "] ---\n";
        s << sel << "\n";
        f.close();
        flashStatusMessage("✓ Sent to Scratchpad", QColor("#4ec9b0"), 2000);
    }
}

void TextEditor::applyPaneDimming() {
    // Dim the non-active pane in split view
    if (!splitViewEnabled || !tabWidget2) return;
    bool pane1Active = tabWidget->currentWidget() && tabWidget->currentWidget()->hasFocus();
    // Apply semi-transparent overlay on the inactive pane container
    // We use GraphicsOpacityEffect on the entire inactive QTabWidget
    QTabWidget *active = pane1Active ? tabWidget : tabWidget2;
    QTabWidget *inactive = pane1Active ? tabWidget2 : tabWidget;
    Q_UNUSED(active);
    inactive->setWindowOpacity(0.65);
}

void TextEditor::propagateV080Settings(CodeEditor *ed) {
    if (!ed) return;
    ed->setStickyScrollEnabled(stickyScrollEnabled);
    ed->setInvisibleCharsEnabled(invisibleCharsEnabled);
    ed->setGitBlameEnabled(gitBlameEnabled);
    ed->setAutoSaveOnFocusLost(autoSaveFocusEnabled);
}

// ── SearchEverywhere Dialog Implementation ────────────────────────────────────

SearchEverywhere::SearchEverywhere(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *container = new QWidget();
    container->setObjectName("seContainer");
    container->setStyleSheet(
        "#seContainer {"
        "  background-color: #1a1a2e;"
        "  border: 1px solid #569cd6;"
        "  border-radius: 10px;"
        "}"
    );
    QVBoxLayout *cl = new QVBoxLayout(container);
    cl->setContentsMargins(10, 10, 10, 10);
    cl->setSpacing(6);

    searchBox = new QLineEdit();
    searchBox->setPlaceholderText("🔍  Search files, actions, recent...");
    searchBox->setStyleSheet(
        "QLineEdit {"
        "  background: #252540;"
        "  color: #cdd6f4;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-size: 15px;"
        "  font-family: Consolas;"
        "}"
    );
    cl->addWidget(searchBox);

    resultList = new QListWidget();
    resultList->setStyleSheet(
        "QListWidget {"
        "  background: transparent;"
        "  color: #cdd6f4;"
        "  border: none;"
        "  font-family: Consolas;"
        "  font-size: 12px;"
        "}"
        "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: #2d4a6e; color: #ffffff; }"
        "QListWidget::item:hover { background-color: #1e3a5a; }"
    );
    resultList->setMaximumHeight(300);
    cl->addWidget(resultList);

    layout->addWidget(container);
    setFixedWidth(580);

    connect(searchBox, &QLineEdit::textChanged, this, &SearchEverywhere::filter);
    connect(resultList, &QListWidget::itemDoubleClicked, this, &SearchEverywhere::runSelected);
    searchBox->installEventFilter(this);

    // Center on parent
    if (parent) {
        QPoint center = parent->geometry().center();
        move(center.x() - width()/2, center.y() - height()/2 - 100);
    }
}

void SearchEverywhere::populate(const QList<QAction*> &actions,
                                const QStringList &recentFiles,
                                const QStringList &openFiles) {
    allActions = actions;
    allRecent = recentFiles;
    allFiles = openFiles;
    searchBox->clear();
    filter("");
    searchBox->setFocus();
}

void SearchEverywhere::filter(const QString &text) {
    resultList->clear();
    QString q = text.trimmed().toLower();

    // Open files
    for (const QString &f : allFiles) {
        QString bn = QFileInfo(f).fileName();
        if (q.isEmpty() || bn.toLower().contains(q) || f.toLower().contains(q)) {
            QListWidgetItem *item = new QListWidgetItem("📄  " + bn);
            item->setData(Qt::UserRole, "file:" + f);
            item->setToolTip(f);
            resultList->addItem(item);
        }
    }
    // Recent files (not already open)
    for (const QString &f : allRecent) {
        QString bn = QFileInfo(f).fileName();
        if (allFiles.contains(f)) continue;
        if (q.isEmpty() || bn.toLower().contains(q) || f.toLower().contains(q)) {
            QListWidgetItem *item = new QListWidgetItem("🕐  " + bn + "  [recent]");
            item->setData(Qt::UserRole, "file:" + f);
            item->setForeground(QColor("#888888"));
            resultList->addItem(item);
        }
    }
    // Actions
    for (QAction *act : allActions) {
        if (act->isSeparator() || act->text().isEmpty()) continue;
        QString name = act->text().remove('&');
        if (q.isEmpty() || name.toLower().contains(q)) {
            QListWidgetItem *item = new QListWidgetItem("⚡  " + name);
            item->setData(Qt::UserRole, "action:" + name);
            item->setForeground(QColor("#569cd6"));
            resultList->addItem(item);
        }
    }

    if (resultList->count() > 0)
        resultList->setCurrentRow(0);
}

void SearchEverywhere::runSelected() {
    QListWidgetItem *item = resultList->currentItem();
    if (!item) return;
    QString data = item->data(Qt::UserRole).toString();
    if (data.startsWith("file:")) {
        emit fileRequested(data.mid(5));
        accept();
    } else if (data.startsWith("action:")) {
        QString name = data.mid(7);
        for (QAction *act : allActions) {
            if (act->text().remove('&') == name) {
                act->trigger();
                break;
            }
        }
        accept();
    }
}

void SearchEverywhere::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) { reject(); return; }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        runSelected(); return;
    }
    if (event->key() == Qt::Key_Down) {
        int next = qMin(resultList->currentRow() + 1, resultList->count() - 1);
        resultList->setCurrentRow(next); return;
    }
    if (event->key() == Qt::Key_Up) {
        int prev = qMax(resultList->currentRow() - 1, 0);
        resultList->setCurrentRow(prev); return;
    }
    QDialog::keyPressEvent(event);
}

bool SearchEverywhere::eventFilter(QObject *obj, QEvent *event) {
    if (obj == searchBox && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
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
// v0.9.0 — Web3Sec & Solidity Pack
// ============================================================

void TextEditor::propagateV090Settings(CodeEditor *ed) {
    if (!ed) return;
    ed->setGasMinimapEnabled(gasMiniMapEnabled);
}

void TextEditor::triggerGodView() {
    CodeEditor *ed = currentEditor();
    if (!ed) return;

    Language lang = ed->getLanguage();

    QTextBlock block = ed->document()->begin();
    int collapsed = 0;
    while (block.isValid()) {
        if (ed->isFoldable(block) && !ed->isFolded(block)) {
            QString text = block.text().trimmed();
            bool isFunctionBody = text.contains("function ") ||
                                  text.endsWith('{') ||
                                  text.contains(") {") ||
                                  text.contains(") public") ||
                                  text.contains(") private") ||
                                  text.contains(") external") ||
                                  text.contains(") internal");
            if (isFunctionBody || lang == Language::Solidity || lang == Language::CPP) {
                ed->toggleFoldAt(block.blockNumber());
                collapsed++;
            }
        }
        block = block.next();
    }

    if (collapsed == 0) {
        QTextBlock b = ed->document()->begin();
        while (b.isValid()) {
            if (ed->isFolded(b)) ed->toggleFoldAt(b.blockNumber());
            b = b.next();
        }
        flashStatusMessage("God View: expanded all blocks");
    } else {
        flashStatusMessage(QString("God View: collapsed %1 function bodies").arg(collapsed));
    }
}

void TextEditor::extractABI() {
    CodeEditor *ed = currentEditor();
    if (!ed) {
        flashStatusMessage("No active editor");
        return;
    }
    QString path = ed->getFileName();
    if (path.isEmpty() || !path.endsWith(".sol", Qt::CaseInsensitive)) {
        flashStatusMessage("Open a .sol file first");
        return;
    }

    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(QFileInfo(path).absolutePath());

    QStringList solcCandidates = {"solc", "solc-0.8", "/usr/local/bin/solc", "/usr/bin/solc"};
    QString solcPath;
    for (const QString &c : solcCandidates) {
        QProcess test;
        test.start(c, {"--version"});
        test.waitForFinished(1000);
        if (test.exitCode() == 0) { solcPath = c; break; }
    }

    if (solcPath.isEmpty()) {
        flashStatusMessage("solc not found — install Solidity compiler");
        proc->deleteLater();
        return;
    }

    flashStatusMessage("⚡ Running solc...");

    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
    this, [this, proc, path](int exitCode, QProcess::ExitStatus) {
        QByteArray out = proc->readAllStandardOutput();
        QByteArray err = proc->readAllStandardError();
        proc->deleteLater();

        if (exitCode != 0) {
            QMessageBox::warning(this, "solc Error",
                QString("solc failed:\n%1").arg(QString::fromUtf8(err).left(1000)));
            return;
        }

        QApplication::clipboard()->setText(QString::fromUtf8(out));

        QPalette p = palette();
        QPalette orig = p;
        p.setColor(QPalette::Window, QColor(0, 80, 0));
        setPalette(p);
        QTimer::singleShot(300, this, [this, orig]() { setPalette(orig); });

        flashStatusMessage(QString("✓ ABI copied to clipboard (%1 bytes)").arg(out.size()));

        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle("ABI / Bytecode — " + QFileInfo(path).fileName());
        dlg->resize(700, 400);
        auto *layout = new QVBoxLayout(dlg);
        auto *te = new QPlainTextEdit(dlg);
        te->setPlainText(QString::fromUtf8(out));
        te->setStyleSheet("background:#0d0d0d;color:#00ff88;font-family:Consolas;font-size:11px;");
        te->setReadOnly(true);
        layout->addWidget(te);
        auto *closeBtn = new QPushButton("Close", dlg);
        connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        layout->addWidget(closeBtn);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    proc->start(solcPath, {"--abi", "--bin", "--optimize", path});
}

void TextEditor::resolveFourByte() {
    CodeEditor *ed = currentEditor();
    QString selectedText;
    if (ed) {
        QTextCursor cur = ed->textCursor();
        selectedText = cur.selectedText().trimmed();
    }

    if (selectedText.isEmpty() || selectedText.length() > 10) {
        bool ok;
        selectedText = QInputDialog::getText(this, "4-Byte Signature Resolver",
            "Enter 4-byte hex selector (e.g. 0xa9059cbb or a9059cbb):",
            QLineEdit::Normal, selectedText.left(10), &ok);
        if (!ok || selectedText.isEmpty()) return;
    }

    QString hex = selectedText;
    if (hex.startsWith("0x", Qt::CaseInsensitive)) hex = hex.mid(2);
    hex = hex.toLower().left(8);

    if (hex.length() != 8) {
        flashStatusMessage("Invalid selector — need exactly 4 bytes (8 hex chars)");
        return;
    }

    flashStatusMessage(QString("\U0001F50D Looking up 0x%1 in 4byte.directory...").arg(hex));

    QUrl url(QString("https://www.4byte.directory/api/v1/signatures/?hex_signature=0x%1").arg(hex));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Jim-Editor/0.9.0");

    QNetworkReply *reply = networkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, hex, ed]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            flashStatusMessage("Network error: " + reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonArray results = obj["results"].toArray();

        if (results.isEmpty()) {
            flashStatusMessage(QString("0x%1 — not found in 4byte.directory").arg(hex));
            return;
        }

        QStringList sigs;
        for (const QJsonValue &v : results)
            sigs << v.toObject()["text_signature"].toString();

        QString best = sigs.first();
        flashStatusMessage(QString("0x%1 \u2192 %2").arg(hex, best));

        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle(QString("4-Byte: 0x%1").arg(hex));
        dlg->setFixedSize(480, 200);
        dlg->setStyleSheet("background:#0a0a1a;color:#00ffff;border:2px solid #00ffff;");
        auto *layout = new QVBoxLayout(dlg);
        auto *title = new QLabel(QString("<div style='color:#00ff88;font-size:16px;font-weight:bold;'>0x%1</div>").arg(hex), dlg);
        title->setTextFormat(Qt::RichText);
        layout->addWidget(title);
        for (const QString &sig : sigs) {
            auto *lbl = new QLabel(sig, dlg);
            lbl->setStyleSheet("color:#ffffff;font-family:Consolas;font-size:13px;padding:2px;");
            lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
            layout->addWidget(lbl);
        }
        auto *copyBtn = new QPushButton("Copy: " + best, dlg);
        copyBtn->setStyleSheet("background:#003333;color:#00ffcc;border:1px solid #00ffcc;padding:4px;");
        connect(copyBtn, &QPushButton::clicked, [best, dlg]() {
            QApplication::clipboard()->setText(best);
            dlg->accept();
        });
        layout->addWidget(copyBtn);

        if (ed) {
            QTextCursor cur = ed->textCursor();
            if (cur.hasSelection()) {
                auto *replaceBtn = new QPushButton("Replace selection with: " + best, dlg);
                replaceBtn->setStyleSheet("background:#003300;color:#00ff88;border:1px solid #00ff88;padding:4px;");
                connect(replaceBtn, &QPushButton::clicked, [ed, best, dlg]() {
                    ed->textCursor().insertText(best);
                    dlg->accept();
                });
                layout->addWidget(replaceBtn);
            }
        }

        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });
}

void TextEditor::triggerPanicButton() {
    QMessageBox::StandardButton reply = QMessageBox::warning(this,
        "\u26A0 PANIC BUTTON — CONFIRM",
        "This will:\n"
        "\u2022 Immediately close Jim\n"
        "\u2022 Overwrite scratchpad with zeroes (unrecoverable)\n"
        "\u2022 Clear all recent files history\n"
        "\u2022 Clear editor settings\n\n"
        "Are you absolutely sure?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QString scratchPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/jim_scratchpad.txt";
    QFile scratchFile(scratchPath);
    if (scratchFile.exists()) {
        qint64 sz = scratchFile.size();
        if (sz > 0 && scratchFile.open(QIODevice::WriteOnly)) {
            QByteArray zeroes(sz, '\0');
            for (int pass = 0; pass < 3; ++pass) {
                scratchFile.seek(0);
                scratchFile.write(zeroes);
                scratchFile.flush();
                if (pass == 1) {
                    zeroes.fill('\xFF');
                } else if (pass == 2) {
                    for (char &c : zeroes) c = (char)(qrand() % 256);
                }
            }
            scratchFile.close();
        }
        scratchFile.remove();
    }

    recentFiles.clear();
    QSettings settings;
    settings.remove("recentFiles");
    settings.remove("geometry");
    settings.remove("windowState");
    settings.sync();

    QApplication::quit();
}

void TextEditor::showStorageSlotVisualizer() {
    CodeEditor *ed = currentEditor();
    if (!ed) { flashStatusMessage("No active editor"); return; }

    QString code = ed->toPlainText();
    Language lang = ed->getLanguage();

    bool isSolidity = lang == Language::Solidity ||
                      ed->getFileName().endsWith(".sol", Qt::CaseInsensitive) ||
                      code.contains("pragma solidity") || code.contains("contract ");

    if (!isSolidity) {
        flashStatusMessage("Storage Slot Visualizer: open a .sol file first");
        return;
    }

    if (!slotVisualizerDock) {
        slotVisualizerWidget = new StorageSlotVisualizerWidget(this);
        slotVisualizerDock = new QDockWidget("\U0001F5C4 Storage Slots", this);
        slotVisualizerDock->setWidget(slotVisualizerWidget);
        slotVisualizerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        addDockWidget(Qt::RightDockWidgetArea, slotVisualizerDock);
        slotVisualizerDock->setStyleSheet(
            "QDockWidget { background:#1a0030; color:#cc88ff; }"
            "QDockWidget::title { background:#2a0050; padding:4px; }");

        connect(slotVisualizerWidget, &StorageSlotVisualizerWidget::jumpToVariable,
        this, [this](const QString &varName) {
            CodeEditor *activeEd = currentEditor();
            if (!activeEd) return;
            QTextDocument *doc = activeEd->document();
            QTextCursor found = doc->find(varName);
            if (!found.isNull()) {
                activeEd->setTextCursor(found);
                activeEd->centerCursor();
            }
        });
    }

    slotVisualizerWidget->analyzeCode(code);
    slotVisualizerDock->show();
    slotVisualizerDock->raise();

    auto vars = SolidityAnalyzer::parseStorageSlots(code);
    flashStatusMessage(QString("Storage Slots: %1 variables in %2 slots analyzed")
        .arg(vars.size())
        .arg(vars.isEmpty() ? 0 : vars.last().slot + 1));
}

void TextEditor::toggleGasMinimap() {
    gasMiniMapEnabled = !gasMiniMapEnabled;
    if (gasMiniMapAct) gasMiniMapAct->setChecked(gasMiniMapEnabled);

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto *ed = qobject_cast<CodeEditor*>(tabWidget->widget(i)))
            ed->setGasMinimapEnabled(gasMiniMapEnabled);
    }
    if (tabWidget2) {
        for (int i = 0; i < tabWidget2->count(); ++i) {
            if (auto *ed = qobject_cast<CodeEditor*>(tabWidget2->widget(i)))
                ed->setGasMinimapEnabled(gasMiniMapEnabled);
        }
    }
    flashStatusMessage(gasMiniMapEnabled ? "\U0001F321 Gas Minimap ON" : "Gas Minimap OFF");
}

void TextEditor::toggleSlitherOverlay() {
    CodeEditor *ed = currentEditor();
    if (!ed) { flashStatusMessage("No active editor"); return; }

    QString path = ed->getFileName();
    if (path.isEmpty()) { flashStatusMessage("Save file first"); return; }

    bool hasSlither = false, hasForge = false;
    {
        QProcess test;
        test.start("slither", {"--version"});
        test.waitForFinished(1500);
        hasSlither = (test.exitCode() == 0);
    }
    {
        QProcess test;
        test.start("forge", {"--version"});
        test.waitForFinished(1500);
        hasForge = (test.exitCode() == 0);
    }

    if (!hasSlither && !hasForge) {
        QMessageBox::information(this, "Slither/Foundry Overlays",
            "Neither slither nor forge was found in PATH.\n\n"
            "Install with:\n  pip install slither-analyzer\n  curl -L https://foundry.paradigm.xyz | bash\n\nfoundryup");
        return;
    }

    QDialog *toolDialog = new QDialog(this);
    toolDialog->setWindowTitle("Run Analysis Tool");
    toolDialog->setStyleSheet("background:#0d0d0d;color:#d4d4d4;");
    auto *layout = new QVBoxLayout(toolDialog);
    layout->addWidget(new QLabel(QString("File: %1").arg(QFileInfo(path).fileName()), toolDialog));

    QComboBox *toolCombo = new QComboBox(toolDialog);
    if (hasSlither) toolCombo->addItem("slither (static analysis)");
    if (hasForge) toolCombo->addItem("forge test (unit tests)");
    layout->addWidget(toolCombo);

    auto *runBtn = new QPushButton("\u25B6 Run", toolDialog);
    auto *cancelBtn = new QPushButton("Cancel", toolDialog);
    auto *btns = new QHBoxLayout();
    btns->addWidget(runBtn);
    btns->addWidget(cancelBtn);
    layout->addLayout(btns);

    connect(cancelBtn, &QPushButton::clicked, toolDialog, &QDialog::reject);
    connect(runBtn, &QPushButton::clicked, toolDialog, [=]() {
        toolDialog->accept();
        QString tool = toolCombo->currentText().contains("slither") ? "slither" : "forge";

        QProcess *proc = new QProcess(this);
        proc->setWorkingDirectory(QFileInfo(path).absolutePath());

        QStringList args;
        QString outFile = QDir::temp().filePath("jim_analysis.json");
        if (tool == "slither") {
            args << path << "--json" << outFile;
        } else {
            args << "test" << "--json-summary";
        }

        flashStatusMessage(QString("\u2699 Running %1...").arg(tool));

        connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this, proc, outFile, tool, ed](int, QProcess::ExitStatus) {
            proc->deleteLater();

            QVector<VulnScanner::Finding> findings;

            QFile jf(outFile);
            if (jf.open(QIODevice::ReadOnly)) {
                QByteArray jsonData = jf.readAll();
                jf.close();
                QJsonDocument jdoc = QJsonDocument::fromJson(jsonData);
                if (!jdoc.isNull()) {
                    QJsonObject root = jdoc.object();
                    QJsonArray detectors = root["results"].toObject()["detectors"].toArray();
                    for (const QJsonValue &det : detectors) {
                        QJsonObject d = det.toObject();
                        QString check = d["check"].toString();
                        QString impact = d["impact"].toString();
                        QJsonArray elements = d["elements"].toArray();
                        for (const QJsonValue &el : elements) {
                            QJsonObject src = el.toObject()["source_mapping"].toObject();
                            int line = src["lines"].toArray().first().toInt() - 1;
                            VulnScanner::Finding f;
                            f.line = line;
                            f.colStart = 0;
                            f.colEnd = 5;
                            f.category = "SLITHER";
                            f.description = check + " [" + impact + "] " + d["description"].toString().left(100);
                            findings.append(f);
                        }
                    }
                }
            }

            QByteArray stdErr = proc->readAllStandardError();
            if (findings.isEmpty() && !stdErr.isEmpty()) {
                static QRegularExpression lineRe(R"(\((\d+)\))");
                QStringList errLines = QString::fromUtf8(stdErr).split('\n');
                for (const QString &el : errLines) {
                    QRegularExpressionMatch m = lineRe.match(el);
                    if (m.hasMatch()) {
                        VulnScanner::Finding f;
                        f.line = m.captured(1).toInt() - 1;
                        f.colStart = 0; f.colEnd = 5;
                        f.category = "SLITHER";
                        f.description = el.left(120);
                        findings.append(f);
                    }
                }
            }

            if (findings.isEmpty()) {
                flashStatusMessage(QString("\u2713 %1: No issues found").arg(tool));
            } else {
                ed->vulnFindings.append(findings);
                ed->viewport()->update();
                ed->update();
                flashStatusMessage(QString("\u2622 %1: %2 findings overlaid on editor").arg(tool).arg(findings.size()));
            }
        });

        proc->start(tool, args);
    });

    toolDialog->exec();
    toolDialog->deleteLater();
}

void TextEditor::showOnChainTracer() {
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("\u26D3 On-Chain Trace Explorer");
    dlg->resize(800, 600);
    dlg->setStyleSheet("background:#080818;color:#00ffaa;");

    auto *layout = new QVBoxLayout(dlg);

    auto *header = new QLabel(
        "<div style='color:#00ffaa;font-size:16px;font-weight:bold;'>\u26D3 Time-Travel On-Chain Trace Explorer</div>"
        "<div style='color:#888;font-size:11px;'>Paste a mainnet tx hash to replay execution against local .sol files</div>",
        dlg);
    header->setTextFormat(Qt::RichText);
    layout->addWidget(header);

    auto *rpcRow = new QHBoxLayout();
    rpcRow->addWidget(new QLabel("RPC Endpoint:", dlg));
    auto *rpcInput = new QLineEdit("https://eth-mainnet.g.alchemy.com/v2/YOUR_KEY", dlg);
    rpcInput->setStyleSheet("background:#0a0a20;color:#aaffaa;border:1px solid #004400;padding:4px;font-family:Consolas;");
    rpcRow->addWidget(rpcInput, 1);
    layout->addLayout(rpcRow);

    auto *txRow = new QHBoxLayout();
    txRow->addWidget(new QLabel("Tx Hash:", dlg));
    auto *txInput = new QLineEdit(dlg);
    txInput->setPlaceholderText("0x...");
    txInput->setStyleSheet("background:#0a0a20;color:#aaffaa;border:1px solid #004400;padding:4px;font-family:Consolas;");
    auto *fetchBtn = new QPushButton("\u25B6 Fetch Trace", dlg);
    fetchBtn->setStyleSheet("background:#003300;color:#00ff88;border:1px solid #00ff88;padding:6px 12px;");
    txRow->addWidget(txInput, 1);
    txRow->addWidget(fetchBtn);
    layout->addLayout(txRow);

    auto *traceOutput = new QPlainTextEdit(dlg);
    traceOutput->setReadOnly(true);
    traceOutput->setPlaceholderText("Trace will appear here...");
    traceOutput->setStyleSheet("background:#050510;color:#00cc88;font-family:Consolas,monospace;font-size:11px;border:1px solid #003333;");
    layout->addWidget(traceOutput, 1);

    auto *statusLbl = new QLabel("Ready", dlg);
    statusLbl->setStyleSheet("color:#666;font-size:10px;");
    layout->addWidget(statusLbl);

    auto *closeBtnRow = new QHBoxLayout();
    auto *closeBtn2 = new QPushButton("Close", dlg);
    closeBtn2->setStyleSheet("background:#1a0000;color:#ff6666;border:1px solid #ff3333;padding:4px 12px;");
    connect(closeBtn2, &QPushButton::clicked, dlg, &QDialog::accept);
    closeBtnRow->addStretch();
    closeBtnRow->addWidget(closeBtn2);
    layout->addLayout(closeBtnRow);

    connect(fetchBtn, &QPushButton::clicked, dlg, [=]() {
        QString txHash = txInput->text().trimmed();
        QString rpcUrl = rpcInput->text().trimmed();

        if (txHash.isEmpty() || rpcUrl.isEmpty()) {
            statusLbl->setText("Enter RPC URL and tx hash");
            return;
        }

        statusLbl->setText("Fetching trace...");
        fetchBtn->setEnabled(false);
        traceOutput->clear();

        QJsonObject rpcCall;
        rpcCall["jsonrpc"] = "2.0";
        rpcCall["method"] = "debug_traceTransaction";
        rpcCall["id"] = 1;
        QJsonArray params;
        params.append(txHash);
        QJsonObject opts;
        opts["disableStorage"] = false;
        opts["disableMemory"] = true;
        opts["disableStack"] = false;
        params.append(opts);
        rpcCall["params"] = params;

        QNetworkRequest req(QUrl(rpcUrl));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setHeader(QNetworkRequest::UserAgentHeader, "Jim-Editor/0.9.0");

        QNetworkReply *reply = networkManager->post(req, QJsonDocument(rpcCall).toJson());

        connect(reply, &QNetworkReply::finished, dlg, [=]() {
            reply->deleteLater();
            fetchBtn->setEnabled(true);

            if (reply->error() != QNetworkReply::NoError) {
                statusLbl->setText("Error: " + reply->errorString());
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();

            if (obj.contains("error")) {
                statusLbl->setText("RPC Error: " + obj["error"].toObject()["message"].toString());
                traceOutput->setPlainText(QString::fromUtf8(data).left(2000));
                return;
            }

            QJsonObject result = obj["result"].toObject();
            QJsonArray structLogs = result["structLogs"].toArray();

            if (structLogs.isEmpty()) {
                statusLbl->setText("No trace data (check tx hash and RPC endpoint)");
                traceOutput->setPlainText(QString::fromUtf8(data).left(1000));
                return;
            }

            QString traceText;
            traceText += QString("Transaction Trace: %1\n").arg(txHash);
            traceText += QString("Gas Used: %1\n").arg(result["gas"].toInt());
            traceText += QString("Steps: %1\n\n").arg(structLogs.size());
            traceText += QString("%-6s  %-16s  %-8s  %s\n").arg("PC", "OPCODE", "GAS", "STACK TOP");
            traceText += QString("-").repeated(70) + "\n";

            int maxSteps = qMin(structLogs.size(), 500);
            for (int i = 0; i < maxSteps; ++i) {
                QJsonObject step = structLogs[i].toObject();
                int pc = step["pc"].toInt();
                QString op = step["op"].toString();
                int gas = step["gas"].toInt();
                QJsonArray stack = step["stack"].toArray();
                QString stackTop = stack.isEmpty() ? "" : stack.last().toString().right(16);
                traceText += QString("%-6d  %-16s  %-8d  %s\n").arg(pc).arg(op).arg(gas).arg(stackTop);
            }
            if (structLogs.size() > maxSteps)
                traceText += QString("... (%1 more steps)\n").arg(structLogs.size() - maxSteps);

            traceOutput->setPlainText(traceText);
            statusLbl->setText(QString("\u2713 Loaded %1 execution steps").arg(structLogs.size()));
        });
    });

    dlg->exec();
    dlg->deleteLater();
}

void TextEditor::showProxyDiff() {
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Select Two .sol Files for Proxy/Implementation Diff",
        currentFolder, "Solidity (*.sol);;All Files (*)");

    if (files.size() < 2) {
        CodeEditor *ed = currentEditor();
        if (ed && ed->getFileName().endsWith(".sol")) {
            files.clear();
            files << ed->getFileName();
            QString second = QFileDialog::getOpenFileName(this,
                "Select Implementation .sol to compare against", currentFolder, "Solidity (*.sol)");
            if (second.isEmpty()) return;
            files << second;
        } else {
            flashStatusMessage("Select two .sol files to compare");
            return;
        }
    }

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("\U0001F504 Proxy/Implementation Storage Diff");
    dlg->resize(1000, 600);
    dlg->setStyleSheet("background:#0a0a0a;color:#d4d4d4;");

    auto *mainLayout = new QVBoxLayout(dlg);

    auto *header = new QLabel(
        "<b style='color:#00aaff;'>Proxy/Implementation Storage Slot Comparison</b><br>"
        "<span style='color:#888;font-size:10px;'>Mismatches indicate storage collision risk</span>",
        dlg);
    header->setTextFormat(Qt::RichText);
    mainLayout->addWidget(header);

    QStringList contents;
    QStringList names;
    for (const QString &f : files.mid(0, 2)) {
        QFile file(f);
        if (file.open(QIODevice::ReadOnly))
            contents << QString::fromUtf8(file.readAll());
        else
            contents << "";
        names << QFileInfo(f).fileName();
    }

    auto slots1 = SolidityAnalyzer::parseStorageSlots(contents[0]);
    auto slots2 = SolidityAnalyzer::parseStorageSlots(contents[1]);

    auto *table = new QTableWidget(dlg);
    int maxSlots = qMax(slots1.size(), slots2.size());
    table->setRowCount(maxSlots);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({names[0] + " Name", names[0] + " Type/Slot",
                                       "\u26A1",
                                       names[1] + " Name", names[1] + " Type/Slot"});
    table->setStyleSheet(
        "QTableWidget { background:#0d0d0d; color:#d4d4d4; gridline-color:#333; }"
        "QHeaderView::section { background:#1a1a1a; color:#aaaaaa; border:1px solid #333; }"
        "QTableWidget::item { padding:4px; }");
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();

    for (int i = 0; i < maxSlots; ++i) {
        bool hasCollision = false;
        if (i < slots1.size() && i < slots2.size()) {
            hasCollision = (slots1[i].slot != slots2[i].slot ||
                           slots1[i].byteSize != slots2[i].byteSize);
        } else {
            hasCollision = true;
        }

        QString v1Name = i < slots1.size() ? slots1[i].name : "";
        QString v1Type = i < slots1.size() ?
            QString("%1 (slot %2)").arg(slots1[i].typeName).arg(slots1[i].slot) : "";
        QString v2Name = i < slots2.size() ? slots2[i].name : "";
        QString v2Type = i < slots2.size() ?
            QString("%1 (slot %2)").arg(slots2[i].typeName).arg(slots2[i].slot) : "";

        table->setItem(i, 0, new QTableWidgetItem(v1Name));
        table->setItem(i, 1, new QTableWidgetItem(v1Type));
        auto *flagItem = new QTableWidgetItem(hasCollision ? "\u26A0 COLLISION" : "\u2713 OK");
        flagItem->setForeground(hasCollision ? QColor(255, 80, 80) : QColor(80, 255, 80));
        table->setItem(i, 2, flagItem);
        table->setItem(i, 3, new QTableWidgetItem(v2Name));
        table->setItem(i, 4, new QTableWidgetItem(v2Type));

        if (hasCollision) {
            for (int c = 0; c < 5; ++c) {
                if (table->item(i, c))
                    table->item(i, c)->setBackground(QColor(60, 10, 10));
            }
        }
    }

    mainLayout->addWidget(table, 1);

    int collisions = 0;
    for (int i = 0; i < maxSlots; ++i) {
        bool coll = false;
        if (i < slots1.size() && i < slots2.size())
            coll = (slots1[i].slot != slots2[i].slot || slots1[i].byteSize != slots2[i].byteSize);
        else coll = true;
        if (coll) ++collisions;
    }

    auto *summary = new QLabel(collisions > 0 ?
        QString("<b style='color:#ff4444;'>\u26A0 WARNING: %1 storage collision(s) detected!</b>").arg(collisions) :
        "<b style='color:#44ff44;'>\u2713 No storage collisions detected</b>", dlg);
    summary->setTextFormat(Qt::RichText);
    mainLayout->addWidget(summary);

    auto *closeBtn3 = new QPushButton("Close", dlg);
    closeBtn3->setStyleSheet("background:#1a0000;color:#ff6666;border:1px solid #ff3333;padding:4px 12px;");
    connect(closeBtn3, &QPushButton::clicked, dlg, &QDialog::accept);
    mainLayout->addWidget(closeBtn3);

    dlg->exec();
    dlg->deleteLater();
}
