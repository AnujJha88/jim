#include "texteditor.h"
#include "linenumberarea.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QSettings>
#include <QPainter>
#include <QTextBlock>
#include <QInputDialog>
#include <QFontDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDockWidget>
#include <QHeaderView>
#include <QDir>
#include <QColorDialog>

// CodeEditor Implementation
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    miniMap = new MiniMap(this);
    miniMap->hide(); // Hidden by default
    
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    
    // Update minimap on scroll and text changes
    connect(this, &CodeEditor::updateRequest, this, [this]() { if (miniMap->isVisible()) miniMap->update(); });
    connect(this->document(), &QTextDocument::contentsChanged, this, [this]() { if (miniMap->isVisible()) miniMap->update(); });
    
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
}

void CodeEditor::applyTheme(const ColorTheme &theme) {
    currentTheme = theme;
    
    QPalette p = palette();
    p.setColor(QPalette::Base, theme.background);
    p.setColor(QPalette::Text, theme.foreground);
    setPalette(p);
    
    QString style = QString("QPlainTextEdit { background-color: %1; color: %2; selection-background-color: %3; }")
        .arg(theme.background.name())
        .arg(theme.foreground.name())
        .arg(theme.selection.name());
    setStyleSheet(style);
    
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    
    if (miniMap->isVisible()) {
        miniMap->setGeometry(QRect(cr.right() - miniMapWidth(), cr.top(), miniMapWidth(), cr.height()));
        setViewportMargins(lineNumberAreaWidth(), 0, miniMapWidth(), 0);
    } else {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = currentTheme.currentLine.isValid() ? currentTheme.currentLine : QColor(Qt::yellow).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    QColor bgColor = currentTheme.lineNumberBg.isValid() ? currentTheme.lineNumberBg : QColor(240, 240, 240);
    QColor fgColor = currentTheme.lineNumberFg.isValid() ? currentTheme.lineNumberFg : Qt::gray;
    painter.fillRect(event->rect(), bgColor);
    
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(fgColor);
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                           Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        autoIndent();
        return;
    }
    
    QString text = event->text();
    if (text.isEmpty()) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }
    
    QTextCursor cursor = textCursor();
    QChar ch = text[0];
    
    // Auto-pairing for brackets
    if (ch == '(' || ch == '[' || ch == '{') {
        QChar closing = ch == '(' ? ')' : ch == '[' ? ']' : '}';
        cursor.beginEditBlock();
        cursor.insertText(QString(ch) + QString(closing));
        cursor.movePosition(QTextCursor::Left);
        cursor.endEditBlock();
        setTextCursor(cursor);
        return;
    }
    
    // Auto-pairing for quotes
    if (ch == '"' || ch == '\'') {
        QChar nextChar = cursor.atEnd() ? QChar() : document()->characterAt(cursor.position());
        
        if (nextChar == ch) {
            // Skip closing quote
            cursor.movePosition(QTextCursor::Right);
            setTextCursor(cursor);
            return;
        } else {
            // Insert pair
            cursor.beginEditBlock();
            cursor.insertText(QString(ch) + QString(ch));
            cursor.movePosition(QTextCursor::Left);
            cursor.endEditBlock();
            setTextCursor(cursor);
            return;
        }
    }
    
    // Auto-skip closing brackets
    if (ch == ')' || ch == ']' || ch == '}') {
        QChar nextChar = cursor.atEnd() ? QChar() : document()->characterAt(cursor.position());
        
        if (nextChar == ch) {
            cursor.movePosition(QTextCursor::Right);
            setTextCursor(cursor);
            return;
        }
    }
    
    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::autoIndent() {
    QTextCursor cursor = textCursor();
    QString previousLine = cursor.block().text();
    
    int indent = 0;
    for (QChar c : previousLine) {
        if (c == ' ') indent++;
        else if (c == '\t') indent += 4;
        else break;
    }
    
    if (previousLine.trimmed().endsWith('{') || previousLine.trimmed().endsWith(':')) {
        indent += 4;
    }
    
    cursor.insertText("\n" + QString(" ").repeated(indent));
    setTextCursor(cursor);
}

void CodeEditor::matchBrackets() {
    // Bracket matching implementation
}

// SyntaxHighlighter Implementation
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    setupRules();
}

void SyntaxHighlighter::setupRules() {
    highlightingRules.clear();
    HighlightingRule rule;
    
    // Keywords
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns = {
        "\\bclass\\b", "\\bconst\\b", "\\benum\\b", "\\bexplicit\\b",
        "\\bfriend\\b", "\\binline\\b", "\\bint\\b", "\\blong\\b",
        "\\bnamespace\\b", "\\boperator\\b", "\\bprivate\\b", "\\bprotected\\b",
        "\\bpublic\\b", "\\bshort\\b", "\\bsignals\\b", "\\bsigned\\b",
        "\\bslots\\b", "\\bstatic\\b", "\\bstruct\\b", "\\btemplate\\b",
        "\\btypedef\\b", "\\btypename\\b", "\\bunion\\b", "\\bunsigned\\b",
        "\\bvirtual\\b", "\\bvoid\\b", "\\bvolatile\\b", "\\bbool\\b",
        "\\bchar\\b", "\\bdouble\\b", "\\bfloat\\b", "\\bif\\b",
        "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\breturn\\b",
        "\\bswitch\\b", "\\bcase\\b", "\\bbreak\\b", "\\bcontinue\\b",
        "\\bauto\\b", "\\busing\\b", "\\binclude\\b", "\\bdefine\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Class names
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\".*\"|'.*'");
    rule.format = stringFormat;
    highlightingRules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // Functions
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Single line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    // Skip empty lines for performance
    if (text.isEmpty()) {
        setCurrentBlockState(0);
        return;
    }
    
    // Handle multi-line comments first
    setCurrentBlockState(0);
    int startIndex = 0;
    
    if (previousBlockState() != 1) {
        startIndex = text.indexOf("/*");
    }
    
    while (startIndex >= 0) {
        int endIndex = text.indexOf("*/", startIndex);
        int commentLength;
        
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + 2;
        }
        
        setFormat(startIndex, commentLength, commentFormat);
        startIndex = text.indexOf("/*", startIndex + commentLength);
    }
    
    // Apply single-line patterns (skip if inside multi-line comment)
    if (previousBlockState() != 1) {
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
}

// TextEditor Implementation
TextEditor::TextEditor(QWidget *parent) : QMainWindow(parent), wordWrapEnabled(false), splitViewEnabled(false), fontSize(11), currentThemeIndex(0) {
    setupUI();
    initializeThemes();
    createActions();
    createMenus();
    createStatusBar();
    applyModernStyle();
    
    readSettings();
    
    setWindowTitle("Jim");
    resize(1200, 800);
    
    newFile();
}

TextEditor::~TextEditor() {
    writeSettings();
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
    connect(closeTabAct, &QAction::triggered, this, [this]() { closeTab(tabWidget->currentIndex()); });
    
    exitAct = new QAction("E&xit", this);
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    cutAct = new QAction("Cu&t", this);
    cutAct->setShortcuts(QKeySequence::Cut);
    connect(cutAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->cut(); });
    
    copyAct = new QAction("&Copy", this);
    copyAct->setShortcuts(QKeySequence::Copy);
    connect(copyAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->copy(); });
    
    pasteAct = new QAction("&Paste", this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    connect(pasteAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->paste(); });
    
    undoAct = new QAction("&Undo", this);
    undoAct->setShortcuts(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->undo(); });
    
    redoAct = new QAction("&Redo", this);
    redoAct->setShortcuts(QKeySequence::Redo);
    connect(redoAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->redo(); });
    
    selectAllAct = new QAction("Select &All", this);
    selectAllAct->setShortcuts(QKeySequence::SelectAll);
    connect(selectAllAct, &QAction::triggered, this, [this]() { if (currentEditor()) currentEditor()->selectAll(); });
    
    findAct = new QAction("&Find...", this);
    findAct->setShortcuts(QKeySequence::Find);
    connect(findAct, &QAction::triggered, this, &TextEditor::findText);
    
    findNextAct = new QAction("Find &Next", this);
    findNextAct->setShortcut(QKeySequence(Qt::Key_F3));
    connect(findNextAct, &QAction::triggered, this, &TextEditor::findNext);
    
    replaceAct = new QAction("&Replace...", this);
    replaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    connect(replaceAct, &QAction::triggered, this, &TextEditor::replaceText);
    
    goToLineAct = new QAction("&Go to Line...", this);
    goToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    connect(goToLineAct, &QAction::triggered, this, &TextEditor::goToLine);
    
    increaseFontAct = new QAction("Increase Font Size", this);
    increaseFontAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect(increaseFontAct, &QAction::triggered, this, &TextEditor::increaseFontSize);
    
    decreaseFontAct = new QAction("Decrease Font Size", this);
    decreaseFontAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(decreaseFontAct, &QAction::triggered, this, &TextEditor::decreaseFontSize);
    
    wordWrapAct = new QAction("Word Wrap", this);
    wordWrapAct->setCheckable(true);
    wordWrapAct->setChecked(wordWrapEnabled);
    connect(wordWrapAct, &QAction::triggered, this, &TextEditor::toggleWordWrap);
    
    splitViewAct = new QAction("Split View", this);
    splitViewAct->setCheckable(true);
    splitViewAct->setChecked(splitViewEnabled);
    splitViewAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));
    connect(splitViewAct, &QAction::triggered, this, &TextEditor::toggleSplitView);
    
    fileTreeAct = new QAction("File Tree", this);
    fileTreeAct->setCheckable(true);
    fileTreeAct->setChecked(true);
    fileTreeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(fileTreeAct, &QAction::triggered, this, &TextEditor::toggleFileTree);
    
    miniMapAct = new QAction("Mini Map", this);
    miniMapAct->setCheckable(true);
    miniMapAct->setChecked(false);
    miniMapAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(miniMapAct, &QAction::triggered, this, &TextEditor::toggleMiniMap);
    
    themeAct = new QAction("Toggle Theme", this);
    themeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(themeAct, &QAction::triggered, this, &TextEditor::changeTheme);
    
    customizeColorsAct = new QAction("Customize Colors...", this);
    connect(customizeColorsAct, &QAction::triggered, this, &TextEditor::customizeColors);
    
    aboutAct = new QAction("&About", this);
    connect(aboutAct, &QAction::triggered, this, &TextEditor::showAbout);
}

void TextEditor::createMenus() {
    fileMenu = menuBar()->addMenu("&File");
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
    
    editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addSeparator();
    editMenu->addAction(selectAllAct);
    
    searchMenu = menuBar()->addMenu("&Search");
    searchMenu->addAction(findAct);
    searchMenu->addAction(findNextAct);
    searchMenu->addAction(replaceAct);
    searchMenu->addAction(goToLineAct);
    
    viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(fileTreeAct);
    viewMenu->addAction(miniMapAct);
    viewMenu->addAction(splitViewAct);
    viewMenu->addSeparator();
    viewMenu->addAction(increaseFontAct);
    viewMenu->addAction(decreaseFontAct);
    viewMenu->addSeparator();
    viewMenu->addAction(wordWrapAct);
    viewMenu->addSeparator();
    viewMenu->addAction(themeAct);
    viewMenu->addAction(customizeColorsAct);
    
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAct);
    
    updateRecentFilesMenu();
}

void TextEditor::createStatusBar() {
    statusLabel = new QLabel("Line 1, Col 1");
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage("Ready");
}

void TextEditor::newFile() {
    CodeEditor *editor = new CodeEditor();
    SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
    highlighters[editor] = highlighter;
    
    QFont font("Consolas", fontSize);
    editor->setFont(font);
    editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    
    // Apply current theme to the new editor
    applyThemeToEditor(editor, highlighter);
    
    connect(editor->document(), &QTextDocument::modificationChanged, this, &TextEditor::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateStatusBar);
    
    int index = tabWidget->addTab(editor, "Untitled");
    tabWidget->setCurrentIndex(index);
    editor->setFocus();
}

void TextEditor::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", "",
        "All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)");
    
    if (!fileName.isEmpty()) {
        // Check if file is already open
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor && editor->getFileName() == fileName) {
                tabWidget->setCurrentIndex(i);
                return;
            }
        }
        loadFile(fileName);
    }
}

void TextEditor::openRecentFile() {
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        loadFile(action->data().toString());
    }
}

bool TextEditor::saveFile() {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;
    
    if (editor->getFileName().isEmpty()) {
        return saveFileAs();
    } else {
        return saveFileToPath(editor->getFileName());
    }
}

bool TextEditor::saveFileAs() {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", "",
        "All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py)");
    
    if (fileName.isEmpty()) return false;
    
    return saveFileToPath(fileName);
}

void TextEditor::closeTab(int index) {
    if (maybeSave(index)) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
        if (editor) {
            highlighters.remove(editor);
        }
        tabWidget->removeTab(index);
        
        if (tabWidget->count() == 0) {
            newFile();
        }
    }
}

void TextEditor::tabChanged(int) {
    updateStatusBar();
    CodeEditor *editor = currentEditor();
    if (editor) {
        QString title = "Jim";
        if (!editor->getFileName().isEmpty()) {
            title = strippedName(editor->getFileName()) + " - " + title;
        }
        if (editor->isModified()) {
            title = "*" + title;
        }
        setWindowTitle(title);
    }
}

void TextEditor::findText() {
    bool ok;
    QString text = QInputDialog::getText(this, "Find", "Find what:",
                                         QLineEdit::Normal, lastSearchText, &ok);
    if (ok && !text.isEmpty()) {
        lastSearchText = text;
        findNext();
    }
}

void TextEditor::findNext() {
    CodeEditor *editor = currentEditor();
    if (!editor || lastSearchText.isEmpty()) return;
    
    if (!editor->find(lastSearchText)) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        editor->setTextCursor(cursor);
        editor->find(lastSearchText);
    }
}

void TextEditor::replaceText() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    
    bool ok;
    QString findText = QInputDialog::getText(this, "Replace", "Find what:",
                                             QLineEdit::Normal, lastSearchText, &ok);
    if (!ok || findText.isEmpty()) return;
    
    QString replaceText = QInputDialog::getText(this, "Replace", "Replace with:",
                                                QLineEdit::Normal, "", &ok);
    if (!ok) return;
    
    lastSearchText = findText;
    QString content = editor->toPlainText();
    content.replace(findText, replaceText);
    editor->setPlainText(content);
}

void TextEditor::goToLine() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    
    bool ok;
    int line = QInputDialog::getInt(this, "Go to Line", "Line number:",
                                    1, 1, editor->document()->blockCount(), 1, &ok);
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
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        statusLabel->setText(QString("Line %1, Col %2").arg(line).arg(col));
    }
}

void TextEditor::increaseFontSize() {
    fontSize++;
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
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
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor) {
                QFont font = editor->font();
                font.setPointSize(fontSize);
                editor->setFont(font);
            }
        }
    }
}

void TextEditor::toggleWordWrap() {
    wordWrapEnabled = !wordWrapEnabled;
    wordWrapAct->setChecked(wordWrapEnabled);
    
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) {
            editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        }
    }
}

void TextEditor::showAbout() {
    QMessageBox::about(this, "About Jim",
        "Jim - Lightweight Text Editor\n\n"
        "Features:\n"
        "• Syntax highlighting\n"
        "• Line numbers & file tree\n"
        "• Multiple tabs & split view\n"
        "• Find & Replace\n"
        "• Auto-indentation\n"
        "• Theme switching\n"
        "• Customizable colors");
}

void TextEditor::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (!maybeSave(i)) {
            event->ignore();
            return;
        }
    }
    writeSettings();
    event->accept();
}

void TextEditor::readSettings() {
    QSettings settings("TextEditor", "Settings");
    recentFiles = settings.value("recentFiles").toStringList();
    fontSize = settings.value("fontSize", 11).toInt();
    wordWrapEnabled = settings.value("wordWrap", false).toBool();
    wordWrapAct->setChecked(wordWrapEnabled);
}

void TextEditor::writeSettings() {
    QSettings settings("TextEditor", "Settings");
    settings.setValue("recentFiles", recentFiles);
    settings.setValue("fontSize", fontSize);
    settings.setValue("wordWrap", wordWrapEnabled);
}

bool TextEditor::maybeSave(int tabIndex) {
    CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(tabIndex));
    if (!editor || !editor->isModified()) return true;
    
    tabWidget->setCurrentIndex(tabIndex);
    
    const QMessageBox::StandardButton ret = QMessageBox::warning(this, "Jim",
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
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Jim",
            QString("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return;
    }
    
    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    CodeEditor *editor = new CodeEditor();
    editor->setPlainText(in.readAll());
    editor->setFileName(fileName);
    editor->document()->setModified(false);
    
    SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
    highlighters[editor] = highlighter;
    
    QFont font("Consolas", fontSize);
    editor->setFont(font);
    editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    
    // Apply current theme to the new editor
    applyThemeToEditor(editor, highlighter);
    
    connect(editor->document(), &QTextDocument::modificationChanged, this, &TextEditor::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateStatusBar);
    
    int index = tabWidget->addTab(editor, strippedName(fileName));
    tabWidget->setCurrentIndex(index);
    
    QApplication::restoreOverrideCursor();
    
    updateRecentFiles(fileName);
    statusBar()->showMessage("File loaded", 2000);
}

bool TextEditor::saveFileToPath(const QString &fileName) {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;
    
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, "Jim",
            QString("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false;
    }
    
    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << editor->toPlainText();
    QApplication::restoreOverrideCursor();
    
    setCurrentFile(fileName);
    updateRecentFiles(fileName);
    statusBar()->showMessage("File saved", 2000);
    return true;
}

void TextEditor::setCurrentFile(const QString &fileName) {
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    
    editor->setFileName(fileName);
    editor->document()->setModified(false);
    
    QString shownName = strippedName(fileName);
    tabWidget->setTabText(tabWidget->currentIndex(), shownName);
    setWindowTitle(shownName + " - Jim");
}

QString TextEditor::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}

void TextEditor::updateRecentFiles(const QString &fileName) {
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }
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

CodeEditor* TextEditor::currentEditor() {
    return qobject_cast<CodeEditor*>(tabWidget->currentWidget());
}

SyntaxHighlighter* TextEditor::currentHighlighter() {
    CodeEditor *editor = currentEditor();
    return editor ? highlighters.value(editor) : nullptr;
}

void TextEditor::toggleSplitView() {
    splitViewEnabled = !splitViewEnabled;
    splitViewAct->setChecked(splitViewEnabled);
    
    if (splitViewEnabled) {
        if (!tabWidget2) {
            tabWidget2 = new QTabWidget();
            tabWidget2->setTabsClosable(true);
            tabWidget2->setMovable(true);
            mainSplitter->addWidget(tabWidget2);
        }
        tabWidget2->show();
    } else {
        if (tabWidget2) {
            tabWidget2->hide();
        }
    }
}

void TextEditor::changeTheme() {
    currentThemeIndex = (currentThemeIndex + 1) % themes.size();
    applyThemeToAllEditors();
    statusBar()->showMessage(QString("Theme: %1").arg(themes[currentThemeIndex].name), 2000);
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
    
    currentThemeIndex = 0;
}

void TextEditor::applyThemeToEditor(CodeEditor *editor, SyntaxHighlighter *highlighter) {
    if (!editor) return;
    
    const ColorTheme &theme = themes[currentThemeIndex];
    editor->applyTheme(theme);
    if (highlighter) {
        highlighter->applyTheme(theme);
    }
}

void TextEditor::applyThemeToAllEditors() {
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) {
            applyThemeToEditor(editor, highlighters.value(editor));
        }
    }
}

void SyntaxHighlighter::applyTheme(const ColorTheme &theme) {
    keywordFormat.setForeground(theme.keyword);
    stringFormat.setForeground(theme.string);
    commentFormat.setForeground(theme.comment);
    numberFormat.setForeground(theme.number);
    functionFormat.setForeground(theme.function);
    classFormat.setForeground(theme.keyword);
    
    setupRules();
    rehighlight();
}

void TextEditor::setupUI() {
    // Main splitter for editor area
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Tab widget for files
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setDocumentMode(true);
    mainSplitter->addWidget(tabWidget);
    
    tabWidget2 = nullptr;
    
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &TextEditor::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &TextEditor::tabChanged);
    
    // File tree dock
    fileTreeDock = new QDockWidget("Files", this);
    fileTreeDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    
    fileTree = new QTreeView();
    fileSystemModel = new QFileSystemModel();
    fileSystemModel->setRootPath(QDir::homePath());
    fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    
    fileTree->setModel(fileSystemModel);
    fileTree->setRootIndex(fileSystemModel->index(QDir::homePath()));
    fileTree->setColumnWidth(0, 250);
    fileTree->setHeaderHidden(false);
    fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTree->setAnimated(true);
    fileTree->setIndentation(20);
    fileTree->setSortingEnabled(true);
    
    // Hide size, type, date columns for cleaner look
    for (int i = 1; i < fileSystemModel->columnCount(); ++i) {
        fileTree->hideColumn(i);
    }
    
    connect(fileTree, &QTreeView::doubleClicked, this, &TextEditor::onFileTreeDoubleClicked);
    
    fileTreeDock->setWidget(fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, fileTreeDock);
}

void TextEditor::applyModernStyle() {
    QString style = R"(
        QMainWindow {
            background-color: #1e1e1e;
            border: 1px solid #3e3e42;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QMenuBar {
            background-color: #2d2d30;
            color: #cccccc;
            border: none;
            border-bottom: 1px solid #3e3e42;
            padding: 2px;
        }
        QMenuBar::item {
            padding: 6px 14px;
            background: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #3e3e42;
        }
        QMenuBar::item:pressed {
            background-color: #094771;
        }
        QMenu {
            background-color: #2d2d30;
            color: #cccccc;
            border: 1px solid #3e3e42;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
            border-radius: 3px;
        }
        QMenu::item:selected {
            background-color: #094771;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3e3e42;
            margin: 4px 8px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #1e1e1e;
            top: -1px;
        }
        QTabBar {
            background-color: #2d2d30;
        }
        QTabBar::tab {
            background-color: #2d2d30;
            color: #969696;
            padding: 10px 20px;
            border: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
            min-width: 80px;
        }
        QTabBar::tab:selected {
            background-color: #1e1e1e;
            color: #ffffff;
            border-bottom: 2px solid #007acc;
        }
        QTabBar::tab:hover:!selected {
            background-color: #3e3e42;
            color: #cccccc;
        }
        QTabBar::close-button {
            subcontrol-position: right;
            margin: 4px;
            padding: 4px;
            border-radius: 2px;
            background-color: transparent;
            width: 16px;
            height: 16px;
        }
        QTabBar::close-button:hover {
            background-color: #e81123;
        }
        QTabBar::close-button:pressed {
            background-color: #c50f1f;
        }
        QStatusBar {
            background-color: #007acc;
            color: #ffffff;
            border: none;
            padding: 4px;
        }
        QStatusBar QLabel {
            background-color: transparent;
            color: #ffffff;
            padding: 2px 8px;
        }
        QDockWidget {
            color: #cccccc;
            border: none;
            titlebar-close-icon: url(close.png);
            titlebar-normal-icon: url(float.png);
        }
        QDockWidget::title {
            background-color: #2d2d30;
            padding: 8px;
            border: none;
            text-align: left;
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
        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNNiA0bDQgNGwtNCA0VjR6IiBmaWxsPSIjY2NjY2NjIi8+PC9zdmc+);
        }
        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNNCAxMGw0LTQgNCA0SDR6IiBmaWxsPSIjY2NjY2NjIi8+PC9zdmc+);
        }
        QHeaderView::section {
            background-color: #2d2d30;
            color: #cccccc;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #3e3e42;
        }
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 12px;
            margin: 0;
            border: none;
        }
        QScrollBar::handle:vertical {
            background-color: #424242;
            min-height: 30px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4e4e4e;
        }
        QScrollBar::handle:vertical:pressed {
            background-color: #5a5a5a;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
        QScrollBar:horizontal {
            background-color: #1e1e1e;
            height: 12px;
            margin: 0;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background-color: #424242;
            min-width: 30px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4e4e4e;
        }
        QScrollBar::handle:horizontal:pressed {
            background-color: #5a5a5a;
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
        }
        QSplitter::handle {
            background-color: #3e3e42;
            width: 1px;
            height: 1px;
        }
        QSplitter::handle:hover {
            background-color: #007acc;
        }
    )";
    
    setStyleSheet(style);
}

void TextEditor::openFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", QDir::homePath());
    if (!folder.isEmpty()) {
        currentFolder = folder;
        fileTree->setRootIndex(fileSystemModel->index(folder));
        statusBar()->showMessage("Opened folder: " + folder, 2000);
    }
}

void TextEditor::toggleFileTree() {
    if (fileTreeDock->isVisible()) {
        fileTreeDock->hide();
    } else {
        fileTreeDock->show();
    }
}

void TextEditor::onFileTreeDoubleClicked(const QModelIndex &index) {
    QString filePath = fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);
    
    if (fileInfo.isFile()) {
        // Check if already open
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
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
    if (!editor) return;
    
    QColor bgColor = QColorDialog::getColor(Qt::white, this, "Choose Background Color");
    if (bgColor.isValid()) {
        QPalette p = editor->palette();
        p.setColor(QPalette::Base, bgColor);
        
        // Auto-adjust text color based on background brightness
        int brightness = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;
        QColor textColor = brightness > 128 ? Qt::black : Qt::white;
        p.setColor(QPalette::Text, textColor);
        
        editor->setPalette(p);
        editor->update();
    }
}

void TextEditor::openFilePath(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        loadFile(filePath);
    }
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

void CodeEditor::miniMapPaintEvent(QPaintEvent *event) {
    QPainter painter(miniMap);
    painter.fillRect(event->rect(), QColor(40, 40, 40));
    
    int totalLines = document()->blockCount();
    if (totalLines == 0) return;
    
    int visibleLines = height() / fontMetrics().height();
    
    // Only draw visible portion for performance
    int startLine = (event->rect().top() * totalLines) / miniMap->height();
    int endLine = (event->rect().bottom() * totalLines) / miniMap->height() + 1;
    
    QTextBlock block = document()->findBlockByLineNumber(qMax(0, startLine));
    int blockNumber = block.blockNumber();
    
    // Draw code lines
    painter.setPen(QColor(180, 180, 180));
    while (block.isValid() && blockNumber <= endLine) {
        int y = (blockNumber * miniMap->height()) / totalLines;
        
        QString text = block.text().trimmed();
        if (!text.isEmpty()) {
            // Draw a simple line
            int lineWidth = qMin(text.length() * 2, miniMap->width() - 10);
            painter.drawLine(5, y, 5 + lineWidth, y);
        }
        
        block = block.next();
        blockNumber++;
    }
    
    // Draw viewport indicator
    int firstVisible = firstVisibleBlock().blockNumber();
    int viewportY = (firstVisible * miniMap->height()) / totalLines;
    int viewportHeight = qMax(10, (visibleLines * miniMap->height()) / totalLines);
    
    painter.fillRect(0, viewportY, miniMap->width(), viewportHeight, QColor(100, 100, 100, 100));
    painter.setPen(QColor(0, 120, 215));
    painter.drawRect(0, viewportY, miniMap->width() - 1, viewportHeight);
}

void TextEditor::toggleMiniMap() {
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) {
            MiniMap *miniMap = editor->getMiniMap();
            if (miniMap) {
                if (miniMapAct->isChecked()) {
                    miniMap->show();
                } else {
                    miniMap->hide();
                }
                // Trigger resize to update margins
                QResizeEvent event(editor->size(), editor->size());
                QApplication::sendEvent(editor, &event);
            }
        }
    }
}
