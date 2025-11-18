#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget {
public:
    LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor;
};

class MiniMap : public QWidget {
    Q_OBJECT
    
public:
    MiniMap(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    CodeEditor *codeEditor;
    void scrollToPosition(int y);
};

#endif
