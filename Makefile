CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
SRCDIR = src
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/lizard

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean install examples

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@
	@echo "Lizard interpreter built successfully!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "Lizard installed to /usr/local/bin/"

examples: $(TARGET)
	@echo "Running examples..."
	./$(TARGET) examples/hello.lz
	./$(TARGET) examples/main.lz

run: $(TARGET)
	./$(TARGET) $(FILE)

debug: CFLAGS += -DDEBUG
debug: $(TARGET)

help:
	@echo "Lizard Programming Language"
	@echo "Usage:"
	@echo "  make          - Build the interpreter"
	@echo "  make clean    - Clean build files"
	@echo "  make install  - Install to system"
	@echo "  make examples - Run example files"
	@echo "  make run FILE=path/to/file.lz - Run specific file"
	@echo "  make debug    - Build with debug symbols"
