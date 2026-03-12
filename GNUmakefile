# Multi-platform Makefile Dispatcher
# Priority given to GNUmakefile by the 'make' command on Linux/WSL.

ifeq ($(OS),Windows_NT)
    # On Windows, we prefer using the PowerShell script for better Qt discovery.
all:
	powershell -ExecutionPolicy Bypass -File .\build.ps1
clean:
	if exist build rmdir /s /q build
	if exist release rmdir /s /q release
	if exist debug rmdir /s /q debug
	if exist Makefile del Makefile
	if exist Makefile.Debug del Makefile.Debug
	if exist Makefile.Release del Makefile.Release
	if exist .qmake.stash del .qmake.stash
else
    # Linux / WSL / macOS
    QMAKE := $(shell command -v qmake6 || command -v qmake)
    
all:
	@if [ -z "$(QMAKE)" ]; then \
		echo "Error: qmake6 or qmake not found."; \
		echo "Install with: sudo apt install qt6-base-dev qt6-tools-dev-tools build-essential"; \
		exit 1; \
	fi
	$(QMAKE) jim.pro && $(MAKE) -f Makefile

clean:
	@if [ -f Makefile ]; then $(MAKE) -f Makefile clean; fi
	rm -f Makefile Makefile.Debug Makefile.Release .qmake.stash jim jim.pro.user
endif

.PHONY: all clean
