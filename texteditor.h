#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTabWidget>
#include <QStackedWidget>
#include <QMap>
#include <QSplitter>
#include <QColor>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDockWidget>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QGraphicsOpacityEffect>
#include <QSplitter>

class LineNumberArea;
class FoldingArea;
class MiniMap;
class QPropertyAnimation;
class HexEditor;
class DisassemblerWidget;
class BinaryInspectorWidget;
class MarkdownPreviewWidget;
class WelcomeWidget;
class BreadcrumbBar;
class TerminalWidget;
class TitleBar;
class AnimationWidget;
class DJVisualizerWindow;
class AIAutocomplete;
class QSoundEffect;
class AudioMonitor;

#include <QDateTime>
#include <QListWidget>
#include "syntaxhighlighter.h"

class KeyHeatmapOverlay;
class VimMode;
class CommandPalette;
class TodoPanel;

// Ghost Replay Event
struct GhostEvent {
    long long timestampMs;
    int position;
    int charsRemoved;
    QString textAdded;
};

// ── Code Graveyard ─────────────────────────────────────────────────────────
class GraveyardWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraveyardWidget(QWidget *parent = nullptr);
    void addSnippet(const QString &code, const QString &source);
signals:
    void resurrectRequested(const QString &code);
private:
    QListWidget *listWidget;
    QVector<QString> snippets;
};

// ── CRT Post-Processing Overlay ────────────────────────────────────────────
class CRTOverlay : public QWidget {
public:
    explicit CRTOverlay(QWidget *parent = nullptr);
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_enabled = false;
};

// ── Laser Particle Overlay ─────────────────────────────────────────────────
class LaserParticleOverlay : public QWidget {
    Q_OBJECT
public:
    explicit LaserParticleOverlay(QWidget *parent = nullptr);
    void spawnSlash(int yPos);
protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
private:
    struct Spark { float x, y, vx, vy, life; };
    QVector<Spark> sparks;
    float slashAlpha = 0.f;
    int slashY = 0;
    int timerId = 0;
};

// ── Cybernetic HUD Widget ──────────────────────────────────────────────────
class HUDWidget : public QWidget {
    Q_OBJECT
public:
    explicit HUDWidget(QWidget *parent = nullptr);
    void addKeystroke(); // call on every keypress for WPM tracking
protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
private:
    QVector<float> cpuHistory;
    QVector<float> memHistory;
    quint64 hexCounter = 0xDEADBEEF00000000ULL;
    float readCpuUsage();
    float readMemUsage();
    quint64 prevIdle = 0, prevTotal = 0;
    // WPM tracking
    QVector<qint64> keystrokeTimestamps;
    int currentWPM = 0;
};

// Language enum is defined in syntaxhighlighter.h (included above)

// Animation widget with multiple effects
class AnimationWidget : public QWidget {
    Q_OBJECT
public:
    enum AnimationType {
        None,
        Matrix,
        Particles,
        Waves,
        Pulse,
        Starfield,
        Rain,
        Snow,
        Fire,
        DJMode
    };
    
    explicit AnimationWidget(QWidget *parent = nullptr);
    void setAnimationType(AnimationType type);
    void cycleAnimation();
    AnimationType getCurrentType() const { return currentType; }
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    
public:
    void setAudioMonitor(AudioMonitor* monitor) { audioMonitor = monitor; }

private:
    AudioMonitor* audioMonitor = nullptr;
    AnimationType currentType;
    int timerId;
    int frame;
    
    struct Particle {
        float x, y, vx, vy;
        int life;
        float size; // Added size for better effects
    };
    QVector<Particle> particles;
    
    struct MatrixColumn {
        int x, y, speed;
        QString text;
    };
    QVector<MatrixColumn> matrixColumns;
    
    void initMatrix();
    void initParticles();
    void initStarfield();
    void initFire();
    
    void drawMatrix(QPainter &painter);
    void drawParticles(QPainter &painter);
    void drawWaves(QPainter &painter);
    void drawPulse(QPainter &painter);
    void drawStarfield(QPainter &painter);
    void drawRain(QPainter &painter);
    void drawSnow(QPainter &painter);
    void drawFire(QPainter &painter);
    void drawDJMode(QPainter &painter);
};

// Dedicated DJ Visualizer Panel
class DJVisualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit DJVisualizerWidget(QWidget *parent = nullptr);
    ~DJVisualizerWidget();
    
    void setAudioMonitor(AudioMonitor* monitor);
    AnimationWidget* visualizerWidget; // Make public for access

private:
    QVBoxLayout* layout;
};

// ColorTheme struct is defined in syntaxhighlighter.h (included above)

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void toggleMaximized();

private:
    QLabel *titleLabel;
    QPoint dragStartPos;
};

class FindBar : public QWidget {
    Q_OBJECT
public:
    explicit FindBar(QWidget *parent = nullptr);
    void showAndFocus(const QString &text = "");
    void setMatchCount(int current, int total);
    QString getSearchText() const;

signals:
    void findNextRequested(const QString &text);
    void findPreviousRequested(const QString &text);
    void textChanged(const QString &text);
    void closeRequested();

private:
    QLineEdit *findInput;
    QLabel *matchLabel;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QPushButton *closeBtn;
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);
    void enableSmoothScrolling(bool enable);
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setFileName(const QString &name) { fileName = name; }
    QString getFileName() const { return fileName; }
    bool isModified() const { return document()->isModified(); }
    void applyTheme(const ColorTheme &theme);
    void miniMapPaintEvent(QPaintEvent *event);
    int miniMapWidth() const { return 120; }
    MiniMap* getMiniMap() const { return miniMap; }
    
    // Code folding
    void foldingAreaPaintEvent(QPaintEvent *event);
    int foldingAreaWidth() const { return 16; }
    void toggleFoldAt(int blockNumber);
    bool isFoldable(const QTextBlock &block) const;
    bool isFolded(const QTextBlock &block) const;
    int findMatchingBrace(const QTextBlock &block) const;
    int findIndentEnd(const QTextBlock &block) const;
    int indentLevel(const QTextBlock &block) const;
    
    void setLanguage(Language lang);
    Language getLanguage() const { return currentLanguage; }
    
    // Search Highlighting
    void setSearchSelections(const QList<QTextCursor> &selections);

    // Multi-cursor
    void addExtraCursor(const QTextCursor &c);
    void clearExtraCursors();
    void selectNextOccurrence();

    // Ghost Replay
    QVector<GhostEvent> ghostLog;
    long long sessionStartTimeMs = 0;
    bool ghostIsRecording = false;
    void startRecordingGhost();
    void logGhostEvent(int pos, int charsRemoved, const QString &textAdded);

    // Kinetic/Laser editing
    void triggerLaserEffect();

    // CRT overlay toggle (forwarded from TextEditor)
    void setCRTEnabled(bool enabled);

    // Data Waterfall minimap
    bool minimapWaterfallEnabled = true;

    // Ambient background tint (blended into theme bg, called by TextEditor timer)
    void setAmbientBackground(QColor tint);

    // Vim mode
    void setVimEnabled(bool enabled);
    bool isVimEnabled() const;

    // v1.9 toggleable features
    void setFocusFadeEnabled(bool enabled);
    void setImagePreviewEnabled(bool enabled);

signals:
    void characterTyped();
    void codeBlockDeleted(const QString &code, const QString &source);
    void keyPressed(int key, const QString &text);   // for KeyHeatmapOverlay
    void vimModeChanged(const QString &modeName);    // "" | "INSERT" | "NORMAL"

public slots:
    void duplicateLine();
    void moveLineUp();
    void moveLineDown();
    void deleteLine();
    void toggleComment();
    void smartHome();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onDocumentContentsChange(int position, int charsRemoved, int charsAdded);

    friend class FoldingArea;

private:
    LineNumberArea *lineNumberArea;
    FoldingArea *foldingArea;
    MiniMap *miniMap;
    CRTOverlay *crtOverlay;
    LaserParticleOverlay *laserOverlay;
    QString fileName;
    ColorTheme currentTheme;
    bool smoothScrollEnabled;
    QPropertyAnimation *scrollAnimation;
    int targetScrollValue;
    Language currentLanguage;
    QList<QTextCursor> searchSelections;
    QList<QTextCursor> extraCursors;
    // Data waterfall minimap
    QVector<int> waterfallDrops;
    int waterfallFrame = 0;
    // Edit heatmap (line number → edit count)
    QMap<int, int> lineEditHeat;
    // Vim mode handler
    VimMode *vimMode = nullptr;
    // v1.9 features
    bool focusFadeEnabled = false;
    bool imagePreviewEnabled = false;
    void autoIndent();
    void matchBrackets();
};

// SyntaxHighlighter class is defined in syntaxhighlighter.h (included above)

// Welcome Screen Widget
class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    WelcomeWidget(QWidget *parent = nullptr);
    void setRecentFiles(const QStringList &files);

signals:
    void openFileRequested();
    void openFolderRequested();
    void recentFileClicked(const QString &filePath);

private:
    QVBoxLayout *recentFilesLayout;
    void setupUI();
};

// Breadcrumb Navigation Bar
class BreadcrumbBar : public QWidget {
    Q_OBJECT
public:
    explicit BreadcrumbBar(QWidget *parent = nullptr);
    void updatePath(const QString &filePath, const QString &symbol);

private:
    QLabel *iconLabel;
    QLabel *pathLabel;
    QLabel *fileLabel;
    QLabel *symbolLabel;
};

// Integrated Terminal Widget
class TerminalWidget : public QWidget {
    Q_OBJECT
public:
    TerminalWidget(QWidget *parent = nullptr);
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

// ── Command Palette ────────────────────────────────────────────────────────
#include <QDialog>
#include <QTreeWidget>
#include <QKeyEvent>
class CommandPalette : public QDialog {
    Q_OBJECT
public:
    explicit CommandPalette(QWidget *parent = nullptr);
    void populate(const QList<QAction*> &actions);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    QLineEdit   *searchBox;
    QListWidget *resultList;
    QList<QAction*> allActions;
    void filter(const QString &text);
    void runSelected();
};

// ── TODO/FIXME Panel ───────────────────────────────────────────────────────
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

class TextEditor : public QMainWindow {
    Q_OBJECT

public:
    TextEditor(QWidget *parent = nullptr);
    ~TextEditor();
    
    void openFilePath(const QString &filePath);
    void openFolderPath(const QString &folderPath);
    
    QAction *djModeAct = nullptr; // Make public for DJVisualizerWindow access
    void toggleDJMode(); // Make public for DJVisualizerWindow access

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void openRecentFile();
    void openFolder();
    bool saveFile();
    bool saveFileAs();
    void closeTab(int index);
    void tabChanged(int index);
    void findText();
    void findNext();
    void findPrevious();
    void onFindTextChanged(const QString &text);
    void closeFindBar();
    void replaceText();
    void goToLine();
    void documentWasModified();
    void updateStatusBar();
    void increaseFontSize();
    void decreaseFontSize();
    void toggleWordWrap();
    void toggleSplitView();
    void toggleFileTree();
    void toggleMiniMap();
    void toggleTerminal();
    void cycleAnimation();
    void toggleAnimationDock();
    void toggleZenMode();
    void toggleTypingSound();
    void changeTheme();
    void customizeColors();
    void showAbout();
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void onFileTreeContextMenu(const QPoint &pos);
    void toggleMarkdownPreview();
    void updateMarkdownPreview();
    void onFileChangedExternally(const QString &path);
    void updateBreadcrumb();
    void showAISettings();
    void toggleAIAutocomplete(bool enabled);
    void onAISuggestion(const QString &suggestion);
    // Tools
    void openDisassembler();
    void openBinaryInspector();
    void openNeuralGraph();
    void openGhostReplay();
    void openScratchpad();

private:
    void createActions();
    void createMenus();
    void createStatusBar();
    void clampToScreen();
    void readSettings();
    void writeSettings();
    bool maybeSave(int tabIndex);
    void loadFile(const QString &fileName);
    bool saveFileToPath(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);
    void updateRecentFiles(const QString &fileName);
    void updateRecentFilesMenu();
    CodeEditor* currentEditor();
    SyntaxHighlighter* currentHighlighter();
    void initializeThemes();
    void applyThemeToEditor(CodeEditor *editor, SyntaxHighlighter *highlighter);
    void applyThemeToAllEditors();
    void setupUI();
    void applyModernStyle();
    void showWelcomeScreen();
    void hideWelcomeScreen();
    void watchFile(const QString &filePath);
    void unwatchFile(const QString &filePath);
    static Language detectLanguage(const QString &fileName);
    QString detectCurrentSymbol(CodeEditor *editor);
    void updateSearchHighlights();

    // Binary tools – open a file path in the respective viewer tab
    void openInDisassembler   (const QString &filePath);
    void openInBinaryInspector(const QString &filePath);

    // Animated panel helpers
    void animateTerminalShow();
    void animateTerminalHide();
    void flashTabLabel(int tabIndex);
    void flashStatusMessage(const QString &msg, const QColor &color, int ms = 2500);

    // Markdown preview helpers
    void connectMarkdownPreview(CodeEditor *editor);
    void disconnectMarkdownPreview();

    QSplitter *mainSplitter;
    QSplitter *verticalSplitter;
    QTabWidget *tabWidget;
    QTabWidget *tabWidget2;
    QDockWidget *fileTreeDock;
    QStackedWidget *fileTreeContainer;
    QWidget *emptyTreeWidget;
    QTreeView *fileTree;
    QFileSystemModel *fileSystemModel;
    QString currentFolder;
    QMap<CodeEditor*, SyntaxHighlighter*> highlighters;
    QLabel *statusLabel;
    QLabel *languageLabel;
    QStringList recentFiles;
    QString lastSearchText;
    bool wordWrapEnabled;
    bool splitViewEnabled;
    int fontSize;
    int currentThemeIndex;
    QVector<ColorTheme> themes;
    
    // Session Time Tracker
    QLabel *sessionTimeLabel;
    QTimer *sessionTimer;
    QDateTime sessionStart;
    int sessionSecondsAccumulated = 0;
    QString sessionDateString;
    bool zenModeActive = false;
    
    // New feature members
    FindBar *findBar;
    QList<QTextCursor> currentMatches;
    int currentMatchIndex;
    
    WelcomeWidget *welcomeWidget;
    BreadcrumbBar *breadcrumbBar;
    TerminalWidget *terminalWidget;
    AnimationWidget *animationWidget;
    QDockWidget *animationDock;
    DJVisualizerWidget *djVisualizerWidget = nullptr;
    QDockWidget *djVisualizerDock = nullptr;
    AIAutocomplete *aiAutocomplete;
    QFileSystemWatcher *fileWatcher;

    QAction *zenModeAct = nullptr;
    QAction *typingSoundAct = nullptr;
    
    QSoundEffect *typingSound = nullptr;
    bool typingSoundEnabled = false;

    AudioMonitor* audioMonitor = nullptr;

    // Animation state
    QVariantAnimation *terminalAnim   = nullptr;
    int                terminalTargetH = 250;      // pixel height to restore
    QGraphicsOpacityEffect *welcomeOpacity = nullptr;

    // Markdown preview
    MarkdownPreviewWidget *markdownPreview  = nullptr;
    QTimer                *markdownTimer    = nullptr;
    CodeEditor            *markdownEditor   = nullptr; // editor currently connected
    
    QMenu *fileMenu;
    QMenu *markdownMenu;
    QMenu *editMenu;
    QMenu *searchMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *pluginsMenu;
    QMenu *helpMenu;
    QMenu *recentFilesMenu;
    
    TitleBar *titleBar;
    QMenuBar *customMenuBar;
    
    QAction *newAct;
    QAction *openAct;
    QAction *openFolderAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *closeAllAct;
    QAction *closeOthersAct;
    QAction *closeTabAct;
    QAction *exitAct;

    // Edit actions
    QAction *undoAct;
    QAction *redoAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *selectAllAct;
    QAction *findAct;
    QAction *findNextAct;
    QAction *replaceAct;
    QAction *goToLineAct;
    
    // Line editing actions
    QAction *duplicateLineAct;
    QAction *moveLineUpAct;
    QAction *moveLineDownAct;
    QAction *deleteLineAct;
    QAction *toggleCommentAct;
    QAction *smartHomeAct; // Added smartHomeAct

    // View actions
    QAction *wordWrapAct;
    QAction *increaseFontAct;
    QAction *decreaseFontAct;
    QAction *splitViewAct;
    QAction *fileTreeAct;
    QAction *miniMapAct;
    QAction *terminalAct;
    QAction *animationAct;
    QAction *toggleAnimationDockAct;
    QAction *themeAct;
    QAction *customizeColorsAct;
    QAction *aiSettingsAct;
    QAction *aiToggleAct;
    QAction *aboutAct;

    // Tools actions
    QAction *disassembleAct;
    QAction *binaryInspectAct;
    QAction *openHexAct;
    QAction *neuralGraphAct;
    QAction *ghostReplayAct;

    // Markdown preview action
    QAction *markdownPreviewAct;

    // v1.7 Cyberpunk features
    GraveyardWidget *graveyardWidget = nullptr;
    QDockWidget *graveyardDock = nullptr;
    QAction *graveyardAct = nullptr;
    QAction *crtAct = nullptr;
    HUDWidget *hudWidget = nullptr;

    // v1.8 features
    KeyHeatmapOverlay *keyHeatmap = nullptr;
    QAction *keyHeatmapAct = nullptr;
    QAction *vimModeAct = nullptr;
    QLabel *vimModeLabel = nullptr;
    QTimer *ambientTimer = nullptr;

    // v1.8.1 features
    QPlainTextEdit *scratchpadEditor = nullptr;
    QAction *scratchpadAct = nullptr;

    // v1.8.1 command palette + todo panel
    CommandPalette *commandPalette = nullptr;
    TodoPanel      *todoPanel      = nullptr;
    QDockWidget    *todoDock       = nullptr;
    QAction        *commandPaletteAct = nullptr;
    QAction        *todoAct           = nullptr;
    void openCommandPalette();
    void toggleTodoPanel();
    void onTodoJump(const QString &filePath, int line);

    // v1.9 features
    QAction *focusFadeAct = nullptr;
    QAction *imagePreviewAct = nullptr;
    QAction *sessionStatsAct = nullptr;
    int sessionKeystrokes = 0;
    int sessionLinesWritten = 0;
    int sessionFilesOpened = 0;
    int sessionPeakWPM = 0;
    QVector<qint64> statsKeystrokeTimestamps;
    void trackKeystroke(int key, const QString &text);

    void toggleGraveyard();
    void toggleCRT();
    void onCodeBlockDeleted(const QString &code, const QString &source);
    void toggleKeyHeatmap();
    void toggleVimMode();
    void updateAmbientTheme();
    // v1.9 feature slots
    void toggleFocusFade();
    void toggleImagePreview();
    void showSessionStats();
};

#endif
