# Detect the operating system
ifeq ($(OS), Windows_NT)
    OS_NAME := Windows
else
    OS_NAME := $(shell uname -s)
endif

# -------------------------------------------------------
# OS-specific settings
# -------------------------------------------------------
ifeq ($(OS_NAME), Windows)
    RM             := del /F /Q
    TARGET_EXT     := .exe
    # Windows: link against the import library and pull in system libs
    LDFLAGS        := -lraylib -lopengl32 -lgdi32 -lwinmm
    # On Windows the raylib headers/lib are usually installed to a known
    # prefix; adjust RAYLIB_PATH if yours differs.
    RAYLIB_PATH    ?= C:/raylib
    CFLAGS         := -Wall -Wextra -std=c99 -I$(RAYLIB_PATH)/include
    LDFLAGS        += -L$(RAYLIB_PATH)/lib

else ifeq ($(OS_NAME), Darwin)
    # macOS
    RM             := rm -rf
    TARGET_EXT     :=
    # Homebrew installs raylib to /opt/homebrew (Apple Silicon) or
    # /usr/local (Intel). pkg-config picks the right paths automatically
    # when raylib is installed via Homebrew.
    RAYLIB_CFLAGS  := $(shell pkg-config --cflags raylib 2>/dev/null)
    RAYLIB_LIBS    := $(shell pkg-config --libs   raylib 2>/dev/null)
    # Fallback if pkg-config is not available
    ifeq ($(RAYLIB_CFLAGS),)
        RAYLIB_PATH    ?= /usr/local
        RAYLIB_CFLAGS  := -I$(RAYLIB_PATH)/include
        RAYLIB_LIBS    := -L$(RAYLIB_PATH)/lib -lraylib
    endif
    CFLAGS         := -Wall -Wextra -std=c99 $(RAYLIB_CFLAGS)
    # macOS requires these frameworks to back OpenGL / window management
    LDFLAGS        := $(RAYLIB_LIBS) \
                      -framework CoreVideo \
                      -framework IOKit \
                      -framework Cocoa \
                      -framework GLUT \
                      -framework OpenGL

else
    # Linux (and other Unix-like systems)
    RM             := rm -rf
    TARGET_EXT     :=
    RAYLIB_CFLAGS  := $(shell pkg-config --cflags raylib 2>/dev/null)
    RAYLIB_LIBS    := $(shell pkg-config --libs   raylib 2>/dev/null)
    ifeq ($(RAYLIB_CFLAGS),)
        RAYLIB_PATH    ?= /usr/local
        RAYLIB_CFLAGS  := -I$(RAYLIB_PATH)/include
        RAYLIB_LIBS    := -L$(RAYLIB_PATH)/lib -lraylib
    endif
    CFLAGS         := -Wall -Wextra -std=c99 $(RAYLIB_CFLAGS)
    # Linux needs these system libraries alongside raylib
    LDFLAGS        := $(RAYLIB_LIBS) \
                      -lGL -lm -lpthread -ldl -lrt -lX11
endif

# -------------------------------------------------------
# Build targets
# -------------------------------------------------------
TARGET  := myapp$(TARGET_EXT)
SOURCES := main.c
OBJECTS := $(SOURCES:.c=.o)
CC      := gcc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(RM) $(OBJECTS) $(TARGET)
