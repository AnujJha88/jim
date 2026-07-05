#ifndef JIM_MINIMAP_H
#define JIM_MINIMAP_H

#include <QWidget>

class CodeEditor;

class MiniMap : public QWidget {
    Q_OBJECT

public:
    explicit MiniMap(CodeEditor *editor);

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
