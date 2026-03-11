#include "texteditor.h"
#include "linenumberarea.h"
#include <QApplication>
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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWheelEvent>

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
  return Language::PlainText;
}

// ============================================================
// CodeEditor Implementation
// ============================================================
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), smoothScrollEnabled(true), targetScrollValue(0),
      currentLanguage(Language::PlainText) {
  lineNumberArea = new LineNumberArea(this);
  foldingArea = new FoldingArea(this);
  miniMap = new MiniMap(this);
  miniMap->hide();

  scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value", this);
  scrollAnimation->setDuration(40);
  scrollAnimation->setEasingCurve(QEasingCurve::OutQuad);

  connect(this, &CodeEditor::blockCountChanged, this,
          &CodeEditor::updateLineNumberAreaWidth);
  connect(this, &CodeEditor::updateRequest, this,
          &CodeEditor::updateLineNumberArea);
  connect(this, &CodeEditor::cursorPositionChanged, this,
          &CodeEditor::highlightCurrentLine);

  QTimer *minimapUpdateTimer = new QTimer(this);
  minimapUpdateTimer->setSingleShot(true);
  minimapUpdateTimer->setInterval(50);
  connect(minimapUpdateTimer, &QTimer::timeout, this, [this]() {
    if (miniMap->isVisible())
      miniMap->update();
  });
  connect(this, &CodeEditor::updateRequest, this,
          [minimapUpdateTimer]() { minimapUpdateTimer->start(); });

  updateLineNumberAreaWidth(0);
  highlightCurrentLine();
  setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
}

void CodeEditor::setLanguage(Language lang) { currentLanguage = lang; }

void CodeEditor::applyTheme(const ColorTheme &theme) {
  currentTheme = theme;
  QPalette p = palette();
  p.setColor(QPalette::Base, theme.background);
  p.setColor(QPalette::Text, theme.foreground);
  setPalette(p);

  QString style = QString("QPlainTextEdit { background-color: %1; color: %2; "
                          "selection-background-color: %3; border: none; }")
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
  int rightMargin = miniMap->isVisible() ? miniMapWidth() : 0;
  setViewportMargins(lineNumberAreaWidth() + foldingAreaWidth(), 0, rightMargin,
                     0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
  if (dy) {
    lineNumberArea->scroll(0, dy);
    foldingArea->scroll(0, dy);
  } else {
    lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    foldingArea->update(0, rect.y(), foldingArea->width(), rect.height());
  }
  if (rect.contains(viewport()->rect()))
    updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
  QPlainTextEdit::resizeEvent(e);
  QRect cr = contentsRect();
  int lnw = lineNumberAreaWidth();
  int fw = foldingAreaWidth();
  lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lnw, cr.height()));
  foldingArea->setGeometry(QRect(cr.left() + lnw, cr.top(), fw, cr.height()));

  if (miniMap->isVisible()) {
    miniMap->setGeometry(QRect(cr.right() - miniMapWidth(), cr.top(),
                               miniMapWidth(), cr.height()));
    setViewportMargins(lnw + fw, 0, miniMapWidth(), 0);
  } else {
    setViewportMargins(lnw + fw, 0, 0, 0);
  }
}

void CodeEditor::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;
  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;
    QColor lineColor = currentTheme.currentLine.isValid()
                           ? currentTheme.currentLine
                           : QColor(Qt::yellow).lighter(160);
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
  painter.setRenderHint(QPainter::TextAntialiasing, false);
  QColor bgColor = currentTheme.lineNumberBg.isValid()
                       ? currentTheme.lineNumberBg
                       : QColor(240, 240, 240);
  QColor fgColor = currentTheme.lineNumberFg.isValid()
                       ? currentTheme.lineNumberFg
                       : Qt::gray;
  painter.fillRect(event->rect(), bgColor);
  painter.setPen(fgColor);

  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());
  int lineHeight = fontMetrics().height();
  int width = lineNumberArea->width() - 5;
  int currentLine = textCursor().blockNumber();

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      if (blockNumber == currentLine)
        painter.setPen(QColor(255, 255, 255));
      else
        painter.setPen(fgColor);
      painter.drawText(0, top, width, lineHeight, Qt::AlignRight,
                       QString::number(blockNumber + 1));
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNumber;
  }
}

// ============================================================
// Code Folding
// ============================================================
bool CodeEditor::isFoldable(const QTextBlock &block) const {
  QString text = block.text().trimmed();
    return text.endsWith('{') || (text.endsWith('(') && text.contains("class ")) ||
           text.startsWith("def ") || text.startsWith("function ") ||
           text.startsWith("class ") || text.endsWith(":");
}

bool CodeEditor::isFolded(const QTextBlock &block) const {
  QTextBlock next = block.next();
  return next.isValid() && !next.isVisible();
}

int CodeEditor::findMatchingBrace(const QTextBlock &block) const {
  QString text = block.text();
  int depth = 0;
  for (const QChar &c : text) {
    if (c == '{')
      depth++;
    if (c == '}')
      depth--;
  }
  if (depth <= 0)
    return block.blockNumber();

  QTextBlock b = block.next();
  while (b.isValid()) {
    QString t = b.text();
    for (const QChar &c : t) {
      if (c == '{')
        depth++;
      if (c == '}')
        depth--;
    }
    if (depth <= 0)
      return b.blockNumber();
    b = b.next();
  }
  return document()->blockCount() - 1;
}

void CodeEditor::toggleFoldAt(int blockNumber) {
  QTextBlock block = document()->findBlockByNumber(blockNumber);
  if (!block.isValid() || !isFoldable(block))
    return;

  bool fold = !isFolded(block);
  int endBlock = findMatchingBrace(block);

  QTextBlock b = block.next();
  while (b.isValid() && b.blockNumber() <= endBlock) {
    b.setVisible(!fold);
    b = b.next();
  }
  document()->markContentsDirty(block.position(), document()->characterCount() -
                                                      block.position());
  updateLineNumberAreaWidth(0);
  viewport()->update();
}

void CodeEditor::foldingAreaPaintEvent(QPaintEvent *event) {
  QPainter painter(foldingArea);
  QColor bgColor = currentTheme.lineNumberBg.isValid()
                       ? currentTheme.lineNumberBg
                       : QColor(240, 240, 240);
  painter.fillRect(event->rect(), bgColor);

  QTextBlock block = firstVisibleBlock();
  int top =
      qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top() &&
        isFoldable(block)) {
      int yCenter = top + (bottom - top) / 2;
      int xCenter = foldingAreaWidth() / 2;

      painter.setPen(QColor(180, 180, 180));
      painter.setBrush(Qt::NoBrush);

      if (isFolded(block)) {
        // Draw right-pointing triangle ▶
        QPolygon tri;
        tri << QPoint(xCenter - 3, yCenter - 4) << QPoint(xCenter + 4, yCenter)
            << QPoint(xCenter - 3, yCenter + 4);
        painter.setBrush(QColor(180, 180, 180));
        painter.drawPolygon(tri);
      } else {
        // Draw down-pointing triangle ▼
        QPolygon tri;
        tri << QPoint(xCenter - 4, yCenter - 3)
            << QPoint(xCenter + 4, yCenter - 3) << QPoint(xCenter, yCenter + 4);
        painter.setBrush(QColor(180, 180, 180));
        painter.drawPolygon(tri);
      }
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
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

  if (ch == '(' || ch == '[' || ch == '{') {
    QChar closing = ch == '(' ? ')' : ch == '[' ? ']' : '}';
    cursor.beginEditBlock();
    cursor.insertText(QString(ch) + QString(closing));
    cursor.movePosition(QTextCursor::Left);
    cursor.endEditBlock();
    setTextCursor(cursor);
    return;
  }

  if (ch == '"' || ch == '\'') {
    QChar nextChar =
        cursor.atEnd() ? QChar() : document()->characterAt(cursor.position());
    if (nextChar == ch) {
      cursor.movePosition(QTextCursor::Right);
      setTextCursor(cursor);
      return;
    } else {
      cursor.beginEditBlock();
      cursor.insertText(QString(ch) + QString(ch));
      cursor.movePosition(QTextCursor::Left);
      cursor.endEditBlock();
      setTextCursor(cursor);
      return;
    }
  }

  if (ch == ')' || ch == ']' || ch == '}') {
    QChar nextChar =
        cursor.atEnd() ? QChar() : document()->characterAt(cursor.position());
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
    if (c == ' ')
      indent++;
    else if (c == '\t')
      indent += 4;
    else
      break;
  }
  if (previousLine.trimmed().endsWith('{') ||
      previousLine.trimmed().endsWith(':'))
    indent += 4;
  cursor.insertText("\n" + QString(" ").repeated(indent));
  setTextCursor(cursor);
}

void CodeEditor::matchBrackets() {}

void CodeEditor::miniMapPaintEvent(QPaintEvent *event) {
  QPainter painter(miniMap);
  painter.fillRect(event->rect(), QColor(40, 40, 40));
  int totalLines = document()->blockCount();
  if (totalLines == 0)
    return;
  int visibleLines = height() / fontMetrics().height();
  int startLine = (event->rect().top() * totalLines) / miniMap->height();
  int endLine = (event->rect().bottom() * totalLines) / miniMap->height() + 1;
  QTextBlock block = document()->findBlockByLineNumber(qMax(0, startLine));
  int blockNumber = block.blockNumber();
  painter.setPen(QColor(180, 180, 180));
  while (block.isValid() && blockNumber <= endLine) {
    int y = (blockNumber * miniMap->height()) / totalLines;
    QString text = block.text().trimmed();
    if (!text.isEmpty()) {
      int lineWidth = qMin(text.length() * 2, miniMap->width() - 10);
      painter.drawLine(5, y, 5 + lineWidth, y);
    }
    block = block.next();
    blockNumber++;
  }
  int firstVisible = firstVisibleBlock().blockNumber();
  int viewportY = (firstVisible * miniMap->height()) / totalLines;
  int viewportHeight =
      qMax(10, (visibleLines * miniMap->height()) / totalLines);
  painter.fillRect(0, viewportY, miniMap->width(), viewportHeight,
                   QColor(100, 100, 100, 100));
  painter.setPen(QColor(0, 120, 215));
  painter.drawRect(0, viewportY, miniMap->width() - 1, viewportHeight);
}

void CodeEditor::wheelEvent(QWheelEvent *event) {
  if (!smoothScrollEnabled) {
    QPlainTextEdit::wheelEvent(event);
    return;
  }
  int numDegrees = event->angleDelta().y() / 8;
  int numSteps = numDegrees / 8;

  if (numSteps == 0) {
    event->accept();
    return;
  }

  QScrollBar *scrollBar = verticalScrollBar();
  int currentValue = scrollBar->value();
  targetScrollValue =
      currentValue - (numSteps * 5); // 5 lines per step for faster scrolling

  targetScrollValue =
      qMax(scrollBar->minimum(), qMin(targetScrollValue, scrollBar->maximum()));
  if (scrollAnimation->state() == QAbstractAnimation::Running)
    scrollAnimation->stop();
  scrollAnimation->setStartValue(currentValue);
  scrollAnimation->setEndValue(targetScrollValue);
  scrollAnimation->start();
  event->accept();
}

void CodeEditor::enableSmoothScrolling(bool enable) {
  smoothScrollEnabled = enable;
}

// ============================================================
// SyntaxHighlighter Implementation
// ============================================================
QRegularExpression SyntaxHighlighter::multiLineCommentStart = QRegularExpression("/\\*");
QRegularExpression SyntaxHighlighter::multiLineCommentEnd = QRegularExpression("\\*/");

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent), currentLanguage(Language::CPP) {
    setupRules();
}

void SyntaxHighlighter::setLanguage(Language lang) {
    currentLanguage = lang;
    setupRules();
    rehighlight();
}

void SyntaxHighlighter::setupRules() {
    highlightingRules.clear();
    switch (currentLanguage) {
        case Language::CPP: setupCppRules(); break;
        case Language::Python: setupPythonRules(); break;
        case Language::JavaScript: setupJavaScriptRules(); break;
        case Language::HTML: setupHtmlRules(); break;
        case Language::CSS: setupCssRules(); break;
        case Language::Rust: setupRustRules(); break;
        case Language::Go: setupGoRules(); break;
        case Language::JSON: setupJsonRules(); break;
        case Language::YAML: setupYamlRules(); break;
        case Language::Markdown: setupMarkdownRules(); break;
        default: setupCppRules(); break;
    }
}

void SyntaxHighlighter::setupCppRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {"\\bclass\\b","\\bconst\\b","\\benum\\b","\\bexplicit\\b","\\bfriend\\b","\\binline\\b","\\bint\\b","\\blong\\b","\\bnamespace\\b","\\boperator\\b","\\bprivate\\b","\\bprotected\\b","\\bpublic\\b","\\bshort\\b","\\bsignals\\b","\\bsigned\\b","\\bslots\\b","\\bstatic\\b","\\bstruct\\b","\\btemplate\\b","\\btypedef\\b","\\btypename\\b","\\bunion\\b","\\bunsigned\\b","\\bvirtual\\b","\\bvoid\\b","\\bvolatile\\b","\\bbool\\b","\\bchar\\b","\\bdouble\\b","\\bfloat\\b","\\bif\\b","\\belse\\b","\\bfor\\b","\\bwhile\\b","\\breturn\\b","\\bswitch\\b","\\bcase\\b","\\bbreak\\b","\\bcontinue\\b","\\bauto\\b","\\busing\\b","\\binclude\\b","\\bdefine\\b","\\bnullptr\\b","\\boverride\\b","\\bfinal\\b","\\bconstexpr\\b","\\bnoexcept\\b","\\btrue\\b","\\bfalse\\b","\\bnew\\b","\\bdelete\\b","\\bthrow\\b","\\btry\\b","\\bcatch\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b"); rule.format = classFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\".*?\"|'.*?'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupPythonRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {"\\bdef\\b","\\bclass\\b","\\bif\\b","\\belif\\b","\\belse\\b","\\bfor\\b","\\bwhile\\b","\\breturn\\b","\\bimport\\b","\\bfrom\\b","\\bas\\b","\\btry\\b","\\bexcept\\b","\\bfinally\\b","\\braise\\b","\\bwith\\b","\\byield\\b","\\blambda\\b","\\band\\b","\\bor\\b","\\bnot\\b","\\bin\\b","\\bis\\b","\\bpass\\b","\\bbreak\\b","\\bcontinue\\b","\\bNone\\b","\\bTrue\\b","\\bFalse\\b","\\bself\\b","\\bglobal\\b","\\bnonlocal\\b","\\bassert\\b","\\basync\\b","\\bawait\\b","\\bdel\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("\".*?\"|'.*?'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*"); rule.format = functionFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupJavaScriptRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {"\\bfunction\\b","\\bvar\\b","\\blet\\b","\\bconst\\b","\\bif\\b","\\belse\\b","\\bfor\\b","\\bwhile\\b","\\breturn\\b","\\bclass\\b","\\bnew\\b","\\bthis\\b","\\bimport\\b","\\bexport\\b","\\bdefault\\b","\\bfrom\\b","\\basync\\b","\\bawait\\b","\\btry\\b","\\bcatch\\b","\\bfinally\\b","\\bthrow\\b","\\bswitch\\b","\\bcase\\b","\\bbreak\\b","\\bcontinue\\b","\\btypeof\\b","\\binstanceof\\b","\\bvoid\\b","\\bnull\\b","\\bundefined\\b","\\btrue\\b","\\bfalse\\b","\\bof\\b","\\bin\\b","\\bdelete\\b","\\byield\\b","\\bsuper\\b","\\bextends\\b","\\bstatic\\b","\\binterface\\b","\\btype\\b","\\benum\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("=>"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\".*?\"|'.*?'|`[^`]*`"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupHtmlRules() {
    HighlightingRule rule;
    tagFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("</?[A-Za-z][A-Za-z0-9]*"); rule.format = tagFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("/?>"); rule.format = tagFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[A-Za-z-]+(?==)"); rule.format = attributeFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\""); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("<!--[^\n]*-->"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("&[A-Za-z]+;"); rule.format = numberFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupCssRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[.#]?[A-Za-z_-][A-Za-z0-9_-]*\\s*(?=\\{)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("[A-Za-z-]+(?=\\s*:)"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*(px|em|rem|%|vh|vw|s|ms)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[0-9a-fA-F]{3,8}\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("@[A-Za-z-]+"); rule.format = keywordFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupRustRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {"\\bfn\\b","\\blet\\b","\\bmut\\b","\\bif\\b","\\belse\\b","\\bfor\\b","\\bwhile\\b","\\breturn\\b","\\bstruct\\b","\\benum\\b","\\bimpl\\b","\\btrait\\b","\\buse\\b","\\bmod\\b","\\bpub\\b","\\bcrate\\b","\\bself\\b","\\bSelf\\b","\\bsuper\\b","\\bmatch\\b","\\bloop\\b","\\bin\\b","\\bas\\b","\\bref\\b","\\bwhere\\b","\\bunsafe\\b","\\basync\\b","\\bawait\\b","\\bmove\\b","\\btype\\b","\\bconst\\b","\\bstatic\\b","\\bdyn\\b","\\btrue\\b","\\bfalse\\b","\\bBox\\b","\\bVec\\b","\\bString\\b","\\bOption\\b","\\bResult\\b","\\bSome\\b","\\bNone\\b","\\bOk\\b","\\bErr\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("\".*?\"|'.'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[A-Z][A-Za-z0-9]+"); rule.format = classFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupGoRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {"\\bfunc\\b","\\bpackage\\b","\\bimport\\b","\\bvar\\b","\\bconst\\b","\\btype\\b","\\bstruct\\b","\\binterface\\b","\\bmap\\b","\\bchan\\b","\\bif\\b","\\belse\\b","\\bfor\\b","\\brange\\b","\\breturn\\b","\\bswitch\\b","\\bcase\\b","\\bdefault\\b","\\bbreak\\b","\\bcontinue\\b","\\bgo\\b","\\bdefer\\b","\\bselect\\b","\\bfallthrough\\b","\\bgoto\\b","\\bnil\\b","\\btrue\\b","\\bfalse\\b","\\bmake\\b","\\bappend\\b","\\blen\\b","\\bcap\\b","\\bstring\\b","\\bint\\b","\\bfloat64\\b","\\bbool\\b","\\bbyte\\b","\\berror\\b","\\bfmt\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("\".*?\"|`[^`]*`"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupJsonRules() {
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\"[^\"]*\"\\s*(?=:)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression(":\\s*\"[^\"]*\""); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\btrue\\b|\\bfalse\\b|\\bnull\\b"); rule.format = keywordFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupYamlRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^[A-Za-z_][A-Za-z0-9_-]*(?=\\s*:)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\btrue\\b|\\bfalse\\b|\\bnull\\b|\\byes\\b|\\bno\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*-\\s"); rule.format = functionFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::setupMarkdownRules() {
    HighlightingRule rule;
    headingFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^#{1,6}\\s.*$"); rule.format = headingFormat; highlightingRules.append(rule);
    boldFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*|__[^_]+__"); rule.format = boldFormat; highlightingRules.append(rule);
    QTextCharFormat italicFmt; italicFmt.setFontItalic(true);
    rule.pattern = QRegularExpression("\\*[^*]+\\*|_[^_]+_"); rule.format = italicFmt; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("`[^`]+`"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("```[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\[.*?\\]\\(.*?\\)"); rule.format = linkFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*[-*+]\\s"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*\\d+\\.\\s"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^>\\s.*$"); rule.format = commentFormat; highlightingRules.append(rule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (text.isEmpty()) { setCurrentBlockState(0); return; }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (currentLanguage == Language::CPP || currentLanguage == Language::JavaScript ||
        currentLanguage == Language::Rust || currentLanguage == Language::Go || currentLanguage == Language::CSS) {
        if (previousBlockState() != 1) startIndex = text.indexOf(multiLineCommentStart);
        while (startIndex >= 0) {
            int endIndex = text.indexOf(multiLineCommentEnd, startIndex);
            int commentLength;
            if (endIndex == -1) { setCurrentBlockState(1); commentLength = text.length() - startIndex; }
            else { commentLength = endIndex - startIndex + 2; }
            setFormat(startIndex, commentLength, commentFormat);
            startIndex = text.indexOf(multiLineCommentStart, startIndex + commentLength);
        }
    }
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

void SyntaxHighlighter::applyTheme(const ColorTheme &theme) {
    keywordFormat.setForeground(theme.keyword);
    stringFormat.setForeground(theme.string);
    commentFormat.setForeground(theme.comment);
    numberFormat.setForeground(theme.number);
    functionFormat.setForeground(theme.function);
    classFormat.setForeground(theme.keyword);
    tagFormat.setForeground(theme.keyword);
    attributeFormat.setForeground(theme.function);
    headingFormat.setForeground(theme.keyword);
    boldFormat.setForeground(theme.foreground);
    linkFormat.setForeground(QColor(86, 156, 214));
    setupRules();
    rehighlight();
}
// ============================================================
// WelcomeWidget Implementation
// ============================================================
WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void WelcomeWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);
    
    QLabel *logo = new QLabel("Jim");
    logo->setStyleSheet("font-size: 64px; font-weight: 300; color: #569cd6; letter-spacing: 8px; font-family: 'Segoe UI', 'Consolas', monospace;");
    logo->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logo);
    
    QLabel *subtitle = new QLabel("Lightweight Code Editor");
    subtitle->setStyleSheet("font-size: 16px; color: #808080; font-weight: 300; letter-spacing: 2px; margin-bottom: 30px;");
    subtitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subtitle);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(16);
    
    QString btnStyle = "QPushButton { background-color: #0e639c; color: #ffffff; border: none; padding: 12px 28px; border-radius: 6px; font-size: 14px; font-weight: 500; min-width: 140px; } QPushButton:hover { background-color: #1177bb; } QPushButton:pressed { background-color: #094771; }";
    
    QPushButton *openFileBtn = new QPushButton("Open File");
    openFileBtn->setStyleSheet(btnStyle);
    connect(openFileBtn, &QPushButton::clicked, this, &WelcomeWidget::openFileRequested);
    buttonLayout->addWidget(openFileBtn);
    
    QPushButton *openFolderBtn = new QPushButton("Open Folder");
    openFolderBtn->setStyleSheet(btnStyle);
    connect(openFolderBtn, &QPushButton::clicked, this, &WelcomeWidget::openFolderRequested);
    buttonLayout->addWidget(openFolderBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    QLabel *recentLabel = new QLabel("Recent Files");
    recentLabel->setStyleSheet("font-size: 13px; color: #cccccc; font-weight: 600; margin-top: 30px; letter-spacing: 1px;");
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
        if (count >= 8) break;
        QPushButton *btn = new QPushButton(QFileInfo(file).fileName());
        btn->setToolTip(file);
        btn->setStyleSheet("QPushButton { background-color: transparent; color: #3794ff; border: none; padding: 6px 16px; font-size: 13px; text-align: center; border-radius: 4px; min-width: 200px; } QPushButton:hover { background-color: #2a2d2e; color: #58b0ff; }");
        QString filePath = file;
        connect(btn, &QPushButton::clicked, this, [this, filePath]() { emit recentFileClicked(filePath); });
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
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(0);
    pathLabel = new QLabel("");
    pathLabel->setStyleSheet("color: #999999; font-size: 12px; font-family: 'Segoe UI', sans-serif;");
    layout->addWidget(pathLabel);
    layout->addStretch();
    setFixedHeight(28);
    setStyleSheet("QWidget { background-color: #252526; border-bottom: 1px solid #3e3e42; }");
}

void BreadcrumbBar::updatePath(const QString &filePath, const QString &symbol) {
    if (filePath.isEmpty()) { pathLabel->setText("Untitled"); return; }
    QFileInfo info(filePath);
    QString display = "<span style='color:#808080;'>" + info.absolutePath() + " &gt; </span>"
                    + "<span style='color:#cccccc;'>" + info.fileName() + "</span>";
    if (!symbol.isEmpty())
        display += "<span style='color:#808080;'> &gt; </span><span style='color:#dcdcaa;'>" + symbol + "</span>";
    pathLabel->setText(display);
}

// ============================================================
// TerminalWidget Implementation
// ============================================================
TerminalWidget::TerminalWidget(QWidget *parent) : QWidget(parent), process(nullptr) {
    setupUI();
}

TerminalWidget::~TerminalWidget() {
    if (process) { process->kill(); process->waitForFinished(1000); }
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
    output->setFont(QFont("Consolas", 11));
    output->setStyleSheet("QPlainTextEdit { background-color: #1e1e1e; color: #cccccc; border: none; padding: 8px; selection-background-color: #264f78; }");
    output->setMaximumBlockCount(5000);
    layout->addWidget(output);
    
    QWidget *inputBar = new QWidget();
    QHBoxLayout *inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(8, 4, 8, 4);
    inputLayout->setSpacing(8);
    QLabel *prompt = new QLabel("$");
    prompt->setStyleSheet("color: #569cd6; font-size: 13px; font-weight: bold; font-family: Consolas;");
    inputLayout->addWidget(prompt);
    input = new QLineEdit();
    input->setFont(QFont("Consolas", 11));
    input->setStyleSheet("QLineEdit { background-color: #2d2d30; color: #cccccc; border: 1px solid #3e3e42; border-radius: 4px; padding: 6px 10px; } QLineEdit:focus { border-color: #007acc; }");
    input->setPlaceholderText("Enter command...");
    connect(input, &QLineEdit::returnPressed, this, &TerminalWidget::executeCommand);
    inputLayout->addWidget(input);
    inputBar->setStyleSheet("background-color: #252526; border-top: 1px solid #3e3e42;");
    layout->addWidget(inputBar);
    
    currentDir = QDir::currentPath();
    setStyleSheet("background-color: #1e1e1e;");
}

void TerminalWidget::startShell() { }

void TerminalWidget::executeCommand() {
    QString cmd = input->text().trimmed();
    if (cmd.isEmpty()) return;
    appendOutput("\n$ " + cmd + "\n");
    input->clear();
    
    if (cmd.startsWith("cd ")) {
        QString dir = cmd.mid(3).trimmed();
        QDir newDir(currentDir);
        if (newDir.cd(dir)) { currentDir = newDir.absolutePath(); appendOutput(currentDir + "\n"); }
        else { appendOutput("cd: no such directory: " + dir + "\n"); }
        return;
    }
    if (cmd == "clear" || cmd == "cls") { output->clear(); return; }
    
    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(currentDir);
    connect(proc, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyRead);
    connect(proc, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyRead);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &TerminalWidget::onProcessFinished);
#ifdef Q_OS_WIN
    proc->start("cmd.exe", QStringList() << "/c" << cmd);
#else
    proc->start("/bin/sh", QStringList() << "-c" << cmd);
#endif
}

void TerminalWidget::onReadyRead() {
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        appendOutput(QString::fromLocal8Bit(proc->readAllStandardOutput()));
        appendOutput(QString::fromLocal8Bit(proc->readAllStandardError()));
    }
}

void TerminalWidget::onProcessFinished(int, QProcess::ExitStatus) {
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        appendOutput(QString::fromLocal8Bit(proc->readAllStandardOutput()));
        appendOutput(QString::fromLocal8Bit(proc->readAllStandardError()));
        proc->deleteLater();
    }
}

void TerminalWidget::appendOutput(const QString &text) {
    QTextCursor cursor = output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    output->setTextCursor(cursor);
    output->ensureCursorVisible();
}

// ============================================================
// TextEditor Implementation (Main Window)
// ============================================================
TextEditor::TextEditor(QWidget *parent) : QMainWindow(parent), wordWrapEnabled(false), splitViewEnabled(false), fontSize(11), currentThemeIndex(0) {
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &TextEditor::onFileChangedExternally);
    
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
}

TextEditor::~TextEditor() { writeSettings(); }

void TextEditor::setupUI() {
    // Vertical splitter: top = editor area, bottom = terminal
    verticalSplitter = new QSplitter(Qt::Vertical, this);
    
    // Container for breadcrumb + editor
    QWidget *editorContainer = new QWidget();
    QVBoxLayout *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    
    // Breadcrumb bar
    breadcrumbBar = new BreadcrumbBar();
    editorLayout->addWidget(breadcrumbBar);
    
    // Main horizontal splitter for editor tabs
    mainSplitter = new QSplitter(Qt::Horizontal);
    tabWidget = new QTabWidget();
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setDocumentMode(true);
    mainSplitter->addWidget(tabWidget);
    tabWidget2 = nullptr;
    
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &TextEditor::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &TextEditor::tabChanged);
    
    editorLayout->addWidget(mainSplitter);
    verticalSplitter->addWidget(editorContainer);
    
    // Welcome widget (stacked on top of editor)
    welcomeWidget = new WelcomeWidget();
    connect(welcomeWidget, &WelcomeWidget::openFileRequested, this, &TextEditor::openFile);
    connect(welcomeWidget, &WelcomeWidget::openFolderRequested, this, &TextEditor::openFolder);
    connect(welcomeWidget, &WelcomeWidget::recentFileClicked, this, [this](const QString &path) { loadFile(path); });
    
    // Terminal widget (hidden by default)
    terminalWidget = new TerminalWidget();
    terminalWidget->hide();
    verticalSplitter->addWidget(terminalWidget);
    verticalSplitter->setStretchFactor(0, 3);
    verticalSplitter->setStretchFactor(1, 1);
    
    setCentralWidget(verticalSplitter);
    
    // File tree dock
    fileTreeDock = new QDockWidget("Explorer", this);
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
    for (int i = 1; i < fileSystemModel->columnCount(); ++i) fileTree->hideColumn(i);
    connect(fileTree, &QTreeView::doubleClicked, this, &TextEditor::onFileTreeDoubleClicked);
    fileTreeDock->setWidget(fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, fileTreeDock);
}

void TextEditor::showWelcomeScreen() {
    welcomeWidget->setRecentFiles(recentFiles);
    // Add as a tab
    int idx = tabWidget->addTab(welcomeWidget, "Welcome");
    tabWidget->setCurrentIndex(idx);
}

void TextEditor::hideWelcomeScreen() {
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->widget(i) == welcomeWidget) {
            tabWidget->removeTab(i);
            break;
        }
    }
}

void TextEditor::watchFile(const QString &filePath) {
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
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor && editor->getFileName() == path) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "File Changed",
                QString("The file '%1' has been modified externally.\nDo you want to reload it?")
                    .arg(QFileInfo(path).fileName()),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                QFile file(path);
                if (file.open(QFile::ReadOnly | QFile::Text)) {
                    int cursorPos = editor->textCursor().position();
                    editor->setPlainText(QTextStream(&file).readAll());
                    editor->document()->setModified(false);
                    QTextCursor cursor = editor->textCursor();
                    cursor.setPosition(qMin(cursorPos, editor->document()->characterCount() - 1));
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
    if (!editor) return "";
    QTextBlock block = editor->textCursor().block();
    // Walk upward to find function/class definition
    while (block.isValid()) {
        QString text = block.text().trimmed();
        QRegularExpression funcRe("(?:void|int|bool|auto|QString|float|double|char|string|fn|func|def|function|pub\\s+fn|async\\s+function)\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(");
        QRegularExpressionMatch m = funcRe.match(text);
        if (m.hasMatch()) return m.captured(1) + "()";
        QRegularExpression classRe("(?:class|struct|enum|interface|trait|impl)\\s+([A-Za-z_][A-Za-z0-9_]*)");
        m = classRe.match(text);
        if (m.hasMatch()) return m.captured(1);
        block = block.previous();
    }
    return "";
}

void TextEditor::updateBreadcrumb() {
    CodeEditor *editor = currentEditor();
    if (!editor) { breadcrumbBar->updatePath("", ""); return; }
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
    viewMenu->addAction(terminalAct);
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
    languageLabel = new QLabel("Plain Text");
    languageLabel->setStyleSheet("padding: 2px 12px; color: #ffffff; background-color: transparent;");
    statusBar()->addPermanentWidget(languageLabel);
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage("Ready");
}

void TextEditor::newFile() {
    hideWelcomeScreen();
    CodeEditor *editor = new CodeEditor();
    SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
    highlighters[editor] = highlighter;
    QFont font("Consolas", fontSize);
    editor->setFont(font);
    editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    applyThemeToEditor(editor, highlighter);
    connect(editor->document(), &QTextDocument::modificationChanged, this, &TextEditor::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateStatusBar);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateBreadcrumb);
    int index = tabWidget->addTab(editor, "Untitled");
    tabWidget->setCurrentIndex(index);
    editor->setFocus();
}

void TextEditor::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", "",
        "All Files (*);;Text Files (*.txt);;C++ Files (*.cpp *.h);;Python Files (*.py);;JavaScript (*.js *.ts);;Rust (*.rs);;Go (*.go)");
    if (!fileName.isEmpty()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor && editor->getFileName() == fileName) { tabWidget->setCurrentIndex(i); return; }
        }
        loadFile(fileName);
    }
}

void TextEditor::openRecentFile() {
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) loadFile(action->data().toString());
}

bool TextEditor::saveFile() {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;
    if (editor->getFileName().isEmpty()) return saveFileAs();
    else return saveFileToPath(editor->getFileName());
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
    if (tabWidget->widget(index) == welcomeWidget) { tabWidget->removeTab(index); return; }
    if (maybeSave(index)) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
        if (editor) {
            unwatchFile(editor->getFileName());
            highlighters.remove(editor);
        }
        tabWidget->removeTab(index);
        if (tabWidget->count() == 0) showWelcomeScreen();
    }
}

void TextEditor::tabChanged(int) {
    updateStatusBar();
    updateBreadcrumb();
    CodeEditor *editor = currentEditor();
    if (editor) {
        QString title = "Jim";
        if (!editor->getFileName().isEmpty()) title = strippedName(editor->getFileName()) + " - " + title;
        if (editor->isModified()) title = "*" + title;
        setWindowTitle(title);
        // Update language label
        Language lang = editor->getLanguage();
        QStringList langNames = {"Plain Text","C++","Python","JavaScript","HTML","CSS","Rust","Go","JSON","YAML","Markdown"};
        languageLabel->setText(langNames[static_cast<int>(lang)]);
    }
}

void TextEditor::findText() {
    bool ok;
    QString text = QInputDialog::getText(this, "Find", "Find what:", QLineEdit::Normal, lastSearchText, &ok);
    if (ok && !text.isEmpty()) { lastSearchText = text; findNext(); }
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
    QString findStr = QInputDialog::getText(this, "Replace", "Find what:", QLineEdit::Normal, lastSearchText, &ok);
    if (!ok || findStr.isEmpty()) return;
    QString replaceStr = QInputDialog::getText(this, "Replace", "Replace with:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    lastSearchText = findStr;
    QString content = editor->toPlainText();
    content.replace(findStr, replaceStr);
    editor->setPlainText(content);
}

void TextEditor::goToLine() {
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    bool ok;
    int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, editor->document()->blockCount(), 1, &ok);
    if (ok) {
        QTextCursor cursor(editor->document()->findBlockByLineNumber(line - 1));
        editor->setTextCursor(cursor);
        editor->centerCursor();
    }
}

void TextEditor::documentWasModified() { tabChanged(tabWidget->currentIndex()); }

void TextEditor::updateStatusBar() {
    CodeEditor *editor = currentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        statusLabel->setText(QString("Ln %1, Col %2").arg(cursor.blockNumber() + 1).arg(cursor.columnNumber() + 1));
    }
}

void TextEditor::increaseFontSize() {
    fontSize++;
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) { QFont font = editor->font(); font.setPointSize(fontSize); editor->setFont(font); }
    }
}

void TextEditor::decreaseFontSize() {
    if (fontSize > 6) {
        fontSize--;
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor) { QFont font = editor->font(); font.setPointSize(fontSize); editor->setFont(font); }
        }
    }
}

void TextEditor::toggleWordWrap() {
    wordWrapEnabled = !wordWrapEnabled;
    wordWrapAct->setChecked(wordWrapEnabled);
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    }
}

void TextEditor::toggleTerminal() {
    if (terminalWidget->isVisible()) terminalWidget->hide();
    else terminalWidget->show();
    terminalAct->setChecked(terminalWidget->isVisible());
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
        if (tabWidget->widget(i) == welcomeWidget) continue;
        if (!maybeSave(i)) { event->ignore(); return; }
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
        case QMessageBox::Save: return saveFile();
        case QMessageBox::Cancel: return false;
        default: break;
    }
    return true;
}

void TextEditor::loadFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Jim", QString("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return;
    }
    hideWelcomeScreen();
    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    CodeEditor *editor = new CodeEditor();
    editor->setPlainText(in.readAll());
    editor->setFileName(fileName);
    editor->document()->setModified(false);
    
    // Auto-detect language
    Language lang = detectLanguage(fileName);
    editor->setLanguage(lang);
    
    SyntaxHighlighter *highlighter = new SyntaxHighlighter(editor->document());
    highlighter->setLanguage(lang);
    highlighters[editor] = highlighter;
    
    QFont font("Consolas", fontSize);
    editor->setFont(font);
    editor->setLineWrapMode(wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    applyThemeToEditor(editor, highlighter);
    
    connect(editor->document(), &QTextDocument::modificationChanged, this, &TextEditor::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateStatusBar);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateBreadcrumb);
    
    int index = tabWidget->addTab(editor, strippedName(fileName));
    tabWidget->setCurrentIndex(index);
    
    QApplication::restoreOverrideCursor();
    
    watchFile(fileName);
    updateRecentFiles(fileName);
    
    // Update language label
    QStringList langNames = {"Plain Text","C++","Python","JavaScript","HTML","CSS","Rust","Go","JSON","YAML","Markdown"};
    languageLabel->setText(langNames[static_cast<int>(lang)]);
    
    statusBar()->showMessage("File loaded", 2000);
}

bool TextEditor::saveFileToPath(const QString &fileName) {
    CodeEditor *editor = currentEditor();
    if (!editor) return false;
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, "Jim", QString("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
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
    unwatchFile(editor->getFileName());
    editor->setFileName(fileName);
    editor->document()->setModified(false);
    // Re-detect language
    Language lang = detectLanguage(fileName);
    editor->setLanguage(lang);
    SyntaxHighlighter *hl = highlighters.value(editor);
    if (hl) { hl->setLanguage(lang); }
    QString shownName = strippedName(fileName);
    tabWidget->setTabText(tabWidget->currentIndex(), shownName);
    setWindowTitle(shownName + " - Jim");
    watchFile(fileName);
}

QString TextEditor::strippedName(const QString &fullFileName) { return QFileInfo(fullFileName).fileName(); }

void TextEditor::updateRecentFiles(const QString &fileName) {
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    while (recentFiles.size() > 10) recentFiles.removeLast();
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

CodeEditor* TextEditor::currentEditor() { return qobject_cast<CodeEditor*>(tabWidget->currentWidget()); }
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
        if (tabWidget2) tabWidget2->hide();
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
    
    currentThemeIndex = 1; // Default to dark
}

void TextEditor::applyThemeToEditor(CodeEditor *editor, SyntaxHighlighter *highlighter) {
    if (!editor) return;
    const ColorTheme &theme = themes[currentThemeIndex];
    editor->applyTheme(theme);
    if (highlighter) highlighter->applyTheme(theme);
}

void TextEditor::applyThemeToAllEditors() {
    for (int i = 0; i < tabWidget->count(); ++i) {
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) applyThemeToEditor(editor, highlighters.value(editor));
    }
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
    if (fileTreeDock->isVisible()) fileTreeDock->hide();
    else fileTreeDock->show();
}

void TextEditor::onFileTreeDoubleClicked(const QModelIndex &index) {
    QString filePath = fileSystemModel->filePath(index);
    QFileInfo fileInfo(filePath);
    if (fileInfo.isFile()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
            if (editor && editor->getFileName() == filePath) { tabWidget->setCurrentIndex(i); return; }
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
        int brightness = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;
        QColor textColor = brightness > 128 ? Qt::black : Qt::white;
        p.setColor(QPalette::Text, textColor);
        editor->setPalette(p);
        editor->update();
    }
}

void TextEditor::openFilePath(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.isFile()) loadFile(filePath);
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
        CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor) {
            MiniMap *miniMap = editor->getMiniMap();
            if (miniMap) {
                if (miniMapAct->isChecked()) miniMap->show();
                else miniMap->hide();
                QResizeEvent event(editor->size(), editor->size());
                QApplication::sendEvent(editor, &event);
            }
        }
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
            background-color: #007acc;
            color: #ffffff;
            border: none;
            padding: 0px;
            font-size: 12px;
        }
        QStatusBar QLabel {
            background-color: transparent;
            color: #ffffff;
            padding: 3px 10px;
            font-size: 12px;
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
            background-color: #252526;
            width: 2px;
            height: 2px;
        }
        QSplitter::handle:hover {
            background-color: #007acc;
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
