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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRandomGenerator>

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
#include "vulnscanner.h"
#include "solidityanalyzer.h"
#include "storageslotvisualizer.h"

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

    // Security Pack
    void setParanoiaMode(bool enabled);
    void setVulnScanEnabled(bool enabled);

    // v1.9 toggleable features
    void setFocusFadeEnabled(bool enabled);
    void setImagePreviewEnabled(bool enabled);

    // v0.8.0 QoL features
    void setStickyScrollEnabled(bool enabled);
    void setInvisibleCharsEnabled(bool enabled);
    void setGitBlameEnabled(bool enabled);
    void fetchGitBlame();
    void setKineticScrollEnabled(bool enabled);
    void setAutoSaveOnFocusLost(bool enabled) { autoSaveOnFocusLost = enabled; }
    bool getAutoSaveOnFocusLost() const { return autoSaveOnFocusLost; }
    void insertFromMimeData(const QMimeData *source) override;

    // v0.9.0 Web3Sec (public so TextEditor can call them)
    void setGasMinimapEnabled(bool en);
    void setMemTraceEnabled(bool en);
    void highlightMemoryTraceLines(const QVector<int> &lines);
    QVector<VulnScanner::Finding> vulnFindings;

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
    void runVulnScan();

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
    // Security Pack
    bool paranoiaMode = false;
    bool vulnScanEnabled = false;
    VulnScanner *vulnScanner = nullptr;
    QTimer *vulnScanTimer = nullptr;
    // v1.9 features
    bool focusFadeEnabled = false;
    bool imagePreviewEnabled = false;
    void autoIndent();
    void matchBrackets();

    // v0.8.0 QoL features
    bool stickyScrollEnabled = false;
    bool invisibleCharsEnabled = false;
    bool gitBlameEnabled = false;
    bool kineticScrollEnabled = false;
    bool autoSaveOnFocusLost = false;
    // Git blame cache: line number -> short annotation string
    QMap<int, QString> blameCache;
    QProcess *blameProcess = nullptr;
    // Kinetic scroll
    double kineticVelocity = 0.0;
    QTimer *kineticTimer = nullptr;

    // v0.9.0 Web3Sec
    bool gasMinimapEnabled;
    bool memTraceEnabled;
    QVector<int> memTraceHighlightLines;
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

// ── Search Everywhere Dialog ──────────────────────────────────────────────
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
    QLineEdit   *searchBox;
    QListWidget *resultList;
    QList<QAction*> allActions;
    QStringList allFiles;
    QStringList allRecent;
    void filter(const QString &text);
    void runSelected();
};

// ── Drag and Drop Split Panes ─────────────────────────────────────────────
class DraggableTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit DraggableTabBar(QWidget *parent = nullptr);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    QPoint dragStartPos;
};

class DraggableTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit DraggableTabWidget(QWidget *parent = nullptr);
signals:
    void tabDroppedOutside();
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
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
    bool maybeSave(int tabIndex, QTabWidget *targetWidget = nullptr);
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
    void selectFont();
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
    
    // Security Pack
    void toggleParanoiaMode();
    void toggleVulnScan();

    // v0.8.0 QoL slots
    void switchHeaderSource();
    void locateCurrentFileInTree();
    void openSearchEverywhere();
    void toggleStickyScroll();
    void toggleInvisibleChars();
    void toggleGitBlame();
    void toggleAutoSaveOnFocusLost();
    void sendSelectionToScratchpad();

    // Web3Sec Pack (v0.8.1)
    void triggerGodView();
    void extractABI();
    void resolveFourByte();
    void triggerPanicButton();

    // v0.9.0 Web3Sec
    void showStorageSlotVisualizer();
    void toggleGasMinimap();
    void toggleSlitherOverlay();
    void showOnChainTracer();
    void showProxyDiff();

private:
    void createActions();
    void createMenus();
    void createStatusBar();
    void clampToScreen();
    void readSettings();
    void writeSettings();
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
    void flashStatusMessage(const QString &msg, const QColor &color = QColor("#4ec9b0"), int ms = 2500);

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
    QLabel *paranoiaLabel = nullptr;
    QStringList recentFiles;
    QString lastSearchText;
    bool wordWrapEnabled;
    bool splitViewEnabled;
    bool paranoiaMode = false;
    int fontSize;
    QString editorFontFamily = "Consolas";
    int currentThemeIndex;
    QVector<ColorTheme> themes;

    // v0.8.0 state
    SearchEverywhere *searchEverywhere = nullptr;
    qint64 lastShiftPressMs = 0;   // for double-Shift detection
    bool stickyScrollEnabled = false;
    bool invisibleCharsEnabled = false;
    bool gitBlameEnabled = false;
    bool autoSaveFocusEnabled = false;
    QGraphicsColorizeEffect *pane2DimEffect = nullptr;
    
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
    
    QNetworkAccessManager *networkManager = nullptr;
    
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
    QAction *selectFontAct = nullptr;
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

    // Web3Sec Actions
    QAction *godViewAct;
    QAction *extractABIAct;
    QAction *resolveFourByteAct;
    QAction *panicButtonAct;
    QAction *neuralGraphAct;
    QAction *ghostReplayAct;

    // v0.9.0 dock widgets
    StorageSlotVisualizerWidget *slotVisualizerWidget = nullptr;
    QDockWidget *slotVisualizerDock = nullptr;

    // v0.9.0 actions
    QAction *storageSlotVizAct = nullptr;
    QAction *gasMiniMapAct = nullptr;
    QAction *slitherOverlayAct = nullptr;
    QAction *onChainTracerAct = nullptr;
    QAction *proxyDiffAct = nullptr;
    QAction *memTraceAct = nullptr;
    bool gasMiniMapEnabled = false;

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

    // Security Pack actions
    QAction *paranoiaModeAct = nullptr;
    QAction *vulnScanAct = nullptr;

    // v0.8.0 QoL actions
    QAction *switchHeaderSourceAct = nullptr;
    QAction *locateInTreeAct = nullptr;
    QAction *searchEverywhereAct = nullptr;
    QAction *stickyScrollAct = nullptr;
    QAction *invisibleCharsAct = nullptr;
    QAction *gitBlameAct = nullptr;
    QAction *autoSaveFocusAct = nullptr;
    QAction *sendToScratchpadAct = nullptr;

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
    // v0.8.0 helpers
    void applyPaneDimming();
    void propagateV080Settings(CodeEditor *ed);
    void propagateV090Settings(CodeEditor *ed);
    void changeEvent(QEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif
