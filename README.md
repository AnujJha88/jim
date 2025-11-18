# Jim

A lightweight, modern text editor built with C++ and Qt6. Designed for developers who want a fast, distraction-free editing experience with powerful features.

**Note:** This project was vibecoded - built through natural conversation and iteration with AI assistance.

**Coming Soon:** Tim - a complete rewrite written entirely from scratch, bringing even more features and performance improvements.

## Features

### Core Editing
- **Syntax Highlighting** - Support for C++, Python, and more with multi-line comment support (/* */)
- **Line Numbers** - Always visible with auto-sizing
- **Auto-Indentation** - Context-aware smart indenting
- **Auto-Pairing** - Automatic bracket (), [], {} and quote "", '' pairing with smart closing
- **Multiple Tabs** - Work on several files simultaneously with improved close buttons
- **Split View** - View two files side by side
- **Word Wrap** - Toggle line wrapping on demand
- **Mini Map** - Code overview for quick navigation (Ctrl+M)

### Navigation
- **File Tree** - Browse and open files from a sidebar (Ctrl+B)
- **Go to Line** - Jump to any line instantly (Ctrl+G)
- **Find & Replace** - Quick text search with wraparound
- **Recent Files** - Access recently opened files
- **Mini Map Navigation** - Click to jump to any part of your code

### Customization
- **Theme Switching** - Toggle between light and dark themes with proper syntax colors (Ctrl+T)
- **Custom Colors** - Personalize background and text colors with auto-adjusting text
- **Font Size Control** - Zoom in/out for comfortable reading (Ctrl+/-)
- **Status Bar** - Real-time line and column position

### Productivity
- **Command Line Support** - Open files and folders from terminal
- **Keyboard Shortcuts** - Vim-inspired workflow
- **Modern UI** - Clean, distraction-free interface with sleek dark theme
- **Performance Optimized** - Fast syntax highlighting and responsive editing

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
- `Ctrl+N` - New file
- `Ctrl+O` - Open file
- `Ctrl+Shift+O` - Open folder
- `Ctrl+S` - Save file
- `Ctrl+Shift+S` - Save as
- `Ctrl+W` - Close tab
- `Ctrl+Q` - Quit

### Editing
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Ctrl+X` - Cut
- `Ctrl+C` - Copy
- `Ctrl+V` - Paste
- `Ctrl+A` - Select all

### Search
- `Ctrl+F` - Find
- `F3` - Find next
- `Ctrl+H` - Replace
- `Ctrl+G` - Go to line

### View
- `Ctrl+B` - Toggle file tree
- `Ctrl+M` - Toggle minimap
- `Ctrl+\` - Toggle split view
- `Ctrl+T` - Toggle theme
- `Ctrl++` - Increase font size
- `Ctrl+-` - Decrease font size

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

### Latest Session
- Added multi-line comment support (/* */)
- Implemented auto-pairing for brackets and quotes
- Added minimap for code overview
- Improved tab close button visibility (red on hover)
- Fixed theme switching to properly update syntax highlighting colors
- Performance optimizations for faster editing
- Reduced input latency
- Optimized syntax highlighting and minimap rendering

## Contributing

Contributions are welcome. Please ensure code follows the existing style and test thoroughly before submitting pull requests.

## License

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
