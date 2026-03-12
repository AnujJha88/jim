# Jim Multi-Platform Makefile
# Detects OS and routes to appropriate build system

ifeq ($(OS),Windows_NT)
    # Windows-specific logic
    BUILD_CMD = powershell -ExecutionPolicy Bypass -File .\build.ps1
    CLEAN_CMD = if exist release rmdir /s /q release && if exist debug rmdir /s /q debug
    RUN_CMD = .\release\jim.exe
else
    # Linux/Unix-specific logic
    QMAKE = $(shell which qmake6 || which qmake)
    ifeq ($(QMAKE),)
        BUILD_CMD = @echo "Error: qmake or qmake6 not found. Please install Qt6 (e.g., sudo apt install qt6-base-dev)"; exit 1
    else
        BUILD_CMD = $(QMAKE) jim.pro && $(MAKE) -f Makefile
    endif
    CLEAN_CMD = $(MAKE) -f Makefile clean || true && rm -rf jim *.o moc_* .qmake.stash
    RUN_CMD = ./jim
    INSTALL_DIR = /usr/local/bin
endif

.PHONY: all clean run install uninstall

all:
	$(BUILD_CMD)

clean:
	$(CLEAN_CMD)

run:
	$(RUN_CMD)

install:
ifeq ($(OS),Windows_NT)
	@echo "Install is not supported via Makefile on Windows. Use build.ps1."
else
	cp jim $(INSTALL_DIR)/jim
	chmod +x $(INSTALL_DIR)/jim
	@echo "Jim installed to $(INSTALL_DIR)/jim"
endif

uninstall:
ifeq ($(OS),Windows_NT)
	@echo "Uninstall is not supported via Makefile on Windows."
else
	rm -f $(INSTALL_DIR)/jim
	@echo "Jim uninstalled from $(INSTALL_DIR)/jim"
endif
