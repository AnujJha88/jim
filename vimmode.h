#ifndef VIMMODE_H
#define VIMMODE_H

#include <QObject>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>

// VimMode — lightweight Vim Normal/Insert modal layer for QPlainTextEdit.
// Drop a VimMode instance into a CodeEditor and call handleKey() from
// keyPressEvent.  Returns true if the event was consumed (caller should not
// pass it to QPlainTextEdit::keyPressEvent).
//
// Implemented motions & commands:
//   Normal mode: h j k l  w b  0 $  G  (gg, dd, yy pending two-key)
//                x X s  u p P  i a A I o O  Ctrl+D/U (half-page)  Esc
//   Insert mode: Esc → Normal
class VimMode : public QObject {
    Q_OBJECT
public:
    enum Mode { Insert, Normal };

    explicit VimMode(QObject *parent = nullptr);

    // Returns true if event was fully handled; caller should return without
    // passing the event down.
    bool handleKey(QKeyEvent *event, QPlainTextEdit *editor);

    Mode currentMode() const { return m_mode; }
    bool isEnabled() const   { return m_enabled; }

    // Enabling resets to Insert mode; disabling restores thin cursor.
    void setEnabled(bool en);

signals:
    void modeChanged(const QString &modeName); // "NORMAL", "INSERT", or "" (disabled)
    void deleteLineRequested();                // emitted by dd so laser effect fires

private:
    bool  m_enabled = false;
    Mode  m_mode    = Insert;
    QChar m_pending;   // first key of a two-key sequence (d, y, g)

    bool handleNormal(QKeyEvent *event, QPlainTextEdit *editor);
    void enterInsert(QPlainTextEdit *editor);
    void enterNormal(QPlainTextEdit *editor);
};

#endif // VIMMODE_H
