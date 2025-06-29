UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
UNAME_M := $(shell uname -m 2>/dev/null || echo unknown)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
LDFLAGS = 
SRCDIR = src
OBJDIR = obj
BINDIR = bin

ifeq ($(UNAME_S),Linux)
    ifneq ($(shell command -v termux-info 2>/dev/null),)
        PLATFORM = android
        # In Termux, PREFIX is usually set by the environment
        # If not set, use a default path
        ifeq ($(PREFIX),)
            PREFIX = /data/data/com.termux/files/usr
        endif
        INSTALL_DIR = $(PREFIX)/bin
    else
        PLATFORM = linux
        INSTALL_DIR = /usr/local/bin
    endif
    TARGET = $(BINDIR)/lizard
    MKDIR = mkdir -p
    RM = rm -rf
    CP = cp
endif

ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    TARGET = $(BINDIR)/lizard
    INSTALL_DIR = /usr/local/bin
    MKDIR = mkdir -p
    RM = rm -rf
    CP = cp
endif

ifeq ($(UNAME_S),MINGW32_NT-*)
    PLATFORM = windows
    TARGET = $(BINDIR)/lizard.exe
    INSTALL_DIR = /usr/local/bin
    MKDIR = mkdir -p
    RM = rm -rf
    CP = cp
endif

ifeq ($(UNAME_S),MINGW64_NT-*)
    PLATFORM = windows
    TARGET = $(BINDIR)/lizard.exe
    INSTALL_DIR = /usr/local/bin
    MKDIR = mkdir -p
    RM = rm -rf
    CP = cp
endif

ifeq ($(UNAME_S),MSYS_NT-*)
    PLATFORM = windows
    TARGET = $(BINDIR)/lizard.exe
    INSTALL_DIR = /usr/local/bin
    MKDIR = mkdir -p
    RM = rm -rf
    CP = cp
endif

ifeq ($(UNAME_S),Windows)
    PLATFORM = windows
    TARGET = $(BINDIR)/lizard.exe
    INSTALL_DIR = /usr/local/bin
    MKDIR = mkdir
    RM = rmdir /s /q
    CP = copy
    OBJDIR_WIN = obj
    BINDIR_WIN = bin
endif

# Source and object files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Default target
.PHONY: all clean install install-user uninstall uninstall-user examples run debug help platform-info

all: platform-info $(TARGET)

platform-info:
	@echo "=== Lizard Build Information ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Architecture: $(UNAME_M)"
	@echo "Compiler: $(CC)"
	@echo "Target: $(TARGET)"
	@echo "Install Directory: $(INSTALL_DIR)"
	@echo "================================"

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Lizard interpreter built successfully for $(PLATFORM)!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

$(BINDIR):
	$(MKDIR) $(BINDIR)

clean:
	@echo "Cleaning build files..."
ifeq ($(PLATFORM),windows)
	@if exist $(OBJDIR_WIN) $(RM) $(OBJDIR_WIN) 2>nul || echo.
	@if exist $(BINDIR_WIN) $(RM) $(BINDIR_WIN) 2>nul || echo.
else
	$(RM) $(OBJDIR) $(BINDIR)
endif
	@echo "Clean completed."

install: $(TARGET)
	@echo "Installing Lizard to $(INSTALL_DIR)..."
ifeq ($(PLATFORM),android)
	@if [ ! -d "$(INSTALL_DIR)" ]; then mkdir -p "$(INSTALL_DIR)"; fi
	$(CP) $(TARGET) $(INSTALL_DIR)/
	@chmod +x $(INSTALL_DIR)/lizard
	@echo "Lizard installed to $(INSTALL_DIR)/"
	@echo "Make sure $(INSTALL_DIR) is in your PATH"
else ifeq ($(PLATFORM),windows)
	$(CP) $(TARGET) $(INSTALL_DIR)
	@echo "Lizard installed to $(INSTALL_DIR)/"
else
	@sudo $(CP) $(TARGET) $(INSTALL_DIR)/
	@echo "Lizard installed to $(INSTALL_DIR)/"
endif

install-user: $(TARGET)
	@echo "Installing Lizard to user directory..."
ifeq ($(PLATFORM),android)
	$(CP) $(TARGET) $(HOME)/.local/bin/ || $(CP) $(TARGET) $(PREFIX)/bin/
else
	@mkdir -p $(HOME)/.local/bin
	$(CP) $(TARGET) $(HOME)/.local/bin/
endif
	@echo "Lizard installed to user directory. Make sure it's in your PATH."

uninstall:
	@echo "Uninstalling Lizard from $(INSTALL_DIR)..."
ifeq ($(PLATFORM),android)
	@if [ -f "$(INSTALL_DIR)/lizard" ]; then \
		rm -f "$(INSTALL_DIR)/lizard" && echo "Lizard uninstalled from $(INSTALL_DIR)/"; \
	else \
		echo "Lizard not found in $(INSTALL_DIR)/"; \
	fi
else ifeq ($(PLATFORM),windows)
	@if exist "$(INSTALL_DIR)\lizard.exe" ( \
		del "$(INSTALL_DIR)\lizard.exe" && echo Lizard uninstalled from $(INSTALL_DIR)/ \
	) else ( \
		echo Lizard not found in $(INSTALL_DIR)/ \
	)
else
	@if [ -f "$(INSTALL_DIR)/lizard" ]; then \
		sudo rm -f "$(INSTALL_DIR)/lizard" && echo "Lizard uninstalled from $(INSTALL_DIR)/"; \
	else \
		echo "Lizard not found in $(INSTALL_DIR)/"; \
	fi
endif

uninstall-user:
	@echo "Uninstalling Lizard from user directory..."
ifeq ($(PLATFORM),android)
	@if [ -f "$(HOME)/.local/bin/lizard" ]; then \
		rm -f "$(HOME)/.local/bin/lizard" && echo "Lizard uninstalled from $(HOME)/.local/bin/"; \
	elif [ -f "$(PREFIX)/bin/lizard" ]; then \
		rm -f "$(PREFIX)/bin/lizard" && echo "Lizard uninstalled from $(PREFIX)/bin/"; \
	else \
		echo "Lizard not found in user directories"; \
	fi
else ifeq ($(PLATFORM),windows)
	@if exist "$(HOME)\.local\bin\lizard.exe" ( \
		del "$(HOME)\.local\bin\lizard.exe" && echo Lizard uninstalled from user directory \
	) else ( \
		echo Lizard not found in user directory \
	)
else
	@if [ -f "$(HOME)/.local/bin/lizard" ]; then \
		rm -f "$(HOME)/.local/bin/lizard" && echo "Lizard uninstalled from $(HOME)/.local/bin/"; \
	else \
		echo "Lizard not found in $(HOME)/.local/bin/"; \
	fi
endif

examples: $(TARGET)
	@echo "Running examples..."
	@if [ -f examples/hello.lz ]; then ./$(TARGET) examples/hello.lz; else echo "examples/hello.lz not found"; fi
	@if [ -f examples/main.lz ]; then ./$(TARGET) examples/main.lz; else echo "examples/main.lz not found"; fi
	
run: $(TARGET)
ifndef FILE
	@echo "Usage: make run FILE=path/to/file.lz"
else
	./$(TARGET) $(FILE)
endif

debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)

static: LDFLAGS += -static
static: $(TARGET)
	@echo "Static build completed!"

dev: CFLAGS += -Wpedantic -Wshadow -Wconversion -Wcast-align -Wstrict-prototypes
dev: debug

release: CFLAGS = -Wall -Wextra -std=c99 -O3 -DNDEBUG
release: clean $(TARGET)
	@echo "Release build completed!"

test-install:
	@echo "Testing installation..."
	@which lizard > /dev/null && echo "Lizard found in PATH" || echo "Lizard not found in PATH"
	@lizard --version 2>/dev/null || echo "Could not run lizard --version"

help:
	@echo "=== Lizard Programming Language Build System ==="
	@echo "Targets:"
	@echo "  all           - Build the interpreter (default)"
	@echo "  clean         - Clean build files"
	@echo "  install       - Install to system directory"
	@echo "  install-user  - Install to user directory"
	@echo "  uninstall     - Uninstall from system directory"
	@echo "  uninstall-user- Uninstall from user directory"
	@echo "  examples      - Run example files"
	@echo "  run FILE=...  - Run specific file"
	@echo "  debug         - Build with debug symbols"
	@echo "  static        - Build static binary"
	@echo "  dev           - Development build with extra warnings"
	@echo "  release       - Optimized release build"
	@echo "  test-install  - Test if installation works"
	@echo "  platform-info - Show platform information"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Platform: $(PLATFORM)"
	@echo "Current target: $(TARGET)"
	@echo "================================================="