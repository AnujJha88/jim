# Jim

A lightweight, native code editor built with C++ and Qt6. Fast, distraction-free, and genuinely useful for developers and researchers who live in their editor.

**Note:** This project was vibecoded — built through natural conversation and iteration with AI assistance.

## Screenshots

<p align="center">
  <img src="Screenshot 2026-03-14 205007.png" width="800" alt="Welcome Screen">
  <br>
  <em>Welcome Screen</em>
</p>

<p align="center">
  <img src="Screenshot 2026-03-14 205023.png" width="800" alt="File Editor and Tree">
  <br>
  <em>File Editor and Tree</em>
</p>

<p align="center">
  <img src="Screenshot 2026-03-14 205031.png" width="800" alt="Animation dock and file explorer">
  <br>
  <em>Animation dock and file explorer</em>
</p>

## Features

### Core Editing
- **Syntax Highlighting** — 11 languages with advanced rules: C/C++ (C++20 & Qt), Python, JavaScript/TypeScript (ES2024), HTML5, CSS3, Rust, Go, JSON, YAML, Markdown, Assembly
- **Language Auto-Detection** — Automatically applies correct highlighting based on file extension
- **Code Folding** — Collapse/expand functions, classes, and blocks with gutter indicators
- **Line Numbers** — Always visible with auto-sizing and current-line highlight
- **Auto-Indentation** — Context-aware smart indenting
- **Auto-Pairing** — Automatic bracket `()`, `[]`, `{}` and quote `""`, `''` pairing with smart closing
- **Multiple Tabs** — Work on several files simultaneously with modification tracking
- **Split View** — View two files side by side (`Ctrl+\`)
- **Word Wrap** — Toggle line wrapping on demand
- **Mini Map** — Code overview with viewport indicator (`Ctrl+M`)
- **Smart Home Key** — Jumps to first non-whitespace character, then start of line
- **Multi-cursor editing** — `Alt+Click` to place arbitrary cursors, `Ctrl+D` to select next occurrence (`v1.6.0`)
- **Auto-close HTML/XML tags** — Detects `</` and auto-completes the closing tag (`v1.6.0`)
- **Color preview** — Show a small color swatch next to hex color codes (`#FF5733`) inline (`v1.6.0`)

### Navigation
- **File Tree** — Browse and open files from a sidebar (`Ctrl+B`)
- **Breadcrumb Navigation** — Shows `folder > file > function` context below the tab bar
- **Go to Line** — Jump to any line instantly (`Ctrl+G`)
- **Find & Replace** — Inline find bar with "Find All Matches" (simultaneous highlighting), match count, and golden-accent current selection
- **Recent Files** — Access recently opened files from the File menu
- **Mini Map Navigation** — Click anywhere to jump to that part of your code

### Developer Tools
- **Integrated Terminal** — Embedded shell panel with animated slide-in/out (`Ctrl+\``)
- **Hex Editor** — Built-in binary file viewer/editor: hex + ASCII display, keyboard navigation, full save support, modification tracking
- **Disassembler** — Native disassembly via `objdump`/`llvm-objdump` with a parsed function list; click any function to jump to it (`Ctrl+Shift+D`)
- **Binary Inspector** — Pure native ELF and PE parser: headers, sections, imports/symbols, MD5 hashing, and string extraction — no external tools required (`Ctrl+Shift+I`)
- **Markdown Preview** — Live split-panel rendered preview with full dark theme, tables, code blocks, images, and task lists (`Ctrl+Shift+M`)
- **File Watcher** — Detects external file changes and prompts to reload
- **AI Autocomplete** — Configurable AI-powered completions via any OpenAI-compatible API; explicit support for Groq, OpenRouter, and Together AI (Plugins menu)

### Binary Analysis Suite
Jim has a native binary analysis workflow requiring no external tools:

| Tool | Trigger | What it does |
|------|---------|--------------|
| Hex Editor | Auto on binary open | View/edit raw bytes |
| Disassembler | `Ctrl+Shift+D` | Disassemble via objdump, parsed function list |
| Binary Inspector | `Ctrl+Shift+I` | ELF/PE headers, sections, imports, strings |

Right-click any file in the Explorer to access all three tools directly.

### Visuals & Effects
- **Animation Cycler** — Matrix, Particles, Waves, Pulse, Starfield, Rain, Snow, Fire, **DJ Mode** (`Ctrl+Shift+A` / `Ctrl+Shift+J`)
- **Themes** — Light, Dark, Monokai, and **Noir Edition** (high-contrast grayscale)
- **Typing Sounds** — Satisfying mechanical keyboard click sounds on each keystroke (Toggle in View menu)
- **Animated Panels** — Smooth slide animations for terminal, fade for welcome screen, flash on tab open
- **Custom Title Bar** — Frameless window with minimize, maximize/restore, and close

### Productivity
- **Line Manipulation** — Duplicate (`Ctrl+D`), Move Up/Down (`Alt+↑/↓`), Delete (`Ctrl+Shift+K`), Toggle Comment (`Ctrl+/`)
- **Trailing Whitespace** — Automatically trimmed on save
- **Command Line Support** — Open files and folders from terminal
- **Comprehensive Shortcuts** — All common operations keyboard-accessible

---

## Installation

### Prerequisites

**Debian / Ubuntu / Kali:**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev-tools qt6-multimedia-dev build-essential
```

**Arch Linux:**
```bash
sudo pacman -S qt6-base qt6-tools
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-devel gcc-c++ make
```

**macOS:**
```bash
brew install qt@6
```

**Windows:**
No prerequisites beyond Qt6 — use the PowerShell build script below. The build script auto-locates Qt6 and MinGW.

> **Note:** The integrated terminal on Linux/macOS uses a simple QProcess-based shell. There is no dependency on `qtermwidget` or any external terminal library — the terminal works out of the box on all platforms.

---

## Build and Run

### Linux / macOS / WSL

```bash
git clone https://github.com/AnujJha88/jim.git
cd jim
make
sudo make install   # installs to /usr/local/bin
```

To uninstall:
```bash
sudo make uninstall
```

### Windows (Native)

```powershell
git clone https://github.com/AnujJha88/jim.git
cd jim
.\build.ps1
.\release\jim.exe
```

The `build.ps1` script automatically:
1. Locates Qt6 and MinGW under `C:\Qt\`
2. Runs `qmake` to generate the Makefile
3. Compiles with `mingw32-make`
4. Runs `windeployqt` to bundle all Qt DLLs alongside the executable

---

## Usage

```bash
jim                        # open editor (welcome screen)
jim myfile.cpp             # open a specific file
jim .                      # open current directory as project
jim /path/to/project       # open a specific folder
```

---

## Keyboard Shortcuts

### File
| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+Shift+O` | Open folder |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+W` | Close tab |
| `Ctrl+Q` | Quit |

### Editing
| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+X` | Cut |
| `Ctrl+C` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+A` | Select all |
| `Ctrl+D` | Duplicate line |
| `Alt+↑` | Move line up |
| `Alt+↓` | Move line down |
| `Ctrl+Shift+K` | Delete line |
| `Ctrl+/` | Toggle line comment |
| `Home` | Smart home (first non-whitespace → start of line) |

### Search
| Shortcut | Action |
|----------|--------|
| `Ctrl+F` | Find (inline bar) |
| `F3` | Find next |
| `Shift+F3` | Find previous |
| `Ctrl+H` | Replace |
| `Ctrl+G` | Go to line |

### View
| Shortcut | Action |
|----------|--------|
| `Ctrl+B` | Toggle file explorer |
| `Ctrl+M` | Toggle mini map |
| `` Ctrl+` `` | Toggle terminal |
| `Ctrl+\` | Toggle split view |
| `Ctrl+T` | Cycle themes (Light, Dark, Monokai, Noir Edition) |
| `Ctrl++` | Increase font size |
| `Ctrl+-` | Decrease font size |
| `Ctrl+Shift+M` | Toggle Markdown preview |
| `Ctrl+Shift+Z` | **Zen Mode** (Hide all UI chrome) |
| `Ctrl+Shift+J` | **DJ Mode** (Audio Visualizer) |

### Tools
| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+H` | Open current file in Hex Editor |
| `Ctrl+Shift+D` | Disassemble file |
| `Ctrl+Shift+I` | Open Binary Inspector |

---

## Configuration

Settings are automatically persisted across sessions:

| Setting | Default |
|---------|---------|
| Font size | 11pt |
| Word wrap | Off |
| Recent files | Last 10 |
| Window geometry | Restored |

Config location: `~/.config/TextEditor/Settings.conf` (Linux/macOS) or the registry (Windows via QSettings).

---

## Recent Updates

### v1.6.0 (Current)
- Added **Multi-cursor Editing** — `Alt+Click` to place multiple cursors; `Ctrl+D` now selects the next occurrence of the current word for simultaneous editing.
- Added **Secret DJ Mode** — An edgy system-audio reactive visualizer built into the animation dock (`Ctrl+Shift+J`).
- Added **Zen Mode** — Pure distraction-free writing: hides the explorer, terminal, tabs, and status bar instantly (`Ctrl+Shift+Z`).
- Added **Typing Sounds** — satisfying, low-latency mechanical click sounds as you type (Toggle via View menu).
- Added **Noir Edition** — A lethal high-contrast grayscale theme with a single red accent for the cursor.
- Added **Auto-close Tags** — Smart detection of `</` triggers automatic closing of HTML/XML tags.
- Added **Inline Color Previews** — Real-time color swatches rendered next to hex codes (`#RRGGBB`).
- Added **Session Time Tracker** — Status bar shows total coding time, persisted across sessions.

### v1.5.0
- Added **Find All Matches** — simultaneous highlighting of all search matches with a golden accent for the current choice; updates in real-time as you navigate.
- Added **Enhanced Syntax Highlighting** — significantly expanded keyword sets for C++20, Python 3.12, JavaScript/TypeScript, Rust, and Go.
- Added **Breadcrumb Symbol Detection** — Real-time regex-based function and class detection in the breadcrumb bar.

### v1.4.0
- Added **Markdown Preview** — live split-panel rendered preview (`Ctrl+Shift+M`). Supports headers, tables, code blocks with language labels, images, task lists, blockquotes, strikethrough, highlights, and raw HTML passthrough. Updates with 400 ms debounce as you type. No external libraries.
- Added **Disassembler** — wraps `objdump`/`llvm-objdump`, displays assembly with full syntax highlighting, parsed function list with click-to-jump (`Ctrl+Shift+D`)
- Added **Binary Inspector** — pure native ELF and PE parser (`Ctrl+Shift+I`): headers, sections, imports, MD5 hash, extracted strings. Handles ELF32/64 LE/BE and PE32/PE32+
- Added **Tools menu** with all binary analysis actions
- Added **right-click context menu** on file tree: Open, Hex Editor, Disassemble, Binary Inspector, Reveal in Explorer
- Added **animated terminal** — smooth slide up/down with easing curve (replaces instant show/hide)
- Added **welcome screen fade** — opacity animation on show/hide
- Added **tab flash** — accent colour flash when a new tab opens

### v1.3.0
- Added **AI Autocomplete** — configurable via any OpenAI-compatible API (Plugins menu)
- Added **inline Find bar** — shows match count, previous/next navigation, live highlight
- Added **Animation Cycler** expanded — Starfield, Rain, Snow, Fire added to Matrix/Particles/Waves/Pulse

### v1.2.2
- Added **Animation Cycler** (`Ctrl+Shift+A`)
- Improved **Hex Editor** — full save support, modification tracking (asterisk in tab)
- Enhanced **build system** — unified `GNUmakefile`, improved `build.ps1`
- Added **Smart Home Key**

### v1.2.1
- Added **Hex Editor** — automatic binary detection, integrated hex/ASCII viewer

### v1.2.0
- Added **Line Editing Power**: Duplicate, Move Up/Down, Delete, Toggle Comment
- Added **Trailing Whitespace** removal on save
- Added **custom frameless title bar**

### v1.1.0
- Welcome screen, Breadcrumb navigation, Integrated terminal
- Code folding, 11-language syntax highlighting, Language auto-detection
- File watcher, Monokai theme, modernised VS Code-inspired UI

### v1.0.0
- Multi-tab editor, syntax highlighting, auto-pairing, minimap, split view

---

## Contributing

Contributions welcome. See [ROADMAP.md](ROADMAP.md) for planned features. Keep PRs focused and match the existing code style.

## License

MIT License — see [LICENSE](LICENSE) for details.