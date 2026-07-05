#ifndef JIM_BREADCRUMB_BAR_H
#define JIM_BREADCRUMB_BAR_H

#include <QWidget>

class QLabel;
class QString;

class BreadcrumbBar : public QWidget {
    Q_OBJECT
public:
    explicit BreadcrumbBar(QWidget *parent = nullptr);
    void updatePath(const QString &filePath, const QString &symbol);

private:
    QLabel *iconLabel;
    QLabel *pathLabel;
    QLabel *fileLabel;
    QLabel *symbolLabel;
};

#endif
