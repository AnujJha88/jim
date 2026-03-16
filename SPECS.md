# Jim — Technical Specifications

## Architecture

### Core Philosophy
- **Native C++17 / Qt6** — no Electron, no web views, no runtime dependencies beyond Qt
- **Zero mandatory external tools** — every feature works out of the box; optional tools (objdump) unlock additional capability
- **Single process** — no LSP daemons, no background services, no telemetry

### Build System
| Platform | Toolchain | Script |
|----------|-----------|--------|
| Linux / macOS / WSL | GCC or Clang + GNU Make | `GNUmakefile` → `qmake` → `make` |
| Windows | MinGW-w64 + Qt6 | `build.ps1` → `qmake` → `mingw32-make` |

**Qt modules used:** `QtCore`, `QtGui`, `QtWidgets`, `QtNetwork`
**Minimum Qt version:** 6.0
**C++ standard:** C++17

---

## Class Structure

### Editor Core

| Class | Base | Role |
|-------|------|------|
| `TextEditor` | `QMainWindow` | Main window — menus, actions, file I/O, tab management, theme coordination, panel orchestration |
| `CodeEditor` | `QPlainTextEdit` | Editor widget — line numbers, folding, auto-indent, bracket pairing, smooth scroll, search highlights |
| `SyntaxHighlighter` | `QSyntaxHighlighter` | Multi-language regex-based highlighting (11 languages) |
| `LineNumberArea` | `QWidget` | Line number gutter, synchronized with editor scroll and fold state |
| `FoldingArea` | `QWidget` | Code folding gutter with ▶/▼ triangle indicators, click
 to toggle |
| `MiniMap` | `QWidget` | Scaled-down code overview with draggable viewport indicator |
| `TitleBar` | `QWidget` | Custom frameless window title bar — drag, double-click to maximise, min/max/close buttons |
| `FindBar` | `QWidget` | Inline find bar — live match highlighting, match counter, previous/next navigation |

### UI Panels

| Class | Base | Role |
|-------|------|------|
| `WelcomeWidget` | `QWidget` | Start screen — recent files list and quick-action buttons, fade in/out animated |
| `BreadcrumbBar` | `QWidget` | Path + current symbol navigation bar below the tab bar |
| `TerminalWidget` | `QWidget` | Embedded shell using `QProcess`, output display, command input, animated slide |
| `AnimationWidget` | `QWidget` | Visual effects widget — Matrix, Particles, Waves, Pulse, Starfield, Rain, Snow, Fire |

### Binary Tools

| Class | Base | Role |
|-------|------|------|
| `HexEditor` | `QWidget` | Binary file viewer/editor — offset column, hex + ASCII dual view, keyboard nav, save support |
| `DisassemblerWidget` | `QWidget` | Disassembler UI — launches `objdump`/`llvm-objdump`, parsed function list, click-to-jump |
| `AsmSyntaxHighlighter` | `QSyntaxHighlighter` | Assembly syntax highlighting — mnemonics, registers, addresses, labels, comments, directives |
| `BinaryInspectorWidget` | `QWidget` | Pure native ELF + PE parser — headers, sections, imports/symbols, strings tab, MD5 |

### Markdown

| Class | Base | Role |
|-------|------|------|
| `MarkdownConverter` | — | Pure static Markdown → HTML converter; no external dependencies |
| `MarkdownPreviewWidget` | `QWidget` | Live split-panel rendered preview using `QTextBrowser`, dark CSS, image path resolution |

### AI

| Class | Base | Role |
|-------|------|------|
| `AIAutocomplete` | `QObject` | Sends context to any OpenAI-compatible API over `QNetworkAccessManager`, emits suggestion signal |
| `AISettingsDialog` | `QDialog` | Configuration dialog for base URL, API key, model name, enable toggle |

---

## Enums


```cpp
enum class Language {
    PlainText, CPP, Python, JavaScript,
    HTML, CSS, Rust, Go, JSON, YAML, Markdown
};

enum class AnimationType {
    None, Matrix, Particles, Waves, Pulse,
    Starfield, Rain, Snow, Fire
};
```

---

## Features

### File Management
- Open / Save / Save As with unsaved-changes detection (`*` prefix in tab title)
- Multiple tabs — closable, movable, document mode
- Recent files menu (last 10, persisted)
- Folder browsing via file tree sidebar with QFileSystemModel
- Command-line file and folder opening (`jim file.cpp`, `jim .`)
- File watcher — detects external changes and prompts to reload (re-watches after save)
- Automatic binary detection (null-byte scan of first 512 bytes)
- Binary files auto-open in Hex Editor tab with `[HEX]` prefix
- Safe exit — prompts to save all modified tabs on close

### Editor
- Line numbers with current-line highlight, auto-width
- Code folding — brace-based block detection, matching brace search, gutter indicators
- Current line highlight
- Auto-indentation (mirrors previous line's leading whitespace)
- Tab → 4 spaces
- Bracket and quote auto-pairing with skip-over-closing behaviour
- Undo / Redo / Cut / Copy / Paste / Select All
- Smooth scroll via `QPropertyAnimation` on vertical scrollbar
- Search highlights — all matches shown simultaneously via `QTextEdit::ExtraSelection`
- **Line power actions:**

| Action | Shortcut |
|--------|----------|
| Duplicate line | `Ctrl+D` |
| Move line up | `Alt+↑` |
| Move line down | `Alt+↓` |
| Delete line | `Ctrl+Shift+K` |
| Toggle comment | `Ctrl+/` |
| Smart Home | `Home` |

- Trailing whitespace auto-trimmed on save

### Syntax Highlighting

| Language | Keywords | Strings | Comments | Numbers | Special |
|----------|----------|---------|----------|---------|---------|
| C / C++ | ✓ | ✓ | `//` `/* */` | ✓ | Qt classes, preprocessor |
| Python | ✓ | ✓ | `#` | ✓ | Decorators |
| JavaScript / TS | ✓ | `` ` `` templates | `//` `/* */` | ✓ | Arrow functions |
| HTML | Tags | ✓ | `<!-- -->` | — | Attributes |
| CSS | Selectors | ✓ | `/* */` | Units | At-rules |
| Rust | ✓ | ✓ | `//` | Type suffixes | Type names |
| Go | ✓ | `` ` `` raw | `//` | ✓ | Built-ins |
| JSON | Keys | Values | — | ✓ | `true` / `false` / `null` |
| YAML | Keys | ✓ | `#` | ✓ | List markers |
| Markdown | — | Inline code | Blockquote | — | Headings, bold, links |
| Assembly | Mnemonics | — | `#` `;` | Hex | Registers, labels, directives |

### Search & Navigation
- Inline find bar — live match count, highlight all, previous / next (`Ctrl+F`, `F3`, `Shift+F3`)
- Replace all (`Ctrl+H`)
- Go to line (`Ctrl+G`)
- Breadcrumb — resolves current function/class via upward regex scan from cursor
- Smart Home — first press → first non-whitespace; second press → column 0

### Binary Analysis Suite

#### Hex Editor
- 8-digit hex offset column
- 16 bytes per row, space-separated hex + ASCII panel
- Keyboard navigation: arrows, Page Up/Down, Home/End, Tab (switch hex↔ASCII)
- Hex editing: type two hex digits to modify a byte, advances cursor automatically
- ASCII editing: type any printable character
- Selection with Shift+arrow
- Vertical scrollbar for files of any size
- Full save support — writes modified bytes back to disk
- Modification tracking — asterisk in tab title, unsaved changes prompt on close

#### Disassembler
- Tries `objdump -d -C --no-show-raw-insn` first, falls back to `llvm-objdump`
- Streams output via `QProcess` (non-blocking)
- Parses function labels with regex `^[0-9a-fA-F]+ <([^>]+)>:`
- Left panel: function list with address
