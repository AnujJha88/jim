#include "texteditor.h"
#include "vimmode.h"
#include "linenumberarea.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QTextBlock>
#include <QFileInfo>
#include <QTimer>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QToolTip>
#include <QUrl>
#include <cmath>
#include <algorithm>

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

  // Overlays (sized in resizeEvent)
  crtOverlay = new CRTOverlay(viewport());
  crtOverlay->hide();
  laserOverlay = new LaserParticleOverlay(viewport());
  laserOverlay->hide();

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
  viewport()->setMouseTracking(true);

  // Ghost replay & graveyard tracking
  connect(document(), &QTextDocument::contentsChange, this,
          &CodeEditor::onDocumentContentsChange);
  startRecordingGhost();

  // Vim mode
  vimMode = new VimMode(this);
  connect(vimMode, &VimMode::modeChanged, this, &CodeEditor::vimModeChanged);
  connect(vimMode, &VimMode::deleteLineRequested, this, &CodeEditor::deleteLine);
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
  // Keep overlays covering the full viewport
  crtOverlay->setGeometry(viewport()->rect());
  laserOverlay->setGeometry(viewport()->rect());
  crtOverlay->raise();
  laserOverlay->raise();

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

void CodeEditor::paintEvent(QPaintEvent *e) {
  QPlainTextEdit::paintEvent(e);

  QPainter painter(viewport());
  painter.setRenderHint(QPainter::Antialiasing);

  QTextBlock block = firstVisibleBlock();
  QPointF offset = contentOffset();
  QRegularExpression hexRegex("#([0-9A-Fa-f]{3}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})\\b");

  while (block.isValid()) {
    QRectF blockRect = blockBoundingGeometry(block).translated(offset);
    if (blockRect.top() > e->rect().bottom()) break;

    if (block.isVisible() && blockRect.bottom() >= e->rect().top()) {
      QString text = block.text();
      QRegularExpressionMatchIterator i = hexRegex.globalMatch(text);
      while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int startPos = match.capturedStart();
        QColor color(match.captured(0));

        if (color.isValid()) {
          int swatchSize = 10;
          QTextCursor endCursor(block);
          endCursor.setPosition(block.position() + startPos + match.capturedLength());
          QRect endRect = cursorRect(endCursor);
          QRect square(endRect.right() + 4, endRect.top() + (endRect.height() - swatchSize)/2, swatchSize, swatchSize);

          painter.setPen(Qt::NoPen);
          painter.setBrush(color);
          painter.drawRoundedRect(square, 2, 2);
          painter.setPen(QColor(100, 100, 100, 150));
          painter.drawRoundedRect(square, 2, 2);
        }
      }
    }
    block = block.next();
  }

  // ── Bracket Pair Colorization ─────────────────────────────────────────────
  {
    static const QColor bColors[3] = {
        QColor(229, 192, 123),  // warm gold   (depth 0)
        QColor(198, 120, 221),  // muted purple (depth 1)
        QColor( 97, 175, 239),  // soft blue    (depth 2)
    };
    const QString opens  = "({[";
    const QString closes = ")}]";

    // Scan backward up to 150 blocks to determine starting bracket depth
    int depth = 0;
    {
        QTextBlock b = firstVisibleBlock().previous();
        int scanned = 0;
        while (b.isValid() && scanned < 150) {
            const QString &t = b.text();
            for (int k = t.size() - 1; k >= 0; --k) {
                if (opens.contains(t[k]))  --depth;
                if (closes.contains(t[k])) ++depth;
            }
            b = b.previous();
            ++scanned;
        }
        if (depth < 0) depth = 0;
    }

    painter.setFont(font());
    QFontMetrics fm = fontMetrics();
    QTextBlock blk = firstVisibleBlock();
    QPointF off = contentOffset();
    while (blk.isValid()) {
        QRectF br = blockBoundingGeometry(blk).translated(off);
        if (br.top() > e->rect().bottom()) break;
        if (blk.isVisible() && br.bottom() >= e->rect().top()) {
            const QString &txt = blk.text();
            for (int k = 0; k < txt.size(); ++k) {
                QChar ch = txt[k];
                bool isOpen  = opens.contains(ch);
                bool isClose = closes.contains(ch);
                if (!isOpen && !isClose) continue;

                if (isClose && depth > 0) --depth;
                QColor col = bColors[depth % 3];
                if (isOpen) ++depth;

                QTextCursor tc(blk);
                tc.setPosition(blk.position() + k);
                QRect cr = cursorRect(tc);
                int cw = fm.horizontalAdvance(ch);
                painter.setPen(col);
                painter.drawText(cr.left(), cr.top(), cw, cr.height(),
                                 Qt::AlignLeft | Qt::AlignVCenter, QString(ch));
            }
        }
        blk = blk.next();
    }
  }

  if (!extraCursors.isEmpty()) {
      QColor cColor = currentTheme.foreground.isValid() ? currentTheme.foreground : Qt::black;
      painter.setPen(cColor);
      for (const QTextCursor &c : extraCursors) {
          QRect rect = cursorRect(c);
          painter.drawLine(rect.topLeft(), rect.bottomLeft());
          painter.drawLine(rect.topLeft() + QPoint(1,0), rect.bottomLeft() + QPoint(1,0));
      }
  }

  // Focus Fade: dim all lines except the current cursor block
  if (focusFadeEnabled) {
      int cursorBlockNum = textCursor().blockNumber();
      QTextBlock blk = firstVisibleBlock();
      QPointF off = contentOffset();
      painter.save();
      while (blk.isValid()) {
          QRectF br = blockBoundingGeometry(blk).translated(off);
          if (br.top() > viewport()->height()) break;
          if (blk.blockNumber() != cursorBlockNum && blk.isVisible()) {
              painter.fillRect(br, QColor(0, 0, 0, 110));
          }
          blk = blk.next();
      }
      painter.restore();
  }
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

  // Pre-compute max heat for normalisation
  int maxHeat = 1;
  for (auto v : lineEditHeat) maxHeat = qMax(maxHeat, v);
  int stripX = lineNumberArea->width() - 3;

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      if (blockNumber == currentLine)
        painter.setPen(QColor(255, 255, 255));
      else
        painter.setPen(fgColor);
      painter.drawText(0, top, width, lineHeight, Qt::AlignRight,
                       QString::number(blockNumber + 1));

      // Edit heatmap strip (3 px, right edge of gutter)
      int heat = lineEditHeat.value(blockNumber, 0);
      if (heat > 0) {
          float ratio = qMin(1.0f, static_cast<float>(heat) / maxHeat);
          int r = static_cast<int>(30  + ratio * 225);
          int g = static_cast<int>(120 - ratio * 80);
          int b = static_cast<int>(200 - ratio * 180);
          painter.fillRect(stripX, top, 3, bottom - top, QColor(r, g, b, 180));
      }
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++blockNumber;
  }
}

// ============================================================
// v1.9 Feature Implementations
// ============================================================
void CodeEditor::setFocusFadeEnabled(bool enabled) {
    focusFadeEnabled = enabled;
    viewport()->update();
}

void CodeEditor::setImagePreviewEnabled(bool enabled) {
    imagePreviewEnabled = enabled;
    if (!enabled)
        QToolTip::hideText();
}

void CodeEditor::mouseMoveEvent(QMouseEvent *event) {
    if (imagePreviewEnabled) {
        QTextCursor cur = cursorForPosition(event->pos());
        QString line = cur.block().text();
        int col = cur.positionInBlock();

        // Match string literals containing image file extensions
        static QRegularExpression imgRe(
            R"([\"'`]([^\"'`\n]*\.(png|jpg|jpeg|gif|bmp|svg|webp|ico))[\"'`]?)",
            QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatchIterator it = imgRe.globalMatch(line);
        bool shown = false;
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            if (col >= m.capturedStart() && col <= m.capturedEnd()) {
                QString imgPath = m.captured(1);
                // Resolve relative to current file's directory
                if (!QFileInfo(imgPath).isAbsolute() && !fileName.isEmpty())
                    imgPath = QFileInfo(fileName).absoluteDir().filePath(imgPath);
                if (QFileInfo::exists(imgPath)) {
                    // Build a rich-text tooltip with a thumbnail
                    QString url = QUrl::fromLocalFile(imgPath).toString();
                    QString tip = QString("<img src='%1' style='max-width:240px;max-height:160px;'>").arg(url);
                    QToolTip::showText(event->globalPosition().toPoint(), tip, this);
                    shown = true;
                }
                break;
            }
        }
        if (!shown)
            QToolTip::hideText();
    }
    QPlainTextEdit::mouseMoveEvent(event);
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

int CodeEditor::indentLevel(const QTextBlock &block) const {
    int spaces = 0;
    for (QChar c : block.text()) {
        if (c == ' ')       spaces++;
        else if (c == '\t') spaces += 4;
        else break;
    }
    return spaces;
}

int CodeEditor::findIndentEnd(const QTextBlock &block) const {
    int base = indentLevel(block);
    int last = block.blockNumber();
    QTextBlock b = block.next();
    while (b.isValid()) {
        if (!b.text().trimmed().isEmpty()) {
            if (indentLevel(b) <= base) break;
            last = b.blockNumber();
        }
        b = b.next();
    }
    return last;
}

void CodeEditor::toggleFoldAt(int blockNumber) {
  QTextBlock block = document()->findBlockByNumber(blockNumber);
  if (!block.isValid() || !isFoldable(block))
    return;

  bool fold = !isFolded(block);
  // Brace blocks use brace matching; indent blocks (Python/YAML/etc) use indent end
  int endBlock = block.text().trimmed().endsWith('{')
      ? findMatchingBrace(block)
      : findIndentEnd(block);

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

void CodeEditor::addExtraCursor(const QTextCursor &c) {
    extraCursors.append(c);
    viewport()->update();
}

void CodeEditor::clearExtraCursors() {
    extraCursors.clear();
    viewport()->update();
}

void CodeEditor::selectNextOccurrence() {
    QTextCursor mainC = textCursor();
    if (!mainC.hasSelection()) {
        mainC.select(QTextCursor::WordUnderCursor);
        setTextCursor(mainC);
        return;
    }
    QString text = mainC.selectedText();
    QTextCursor searchStart = extraCursors.isEmpty() ? mainC : extraCursors.last();
    QTextCursor nextC = document()->find(text, searchStart);
    if (!nextC.isNull()) {
        addExtraCursor(nextC);
    }
}

void CodeEditor::mousePressEvent(QMouseEvent *event) {
    if (event->modifiers() & Qt::AltModifier) {
        QTextCursor c = cursorForPosition(event->pos());
        addExtraCursor(c);
        return;
    }
    clearExtraCursors();
    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent *event) {
  // Always emit keyPressed for heatmap tracking (before any returns)
  emit keyPressed(event->key(), event->text());

  // Vim mode — intercepts keys when enabled
  if (vimMode && vimMode->handleKey(event, this))
      return;

  if (event->key() == Qt::Key_Escape && !extraCursors.isEmpty()) {
      clearExtraCursors();
      return;
  }

  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    emit characterTyped();
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

  emit characterTyped();

  if (!extraCursors.isEmpty()) {
      QTextCursor mainCursor = textCursor();
      mainCursor.beginEditBlock();
      int key = event->key();
      for (int i=0; i<extraCursors.size(); i++) {
          QTextCursor &c = extraCursors[i];
          if (key == Qt::Key_Backspace) c.deletePreviousChar();
          else if (key == Qt::Key_Delete) c.deleteChar();
          else if (text[0].isPrint()) c.insertText(text);
      }
      mainCursor.endEditBlock();
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

  // Graveyard: catch large selections being deleted/cut
  bool isCut    = (ch == QChar(24));  // Ctrl+X = ASCII 24
  bool isDel    = (event->key() == Qt::Key_Delete    && cursor.hasSelection());
  bool isBacksp = (event->key() == Qt::Key_Backspace && cursor.hasSelection());
  if (isCut || isDel || isBacksp) {
      if (cursor.hasSelection()) {
          QString sel = cursor.selectedText();
          int newlineCount = sel.count('\n') + sel.count(QChar::ParagraphSeparator);
          if (newlineCount >= 3) {
              emit codeBlockDeleted(
                  sel.replace(QChar::ParagraphSeparator, '\n'),
                  fileName.isEmpty() ? "untitled" : QFileInfo(fileName).fileName());
          }
      }
  }

  // Auto-close HTML/XML tags
  if (ch == '/') {
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
    if (c.selectedText() == "<") {
        QString textBefore = document()->toPlainText().left(textCursor().position() - 1);
        QStringList openTags;
        QRegularExpression tagRegex("<(/?)(\\w+)[^>]*>");
        QRegularExpressionMatchIterator i = tagRegex.globalMatch(textBefore);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            bool isClosing = !match.captured(1).isEmpty();
            QString tagName = match.captured(2);
            if (isClosing) {
                if (!openTags.isEmpty() && openTags.last() == tagName) {
                    openTags.removeLast();
                }
            } else {
                if (!match.captured(0).endsWith("/>") &&
                    tagName.toLower() != "br" && tagName.toLower() != "hr" &&
                    tagName.toLower() != "img" && tagName.toLower() != "meta" &&
                    tagName.toLower() != "link" && tagName.toLower() != "input") {
                    openTags.append(tagName);
                }
            }
        }
        if (!openTags.isEmpty()) {
            QString tagToClose = openTags.last();
            QPlainTextEdit::keyPressEvent(event);
            QTextCursor insertC = textCursor();
            insertC.insertText(tagToClose + ">");
            return;
        }
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

void CodeEditor::setCRTEnabled(bool enabled) {
    crtOverlay->setEnabled(enabled);
    if (enabled) {
        crtOverlay->setGeometry(viewport()->rect());
        crtOverlay->raise();
    }
}

void CodeEditor::setVimEnabled(bool enabled) {
    if (vimMode) vimMode->setEnabled(enabled);
}

bool CodeEditor::isVimEnabled() const {
    return vimMode && vimMode->isEnabled();
}

void CodeEditor::setAmbientBackground(QColor tint) {
    QColor bg = currentTheme.background.isValid() ? currentTheme.background : QColor(30, 30, 30);
    if (tint.isValid() && tint.alpha() > 0) {
        float a = tint.alphaF();
        bg = QColor(
            qBound(0, static_cast<int>(bg.red()   * (1 - a) + tint.red()   * a), 255),
            qBound(0, static_cast<int>(bg.green() * (1 - a) + tint.green() * a), 255),
            qBound(0, static_cast<int>(bg.blue()  * (1 - a) + tint.blue()  * a), 255));
    }
    // Update stylesheet (takes precedence over palette in Qt)
    QString style = QString("QPlainTextEdit { background-color: %1; color: %2; "
                            "selection-background-color: %3; border: none; }")
                        .arg(bg.name())
                        .arg(currentTheme.foreground.name())
                        .arg(currentTheme.selection.name());
    setStyleSheet(style);
    QPalette p = palette();
    p.setColor(QPalette::Base, bg);
    setPalette(p);
    viewport()->update();
}

void CodeEditor::triggerLaserEffect() {
    QTextCursor c = textCursor();
    QRect lineRect = cursorRect(c);
    laserOverlay->setGeometry(viewport()->rect());
    laserOverlay->show();
    laserOverlay->raise();
    laserOverlay->spawnSlash(lineRect.center().y());
}

// Ghost Replay implementation
void CodeEditor::startRecordingGhost() {
    ghostLog.clear();
    sessionStartTimeMs = QDateTime::currentMSecsSinceEpoch();
    ghostIsRecording = true;
}

void CodeEditor::logGhostEvent(int pos, int charsRemoved, const QString &textAdded) {
    GhostEvent event;
    event.timestampMs = QDateTime::currentMSecsSinceEpoch() - sessionStartTimeMs;
    event.position = pos;
    event.charsRemoved = charsRemoved;
    event.textAdded = textAdded;
    ghostLog.append(event);
}

void CodeEditor::onDocumentContentsChange(int position, int charsRemoved, int charsAdded) {
    if (!ghostIsRecording) return;

    if (sessionStartTimeMs == 0) {
        startRecordingGhost();
    }

    QString addedStr;
    if (charsAdded > 0) {
        QTextCursor c(document());
        c.setPosition(position);
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, charsAdded);
        addedStr = c.selectedText();
    }

    logGhostEvent(position, charsRemoved, addedStr);
}

void CodeEditor::miniMapPaintEvent(QPaintEvent *event) {
  QPainter painter(miniMap);
  painter.fillRect(event->rect(), QColor(10, 14, 10)); // darker bg for waterfall

  // ── Data Waterfall: Matrix-style digital rain ─────────────────────────────
  if (minimapWaterfallEnabled) {
      int cols = miniMap->width() / 6; // one raindrop column per 6px
      // Grow / init the drops vector as needed
      while (waterfallDrops.size() < cols)
          waterfallDrops.append(QRandomGenerator::global()->bounded(miniMap->height()));
      waterfallFrame++;
      const QChar digits[] = {
          '0','1','2','3','4','5','6','7','8','9',
          'A','B','C','D','E','F','X','Z','<','>','{','}'
      };
      QFont wfFont("Consolas", 6);
      painter.setFont(wfFont);
      for (int col = 0; col < cols && col < waterfallDrops.size(); ++col) {
          int x = col * 6;
          int dropY = waterfallDrops[col];
          // Trail
          for (int trail = 0; trail < 10; ++trail) {
              int ty = dropY - trail * 8;
              if (ty < 0) continue;
              int alpha = 180 - trail * 18;
              if (alpha <= 0) break;
              QColor c = (trail == 0) ? QColor(180, 255, 180, alpha)
                                      : QColor(0, 160 - trail * 12, 0, alpha);
              painter.setPen(c);
              int charIdx = (waterfallFrame + col * 7 + trail * 3) %
                            static_cast<int>(sizeof(digits) / sizeof(digits[0]));
              painter.drawText(x, ty, QString(digits[charIdx]));
          }
          // Advance drop
          if ((waterfallFrame + col * 3) % 2 == 0) {
              waterfallDrops[col] += 8;
              if (waterfallDrops[col] > miniMap->height() + 80)
                  waterfallDrops[col] = -(QRandomGenerator::global()->bounded(80));
          }
      }
  }

  int totalLines = document()->blockCount();
  if (totalLines == 0) return;
  int visibleLines = height() / fontMetrics().height();
  int startLine = (event->rect().top() * totalLines) / miniMap->height();
  int endLine = (event->rect().bottom() * totalLines) / miniMap->height() + 1;
  QTextBlock block = document()->findBlockByLineNumber(qMax(0, startLine));
  int blockNumber = block.blockNumber();
  // Code overview overlay — semi-transparent cyan lines over the waterfall
  painter.setPen(QColor(0, 255, 200, 80));
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
                   QColor(0, 200, 100, 40));
  painter.setPen(QColor(0, 255, 120, 180));
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
  // Kinetic laser effect on delete
  triggerLaserEffect();
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
