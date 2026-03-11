# Jim — Development Roadmap

> A lightweight, fast code editor built with C++ and Qt6.

## v1.0 — Released

**Core**: Syntax highlighting (C++, Python), line numbers, auto-indentation, bracket/quote auto-pairing, tabs, split view, minimap, word wrap, find & replace, go-to-line, recent files, file tree, themes (Light/Dark), CLI support.

---

## v1.1 — Current Release

### Implemented
- [x] Welcome Screen with recent files and quick actions
- [x] Breadcrumb Navigation (`folder > file > function`)
- [x] Integrated Terminal (Ctrl+`)
- [x] Code Folding with gutter indicators
- [x] 11-Language Syntax Highlighting (C/C++, Python, JS/TS, HTML, CSS, Rust, Go, JSON, YAML, Markdown)
- [x] Language Auto-Detection from file extension
- [x] File Watcher with external change reload prompt
- [x] Monokai theme
- [x] Modernized VS Code-inspired UI

### Remaining for v1.1
- [ ] Regex search with toggle
- [ ] Case-sensitive / whole-word search toggles
- [ ] Highlight all search matches
- [ ] Replace with confirmation (one-by-one)
- [ ] Search in selection

---

## v1.2 — Line Editing Power

- [ ] Duplicate line (`Ctrl+D`)
- [ ] Move line up/down (`Alt+↑/↓`)
- [ ] Delete line (`Ctrl+Shift+K`)
- [ ] Comment/uncomment toggle (`Ctrl+/`)
- [ ] Join lines (`Ctrl+J`)
- [ ] Sort lines (ascending/descending)
- [ ] Trim trailing whitespace on save
- [ ] Smart home key (jump to first non-whitespace)
- [ ] Tab context menu (Close Others, Close All, Close to the Right, Copy Path)
- [ ] Drag-and-drop file opening

---

## v1.5 — Multi-Cursor & Intelligence

### Multi-Cursor Editing
- [ ] Add cursor at mouse click (`Alt+Click`)
- [ ] Select next occurrence (`Ctrl+D`)
- [ ] Select all occurrences (`Ctrl+Shift+L`)
- [ ] Column/box selection mode (`Shift+Alt+drag`)
- [ ] Add cursor above/below (`Ctrl+Alt+↑/↓`)

### Auto-Completion
- [ ] Buffer word completion (words from current file)
- [ ] Path completion in strings
- [ ] Snippet support (user-defined templates)
- [ ] Bracket completion for multi-line blocks
- [ ] Emmet abbreviation expansion (HTML/CSS)

### Bookmarks
- [ ] Toggle bookmark (`Ctrl+F2`)
- [ ] Jump to next/previous bookmark
- [ ] Bookmark panel (list all bookmarks)
- [ ] Named bookmarks

### Indent Guides
- [ ] Vertical indent guide lines
- [ ] Active indent guide highlighting
- [ ] Rainbow indent colors (optional)

---

## v2.0 — Developer Ecosystem

### LSP Integration
- [ ] Language Server Protocol client
- [ ] Go-to-definition (`F12`)
- [ ] Find all references (`Shift+F12`)
- [ ] Hover documentation
- [ ] Inline diagnostics (errors, warnings, hints)
- [ ] Code actions (quick fixes)
- [ ] Rename symbol (`F2`)
- [ ] Signature help in function calls
- [ ] Workspace symbol search

### Git Integration
- [ ] Gutter indicators (added/modified/deleted lines)
- [ ] File status in file tree (modified, untracked, staged)
- [ ] Inline blame annotations
- [ ] Diff view (side-by-side and inline)
- [ ] Stage/unstage/commit from editor
- [ ] Branch indicator in status bar
- [ ] Git log viewer

### Code Formatting
- [ ] Format on save (configurable)
- [ ] Format selection
- [ ] Integration with clang-format, prettier, rustfmt, gofmt, black
- [ ] `.editorconfig` support
- [ ] Custom format rules per language

### Advanced Terminal
- [ ] Multiple terminal instances (tabs)
- [ ] Split terminal panes
- [ ] Terminal profiles (bash, zsh, PowerShell)
- [ ] Clickable file paths in output
- [ ] Terminal themes matching editor theme

---

## v2.5 — Project Intelligence

### Fuzzy Finder
- [ ] Quick file open (`Ctrl+P`)
- [ ] Command palette (`Ctrl+Shift+P`)
- [ ] Go to symbol (`Ctrl+Shift+O`)
- [ ] Fuzzy matching with scoring
- [ ] Recent files priority

### Project Management
- [ ] Workspace/project files (`.jim-workspace`)
- [ ] Multi-root workspaces
- [ ] Project-specific settings
- [ ] Task runner integration (npm scripts, Makefile targets, cargo)
- [ ] Build output panel with error navigation

### Search & Replace (Advanced)
- [ ] Search across files (`Ctrl+Shift+F`)
- [ ] Replace across files
- [ ] Search with include/exclude glob patterns
- [ ] Search results panel with preview
- [ ] Search history with quick recall

### File Management
- [ ] Encoding detection & conversion (UTF-8, UTF-16, Latin-1)
- [ ] Line ending conversion (LF/CRLF/CR)
- [ ] File comparison/diff tool
- [ ] New file from template
- [ ] Rename file from tab
- [ ] Reveal in file explorer

---

## v3.0 — Extensibility

### Plugin System
- [ ] Lua scripting API for plugins
- [ ] Plugin manager (install, update, disable)
- [ ] Event hooks (on save, on open, on key)
- [ ] Custom commands and keybindings via plugins
- [ ] Plugin marketplace/registry

### Theme System
- [ ] Theme editor (visual color picker)
- [ ] Import VS Code themes (JSON)
- [ ] Per-language color overrides
- [ ] Icon themes for file tree
- [ ] Font ligature support

### Keybinding Customization
- [ ] Keybinding editor UI
- [ ] Vim keybindings mode
- [ ] Emacs keybindings mode
- [ ] Import keymaps from other editors
- [ ] Macro recording and playback (`Ctrl+Shift+M`)

### Debugger Integration
- [ ] DAP (Debug Adapter Protocol) support
- [ ] Breakpoints (line, conditional, logpoint)
- [ ] Variable watch panel
- [ ] Call stack view
- [ ] Step through, step over, step out
- [ ] Debug console

---

## Distribution

### Linux
- [ ] AppImage distribution
- [ ] Flatpak package
- [ ] `.desktop` file for app menu
- [ ] Snap package
- [ ] AUR package (Arch)

### Windows
- [ ] NSIS installer
- [ ] Context menu integration ("Open with Jim")
- [ ] File association for common extensions
- [ ] Portable version (no install)
- [ ] Windows Store package

### macOS
- [ ] `.app` bundle
- [ ] Homebrew formula
- [ ] DMG installer
- [ ] Spotlight integration

### CI/CD
- [ ] GitHub Actions build pipeline
- [ ] Automated release artifacts
- [ ] Cross-platform build matrix
- [ ] Code coverage reports
- [ ] Static analysis (clang-tidy)

---

## Performance Targets

| Metric | Current | Target |
|--------|---------|--------|
| Startup | <500ms | <200ms |
| Memory (base) | 15-20 MB | <10 MB |
| File load (10MB) | ~1s | <500ms |
| UI framerate | 60 FPS | 60 FPS |
| Frame time | <16ms | <16ms |

---

## Non-Goals

Jim intentionally avoids these to stay lightweight:
- Electron or web-based architecture
- Cloud sync or telemetry
- Built-in AI code generation
- Full IDE-level project refactoring
- Jupyter notebook support

---

## Contributing

Pick an unchecked item, open a PR, and keep it focused. See the [README](README.md) for build instructions.
