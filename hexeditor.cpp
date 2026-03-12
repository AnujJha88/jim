#include "hexeditor.h"
#include <QPainter>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFile>
#include <QFontMetrics>
#include <QApplication>

HexEditor::HexEditor(QWidget *parent)
    : QWidget(parent)
    , m_cursorPosition(0)
    , m_selectionStart(-1)
    , m_selectionEnd(-1)
    , m_bytesPerLine(16)
    , m_addressWidth(8)
    , m_readOnly(false)
    , m_modified(false)
    , m_cursorInHexArea(true)
    , m_nibblePosition(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setFont(QFont("Courier", 10));
    
    QFontMetrics fm(font());
    m_charWidth = fm.horizontalAdvance('0');
    m_charHeight = fm.height();
    
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this]() { update(); });
    
    updateScrollBar();
}

void HexEditor::setData(const QByteArray &data) {
    m_data = data;
    m_cursorPosition = 0;
    m_selectionStart = -1;
    m_selectionEnd = -1;
    setModified(false);
    updateScrollBar();
    update();
    emit dataChanged();
}

void HexEditor::clear() {
    m_data.clear();
    m_cursorPosition = 0;
    m_selectionStart = -1;
    m_selectionEnd = -1;
    updateScrollBar();
    update();
    emit dataChanged();
}

bool HexEditor::loadFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    setData(file.readAll());
    return true;
}

bool HexEditor::saveFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(m_data);
    setModified(false);
    return true;
}

void HexEditor::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        emit modificationChanged(m_modified);
    }
}

void HexEditor::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));
    
    if (m_data.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }
    
    int firstLine = m_scrollBar->value();
    int lastLine = firstLine + visibleLines();
    
    int y = 5;
    qint64 offset = firstLine * m_bytesPerLine;
    
    for (int line = firstLine; line <= lastLine && offset < m_data.size(); ++line) {
        // Draw address
        painter.setPen(QColor(100, 149, 237));
        QString address = QString("%1").arg(offset, m_addressWidth, 16, QChar('0')).toUpper();
        painter.drawText(5, y + m_charHeight, address);
        
        int hexX = 5 + (m_addressWidth + 2) * m_charWidth;
        int asciiX = hexX + (m_bytesPerLine * 3 + 2) * m_charWidth;
        
        // Draw hex and ASCII
        for (int i = 0; i < m_bytesPerLine && offset + i < m_data.size(); ++i) {
            unsigned char byte = static_cast<unsigned char>(m_data[offset + i]);
            qint64 pos = offset + i;
            
            // Highlight selection
            bool isSelected = (m_selectionStart >= 0 && pos >= m_selectionStart && pos <= m_selectionEnd);
            bool isCursor = (pos == m_cursorPosition);
            
            // Draw hex byte
            int hexByteX = hexX + i * 3 * m_charWidth;
            if (isSelected) {
                painter.fillRect(hexByteX, y, m_charWidth * 2, m_charHeight, QColor(0, 120, 215, 100));
            }
            if (isCursor && m_cursorInHexArea) {
                painter.fillRect(hexByteX, y, m_charWidth * 2, m_charHeight, QColor(255, 255, 255, 50));
            }
            
            painter.setPen(Qt::white);
            QString hexByte = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();
            painter.drawText(hexByteX, y + m_charHeight, hexByte);
            
            // Draw ASCII character
            int asciiByteX = asciiX + i * m_charWidth;
            if (isSelected) {
                painter.fillRect(asciiByteX, y, m_charWidth, m_charHeight, QColor(0, 120, 215, 100));
            }
            if (isCursor && !m_cursorInHexArea) {
                painter.fillRect(asciiByteX, y, m_charWidth, m_charHeight, QColor(255, 255, 255, 50));
            }
            
            painter.setPen(QColor(180, 180, 180));
            QChar ch = (byte >= 32 && byte < 127) ? QChar(byte) : QChar('.');
            painter.drawText(asciiByteX, y + m_charHeight, ch);
        }
        
        y += m_charHeight + 2;
        offset += m_bytesPerLine;
    }
}

void HexEditor::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_scrollBar->setGeometry(width() - 20, 0, 20, height());
    updateScrollBar();
}

void HexEditor::keyPressEvent(QKeyEvent *event) {
    if (m_readOnly && event->key() != Qt::Key_Left && event->key() != Qt::Key_Right &&
        event->key() != Qt::Key_Up && event->key() != Qt::Key_Down &&
        event->key() != Qt::Key_PageUp && event->key() != Qt::Key_PageDown) {
        return;
    }
    
    switch (event->key()) {
        case Qt::Key_Left:
            if (m_cursorPosition > 0) {
                m_cursorPosition--;
                ensureCursorVisible();
                update();
            }
            break;
            
        case Qt::Key_Right:
            if (m_cursorPosition < m_data.size() - 1) {
                m_cursorPosition++;
                ensureCursorVisible();
                update();
            }
            break;
            
        case Qt::Key_Up:
            if (m_cursorPosition >= m_bytesPerLine) {
                m_cursorPosition -= m_bytesPerLine;
                ensureCursorVisible();
                update();
            }
            break;
            
        case Qt::Key_Down:
            if (m_cursorPosition + m_bytesPerLine < m_data.size()) {
                m_cursorPosition += m_bytesPerLine;
                ensureCursorVisible();
                update();
            }
            break;
            
        case Qt::Key_PageUp:
            m_cursorPosition = qMax(0LL, m_cursorPosition - m_bytesPerLine * visibleLines());
            ensureCursorVisible();
            update();
            break;
            
        case Qt::Key_PageDown:
            m_cursorPosition = qMin((qint64)m_data.size() - 1, m_cursorPosition + m_bytesPerLine * visibleLines());
            ensureCursorVisible();
            update();
            break;
            
        case Qt::Key_Home:
            m_cursorPosition = 0;
            ensureCursorVisible();
            update();
            break;
            
        case Qt::Key_End:
            m_cursorPosition = m_data.size() - 1;
            ensureCursorVisible();
            update();
            break;
            
        case Qt::Key_Tab:
            m_cursorInHexArea = !m_cursorInHexArea;
            update();
            break;
            
        default:
            if (!m_readOnly && m_cursorInHexArea) {
                QString text = event->text().toUpper();
                if (text.length() == 1 && text[0].isDigit()) {
                    int value = text[0].digitValue();
                    unsigned char byte = static_cast<unsigned char>(m_data[m_cursorPosition]);
                    
                    if (!m_nibblePosition) {
                        byte = (byte & 0x0F) | (value << 4);
                        m_nibblePosition = true;
                    } else {
                        byte = (byte & 0xF0) | value;
                        m_nibblePosition = false;
                        if (m_cursorPosition < m_data.size() - 1) {
                            m_cursorPosition++;
                        }
                    }
                    
                    m_data[m_cursorPosition] = byte;
                    setModified(true);
                    emit dataChanged();
                    update();
                } else if (text.length() == 1 && text[0] >= 'A' && text[0] <= 'F') {
                    int value = text[0].toLatin1() - 'A' + 10;
                    unsigned char byte = static_cast<unsigned char>(m_data[m_cursorPosition]);
                    
                    if (!m_nibblePosition) {
                        byte = (byte & 0x0F) | (value << 4);
                        m_nibblePosition = true;
                    } else {
                        byte = (byte & 0xF0) | value;
                        m_nibblePosition = false;
                        if (m_cursorPosition < m_data.size() - 1) {
                            m_cursorPosition++;
                        }
                    }
                    
                    m_data[m_cursorPosition] = byte;
                    setModified(true);
                    emit dataChanged();
                    update();
                }
            } else if (!m_readOnly && !m_cursorInHexArea) {
                QString text = event->text();
                if (text.length() == 1 && text[0].isPrint()) {
                    m_data[m_cursorPosition] = text[0].toLatin1();
                    if (m_cursorPosition < m_data.size() - 1) {
                        m_cursorPosition++;
                    }
                    setModified(true);
                    emit dataChanged();
                    update();
                }
            }
            break;
    }
    
    emit currentAddressChanged(m_cursorPosition);
}

void HexEditor::mousePressEvent(QMouseEvent *event) {
    bool inHexArea;
    qint64 pos = positionFromPoint(event->pos(), inHexArea);
    
    if (pos >= 0 && pos < m_data.size()) {
        m_cursorPosition = pos;
        m_cursorInHexArea = inHexArea;
        m_nibblePosition = false;
        
        if (event->modifiers() & Qt::ShiftModifier) {
            if (m_selectionStart < 0) {
                m_selectionStart = m_cursorPosition;
            }
            m_selectionEnd = m_cursorPosition;
        } else {
            m_selectionStart = -1;
            m_selectionEnd = -1;
        }
        
        update();
        emit currentAddressChanged(m_cursorPosition);
    }
}

void HexEditor::wheelEvent(QWheelEvent *event) {
    int numDegrees = event->angleDelta().y() / 8;
    int numSteps = numDegrees / 15;
    
    int newValue = m_scrollBar->value() - numSteps;
    m_scrollBar->setValue(newValue);
    
    event->accept();
}

void HexEditor::updateScrollBar() {
    int totalLines = (m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine;
    int visible = visibleLines();
    
    m_scrollBar->setRange(0, qMax(0, totalLines - visible));
    m_scrollBar->setPageStep(visible);
    m_scrollBar->setSingleStep(1);
}

void HexEditor::ensureCursorVisible() {
    int line = m_cursorPosition / m_bytesPerLine;
    int firstVisible = m_scrollBar->value();
    int lastVisible = firstVisible + visibleLines() - 1;
    
    if (line < firstVisible) {
        m_scrollBar->setValue(line);
    } else if (line > lastVisible) {
        m_scrollBar->setValue(line - visibleLines() + 1);
    }
}

qint64 HexEditor::positionFromPoint(const QPoint &pos, bool &inHexArea) {
    int line = (pos.y() - 5) / (m_charHeight + 2);
    line += m_scrollBar->value();
    
    int hexX = 5 + (m_addressWidth + 2) * m_charWidth;
    int asciiX = hexX + (m_bytesPerLine * 3 + 2) * m_charWidth;
    
    if (pos.x() >= hexX && pos.x() < asciiX) {
        // In hex area
        inHexArea = true;
        int byteIndex = (pos.x() - hexX) / (3 * m_charWidth);
        return line * m_bytesPerLine + byteIndex;
    } else if (pos.x() >= asciiX) {
        // In ASCII area
        inHexArea = false;
        int byteIndex = (pos.x() - asciiX) / m_charWidth;
        return line * m_bytesPerLine + byteIndex;
    }
    
    return -1;
}

QRect HexEditor::hexAreaRect() const {
    int hexX = 5 + (m_addressWidth + 2) * m_charWidth;
    int width = m_bytesPerLine * 3 * m_charWidth;
    return QRect(hexX, 0, width, height());
}

QRect HexEditor::asciiAreaRect() const {
    int hexX = 5 + (m_addressWidth + 2) * m_charWidth;
    int asciiX = hexX + (m_bytesPerLine * 3 + 2) * m_charWidth;
    int width = m_bytesPerLine * m_charWidth;
    return QRect(asciiX, 0, width, height());
}

int HexEditor::visibleLines() const {
    return (height() - 10) / (m_charHeight + 2);
}
