# Jim — Technical Specifications

## Architecture

### Core Components
- **Language:** C++17
- **GUI Framework:** Qt6 (Core, Gui, Widgets)
- **Build System:** GNU Make
- **Minimum Qt Version:** 6.0

### Class Structure

| Class | Base | Role |
|-------|------|------|
| `TextEditor` | `QMainWindow` | Main window, menus, actions, file management, theme coordination |
| `CodeEditor` | `QPlainTextEdit` | Editor widget with line numbers, folding, auto-indent, bracket pairing |
| `SyntaxHighlighter` | `QSyntaxHighlighter` | Multi-language pattern-based highlighting (11 languages) |
| `HexEditor` | `QWidget` | Binary file viewer/editor with hex and ASCII display |
| `LineNumberArea` | `QWidget` | Line number gutter, synchronized with editor scroll |
| `FoldingArea` | `QWidget` | Code folding indicators (▶/▼), click to toggle |
| `MiniMap` | `QWidget` | Code overview with viewport indicator |
| `WelcomeWidget` | `QWidget` | Start screen with recent files and action buttons |
| `BreadcrumbBar` | `QWidget` | Path + symbol navigation bar below tabs |
| `TerminalWidget` | `QWidget` | Embedded shell with QProcess, output display, command input |
| `AnimationWidget` | `QWidget` | Visual effects widget (Matrix, Particles, Waves, Pulse) |
| `TitleBar` | `QWidget` | Custom frameless window title bar with controls |

### Enums
- `Language` — `PlainText`, `CPP`, `Python`, `JavaScript`, `HTML`, `CSS`, `Rust`, `Go`, `JSON`, `YAML`, `Markdown`
- `AnimationType` — `None`, `Matrix`, `Particles`, `Waves`, `Pulse`

## Features

### File Management
- Open/Save/Save As with unsaved changes detection
- Multiple file tabs with close buttons
- Recent files menu (last 10)
- Folder browsing via file tree sidebar
- Command-line file/folder opening
- File watcher with external change detection and reload prompt
- Automatic binary file detection (null byte checking)
- Binary files open in hex editor with `[HEX]` tab prefix
- **Change Tracking:** Asterisk `*` in tab title for unsaved changes (both text and hex)
- **Safe Exit:** Prompts to save modified files on close

### Editor
- Line numbers with current-line highlight
- Code folding (brace-based block detection)
- Current line highlighting
- Auto-indentation (context-aware)
- Tab to spaces (4 spaces)
- Bracket and quote auto-pairing with skip-closing
- Undo/Redo/Cut/Copy/Paste/Select All
- **Line Power Actions:** Duplicate (`Ctrl+D`), Move Up/Down (`Alt+Up/Down`), Delete (`Ctrl+Shift+K`), and Toggle Comments (`Ctrl+/`)

### Syntax Highlighting

| Language | Keywords | Strings | Comments | Numbers | Special |
|----------|----------|---------|----------|---------|---------|
| C/C++ | ✓ | ✓ | `//`, `/* */` | ✓ | Qt classes, preprocessor |
| Python | ✓ | ✓ | `#` | ✓ | Decorators |
| JavaScript/TS | ✓ | `` ` `` templates | `//`, `/* */` | ✓ | Arrow functions |
| HTML | Tags | ✓ | `<!-- -->` | Entities | Attributes |
| CSS | Selectors | ✓ | `/* */` | Units | At-rules |
| Rust | ✓ | ✓ | `//` | Type suffixes | Type names |
| Go | ✓ | `` ` `` raw | `//` | ✓ | Built-in funcs |
| JSON | Keys | Values | — | ✓ | true/false/null |
| YAML | Keys | ✓ | `#` | ✓ | List markers |
| Markdown | — | Inline code | Blockquote | — | Headings, bold, links |

### Search & Navigation
- Find with wraparound
- Find next (F3)
- Replace all
- Go to line (Ctrl+G)
- Breadcrumb navigation (folder > file > function)
- **Smart Home:** Jumps to first non-whitespace character, then start of line

### UI & Theming
- 3 themes: Light, Dark, Monokai
- Custom background colors with auto text adjustment
- Modern VS Code-inspired styling
- Transparent scrollbars with rounded handles
- Welcome screen with recent files
- Language indicator in status bar
- **Animation Cycler:** Cycle through visual backgrounds (Ctrl+A)
- **Custom Title Bar:** Native-looking frameless UI with minimize/maximize/close

### View Options
- File tree sidebar (toggle Ctrl+B)
- Split view (horizontal, Ctrl+\\)
- Integrated terminal (Ctrl+`) with dynamic working directory matching open folder
- Hex editor for binary files (automatic detection)
- Mini map (Ctrl+M)
- Word wrap toggle
- Font size adjustment
- **Animation Dock:** Toggleable side dock for animation effects

### Hex Editor Features
- Address column (8-digit hex addresses)
- Hex view (16 bytes per line, space-separated)
- ASCII view (printable characters, dots for non-printable)
- Keyboard navigation (arrows, Page Up/Down, Home/End)
- Hex editing mode (type hex digits to modify bytes)
- ASCII editing mode (type characters to modify bytes)
- Tab key to switch between hex and ASCII areas
- Selection support with visual highlighting
- Vertical scrollbar for large files
- **Save Functionality:** Full support for saving modifications back to disk
- **Undo Integration:** Basic modification tracking and restoration

## Performance

| Metric | Value |
|--------|-------|
| Base memory | ~15-20 MB |
| Per file | ~1-2 MB |
| Cold start | <500ms |
| File argument | <700ms |
| Small files (<1MB) | Instant |
| Medium files (1-10MB) | <1 second |

## Platform Support

| Platform | Status | Dependencies |
|----------|--------|-------------|
| Linux (Ubuntu/Kali/Arch) | Native | qt6-base-dev, build-essential |
| Windows 10/11 | Native | Qt6 MinGW/MSVC (via build.ps1) |
| macOS | Supported | Homebrew Qt6 |

## Build Configuration

- **Compiler flags:** `-std=c++17 -Wall -Wextra -O3 -march=native -fPIC`
- **Qt modules:** QtCore, QtGui, QtWidgets
- **Build artifacts:** `*.o`, `moc_*.cpp`, `jim` (Linux) / `jim.exe` (Windows)

### File Structure
```
jim/
├── main.cpp              # Entry point + CLI argument handling
├── texteditor.h          # Main editor + Animation class declarations
├── texteditor.cpp        # Main editor + Animation implementations
├── hexeditor.h           # Hex editor class declaration
├── hexeditor.cpp         # Hex editor implementation
├── linenumberarea.h      # LineNumberArea + FoldingArea + MiniMap headers
├── linenumberarea.cpp    # Widget implementations
├── jim.pro               # Qt project file
├── build.ps1             # Windows build script (PowerShell)
├── GNUmakefile           # Cross-platform build dispatcher
├── Makefile              # Generated build configuration
├── README.md             # User documentation
├── SPECS.md              # This file
├── ROADMAP.md            # Development roadmap
└── .gitignore            # Git ignore rules
```

## Configuration

| Setting | Location | Format |
|---------|----------|--------|
| Config file | `~/.config/TextEditor/Settings.conf` | Qt QSettings (INI) |
| Recent files | Max 10 | Persisted |
| Font size | Default 11pt | Persisted |
| Word wrap | Default off | Persisted |

## Security
- No network operations, telemetry, or cloud sync
- No automatic file execution or external process spawning (except terminal)
- Read/write permissions respected
- Settings stored locally only
