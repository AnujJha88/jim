#ifndef JIM_FOLDING_AREA_H
#define JIM_FOLDING_AREA_H

#include <QWidget>

class CodeEditor;

class FoldingArea : public QWidget {
    Q_OBJECT
public:
    explicit FoldingArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodeEditor *codeEditor;
};

#endif
