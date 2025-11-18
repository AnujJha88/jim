# Jim - Technical Specifications

## Architecture

### Core Components
- **Language:** C++17
- **GUI Framework:** Qt6 (Core, Gui, Widgets)
- **Build System:** GNU Make
- **Minimum Qt Version:** 6.0

### Class Structure

#### TextEditor (Main Window)
- Manages application lifecycle
- Handles menu bar and actions
- Coordinates multiple editor instances
- Manages file operations and recent files
- Controls theme switching and UI customization

#### CodeEditor (Text Widget)
- Extends QPlainTextEdit
- Implements line number area
- Handles auto-indentation
- Manages cursor and selection
- Applies syntax highlighting themes

#### SyntaxHighlighter
- Extends QSyntaxHighlighter
- Pattern-based token recognition
- Real-time syntax coloring
- Theme-aware color schemes

#### LineNumberArea
- Custom widget for line numbers
- Auto-sizing based on line count
- Synchronized scrolling with editor

## Features Breakdown

### File Management
- Open/Save/Save As operations
- Multiple file tabs with close buttons
- Recent files menu (last 10 files)
- Folder browsing via file tree
- Command-line file/folder opening
- Unsaved changes detection

### Editor Features
- Line numbers with dynamic width
- Current line highlighting
- Auto-indentation (context-aware)
- Tab to spaces conversion (4 spaces)
- Undo/Redo support
- Cut/Copy/Paste operations
- Select all functionality

### Syntax Highlighting
Supported patterns:
- Keywords (language-specific)
- Strings (single and double quoted)
- Comments (single-line)
- Numbers (integers and decimals)
- Function names
- Class names

Languages with highlighting:
- C/C++
- Python (partial)
- Generic programming constructs

### Search & Navigation
- Find text with wraparound
- Find next occurrence
- Replace all instances
- Go to line number
- Status bar with line/column position

### UI & Theming
- Dark theme (default)
- Light theme
- Custom background colors
- Auto text color adjustment
- Modern flat design
- Smooth scrollbars
- Tab document mode

### View Options
- File tree sidebar (toggle)
- Split view (horizontal)
- Word wrap toggle
- Font size adjustment (+/-)
- Multiple tab support

## Performance Characteristics

### Memory Usage
- Base application: ~15-20 MB
- Per open file: ~1-2 MB (depends on file size)
- Syntax highlighting: Minimal overhead (real-time)

### Startup Time
- Cold start: <500ms
- With file argument: <700ms
- With folder argument: <800ms

### File Handling
- Small files (<1MB): Instant loading
- Medium files (1-10MB): <1 second
- Large files (>10MB): May experience lag in syntax highlighting

## Platform Support

### Linux
- Tested on: Debian, Ubuntu, Kali, Arch
- Dependencies: qt6-base-dev, build-essential
- Installation path: /usr/local/bin

### macOS
- Requires: Homebrew Qt6
- Native look and feel
- Retina display support

### Windows
- Requires: Qt6 MinGW or MSVC build
- Native Windows styling
- Portable executable

## Configuration

### Settings Storage
- Location: ~/.config/TextEditor/Settings.conf
- Format: Qt QSettings (INI-style)

### Saved Preferences
- Recent files list
- Font size
- Word wrap state
- Window geometry
- Last opened folder

## Build Configuration

### Compiler Flags
- `-std=c++17` - C++17 standard
- `-Wall -Wextra` - All warnings
- `-O2` - Optimization level 2
- `-fPIC` - Position independent code

### Qt Modules Required
- QtCore - Core functionality
- QtGui - GUI components
- QtWidgets - Widget toolkit

### Build Artifacts
- Object files: *.o
- MOC files: moc_*.cpp
- Executable: jim (or jim.exe)

## Code Metrics

### Lines of Code
- Header files: ~200 lines
- Implementation: ~1200 lines
- Total: ~1400 lines

### File Structure
```
jim/
├── main.cpp              # Entry point
├── texteditor.h          # Main window header
├── texteditor.cpp        # Main window implementation
├── linenumberarea.h      # Line numbers header
├── linenumberarea.cpp    # Line numbers implementation
├── Makefile              # Build configuration
├── README.md             # Documentation
├── SPECS.md              # This file
└── .gitignore            # Git ignore rules
```

## Dependencies

### Runtime Dependencies
- Qt6Core
- Qt6Gui
- Qt6Widgets
- System C++ runtime

### Build Dependencies
- g++ or clang (C++17 support)
- Qt6 development files
- moc (Meta-Object Compiler)
- make

## Known Limitations

### Current Constraints
- Single-line comment support only (no multi-line)
- Limited language support for syntax highlighting
- No plugin system
- No regex search
- No code folding
- No minimap
- No bracket auto-completion
- No multiple cursors
- No Git integration

### Performance Limitations
- Large files (>50MB) may cause UI lag
- Syntax highlighting not optimized for huge files
- No virtual scrolling for massive documents

## Security Considerations

### File Operations
- No automatic file execution
- Read/write permissions respected
- No network operations
- No external process spawning

### User Data
- Settings stored locally only
- No telemetry or analytics
- No cloud synchronization
- No automatic updates

## Accessibility

### Current Support
- Keyboard navigation
- Standard shortcuts
- High contrast themes available
- Resizable fonts

### Not Yet Implemented
- Screen reader support
- Voice control
- Accessibility API integration

## Future Compatibility

### Planned Qt6 Features
- Qt6 LTS support
- Modern OpenGL rendering
- Wayland support (Linux)
- High DPI scaling improvements

### Backward Compatibility
- Settings migration from older versions
- File format compatibility maintained
- No breaking changes in minor versions
