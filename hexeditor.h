#ifndef HEXEDITOR_H
#define HEXEDITOR_H

#include <QWidget>
#include <QScrollBar>
#include <QByteArray>
#include <QFont>

class HexEditor : public QWidget {
    Q_OBJECT

public:
    explicit HexEditor(QWidget *parent = nullptr);
    
    void setData(const QByteArray &data);
    QByteArray data() const { return m_data; }
    void clear();
    
    bool loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; }
    bool isReadOnly() const { return m_readOnly; }

    bool isModified() const { return m_modified; }
    void setModified(bool modified);
    
    void setAddressWidth(int width) { m_addressWidth = width; update(); }
    void setBytesPerLine(int bytes) { m_bytesPerLine = bytes; updateScrollBar(); update(); }

signals:
    void dataChanged();
    void modificationChanged(bool modified);
    void currentAddressChanged(qint64 address);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QByteArray m_data;
    QScrollBar *m_scrollBar;
    
    qint64 m_cursorPosition;
    qint64 m_selectionStart;
    qint64 m_selectionEnd;
    
    int m_bytesPerLine;
    int m_addressWidth;
    int m_charWidth;
    int m_charHeight;
    
    bool m_readOnly;
    bool m_modified;
    bool m_cursorInHexArea;
    bool m_nibblePosition; // false = high nibble, true = low nibble
    
    void updateScrollBar();
    void ensureCursorVisible();
    qint64 positionFromPoint(const QPoint &pos, bool &inHexArea);
    QRect hexAreaRect() const;
    QRect asciiAreaRect() const;
    int visibleLines() const;
};

#endif
