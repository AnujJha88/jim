#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTabWidget>
#include <QMap>
#include <QSplitter>
#include <QColor>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDockWidget>

class LineNumberArea;
class MiniMap;

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

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setFileName(const QString &name) { fileName = name; }
    QString getFileName() const { return fileName; }
    bool isModified() const { return document()->isModified(); }
    void applyTheme(const ColorTheme &theme);
    void miniMapPaintEvent(QPaintEvent *event);
    int miniMapWidth() const { return 120; }
    MiniMap* getMiniMap() const { return miniMap; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea *lineNumberArea;
    MiniMap *miniMap;
    QString fileName;
    ColorTheme currentTheme;
    void autoIndent();
    void matchBrackets();
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *parent = nullptr);
    void applyTheme(const ColorTheme &theme);

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
    
    void setupRules();
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
    void changeTheme();
    void customizeColors();
    void showAbout();
    void onFileTreeDoubleClicked(const QModelIndex &index);

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

    QSplitter *mainSplitter;
    QTabWidget *tabWidget;
    QTabWidget *tabWidget2;
    QDockWidget *fileTreeDock;
    QTreeView *fileTree;
    QFileSystemModel *fileSystemModel;
    QString currentFolder;
    QMap<CodeEditor*, SyntaxHighlighter*> highlighters;
    QLabel *statusLabel;
    QStringList recentFiles;
    QString lastSearchText;
    bool wordWrapEnabled;
    bool splitViewEnabled;
    int fontSize;
    int currentThemeIndex;
    QVector<ColorTheme> themes;
    
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *searchMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QMenu *recentFilesMenu;
    
    QAction *newAct;
    QAction *openAct;
    QAction *openFolderAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *closeTabAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *undoAct;
    QAction *redoAct;
    QAction *selectAllAct;
    QAction *findAct;
    QAction *findNextAct;
    QAction *replaceAct;
    QAction *goToLineAct;
    QAction *increaseFontAct;
    QAction *decreaseFontAct;
    QAction *wordWrapAct;
    QAction *splitViewAct;
    QAction *fileTreeAct;
    QAction *miniMapAct;
    QAction *themeAct;
    QAction *customizeColorsAct;
    QAction *aboutAct;
};

#endif
