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
