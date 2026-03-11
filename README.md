# Jim

A lightweight, modern code editor built with C++ and Qt6. Designed for developers who want a fast, distraction-free editing experience with powerful features.

**Note:** This project was vibecoded — built through natural conversation and iteration with AI assistance.

## Features

### Core Editing
- **Syntax Highlighting** — 11 languages: C/C++, Python, JavaScript/TypeScript, HTML, CSS, Rust, Go, JSON, YAML, Markdown
- **Language Auto-Detection** — Automatically applies correct highlighting based on file extension
- **Code Folding** — Collapse/expand functions, classes, and blocks with gutter indicators
- **Line Numbers** — Always visible with auto-sizing and current-line highlight
- **Auto-Indentation** — Context-aware smart indenting
- **Auto-Pairing** — Automatic bracket (), [], {} and quote "", '' pairing with smart closing
- **Multiple Tabs** — Work on several files simultaneously
- **Split View** — View two files side by side
- **Word Wrap** — Toggle line wrapping on demand
- **Mini Map** — Code overview for quick navigation (Ctrl+M)

### Navigation
- **File Tree** — Browse and open files from a sidebar (Ctrl+B)
- **Breadcrumb Navigation** — Shows `folder > file > function` context below the tab bar
- **Go to Line** — Jump to any line instantly (Ctrl+G)
- **Find & Replace** — Quick text search with wraparound
- **Recent Files** — Access recently opened files
- **Mini Map Navigation** — Click to jump to any part of your code

### Developer Tools
- **Integrated Terminal** — Embedded shell panel with command execution (Ctrl+`)
- **File Watcher** — Detects external file changes and prompts to reload
- **Welcome Screen** — Quick access to recent files and actions on launch

### Customization
- **3 Themes** — Light, Dark, and Monokai with proper syntax colors (Ctrl+T)
- **Multiple Color Themes:** Built-in Light, Dark, and Monokai themes. easily switchable.
- **Custom Colors** — Personalize background and text colors with auto-adjusting text
- **Font Size Control** — Zoom in/out for comfortable reading (Ctrl+/-)
- **Status Bar** — Real-time line/column position and language indicator
- **Custom Modern Title Bar:** A sleek, frameless window title bar with custom minimize, maximize/restore, and close buttons on Windows/Linux environments.

### Productivity
- **Command Line Support** — Open files and folders from terminal
- **Keyboard Shortcuts** — Comprehensive shortcut system
- **Modern UI** — VS Code-inspired dark interface with transparent scrollbars
- **Performance Optimized** — Fast syntax highlighting and responsive editing

## Installation

### Prerequisites

**Debian/Ubuntu/Kali:**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev-tools build-essential
```

**Arch Linux:**
```bash
sudo pacman -S qt6-base
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-devel gcc-c++ make
```

**macOS:**
```bash
brew install qt@6
```

### Build and Install

```bash
make
sudo make install
```

### Uninstall

```bash
sudo make uninstall
```

## Usage

Launch the editor:
```bash
jim
```

Open a specific file:
```bash
jim myfile.cpp
```

Open current directory:
```bash
jim .
```

Open a specific folder:
```bash
jim /path/to/project
```

## Keyboard Shortcuts

### File Operations
| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+Shift+O` | Open folder |
| `Ctrl+S` | Save file |
| `Ctrl+Shift+S` | Save as |
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

### Search
| Shortcut | Action |
|----------|--------|
| `Ctrl+F` | Find |
| `F3` | Find next |
| `Ctrl+H` | Replace |
| `Ctrl+G` | Go to line |

### View
| Shortcut | Action |
|----------|--------|
| `Ctrl+B` | Toggle file tree |
| `Ctrl+M` | Toggle minimap |
| `` Ctrl+` `` | Toggle terminal |
| `Ctrl+\` | Toggle split view |
| `Ctrl+T` | Cycle themes |
| `Ctrl++` | Increase font size |
| `Ctrl+-` | Decrease font size |

## Building from Source

### Requirements
- Qt6 (Core, Gui, Widgets modules)
- C++17 compatible compiler
- Make

### Compile
```bash
git clone https://github.com/AnujJha88/jim.git
cd jim
make
```

### Development Build
```bash
make clean
make
./jim
```

## Configuration

Settings are automatically saved and include:
- Recent files list
- Font size preference
- Word wrap state
- Window geometry

Configuration is stored in `~/.config/TextEditor/Settings.conf`


## Recent Updates

### v1.2
- Added **Line Editing Power**: Duplicate (`Ctrl+D`), Move Up/Down (`Alt+Up/Down`), Delete (`Ctrl+Shift+K`), and Toggle Comments (`Ctrl+/`).
- Configured a Smart Home Key to navigate to the first non-whitespace block.
- Added automatic Trailing Whitespace removal upon saving files.
- Replaced standard OS window decorations with a custom Frameless Title Bar (minimize, maximize, close).
- Added a VS Code-style "Open Folder" Empty State to the File Explorer when initialized without arguments.
- Fixed layout to allow `QMenuBar` rendering below the custom Title Bar.

### v1.1
- Added Welcome Screen with recent files and quick-action buttons
- Added Breadcrumb Navigation (folder > file > function)
- Added Integrated Terminal panel (Ctrl+`)
- Added Code Folding with clickable gutter indicators
- Added syntax highlighting for 9 new languages: JavaScript/TypeScript, HTML, CSS, Rust, Go, JSON, YAML, Markdown
- Added Language Auto-Detection from file extension
- Added File Watcher — detects external changes and prompts to reload
- Added Monokai theme (3 themes total)
- Modernized UI — VS Code-inspired styling, transparent scrollbars, rounded menus
- Language indicator in status bar

### v1.0
- Added multi-line comment support (/* */)
- Implemented auto-pairing for brackets and quotes
- Added minimap for code overview
- Improved tab close button visibility (red on hover)
- Fixed theme switching to properly update syntax highlighting colors
- Performance optimizations for faster editing
- Reduced input latency
- Optimized syntax highlighting and minimap rendering

## Contributing

Contributions are welcome. See the [ROADMAP](ROADMAP.md) for planned features. Please ensure code follows the existing style and test thoroughly before submitting pull requests.

## License

MIT License — see [LICENSE](LICENSE) for details.
