#include "linenumberarea.h"
#include "texteditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMouseEvent>

LineNumberArea::LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {
}

QSize LineNumberArea::sizeHint() const {
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    codeEditor->lineNumberAreaPaintEvent(event);
}

// FoldingArea Implementation
FoldingArea::FoldingArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {
    setCursor(Qt::PointingHandCursor);
}

QSize FoldingArea::sizeHint() const {
    return QSize(codeEditor->foldingAreaWidth(), 0);
}

void FoldingArea::paintEvent(QPaintEvent *event) {
    codeEditor->foldingAreaPaintEvent(event);
}

void FoldingArea::mousePressEvent(QMouseEvent *event) {
    // Determine which block was clicked
    QTextBlock block = codeEditor->firstVisibleBlock();
    int top = qRound(codeEditor->blockBoundingGeometry(block).translated(codeEditor->contentOffset()).top());
    int bottom = top + qRound(codeEditor->blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->pos().y()) {
        if (block.isVisible() && bottom >= event->pos().y()) {
            if (codeEditor->isFoldable(block)) {
                codeEditor->toggleFoldAt(block.blockNumber());
                codeEditor->viewport()->update();
                update();
            }
            break;
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(codeEditor->blockBoundingRect(block).height());
    }
}

// MiniMap Implementation
MiniMap::MiniMap(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {
    setMouseTracking(true);
}

QSize MiniMap::sizeHint() const {
    return QSize(codeEditor->miniMapWidth(), 0);
}

void MiniMap::paintEvent(QPaintEvent *event) {
    codeEditor->miniMapPaintEvent(event);
}

void MiniMap::mousePressEvent(QMouseEvent *event) {
    scrollToPosition(event->pos().y());
}

void MiniMap::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        scrollToPosition(event->pos().y());
    }
}

void MiniMap::scrollToPosition(int y) {
    int totalLines = codeEditor->document()->blockCount();
    int targetLine = (y * totalLines) / height();
    
    QTextCursor cursor(codeEditor->document()->findBlockByLineNumber(targetLine));
    codeEditor->setTextCursor(cursor);
    codeEditor->centerCursor();
}
