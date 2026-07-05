#ifndef KEYHEATMAP_H
#define KEYHEATMAP_H

#include <QWidget>
#include <QMap>

// KeyHeatmapOverlay — a floating, click-to-dismiss QWERTY heatmap that shows
// which keys you've pressed most during the session.  Each key is coloured
// from cold blue (rare) → teal → yellow → red (frequent).
class KeyHeatmapOverlay : public QWidget {
    Q_OBJECT
public:
    explicit KeyHeatmapOverlay(QWidget *parent = nullptr);

    // Call this whenever the user presses a key (text = event->text(), key = event->key())
    void recordKey(const QString &text, int qtKey);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override { hide(); }
    void showEvent(QShowEvent *) override;

private:
    QMap<QString, int> m_freq;
    QColor heatColor(float ratio) const;
};

#endif // KEYHEATMAP_H
