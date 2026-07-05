#ifndef JIM_TITLE_BAR_H
#define JIM_TITLE_BAR_H

#include <QPoint>
#include <QWidget>

class QLabel;
class QMouseEvent;
class QString;

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void toggleMaximized();

private:
    QLabel *titleLabel;
    QPoint dragStartPos;
};

#endif
