#include "hexeditor.h"
#include <QPainter>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QFile>
#include <QFontMetrics>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>

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

// ---------------------------------------------------------------------------
// patchBytes — overwrite m_data[address .. address+len-1] with newBytes
// ---------------------------------------------------------------------------
void HexEditor::patchBytes(qint64 address, const QByteArray &newBytes)
{
    if (address < 0 || address + newBytes.size() > m_data.size()) return;
    for (int i = 0; i < newBytes.size(); ++i)
        m_data[address + i] = newBytes[i];
    setModified(true);
    emit dataChanged();
    update();
}

// ---------------------------------------------------------------------------
// contextMenuEvent — right-click menu with copy / patch actions
// ---------------------------------------------------------------------------
void HexEditor::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_data.isEmpty()) return;

    static const QString menuStyle =
        "QMenu { background-color: #252526; color: #d4d4d4; border: 1px solid #3c3c3c; }"
        "QMenu::item:selected { background-color: #094771; }";

    QMenu menu(this);
    menu.setStyleSheet(menuStyle);

    QAction *copyHexAct    = menu.addAction("Copy Hex");
    QAction *copyAddrAct   = menu.addAction("Copy Address");
    menu.addSeparator();
    QAction *patchBytesAct = menu.addAction("Patch Bytes...");
    QAction *patchAsmAct   = menu.addAction("Patch Instruction (x86)...");

    // Disable patch actions in read-only mode
    patchBytesAct->setEnabled(!m_readOnly);
    patchAsmAct->setEnabled(!m_readOnly);

    QAction *chosen = menu.exec(event->globalPos());
    if (!chosen) return;

    // -------------------------------------------------------------------
    // Copy Hex
    // -------------------------------------------------------------------
    if (chosen == copyHexAct) {
        QByteArray bytes;
        if (m_selectionStart >= 0 && m_selectionEnd >= m_selectionStart) {
            bytes = m_data.mid(static_cast<int>(m_selectionStart),
                               static_cast<int>(m_selectionEnd - m_selectionStart + 1));
        } else {
            bytes = m_data.mid(static_cast<int>(m_cursorPosition), 1);
        }

        QString hex;
        for (int i = 0; i < bytes.size(); ++i) {
            if (i > 0) hex += ' ';
            hex += QString("%1").arg(static_cast<unsigned char>(bytes[i]), 2, 16, QChar('0')).toUpper();
        }
        QApplication::clipboard()->setText(hex);
    }

    // -------------------------------------------------------------------
    // Copy Address
    // -------------------------------------------------------------------
    else if (chosen == copyAddrAct) {
        QString addrStr = QString("%1").arg(m_cursorPosition, m_addressWidth, 16, QChar('0')).toUpper();
        QApplication::clipboard()->setText(addrStr);
    }

    // -------------------------------------------------------------------
    // Patch Bytes...
    // -------------------------------------------------------------------
    else if (chosen == patchBytesAct) {
        // Show the current byte as a hint
        unsigned char curByte = static_cast<unsigned char>(m_data[m_cursorPosition]);
        QString curHex = QString("%1").arg(curByte, 2, 16, QChar('0')).toUpper();

        bool ok = false;
        QString input = QInputDialog::getText(
            this,
            "Patch Bytes",
            QString("Current byte at 0x%1: %2\nEnter new hex bytes (e.g. \"90 90\" or \"9090\"):")
                .arg(m_cursorPosition, m_addressWidth, 16, QChar('0')).toUpper()
                .arg(curHex),
            QLineEdit::Normal,
            curHex,
            &ok
        );
        if (!ok || input.trimmed().isEmpty()) return;

        // Accept both "90 90" and "9090" formats
        QString cleaned = input.simplified().remove(' ');
        if (cleaned.length() % 2 != 0) {
            QMessageBox::warning(this, "Patch Bytes", "Invalid hex input — odd number of nibbles.");
            return;
        }

        QByteArray newBytes;
        bool parseOk = true;
        for (int i = 0; i < cleaned.length(); i += 2) {
            bool byteOk = false;
            unsigned char b = static_cast<unsigned char>(cleaned.mid(i, 2).toUInt(&byteOk, 16));
            if (!byteOk) { parseOk = false; break; }
            newBytes.append(static_cast<char>(b));
        }
        if (!parseOk || newBytes.isEmpty()) {
            QMessageBox::warning(this, "Patch Bytes", "Invalid hex input — could not parse bytes.");
            return;
        }

        if (m_cursorPosition + newBytes.size() > m_data.size()) {
            QMessageBox::warning(this, "Patch Bytes", "Patch extends beyond end of data.");
            return;
        }

        emit patchRequested(m_cursorPosition, newBytes);
        patchBytes(m_cursorPosition, newBytes);
    }

    // -------------------------------------------------------------------
    // Patch Instruction (x86)...
    // -------------------------------------------------------------------
    else if (chosen == patchAsmAct) {
        bool ok = false;
        QString mnemonic = QInputDialog::getText(
            this,
            "Patch x86 Instruction",
            "Enter instruction mnemonic (NOP, INT3, RET, RETN):",
            QLineEdit::Normal,
            "NOP",
            &ok
        ).toUpper().trimmed();
        if (!ok || mnemonic.isEmpty()) return;

        // Map mnemonic to opcode byte
        QMap<QString, unsigned char> opcodeMap;
        opcodeMap["NOP"]  = 0x90;
        opcodeMap["INT3"] = 0xCC;
        opcodeMap["RET"]  = 0xC3;
        opcodeMap["RETN"] = 0xC3;

        if (!opcodeMap.contains(mnemonic)) {
            QMessageBox::warning(this, "Patch Instruction",
                                 QString("Unknown mnemonic '%1'.\nSupported: NOP, INT3, RET, RETN.").arg(mnemonic));
            return;
        }

        unsigned char opcode = opcodeMap[mnemonic];

        // For NOP, fill the entire selection if one exists
        int patchLen = 1;
        qint64 patchAddr = m_cursorPosition;
        if (mnemonic == "NOP" && m_selectionStart >= 0 && m_selectionEnd >= m_selectionStart) {
            patchAddr = m_selectionStart;
            patchLen  = static_cast<int>(m_selectionEnd - m_selectionStart + 1);
        }

        if (patchAddr + patchLen > m_data.size()) {
            QMessageBox::warning(this, "Patch Instruction", "Patch extends beyond end of data.");
            return;
        }

        QByteArray newBytes(patchLen, static_cast<char>(opcode));
        emit patchRequested(patchAddr, newBytes);
        patchBytes(patchAddr, newBytes);

        QMessageBox::information(
            this,
            "Patch Applied",
            QString("Wrote %1 x %2 byte(s) of %3 (0x%4) at 0x%5.")
                .arg(patchLen)
                .arg(1)
                .arg(mnemonic)
                .arg(opcode, 2, 16, QChar('0')).toUpper()
                .arg(patchAddr, m_addressWidth, 16, QChar('0')).toUpper()
        );
    }
}
