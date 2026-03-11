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

## v1.2 — Line Editing Power (COMPLETED)
Focuses on making the editor more productive for keyboard-centric developers.
- [x] **Line Manipulation:** Duplicate, move up/down, delete lines using standard shortcuts (`Ctrl+D`, `Alt+Up/Down`, `Ctrl+Shift+K`).
- [x] **Smart Navigation:** Smart Home key (jumps to first non-whitespace character, then start of line).
- [x] **Code Modification:** Toggle comment for single or multiple lines (`Ctrl+/`).
- [x] **Formatting:** Automatic trimming of trailing whitespace on save.
- [x] **UI Polish:** Custom Frameless Window Title Bar with matching dark aesthetics and draggable mechanics.

## v1.3 — Search, Replace & Navigation (Planned)
Focuses on enhancing the ability to find and move through code quickly.
- [ ] **Advanced Search:** Regex support, match case, match whole word in Find/Replace.
- [ ] **Find in Files:** Workspace-wide search (`Ctrl+Shift+F`) with an interactive results panel.
- [ ] **Go to Symbol:** Quick navigation to classes/functions within the current file (`Ctrl+Shift+O`).
- [ ] **Command Palette:** Centralized palette to execute any editor action (`Ctrl+Shift+P`).

## v1.5 — Multi-Cursor & Intelligence
- [ ] Snippet support (user-defined templates)
- [ ] Bracket completion for multi-line blocks
- [ ] Emmet abbreviation expansion (HTML/CSS)
- [ ] Multi-cursor selection (`Alt+Click`)
- [ ] Select next occurrence (`Ctrl+D` mapping update)
- [ ] Column selection mode (`Shift+Alt+Drag`)
- [ ] Smart auto-closing tags (HTML/XML)

### Typography & Fonts
- [ ] Configurable font family and fallback fonts
- [ ] Font ligature support (e.g., Fira Code, Cascadia Code)
- [ ] Dynamic font size configuration (`Ctrl+MouseWheel`)
- [ ] Custom line height and letter spacing adjustments

### Bookmarks
- [ ] Toggle bookmark (`Ctrl+F2`)
- [ ] Jump to next/previous bookmark
- [ ] Bookmark panel (list all bookmarks)
- [ ] Named bookmarks

### Indent Guides
- [ ] Vertical indent guide lines
- [ ] Active indent guide highlighting
---

## Unique Differentiators (The Native Advantage)

Since Jim is a dynamically compiled native C++ application (unlike memory-heavy Electron editors like VS Code), we can leverage unique performance and system-level features to stand out:


### 1. Built-In Hex Editor & Binary Analysis
- [ ] Native, split-view hex editor mode
- [ ] Inspect and manipulate binary files directly without external extensions
- [ ] Value inspector (view bytes as int8, int16, int32, float, etc.)

### 2. Hyper-Minimalist "Zen Mode"
- [ ] Press `F11` to hide *all* UI elements (explorer, terminal, tabs, titlebar)
- [ ] Seamless edge-fading of text for pure distraction-free algorithmic thinking
- [ ] Subtle, non-intrusive block cursor

### 3. Hardware-Accelerated Architecture Visualizer
- [ ] Utilize `Qt3D` or `OpenGL` to generate a 3D interactive dependency graph of the current project
- [ ] Fly through your codebase to visually understand complex includes and module hierarchies natively

### 4. Integrated Disassembler & Executable Inspector
- [ ] For C/C++ projects, right-click a function to instantly see its assembly output (via `objdump`/`llvm-objdump`)
- [ ] Inspect executable headers (ELF/PE) and symbol tables directly in the editor

### 5. Native Build & Resource Profiler
- [ ] Real-time visualization of CPU/Memory/IO usage for your build processes and sub-processes
- [ ] Visual build timeline to identify bottlenecks in your compilation pipeline natively without external tools

### 6. "Ghost" Debugger Overlays
- [ ] High-performance inline variable value overlays (ghost text) during active debug sessions
- [ ] Observe data flow through loops and functions without needing to hover or use a separate watch panel


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

---

## v3.5 — AI & Assistance (Brainstormed)

### Generative AI Integration
- [ ] Inline code completions (Ghost text)
- [ ] Chat panel for project-aware questions
- [ ] Explain code block feature
- [ ] Generate unit tests from selection
- [ ] Automatic commit message generation

### Advanced Refactoring
- [ ] Extract method/function logic
- [ ] Extract variable/constant
- [ ] Inline variable
- [ ] Smart symbol rename across project boundaries

---

## v4.0 — Collaborative Editing (Brainstormed)

### Real-Time Sync
- [ ] Host/Join collaborative coding sessions
- [ ] Remote cursors and selections with user nameplates
- [ ] Follow user feature (jump to their cursor)
- [ ] Live chat overlay for participants
- [ ] WebRTC peer-to-peer connection for low latency

---

## v4.5 — Advanced Debugging (Brainstormed)

### Integrated Debugger Protocol (DAP)
- [ ] Breakpoint toggling in gutter
- [ ] Conditional breakpoints and logpoints
- [ ] Call stack panel
- [ ] Variable and Watch inspector
- [ ] Step over, Step into, Step out controls
- [ ] Hover over variables to inspect state during active debug sessions

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
