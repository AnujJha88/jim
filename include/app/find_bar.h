#ifndef JIM_FIND_BAR_H
#define JIM_FIND_BAR_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;

class FindBar : public QWidget {
    Q_OBJECT
public:
    explicit FindBar(QWidget *parent = nullptr);
    void showAndFocus(const QString &text = "");
    void setMatchCount(int current, int total);
    QString getSearchText() const;

signals:
    void findNextRequested(const QString &text);
    void findPreviousRequested(const QString &text);
    void textChanged(const QString &text);
    void closeRequested();

private:
    QLineEdit *findInput;
    QLabel *matchLabel;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QPushButton *closeBtn;
};

#endif
