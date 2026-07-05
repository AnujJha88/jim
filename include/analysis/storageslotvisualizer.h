#ifndef STORAGESLOTVISUALIZER_H
#define STORAGESLOTVISUALIZER_H
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include "solidityanalyzer.h"

class StorageSlotVisualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit StorageSlotVisualizerWidget(QWidget *parent = nullptr);
    void analyzeCode(const QString &solidityCode);
    void clear();
signals:
    void jumpToVariable(const QString &varName);
protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
private:
    QVector<SolidityAnalyzer::StorageVar> m_vars;
    int m_hoveredVar = -1;  // index into m_vars
    int m_slotCount = 0;
    // Layout constants
    static constexpr int kSlotH   = 32;    // height per slot row
    static constexpr int kLabelW  = 120;   // left label width
    static constexpr int kByteW   = 18;    // width per byte column
    static constexpr int kHeaderH = 24;    // header row height
    static constexpr int kLegendH = 24;    // legend strip height
    int varIndexAtPoint(const QPoint &pt) const;
    QRect varRect(int varIdx) const;
};
#endif
