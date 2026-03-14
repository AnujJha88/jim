#include "texteditor.h"
#include "hexeditor.h"
#include "linenumberarea.h"
#include "aiautocomplete.h"
#include "aisettingsdialog.h"
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
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProcessEnvironment>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
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
  setPlaceholderText("Start typing code...");
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

void CodeEditor::setSearchSelections(const QList<QTextCursor> &selections) {
  searchSelections = selections;
  highlightCurrentLine();
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
  
  // Search highlights
  for (const QTextCursor &cursor : searchSelections) {
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(62, 62, 66)); // Subtle secondary highlight
    selection.cursor = cursor;
    extraSelections.append(selection);
  }

  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;
    QColor lineColor = currentTheme.currentLine.isValid()
                           ? currentTheme.currentLine
                           : QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    
    // If the current cursor is within a search match, highlight it more prominently
    for (int i = 0; i < searchSelections.size(); ++i) {
        if (searchSelections[i].selectionStart() == selection.cursor.selectionStart() &&
            searchSelections[i].selectionEnd() == selection.cursor.selectionEnd()) {
            selection.format.setBackground(QColor(163, 115, 20, 150)); // Golden highlight for current match
            break;
        }
    }
    
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
  return text.endsWith('{') ||
         (text.endsWith('(') && text.contains("class ")) ||
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

  if (event->key() == Qt::Key_Home) {
    if (event->modifiers() == Qt::NoModifier ||
        event->modifiers() == Qt::ShiftModifier) {
      smartHome();
      // To handle Shift+Home selection, we could enhance smartHome to take a
      // KeepAnchor flag, but simple smartHome on Home press is a good start.
      return;
    }
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

void CodeEditor::duplicateLine() {
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  QString textText = cursor.selectedText();
  cursor.movePosition(QTextCursor::EndOfBlock);
  cursor.insertText("\n" + textText);
  cursor.endEditBlock();
}

void CodeEditor::moveLineUp() {
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if (cursor.blockNumber() == 0) {
    cursor.endEditBlock();
    return;
  }

  QTextBlock currentBlock = cursor.block();
  QTextBlock prevBlock = currentBlock.previous();

  // Select current block + trailing newline
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  if (!cursor.atEnd())
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QString text = cursor.selectedText();
  cursor.removeSelectedText();

  cursor.setPosition(prevBlock.position());
  cursor.insertText(text);
  cursor.endEditBlock();
}

void CodeEditor::moveLineDown() {
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  if (cursor.blockNumber() == document()->blockCount() - 1) {
    cursor.endEditBlock();
    return;
  }

  QTextBlock currentBlock = cursor.block();
  QTextBlock nextBlock = currentBlock.next();

  // Select current block + trailing newline
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  if (!cursor.atEnd())
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  QString text = cursor.selectedText();
  cursor.removeSelectedText();

  cursor.setPosition(nextBlock.position());
  cursor.movePosition(QTextCursor::EndOfBlock);
  if (cursor.atEnd())
    cursor.insertText("\n" + text.trimmed());
  else {
    cursor.movePosition(QTextCursor::NextCharacter);
    cursor.insertText(text);
  }

  cursor.endEditBlock();
}

void CodeEditor::deleteLine() {
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  if (!cursor.atEnd())
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  cursor.removeSelectedText();
  cursor.endEditBlock();
}

void CodeEditor::toggleComment() {
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();

  QString prefix = "//";
  if (currentLanguage == Language::Python || currentLanguage == Language::YAML)
    prefix = "#";
  else if (currentLanguage == Language::HTML)
    prefix = "<!--";

  int startBlock = cursor.selectionStart();
  int endBlock = cursor.selectionEnd();
  QTextCursor iterCursor(document());
  iterCursor.setPosition(startBlock);
  int firstBlockNum = iterCursor.blockNumber();
  iterCursor.setPosition(endBlock);
  int lastBlockNum = iterCursor.blockNumber();

  // Check if commenting or uncommenting
  bool allCommented = true;
  for (int i = firstBlockNum; i <= lastBlockNum; ++i) {
    QTextBlock block = document()->findBlockByLineNumber(i);
    QString text = block.text().trimmed();
    if (!text.isEmpty() && !text.startsWith(prefix)) {
      allCommented = false;
      break;
    }
  }

  for (int i = firstBlockNum; i <= lastBlockNum; ++i) {
    QTextBlock block = document()->findBlockByLineNumber(i);
    QString text = block.text();
    if (text.trimmed().isEmpty())
      continue;

    iterCursor.setPosition(block.position());
    if (allCommented) {
      int idx = text.indexOf(prefix);
      if (idx != -1) {
        iterCursor.setPosition(block.position() + idx);
        iterCursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor, prefix.length());
        if (prefix == "<!--") {
          // special HTML uncommenting
          QString full = text;
          int endIdx = full.indexOf("-->", idx);
          if (endIdx != -1) {
            QTextCursor endCur(document());
            endCur.setPosition(block.position() + endIdx);
            endCur.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor, 3);
            endCur.removeSelectedText();
          }
        }
        iterCursor.removeSelectedText();
        // Remove trailing space if exists
        if (iterCursor.block().text().length() > iterCursor.positionInBlock() &&
            iterCursor.block().text().at(iterCursor.positionInBlock()) == ' ') {
          iterCursor.deleteChar();
        }
      }
    } else {
      // Find first non-whitespace
      int idx = 0;
      while (idx < text.length() && text.at(idx).isSpace())
        idx++;
      iterCursor.setPosition(block.position() + idx);
      if (prefix == "<!--") {
        iterCursor.insertText(prefix + " ");
        iterCursor.movePosition(QTextCursor::EndOfBlock);
        iterCursor.insertText(" -->");
      } else {
        iterCursor.insertText(prefix + " ");
      }
    }
  }
  cursor.endEditBlock();
}

void CodeEditor::smartHome() {
  QTextCursor cursor = textCursor();
  QTextBlock block = cursor.block();
  QString text = block.text();

  int firstNonSpace = 0;
  while (firstNonSpace < text.length() && text.at(firstNonSpace).isSpace()) {
    firstNonSpace++;
  }

  int currentPos = cursor.positionInBlock();
  if (currentPos == firstNonSpace) {
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
  } else {
    cursor.setPosition(block.position() + firstNonSpace,
                       QTextCursor::MoveAnchor);
  }
  setTextCursor(cursor);
}

// ============================================================

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
// SyntaxHighlighter Implementation
// ============================================================
QRegularExpression SyntaxHighlighter::multiLineCommentStart =
    QRegularExpression("/\\*");
QRegularExpression SyntaxHighlighter::multiLineCommentEnd =
    QRegularExpression("\\*/");

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent), currentLanguage(Language::CPP) {
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
  case Language::CPP:
    setupCppRules();
    break;
  case Language::Python:
    setupPythonRules();
    break;
  case Language::JavaScript:
    setupJavaScriptRules();
    break;
  case Language::HTML:
    setupHtmlRules();
    break;
  case Language::CSS:
    setupCssRules();
    break;
  case Language::Rust:
    setupRustRules();
    break;
  case Language::Go:
    setupGoRules();
    break;
  case Language::JSON:
    setupJsonRules();
    break;
  case Language::YAML:
    setupYamlRules();
    break;
  case Language::Markdown:
    setupMarkdownRules();
    break;
  default:
    setupCppRules();
    break;
  }
}

void SyntaxHighlighter::setupCppRules() {
  HighlightingRule rule;

  // Preprocessor directives
  QTextCharFormat preprocessorFormat;
  preprocessorFormat.setForeground(keywordFormat.foreground());
  preprocessorFormat.setFontWeight(QFont::Bold);
  QStringList preprocessors = {
      "#include", "#define", "#pragma", "#if", "#ifdef", "#ifndef",
      "#elif",    "#else",   "#endif",  "#error", "#warning", "#undef",
      "#line",    "#using"};
  for (const QString &p : preprocessors) {
    rule.pattern = QRegularExpression(p + "\\b");
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);
  }

  keywordFormat.setFontWeight(QFont::Bold);
  QStringList kw = {
      "\\balignas\\b",       "\\balignof\\b",       "\\band\\b",
      "\\band_eq\\b",        "\\basm\\b",           "\\bauto\\b",
      "\\bbitand\\b",        "\\bbitor\\b",         "\\bbool\\b",
      "\\bbreak\\b",         "\\bcase\\b",          "\\bcatch\\b",
      "\\bchar\\b",          "\\bchar8_t\\b",       "\\bchar16_t\\b",
      "\\bchar32_t\\b",      "\\bclass\\b",         "\\bcompl\\b",
      "\\bconcept\\b",       "\\bconst\\b",         "\\bconsteval\\b",
      "\\bconstexpr\\b",      "\\bconstinit\\b",     "\\bconst_cast\\b",
      "\\bcontinue\\b",      "\\bco_await\\b",      "\\bco_return\\b",
      "\\bco_yield\\b",      "\\bdecltype\\b",      "\\bdefault\\b",
      "\\bdelete\\b",        "\\bdo\\b",            "\\bdouble\\b",
      "\\bdynamic_cast\\b",  "\\belse\\b",          "\\benum\\b",
      "\\bexplicit\\b",      "\\bexport\\b",        "\\bextern\\b",
      "\\bfalse\\b",         "\\bfinal\\b",         "\\bfloat\\b",
      "\\bfor\\b",           "\\bfriend\\b",        "\\bgoto\\b",
      "\\bif\\b",            "\\binline\\b",        "\\bint\\b",
      "\\blong\\b",          "\\bmutable\\b",       "\\bnamespace\\b",
      "\\bnew\\b",           "\\bnoexcept\\b",      "\\bnot\\b",
      "\\bnot_eq\\b",        "\\bnullptr\\b",       "\\boperator\\b",
      "\\bor\\b",            "\\bor_eq\\b",         "\\boverride\\b",
      "\\bprivate\\b",       "\\bprotected\\b",     "\\bpublic\\b",
      "\\breinterpret_cast\\b", "\\brequires\\b",     "\\breturn\\b",
      "\\bshort\\b",         "\\bsignals\\b",       "\\bsigned\\b",
      "\\bsizeof\\b",        "\\bslots\\b",         "\\bstatic\\b",
      "\\bstatic_assert\\b", "\\bstatic_cast\\b",   "\\bstruct\\b",
      "\\bswitch\\b",        "\\btemplate\\b",      "\\bthis\\b",
      "\\bthread_local\\b",  "\\bthrow\\b",         "\\btrue\\b",
      "\\btry\\b",           "\\btypedef\\b",       "\\btypeid\\b",
      "\\btypename\\b",      "\\bunion\\b",         "\\bunsigned\\b",
      "\\busing\\b",         "\\bvirtual\\b",       "\\bvoid\\b",
      "\\bvolatile\\b",      "\\bwchar_t\\b",       "\\bwhile\\b",
      "\\bxor\\b",           "\\bxor_eq\\b"};
  for (const QString &p : kw) {
    rule.pattern = QRegularExpression(p);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }
  classFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
  rule.format = classFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\".*?\"|'.*?'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  functionFormat.setFontItalic(true);
  rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("//[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupPythonRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  QStringList kw = {
      "\\band\\b",      "\\bas\\b",    "\\bassert\\b", "\\basync\\b",
      "\\bawait\\b",    "\\bbreak\\b",  "\\bclass\\b",  "\\bcontinue\\b",
      "\\bdef\\b",      "\\bdel\\b",    "\\belif\\b",   "\\belse\\b",
      "\\bexcept\\b",   "\\bFalse\\b",  "\\bfinally\\b", "\\bfor\\b",
      "\\bfrom\\b",     "\\bglobal\\b", "\\bif\\b",      "\\bimport\\b",
      "\\bin\\b",       "\\bis\\b",     "\\blambda\\b",  "\\bNone\\b",
      "\\bnonlocal\\b", "\\bnot\\b",    "\\bor\\b",      "\\bpass\\b",
      "\\braise\\b",    "\\breturn\\b", "\\bTrue\\b",   "\\btry\\b",
      "\\bwhile\\b",    "\\bwith\\b",    "\\byield\\b",   "\\bmatch\\b",
      "\\bcase\\b"};
  for (const QString &p : kw) {
    rule.pattern = QRegularExpression(p);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  // Python built-ins
  QTextCharFormat builtinFormat;
  builtinFormat.setForeground(functionFormat.foreground());
  builtinFormat.setFontItalic(true);
  QStringList builtins = {
      "\\babs\\b",   "\\ball\\b",      "\\bany\\b",    "\\bbin\\b",
      "\\bbool\\b",  "\\bdict\\b",     "\\bdir\\b",    "\\benumerate\\b",
      "\\beval\\b",  "\\bfloat\\b",    "\\binput\\b",  "\\bint\\b",
      "\\blen\\b",   "\\blist\\b",     "\\bmax\\b",    "\\bmin\\b",
      "\\bopen\\b",  "\\bprint\\b",    "\\brange\\b",  "\\bround\\b",
      "\\bstr\\b",   "\\bsum\\b",      "\\btuple\\b",  "\\btype\\b",
      "\\bzip\\b",   "\\bself\\b"};
  for (const QString &p : builtins) {
    rule.pattern = QRegularExpression(p);
    rule.format = builtinFormat;
    highlightingRules.append(rule);
  }

  rule.pattern = QRegularExpression("\".*?\"|'.*?'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  functionFormat.setFontItalic(true);
  rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("#[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*");
  rule.format = functionFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupJavaScriptRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  QStringList kw = {
      "\\bbreak\\b",      "\\bcase\\b",       "\\bcatch\\b",      "\\bclass\\b",
      "\\bconst\\b",      "\\bcontinue\\b",   "\\bdebugger\\b",   "\\bdefault\\b",
      "\\bdelete\\b",     "\\bdo\\b",         "\\belse\\b",       "\\bexport\\b",
      "\\bextends\\b",     "\\bfalse\\b",      "\\bfinally\\b",    "\\bfor\\b",
      "\\bfunction\\b",    "\\bif\\b",         "\\bimport\\b",     "\\bin\\b",
      "\\binstanceof\\b",  "\\bnew\\b",        "\\bnull\\b",       "\\breturn\\b",
      "\\bsuper\\b",      "\\bswitch\\b",     "\\bthis\\b",       "\\bthrow\\b",
      "\\btrue\\b",       "\\btry\\b",        "\\btypeof\\b",     "\\bvar\\b",
      "\\bvoid\\b",       "\\bwhile\\b",      "\\bwith\\b",       "\\bawait\\b",
      "\\blet\\b",        "\\bstatic\\b",     "\\byield\\b",      "\\benum\\b",
      "\\bimplements\\b", "\\binterface\\b",  "\\bpackage\\b",    "\\bprivate\\b",
      "\\bprotected\\b",  "\\bpublic\\b",     "\\basync\\b",      "\\bof\\b",
      "\\btype\\b",       "\\bfrom\\b"};
  for (const QString &p : kw) {
    rule.pattern = QRegularExpression(p);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }
  rule.pattern = QRegularExpression("=>");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\".*?\"|'.*?'|`[^`]*`");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  functionFormat.setFontItalic(true);
  rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("//[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupHtmlRules() {
  HighlightingRule rule;
  tagFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("</?[A-Za-z][A-Za-z0-9]*");
  rule.format = tagFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("/?>");
  rule.format = tagFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b[A-Za-z-]+(?==)");
  rule.format = attributeFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\"[^\"]*\"");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("<!--[^\n]*-->");
  rule.format = commentFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("&[A-Za-z]+;");
  rule.format = numberFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupCssRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("[.#]?[A-Za-z_-][A-Za-z0-9_-]*\\s*(?=\\{)");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("[A-Za-z-]+(?=\\s*:)");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern =
      QRegularExpression("\\b[0-9]+\\.?[0-9]*(px|em|rem|%|vh|vw|s|ms)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("#[0-9a-fA-F]{3,8}\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("@[A-Za-z-]+");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupRustRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  QStringList kw = {
      "\\bas\\b",       "\\basync\\b",    "\\bawait\\b",    "\\bbreak\\b",
      "\\bconst\\b",    "\\bcontinue\\b", "\\bcrate\\b",    "\\bdyn\\b",
      "\\belse\\b",     "\\benum\\b",     "\\bextern\\b",   "\\bfalse\\b",
      "\\bfn\\b",       "\\bfor\\b",      "\\bif\\b",       "\\bimpl\\b",
      "\\bin\\b",       "\\blet\\b",      "\\bloop\\b",     "\\bmatch\\b",
      "\\bmod\\b",      "\\bmove\\b",     "\\bmut\\b",      "\\bpub\\b",
      "\\bref\\b",      "\\breturn\\b",   "\\bself\\b",     "\\bSelf\\b",
      "\\bstatic\\b",   "\\bstruct\\b",   "\\bsuper\\b",    "\\btrait\\b",
      "\\btrue\\b",     "\\btype\\b",     "\\bunion\\b",    "\\bunsafe\\b",
      "\\buse\\b",      "\\bwhere\\b",    "\\bwhile\\b",    "\\babstract\\b",
      "\\bbecome\\b",   "\\bbox\\b",      "\\bdo\\b",       "\\bfinal\\b",
      "\\boverride\\b", "\\bpriv\\b",     "\\bvirtual\\b",  "\\byield\\b",
      "\\btry\\b"};
  for (const QString &p : kw) {
    rule.pattern = QRegularExpression(p);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }
  rule.pattern = QRegularExpression("\".*?\"|'.'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  functionFormat.setFontItalic(true);
  rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("//[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
  classFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("[A-Z][A-Za-z0-9]+");
  rule.format = classFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupGoRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  QStringList kw = {
      "\\bbreak\\b",    "\\bcase\\b",       "\\bchan\\b",      "\\bconst\\b",
      "\\bcontinue\\b", "\\bdefault\\b",    "\\bdefer\\b",     "\\belse\\b",
      "\\bfallthrough\\b", "\\bfor\\b",      "\\bfunc\\b",      "\\bgo\\b",
      "\\bgoto\\b",     "\\bif\\b",         "\\bimport\\b",    "\\binterface\\b",
      "\\bmap\\b",      "\\bpackage\\b",    "\\brange\\b",     "\\breturn\\b",
      "\\bselect\\b",   "\\bstruct\\b",     "\\bswitch\\b",    "\\btype\\b",
      "\\bvar\\b",      "\\bnil\\b",        "\\btrue\\b",      "\\bfalse\\b"};
  for (const QString &p : kw) {
    rule.pattern = QRegularExpression(p);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  // Go built-in types and functions
  QTextCharFormat builtinFormat;
  builtinFormat.setForeground(functionFormat.foreground());
  builtinFormat.setFontItalic(true);
  QStringList builtins = {
      "\\bappend\\b", "\\bcap\\b",   "\\bclose\\b",  "\\bcomplex\\b",
      "\\bcopy\\b",   "\\bdelete\\b", "\\bimag\\b",   "\\blen\\b",
      "\\bmake\\b",   "\\bnew\\b",    "\\bpanic\\b",  "\\bprint\\b",
      "\\bprintln\\b", "\\breal\\b",   "\\brecover\\b", "\\bbool\\b",
      "\\bbyte\\b",   "\\bcomplex64\\b", "\\bcomplex128\\b", "\\berror\\b",
      "\\bfloat32\\b", "\\bfloat64\\b", "\\bint\\b",    "\\bint8\\b",
      "\\bint16\\b",  "\\bint32\\b",  "\\bint64\\b",  "\\brune\\b",
      "\\bstring\\b", "\\buint\\b",   "\\buint8\\b",  "\\buint16\\b",
      "\\buint32\\b", "\\buint64\\b", "\\buintptr\\b"};
  for (const QString &p : builtins) {
    rule.pattern = QRegularExpression(p);
    rule.format = builtinFormat;
    highlightingRules.append(rule);
  }

  rule.pattern = QRegularExpression("\".*?\"|`[^`]*`|'.*?'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  functionFormat.setFontItalic(true);
  rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  rule.format = functionFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("//[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupJsonRules() {
  HighlightingRule rule;
  rule.pattern = QRegularExpression("\"[^\"]*\"\\s*(?=:)");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression(":\\s*\"[^\"]*\"");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\btrue\\b|\\bfalse\\b|\\bnull\\b");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupYamlRules() {
  HighlightingRule rule;
  keywordFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("^[A-Za-z_][A-Za-z0-9_-]*(?=\\s*:)");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression(
      "\\btrue\\b|\\bfalse\\b|\\bnull\\b|\\byes\\b|\\bno\\b");
  rule.format = numberFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("#[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("^\\s*-\\s");
  rule.format = functionFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::setupMarkdownRules() {
  HighlightingRule rule;
  headingFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("^#{1,6}\\s.*$");
  rule.format = headingFormat;
  highlightingRules.append(rule);
  boldFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*|__[^_]+__");
  rule.format = boldFormat;
  highlightingRules.append(rule);
  QTextCharFormat italicFmt;
  italicFmt.setFontItalic(true);
  rule.pattern = QRegularExpression("\\*[^*]+\\*|_[^_]+_");
  rule.format = italicFmt;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("`[^`]+`");
  rule.format = stringFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("```[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("\\[.*?\\]\\(.*?\\)");
  rule.format = linkFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("^\\s*[-*+]\\s");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("^\\s*\\d+\\.\\s");
  rule.format = keywordFormat;
  highlightingRules.append(rule);
  rule.pattern = QRegularExpression("^>\\s.*$");
  rule.format = commentFormat;
  highlightingRules.append(rule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
  if (text.isEmpty()) {
    setCurrentBlockState(0);
    return;
  }
  setCurrentBlockState(0);
  int startIndex = 0;
  if (currentLanguage == Language::CPP ||
      currentLanguage == Language::JavaScript ||
      currentLanguage == Language::Rust || currentLanguage == Language::Go ||
      currentLanguage == Language::CSS) {
    if (previousBlockState() != 1)
      startIndex = text.indexOf(multiLineCommentStart);
    while (startIndex >= 0) {
      int endIndex = text.indexOf(multiLineCommentEnd, startIndex);
      int commentLength;
      if (endIndex == -1) {
        setCurrentBlockState(1);
        commentLength = text.length() - startIndex;
      } else {
        commentLength = endIndex - startIndex + 2;
      }
      setFormat(startIndex, commentLength, commentFormat);
      startIndex =
          text.indexOf(multiLineCommentStart, startIndex + commentLength);
    }
  }
  if (previousBlockState() != 1) {
    for (const HighlightingRule &rule : highlightingRules) {
      QRegularExpressionMatchIterator matchIterator =
          rule.pattern.globalMatch(text);
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
// AnimationWidget Implementation
// ============================================================
AnimationWidget::AnimationWidget(QWidget *parent)
    : QWidget(parent), currentType(None), timerId(0), frame(0) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent;");
    initMatrix();
    initParticles();
}

void AnimationWidget::setAnimationType(AnimationType type) {
    if (timerId) {
        killTimer(timerId);
        timerId = 0;
    }
    
    currentType = type;
    frame = 0;
    
    if (type != None) {
        timerId = startTimer(50); // 20 FPS
    }
    
    if (type == Matrix) initMatrix();
    else if (type == Particles || type == Waves || type == Pulse || type == Rain || type == Snow) initParticles();
    else if (type == Starfield) initStarfield();
    else if (type == Fire) initFire();
    
    update();
}

void AnimationWidget::cycleAnimation() {
    int next = (static_cast<int>(currentType) + 1) % 9;
    setAnimationType(static_cast<AnimationType>(next));
}

void AnimationWidget::initMatrix() {
    matrixColumns.clear();
    for (int i = 0; i < 30; i++) {
        MatrixColumn col;
        col.x = QRandomGenerator::global()->bounded(width() + 100);
        col.y = -(QRandomGenerator::global()->bounded(500));
        col.speed = 2 + QRandomGenerator::global()->bounded(5);
        col.text = QString("01").at(QRandomGenerator::global()->bounded(2));
        matrixColumns.append(col);
    }
}

void AnimationWidget::initParticles() {
    particles.clear();
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width());
        p.y = QRandomGenerator::global()->bounded(height());
        p.vx = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
        p.vy = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
        p.life = 255;
        p.size = 2.0f;
        particles.append(p);
    }
}

void AnimationWidget::initStarfield() {
    particles.clear();
    for (int i = 0; i < 100; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width()) - width() / 2;
        p.y = QRandomGenerator::global()->bounded(height()) - height() / 2;
        p.vx = p.x / 50.0f;
        p.vy = p.y / 50.0f;
        p.life = 255;
        p.size = 0.5f + (QRandomGenerator::global()->bounded(100) / 50.0f);
        particles.append(p);
    }
}

void AnimationWidget::initFire() {
    particles.clear();
    for (int i = 0; i < 60; i++) {
        Particle p;
        p.x = QRandomGenerator::global()->bounded(width());
        p.y = height() + QRandomGenerator::global()->bounded(50);
        p.vx = (QRandomGenerator::global()->bounded(40) - 20) / 10.0f;
        p.vy = -(2.0f + QRandomGenerator::global()->bounded(40) / 10.0f);
        p.life = 100 + QRandomGenerator::global()->bounded(155);
        p.size = 5.0f + QRandomGenerator::global()->bounded(10);
        particles.append(p);
    }
}

void AnimationWidget::timerEvent(QTimerEvent *) {
    frame++;
    
    // Update matrix
    for (auto &col : matrixColumns) {
        col.y += col.speed;
        if (col.y > height()) {
            col.y = -20;
            col.x = QRandomGenerator::global()->bounded(width());
        }
    }
    
    // Update particles
    for (auto &p : particles) {
        if (currentType == Starfield) {
            p.x += p.vx;
            p.y += p.vy;
            p.vx *= 1.05f;
            p.vy *= 1.05f;
            if (qAbs(p.x) > width() / 2 || qAbs(p.y) > height() / 2) {
                p.x = QRandomGenerator::global()->bounded(10) - 5;
                p.y = QRandomGenerator::global()->bounded(10) - 5;
                p.vx = p.x / 2.0f;
                p.vy = p.y / 2.0f;
            }
        } else if (currentType == Rain) {
            p.y += 15.0f;
            if (p.y > height()) {
                p.y = -20;
                p.x = QRandomGenerator::global()->bounded(width());
            }
        } else if (currentType == Snow) {
            p.y += 2.0f;
            p.x += qSin(frame * 0.1f + p.life) * 1.5f;
            if (p.y > height()) {
                p.y = -10;
                p.x = QRandomGenerator::global()->bounded(width());
            }
        } else if (currentType == Fire) {
            p.x += p.vx;
            p.y += p.vy;
            p.life -= 5;
            if (p.life <= 0) {
                p.x = QRandomGenerator::global()->bounded(width());
                p.y = height() + 10;
                p.vx = (QRandomGenerator::global()->bounded(40) - 20) / 10.0f;
                p.vy = -(2.0f + QRandomGenerator::global()->bounded(40) / 10.0f);
                p.life = 255;
            }
        } else {
            p.x += p.vx;
            p.y += p.vy;
            p.life--;
            
            if (p.life <= 0 || p.x < 0 || p.x > width() || p.y < 0 || p.y > height()) {
                p.x = QRandomGenerator::global()->bounded(width());
                p.y = QRandomGenerator::global()->bounded(height());
                p.vx = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
                p.vy = (QRandomGenerator::global()->bounded(100) - 50) / 50.0f;
                p.life = 255;
            }
        }
    }
    
    update();
}

void AnimationWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    switch (currentType) {
        case Matrix:
            drawMatrix(painter);
            break;
        case Particles:
            drawParticles(painter);
            break;
        case Waves:
            drawWaves(painter);
            break;
        case Pulse:
            drawPulse(painter);
            break;
        case Starfield:
            drawStarfield(painter);
            break;
        case Rain:
            drawRain(painter);
            break;
        case Snow:
            drawSnow(painter);
            break;
        case Fire:
            drawFire(painter);
            break;
        default:
            break;
    }
}

void AnimationWidget::drawStarfield(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    for (const auto &p : particles) {
        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(p.x + width()/2, p.y + height()/2), p.size, p.size);
    }
}

void AnimationWidget::drawRain(QPainter &painter) {
    painter.setPen(QPen(QColor(100, 149, 237, 150), 2));
    for (const auto &p : particles) {
        painter.drawLine(QPointF(p.x, p.y), QPointF(p.x, p.y + 10));
    }
}

void AnimationWidget::drawSnow(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 200));
    for (const auto &p : particles) {
        painter.drawEllipse(QPointF(p.x, p.y), 3, 3);
    }
}

void AnimationWidget::drawFire(QPainter &painter) {
    for (const auto &p : particles) {
        int r = 255;
        int g = qMax(0, 255 - (255 - p.life) * 2);
        int b = 0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(r, g, b, p.life));
        painter.drawEllipse(QPointF(p.x, p.y), p.size * p.life / 255.0f, p.size * p.life / 255.0f);
    }
}

void AnimationWidget::drawMatrix(QPainter &painter) {
    painter.setFont(QFont("Consolas", 12));
    for (const auto &col : matrixColumns) {
        int alpha = 255;
        for (int i = 0; i < 10; i++) {
            painter.setPen(QColor(0, 255, 0, alpha));
            painter.drawText(col.x, col.y - i * 20, col.text);
            alpha = qMax(0, alpha - 30);
        }
    }
}

void AnimationWidget::drawParticles(QPainter &painter) {
    for (const auto &p : particles) {
        int alpha = qMin(255, p.life);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(100, 149, 237, alpha));
        painter.drawEllipse(QPointF(p.x, p.y), 2, 2);
    }
}

void AnimationWidget::drawWaves(QPainter &painter) {
    painter.setPen(QPen(QColor(0, 120, 215, 100), 2));
    
    for (int wave = 0; wave < 3; wave++) {
        QPainterPath path;
        bool first = true;
        for (int x = 0; x < width(); x += 5) {
            float y = height() / 2 + 50 * qSin((x + frame * 2 + wave * 100) * 0.02);
            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    }
}

void AnimationWidget::drawPulse(QPainter &painter) {
    int centerX = width() / 2;
    int centerY = height() / 2;
    
    for (int i = 0; i < 5; i++) {
        int radius = ((frame + i * 20) % 200);
        int alpha = 255 - (radius * 255 / 200);
        painter.setPen(QPen(QColor(100, 149, 237, alpha), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPoint(centerX, centerY), radius, radius);
    }
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
  tabWidget = new QTabWidget();
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
  connect(fileTree, &QTreeView::doubleClicked, this,
          &TextEditor::onFileTreeDoubleClicked);
  fileTreeContainer->addWidget(fileTree);

  // Set default state to Empty (0)
  fileTreeContainer->setCurrentIndex(0);

  fileTreeDock->setWidget(fileTreeContainer);
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
  duplicateLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  connect(duplicateLineAct, &QAction::triggered, this, [this]() {
    if (currentEditor())
      currentEditor()->duplicateLine();
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

  customizeColorsAct = new QAction("Customize Colors...", this);
  connect(customizeColorsAct, &QAction::triggered, this,
          &TextEditor::customizeColors);

  aboutAct = new QAction("&About", this);
  connect(aboutAct, &QAction::triggered, this, &TextEditor::showAbout);
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
  viewMenu->addSeparator();
  viewMenu->addAction(wordWrapAct);
  viewMenu->addSeparator();
  viewMenu->addAction(themeAct);
  viewMenu->addAction(customizeColorsAct);

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

void TextEditor::createStatusBar() {
  statusLabel = new QLabel("Line 1, Col 1");
  languageLabel = new QLabel("Plain Text");
  languageLabel->setStyleSheet(
      "padding: 2px 12px; color: #ffffff; background-color: transparent;");
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
  int index = tabWidget->addTab(editor, "Untitled");
  tabWidget->setCurrentIndex(index);
  editor->setFocus();
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

void TextEditor::toggleTerminal() {
  if (terminalWidget->isVisible()) {
    terminalWidget->hide();
  } else {
    terminalWidget->show();
    // Ensure proper sizing when showing terminal
    QList<int> sizes = verticalSplitter->sizes();
    int total = sizes[0] + sizes[1];
    if (total > 0) {
      sizes[0] = total * 0.7;
      sizes[1] = total * 0.3;
      verticalSplitter->setSizes(sizes);
    }
  }
  terminalAct->setChecked(terminalWidget->isVisible());
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

    QFont font("Consolas", fontSize);
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
                             "JSON",       "YAML", "Markdown"};
    languageLabel->setText(langNames[static_cast<int>(lang)]);
  }

  statusBar()->showMessage("File loaded", 2000);
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
      tabWidget2 = new QTabWidget();
      tabWidget2->setTabsClosable(true);
      tabWidget2->setMovable(true);
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
