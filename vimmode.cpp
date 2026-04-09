#include "vimmode.h"

VimMode::VimMode(QObject *parent) : QObject(parent) {}

void VimMode::setEnabled(bool en) {
    m_enabled = en;
    if (!en) {
        m_mode    = Insert;
        m_pending = QChar();
    }
    emit modeChanged(en ? (m_mode == Normal ? "NORMAL" : "INSERT") : "");
}

void VimMode::enterInsert(QPlainTextEdit *editor) {
    m_mode = Insert;
    editor->setCursorWidth(1);      // thin line cursor in insert mode
    emit modeChanged("INSERT");
}

void VimMode::enterNormal(QPlainTextEdit *editor) {
    // Move one left (unless at line start) — mirrors real Vim behaviour
    QTextCursor c = editor->textCursor();
    if (!c.atBlockStart()) c.movePosition(QTextCursor::Left);
    editor->setTextCursor(c);
    m_mode = Normal;
    // Block cursor: set width to one character width
    editor->setCursorWidth(editor->fontMetrics().averageCharWidth());
    emit modeChanged("NORMAL");
}

bool VimMode::handleKey(QKeyEvent *event, QPlainTextEdit *editor) {
    if (!m_enabled) return false;

    if (m_mode == Insert) {
        if (event->key() == Qt::Key_Escape) {
            enterNormal(editor);
            return true;
        }
        return false;   // insert mode: let editor handle all other keys normally
    }

    return handleNormal(event, editor);
}

bool VimMode::handleNormal(QKeyEvent *event, QPlainTextEdit *editor) {
    const QString text = event->text();
    const int     key  = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    // ── Two-key pending sequences ────────────────────────────────────────────
    if (!m_pending.isNull()) {
        QChar pending = m_pending;
        m_pending = QChar();   // clear before any early returns

        if (pending == 'd' && text == "d") {
            // dd — delete current line
            QTextCursor c = editor->textCursor();
            c.beginEditBlock();
            c.movePosition(QTextCursor::StartOfLine);
            c.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            c.removeSelectedText();
            // Remove the trailing newline (or leading if at end of doc)
            if (!c.atEnd())
                c.deleteChar();
            else if (!c.atStart())
                c.deletePreviousChar();
            c.endEditBlock();
            editor->setTextCursor(c);
            emit deleteLineRequested();
            return true;
        }
        if (pending == 'y' && text == "y") {
            // yy — yank current line into clipboard
            QTextCursor c = editor->textCursor();
            c.movePosition(QTextCursor::StartOfLine);
            c.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            QApplication::clipboard()->setText(c.selectedText() + "\n");
            c.clearSelection();
            editor->setTextCursor(c);
            return true;
        }
        if (pending == 'g' && text == "g") {
            // gg — go to top of file
            QTextCursor c = editor->textCursor();
            c.movePosition(QTextCursor::Start);
            editor->setTextCursor(c);
            return true;
        }
        return true;    // swallow unknown two-key sequence
    }

    QTextCursor c = editor->textCursor();

    // ── Navigation ───────────────────────────────────────────────────────────
    if (text == "h") { c.movePosition(QTextCursor::Left);        editor->setTextCursor(c); return true; }
    if (text == "l") { c.movePosition(QTextCursor::Right);       editor->setTextCursor(c); return true; }
    if (text == "j") { c.movePosition(QTextCursor::Down);        editor->setTextCursor(c); return true; }
    if (text == "k") { c.movePosition(QTextCursor::Up);          editor->setTextCursor(c); return true; }
    if (text == "w") { c.movePosition(QTextCursor::NextWord);    editor->setTextCursor(c); return true; }
    if (text == "b") { c.movePosition(QTextCursor::PreviousWord);editor->setTextCursor(c); return true; }
    if (text == "0") { c.movePosition(QTextCursor::StartOfLine); editor->setTextCursor(c); return true; }
    if (text == "$") { c.movePosition(QTextCursor::EndOfLine);   editor->setTextCursor(c); return true; }
    if (text == "G") { c.movePosition(QTextCursor::End);         editor->setTextCursor(c); return true; }

    // Ctrl+D / Ctrl+U — half-page scroll
    if (key == Qt::Key_D && mods == Qt::ControlModifier) {
        auto *sb = editor->verticalScrollBar();
        sb->setValue(sb->value() + sb->pageStep() / 2);
        c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, editor->verticalScrollBar()->pageStep() / 4);
        editor->setTextCursor(c);
        return true;
    }
    if (key == Qt::Key_U && mods == Qt::ControlModifier) {
        auto *sb = editor->verticalScrollBar();
        sb->setValue(sb->value() - sb->pageStep() / 2);
        c.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, editor->verticalScrollBar()->pageStep() / 4);
        editor->setTextCursor(c);
        return true;
    }

    // ── Editing ──────────────────────────────────────────────────────────────
    if (text == "x") { c.deleteChar();           editor->setTextCursor(c); return true; }
    if (text == "X") { c.deletePreviousChar();    editor->setTextCursor(c); return true; }
    if (text == "u") { editor->undo();  return true; }
    if (key == Qt::Key_R && mods == Qt::ControlModifier) { editor->redo(); return true; }

    if (text == "p") {
        // paste after cursor
        c.movePosition(QTextCursor::Right); editor->setTextCursor(c);
        editor->paste();
        return true;
    }
    if (text == "P") { editor->paste(); return true; }

    // ── Pending two-key starters ─────────────────────────────────────────────
    if (text == "d") { m_pending = 'd'; return true; }
    if (text == "y") { m_pending = 'y'; return true; }
    if (text == "g") { m_pending = 'g'; return true; }

    // ── Mode transitions (→ Insert) ──────────────────────────────────────────
    if (text == "i") { enterInsert(editor); return true; }

    if (text == "a") {
        c.movePosition(QTextCursor::Right); editor->setTextCursor(c);
        enterInsert(editor); return true;
    }
    if (text == "A") {
        c.movePosition(QTextCursor::EndOfLine); editor->setTextCursor(c);
        enterInsert(editor); return true;
    }
    if (text == "I") {
        c.movePosition(QTextCursor::StartOfLine); editor->setTextCursor(c);
        enterInsert(editor); return true;
    }

    if (text == "o") {
        // new line below + insert
        c.movePosition(QTextCursor::EndOfLine);
        QString indent;
        for (QChar ch : c.block().text()) { if (ch == ' ' || ch == '\t') indent += ch; else break; }
        c.insertText("\n" + indent);
        editor->setTextCursor(c);
        enterInsert(editor);
        return true;
    }
    if (text == "O") {
        // new line above + insert
        c.movePosition(QTextCursor::StartOfLine);
        QString indent;
        for (QChar ch : c.block().text()) { if (ch == ' ' || ch == '\t') indent += ch; else break; }
        c.insertText(indent + "\n");
        c.movePosition(QTextCursor::Up);
        c.movePosition(QTextCursor::EndOfLine);
        editor->setTextCursor(c);
        enterInsert(editor);
        return true;
    }
    if (text == "s") {
        // delete char under cursor, enter insert
        c.deleteChar(); editor->setTextCursor(c);
        enterInsert(editor);
        return true;
    }
    if (text == "C") {
        // delete to end of line, enter insert
        c.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        c.removeSelectedText();
        editor->setTextCursor(c);
        enterInsert(editor);
        return true;
    }

    // Escape in normal mode clears selection
    if (key == Qt::Key_Escape) {
        c.clearSelection(); editor->setTextCursor(c);
        return true;
    }

    return true;    // swallow everything else in normal mode (no accidental insertions)
}
