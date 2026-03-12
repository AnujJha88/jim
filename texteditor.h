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

class LineNumberArea;
class FoldingArea;
class MiniMap;
class QPropertyAnimation;
class HexEditor;
class WelcomeWidget;
class BreadcrumbBar;
class TerminalWidget;
class TitleBar;

// Language enum for syntax highlighting
enum class Language {
    PlainText,
    CPP,
    Python,
    JavaScript,
    HTML,
    CSS,
    Rust,
    Go,
    JSON,
    YAML,
    Markdown
};

struct ColorTheme {
    QString name;
    QColor background;
    QColor foreground;
    QColor lineNumberBg;
    QColor lineNumberFg;
    QColor currentLine;
    QColor selection;
    QColor keyword;
    QColor string;
    QColor comment;
    QColor number;
    QColor function;
};

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
    
    void setLanguage(Language lang);
    Language getLanguage() const { return currentLanguage; }

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

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

    friend class FoldingArea;

private:
    LineNumberArea *lineNumberArea;
    FoldingArea *foldingArea;
    MiniMap *miniMap;
    QString fileName;
    ColorTheme currentTheme;
    bool smoothScrollEnabled;
    QPropertyAnimation *scrollAnimation;
    int targetScrollValue;
    Language currentLanguage;
    void autoIndent();
    void matchBrackets();
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *parent = nullptr);
    void applyTheme(const ColorTheme &theme);
    void setLanguage(Language lang);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat tagFormat;
    QTextCharFormat attributeFormat;
    QTextCharFormat headingFormat;
    QTextCharFormat boldFormat;
    QTextCharFormat linkFormat;
    
    Language currentLanguage;
    
    // Cached patterns for performance
    static QRegularExpression multiLineCommentStart;
    static QRegularExpression multiLineCommentEnd;
    
    void setupRules();
    void setupCppRules();
    void setupPythonRules();
    void setupJavaScriptRules();
    void setupHtmlRules();
    void setupCssRules();
    void setupRustRules();
    void setupGoRules();
    void setupJsonRules();
    void setupYamlRules();
    void setupMarkdownRules();
};

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
    BreadcrumbBar(QWidget *parent = nullptr);
    void updatePath(const QString &filePath, const QString &symbol);

private:
    QLabel *pathLabel;
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
    void onReadyRead();

private:
    QPlainTextEdit *output;
    QLineEdit *input;
    QProcess *process;
    QString currentDir;
    void setupUI();
    void startShell();
    void appendOutput(const QString &text);
};

class TextEditor : public QMainWindow {
    Q_OBJECT

public:
    TextEditor(QWidget *parent = nullptr);
    ~TextEditor();
    
    void openFilePath(const QString &filePath);
    void openFolderPath(const QString &folderPath);

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
    void changeTheme();
    void customizeColors();
    void showAbout();
    void onFileTreeDoubleClicked(const QModelIndex &index);
    void onFileChangedExternally(const QString &path);
    void updateBreadcrumb();

private:
    void createActions();
    void createMenus();
    void createStatusBar();
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
    
    // New feature members
    WelcomeWidget *welcomeWidget;
    BreadcrumbBar *breadcrumbBar;
    TerminalWidget *terminalWidget;
    QFileSystemWatcher *fileWatcher;
    
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *searchMenu;
    QMenu *viewMenu;
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
    QAction *themeAct;
    QAction *customizeColorsAct;
    QAction *aboutAct;
};

#endif
