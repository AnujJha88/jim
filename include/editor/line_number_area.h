#ifndef JIM_LINE_NUMBER_AREA_H
#define JIM_LINE_NUMBER_AREA_H

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor;
};

#endif
