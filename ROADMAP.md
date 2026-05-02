# Jim — Development Roadmap

> A lightweight, fast code editor built with C++ and Qt6.

## v0.0.0 — Released

**Core**: Syntax highlighting (C++, Python), line numbers, auto-indentation, bracket/quote auto-pairing, tabs, split view, minimap, word wrap, find & replace, go-to-line, recent files, file tree, themes (Light/Dark), CLI support.

---

## v0.1.0

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
- [x] Regex search with toggle
- [x] Case-sensitive / whole-word search toggles
- [x] Highlight all search matches
- [x] Replace with confirmation (one-by-one)
- [x] Search in selection

---

## v0.2.0 — Line Editing Power (COMPLETED)
Focuses on making the editor more productive for keyboard-centric developers.
- [x] **Line Manipulation:** Duplicate, move up/down, delete lines using standard shortcuts (`Ctrl+D`, `Alt+Up/Down`, `Ctrl+Shift+K`).
- [x] **Smart Navigation:** Smart Home key (jumps to first non-whitespace character, then start of line).
- [x] **Code Modification:** Toggle comment for single or multiple lines (`Ctrl+/`).
- [x] **Formatting:** Automatic trimming of trailing whitespace on save.
- [x] **UI Polish:** Custom Frameless Window Title Bar with matching dark aesthetics and draggable mechanics.

## v0.2.1 — Hex Editor & Binary Support (COMPLETED)
- [x] **Hex Editor:** Integrated hex/ASCII viewer for binary files.
- [x] **Auto-Detection:** Automatic binary file detection (checks for null bytes).
- [x] **Dual View:** Side-by-side hex and ASCII display with address column.
- [x] **Navigation:** Keyboard navigation (arrows, Page Up/Down, Home/End).
- [x] **Editing:** Hex and ASCII editing modes with Tab to switch.
- [x] **Tab Integration:** Binary files open with `[HEX]` prefix.

## v0.2.2 — Visuals & Precision (COMPLETED)
- [x] **Animation Cycler:** Matrix, Particles, Waves, and Pulse effects in a dedicated dockable panel (`Ctrl+A` to cycle).
- [x] **Hex Editor Persistence:** Save modifications back to binary files with full disk writing support.
- [x] **State Awareness:** Asterisk `*` in tab titles for unsaved changes in both text and hex editors.
- [x] **Safe Exit:** Built-in safeguards to prevent data loss on window close.
- [x] **Build Dispatcher:** Unified `GNUmakefile` for seamless switching between Linux/WSL and Windows environments.
- [x] **Smart Home:** Productivity-focused Home key behavior (first non-whitespace then start of line).

---

## v0.3.0 — Search, Replace & Navigation (COMPLETED)
Focuses on enhancing the ability to find and move through code quickly.
- [x] **Advanced Search:** Regex support, match case, match whole word in Find/Replace.
- [x] **AI Autocomplete:** Configurable AI-powered completions via OpenAI-compatible APIs.
- [x] **Inline Find Bar:** Match count, previous/next navigation, live highlight.
- [x] **Animation Cycler Expansion:** Starfield, Rain, Snow, Fire added.

---

## v0.4.0 — Heavy Tools & Visuals (COMPLETED)
- [x] **Markdown Preview:** Live split-panel rendered preview (`Ctrl+Shift+M`).
- [x] **Disassembler:** Integrated assembly view via `objdump`/`llvm-objdump` (`Ctrl+Shift+D`).
- [x] **Binary Inspector:** ELF/PE parser for headers, sections, imports (`Ctrl+Shift+I`).
- [x] **Animated Panels:** Smooth slide animations for terminal and UI elements.
- [x] **Context Menu:** File tree right-click actions for all binary tools.

---

## v0.4.5 — Precision & Intelligence (COMPLETED)
- [x] **Find All Matches:** Simultaneous highlighting with golden-accent current selection.
- [x] **Enhanced Syntax Highlighting:** Expanded rules for C++20, Python 3.12, ES2024.
- [x] **Breadcrumb Symbol Detection:** Real-time function/class detection in breadcrumbs.
- [x] **AI Provider Flexibility:** Support for Groq, OpenRouter, Together AI.
- [x] **Reveal in Explorer:** System file manager integration.

---

## v0.5.0 — The "Edgy" Feature Pack (COMPLETED)
- [x] **Multi-Cursor Editing:** `Alt+Click` to place multiple cursors and `Ctrl+D` to select next occurrence.
- [x] **Zen Mode:** Distraction-free writing with `Ctrl+Shift+Z`.
- [x] **Secret DJ Mode:** System-audio reactive visualizer in the animation dock (`Ctrl+Shift+J`).
- [x] **Typing Sounds:** Low-latency mechanical click sounds.
- [x] **Auto-Close Tags:** Smart HTML/XML tag completion.
- [x] **Inline Color Previews:** Real-time hex code swatches.
- [x] **Session Tracker:** Persisted coding time tracking in the status bar.
- [x] **Noir Edition Theme:** High-contrast grayscale theme with a lethal red accent.

---

## v0.6.0 — The "Cyberpunk" Pack (COMPLETED)
- [x] **CRT & Cyberpunk Post-Processing:** Scanlines, vignette, chromatic aberration (red/cyan edge fringe), and phosphor bloom overlay toggled via View menu.
- [x] **The Code Graveyard:** Dockable panel that automatically catches deleted code blocks (>3 lines via cut/delete/backspace) with timestamps and one-click resurrection.
- [x] ~~**Audio-Reactive Syntax Highlighting**~~ — removed (too distracting during heavy music).
- [x] **"Data Waterfall" Minimap:** Matrix-style digital rain permanently animated as the minimap background with code overview overlay on top.
- [x] **Cybernetic HUD Status Bar:** Telemetry widget with rolling hex address counter and real-time CPU/Memory sparklines (Linux: `/proc/stat` + `/proc/meminfo`).
- [x] **Kinetic / Laser Editing Mechanics:** Red laser slash + dissolving spark particles animate on the deleted line whenever `Ctrl+Shift+K` is used.
- [x] **The Neural Code Graph:** Force-directed graph visualising `#include` dependencies across the workspace (`Ctrl+Shift+N`).
- [x] **"Ghost" Replay Mode:** Watches a replay of every edit made since the file was opened, re-typing the session at 150ms/event (`Ctrl+Shift+G`).

---

## v0.7.0 — The "Hacker" Pack (COMPLETED)
- [x] **Bracket Pair Colorization:** Opening and closing `()`, `[]`, `{}` are coloured in three rotating neon colours (cyan/magenta/yellow) with depth-tracking — painted directly in `paintEvent` for zero-lag rendering.
- [x] **Vim Mode:** Full modal editing with Normal/Insert/Visual parity — `hjkl`, `w/b`, `0/$`, `G/gg`, `dd`, `yy`, `i/a/A/I/o/O/s/C`, `u`, `Ctrl+R`, `p/P`, `Ctrl+D/U`. Status-bar pill updates on every mode transition. (`Ctrl+Alt+V`)
- [x] **Keystroke Heatmap Overlay:** QWERTY keyboard rendered as a floating overlay, each key tinted from dark-grey (cold) through blue/teal/yellow to red (hot) by typing frequency. Click to dismiss. (`Ctrl+Alt+K`)
- [x] **Time-of-Day Ambient Theme:** Background tint drifts automatically every 10 minutes: dawn (warm amber) → daytime (neutral) → dusk (dusty rose) → midnight (deep indigo).
- [x] **Edit Heatmap Strip:** A 3 px wide heat strip on the left edge of the line-number gutter colour-codes every line blue→red by how many edits it has received since file open.
- [x] **Live WPM Counter:** Real-time words-per-minute counter in the Cybernetic HUD, calculated from keystrokes in a rolling 60-second window.
- [x] **Major Refactor:** `texteditor.cpp` split into `codeeditor.cpp`, `animationwidget.cpp`, `uiwidgets.cpp`, and `overlays.cpp` — each containing a focused set of classes. `SyntaxHighlighter` extracted to its own `syntaxhighlighter.cpp/.h`. `VimMode` and `KeyHeatmapOverlay` each live in their own file pairs.
- [x] **Bug fix:** DJ Mode dock no longer pushes the window bottom off-screen — window `maximumHeight` is clamped to available screen height before the dock is attached.

---

## v0.7.1 — Productivity & Ambient Pack (COMPLETED — all items shipped)
- [x] **Indent-Based Folding** — fold Python, YAML, and Markdown blocks by indentation level in addition to the existing brace-based folding for C++/JS/etc.
- [x] **Persistent Scratchpad** — a special always-available tab (`Ctrl+Alt+S`) for mid-session notes; auto-saved to disk so nothing is lost between sessions.
- [x] **Focus Fade** — All lines outside the current block dim while editing, snapping back instantly when the cursor moves. Pure `paintEvent` overlay — zero layout changes. Toggle via View menu.
- [x] **Inline Image Preview** — Hover over any string literal containing an image path (`.png`, `.jpg`, `.svg`, `.gif`, `.bmp`, `.webp`, `.ico`) to see a thumbnail tooltip; path resolved relative to the current file. Toggle via View menu.
- [x] **Session Statistics** — Tracks keystrokes, lines written, files opened, active coding time, and peak WPM across the session; displayed as a styled stats card dialog (`Tools > Session Statistics`).
- [x] **Command Palette** (`Ctrl+Shift+P`) — fuzzy-searchable frameless popup over all editor actions; arrow keys to navigate, Enter to trigger, Escape to dismiss.
- [x] **TODO/FIXME Panel** — dockable bottom panel that scans all open editors for `TODO`, `FIXME`, `HACK`, `NOTE`, `BUG` tags; colour-coded by type, click to jump to file+line.

---

## v0.8.0 — The Flow State Pack (Quality of Life)

Focusing on buttery-smooth UX, preventing papercuts, and keeping you in the zone.

### 1. Sticky Scroll (Context Headers)
- [x] As you scroll down large functions or classes, the definition line "sticks" to the top edge of the editor so you never forget what scope you are in.

### 2. Double-Shift "Search Everywhere"
- [x] Tap `Shift` twice quickly to instantly summon a unified floating search bar that queries files, symbols, command palette actions, and recent history all at once.

### 3. Smart Paste (Auto-Indent)
- [x] When pasting a block of code, automatically adjust the indentation of the entire pasted block to perfectly match the surrounding scope.

### 4. Kinetic Smooth Scrolling
- [x] Replace chunky line-by-line wheel scrolling with a buttery-smooth physics-based kinetic scroll with easing.

### 5. Auto-Save on Focus Lost
- [x] Automatically save the active file whenever the editor window loses focus or you switch to the terminal.

### 6. Invisible Character Rendering
- [x] Optionally render trailing whitespaces, mixed tabs, and zero-width characters as faint, styled red dots to catch formatting errors before committing.

### 7. Drag-and-Drop Split Panes
- [ ] *(Skipped)* Grab any tab and drag it to the left/right/bottom edge of the editor to instantly split the view visually.

### 8. Quick-Switch Header/Source (`Alt+O`)
- [x] Instantly toggle between `file.cpp` and `file.h`. Ask to create from template if it doesn't exist.

### 9. In-Line Git Blame Annotations
- [x] Toggle "Blame Mode" to render faint, greyed-out text at the end of the current line showing who last modified it and when (e.g., `Anuj, 2 days ago • "fixed null pointer"`).

### 10. Scratchpad "Send-To"
- [x] Context menu option to instantly copy highlighted code and append it to `jim_scratchpad.txt` with a timestamp without breaking flow.

### 11. Dim Inactive Panes
- [x] In Split View (`Ctrl+\`), slightly dim the pane that does not have cursor focus to provide a subconscious visual anchor.

### 12. Tear-Off Tabs (Multi-Window Support)
- [ ] *(Deferred)* Click and drag any tab outside the main Jim editor to spawn a new floating window that shares the same backend state.

### 13. "Locate Current File" in Tree
- [x] Shortcut (`Ctrl+Alt+L`) to instantly snap the File Explorer open, scroll to, and highlight the currently active file.

### 14. Visual URL Paste (Markdown/Doc Mode)
- [x] Pasting a URL over a highlighted word in Markdown formats it automatically: `[highlighted_word](https://...)`.

### 15. Hex Color Picker Pop-up
- [x] `Ctrl+Click` an inline hex code (`#FF5733`) to spawn a sleek native color wheel. Dragging updates the hex string in the code in real-time.

---

## v0.9.0 — The Web3Sec & Solidity Pack (COMPLETED)

The ultimate expansion for smart contract auditors, hackers, and security researchers.

### 1. Storage Slot Visualizer (Tetris for State Variables)
- [x] Parse state variables in the active `.sol` file and visually stack them into a 32-byte grid diagram dock.
- [x] Highlight wasted/unpacked bytes in red, and tightly packed combinations (`uint128`/`bool`) in green.
- [x] Warn on upgradeable proxy storage collisions natively (via Proxy/Implementation Diff).

### 2. Native Reentrancy Heatmap
- [x] Lightweight AST parse to detect Checks-Effects-Interactions pattern failures.
- [x] Draw a glowing red "bloodline" down the left gutter connecting external calls (`call.value()`) to vulnerable state changes that occur *after* the call.

### 3. EVM Opcode/Gas HUD
- [x] Hover over `sload`, `mstore`, `keccak256` etc. in Solidity/Yul files to see a holographic dark-mode tooltip showing exact Gas cost and description.
- [x] 30+ EVM opcodes mapped with post-EIP-2929 warm-access gas costs.

### 4. Zero-Click ABI/Bytecode Extractor
- [x] Keyboard shortcut (`Ctrl+Alt+C`) runs `solc` in the background on the current file.
- [x] Flash the screen green (Matrix style) and dump the ABI JSON or bytecode directly to the clipboard without leaving the editor.

### 5. Slither / Foundry Native Overlays
- [x] Native, asynchronous hooking into `slither` or `forge test`.
- [x] Parse JSON output and draw laser underline findings from Slither directly on editor lines.

### 6. Four-Byte Signature Resolver (Hex-Translator)
- [x] Select any 4-byte hex string (e.g., `0xa9059cbb`), hit `Ctrl+Alt+F`.
- [x] Jim hits the 4byte.directory API and renders a holographic popup showing `transfer(address,uint256)` with one-click replacement.

### 7. State-Shadowing Radar
- [x] Lightweight background scope-check to detect shadowing of state variables by local ones.
- [x] Flagged via vuln scanner with SHADOWING category and laser underline.

### 8. Precision & Decimal Expansion Tooltips
- [x] Hover over math operations involving exponents (e.g., `1e18` or `10**18`) to see a tooltip with the fully expanded numeric value (`1,000,000,000,000,000,000`).
- [x] Detect division operators occurring *before* multiplication and flag as DIV_BEFORE_MUL.

### 9. "God View" Contract Collapse (Spec-Mode)
- [x] `Ctrl+Alt+G` collapses every foldable function body, leaving only NatSpec comments, function signatures, and modifiers.
- [x] Turns massive monolithic contracts into instantly readable, high-level architectural spec documents.

### 10. Memory Pointer / Yul Tracer
- [x] Click on `0x40` (free memory pointer) or any specific memory offset in a Yul block.
- [x] Simultaneously highlight every `mload` and `mstore` that touches that exact offset with a cyan highlight.

### 11. "Time-Travel" On-Chain Trace Explorer
- [x] Paste a mainnet Tx Hash + RPC endpoint to fetch exact execution traces via `debug_traceTransaction`.
- [x] Displays formatted step-by-step opcode execution trace (PC, opcode, gas, stack top).

### 12. Gas Cost Topographic Minimap
- [x] Toggle minimap into a heat map of gas consumption (`Web3Sec > Gas Topographic Minimap`).
- [x] Unbounded loops or heavy storage writes (`sstore`, `keccak256`, `call`) glow bright red; view functions remain cool blue.

### 13. ERC Strict-Interface Enforcer
- [x] Background check against EIP specs for `is ERC20`, `is ERC721`, `is ERC1155`.
- [x] Missing required functions reported via vuln scanner with ERC_MISSING category.

### 14. Proxy/Implementation Sync Diff
- [x] Side-by-side storage slot comparison table when auditing two `.sol` files.
- [x] Collision rows highlighted red with `⚠ COLLISION` marker.

### 15. Cyber-Encoder Ring (Live JWT/Base64/Hex Decoding)
- [x] Hold `Alt` over any Base64, URL-encoded payload, or Hex to see a floating HUD with decoded plaintext.
- [x] Supports Base64, hex (`0x...`), and URL percent-encoding in the same tooltip.

### 16. "Panic Button" (Anti-Forensic Killswitch)
- [x] `Ctrl+Alt+Shift+Delete` instantly closes Jim, shreds the `jim_scratchpad.txt` file (3-pass DoD overwrite then delete), and wipes the recent files registry.

### 17. Live Binary Patching (Hex Editor)
- [x] Right-click any byte in the Hex Editor → "Patch Bytes..." or "Patch Instruction (x86)...".
- [x] Supports NOP (0x90), INT3 (0xCC), RET (0xC3), and arbitrary hex byte patching.

### 18. Regex ReDoS (Catastrophic Backtracking) Scanner
- [x] Static analysis to find RegEx Denial of Service vulnerabilities in JS/Python files.
- [x] Detects nested quantifier patterns (like `(a+)+$`) and flags with REDOS category.

## v0.10.0 — Intelligence & Ecosystem
- [ ] Snippet support (user-defined templates)
- [ ] Bracket completion for multi-line blocks
- [ ] Emmet abbreviation expansion (HTML/CSS)
- [ ] Column selection mode (`Shift+Alt+Drag`)

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
- [x] Native hex editor mode with automatic binary detection
- [x] Inspect and manipulate binary files directly without external extensions
- [ ] Value inspector (view bytes as int8, int16, int32, float, etc.)
- [ ] Split-view mode (hex + text editor side-by-side)

### 2. Hyper-Minimalist "Zen Mode"
- [x] Hide *all* UI elements (explorer, terminal, tabs, titlebar) via `Ctrl+Shift+Z`
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

## v0.10.1 — Developer Ecosystem

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

## v0.10.5 — Project Intelligence

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

## v0.10.8 — Extensibility

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

## v0.10.9 — AI & Assistance (Brainstormed)

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

## v1.0.0 — Collaborative Editing (Brainstormed)

### Real-Time Sync
- [ ] Host/Join collaborative coding sessions
- [ ] Remote cursors and selections with user nameplates
- [ ] Follow user feature (jump to their cursor)
- [ ] Live chat overlay for participants
- [ ] WebRTC peer-to-peer connection for low latency

---

## v1.1.0 — Advanced Debugging (Brainstormed)

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
