APP := pet-retro-mini
SRC_DIR := src/launcher
BRIDGE_DIR := src/bridge
BUILD_DIR := build
DIST_DIR := dist

CC ?= gcc
WINDRES ?= windres

UNAME_S := $(shell uname -s 2>/dev/null)
MINGW_CC := $(shell $(CC) -dumpmachine 2>/dev/null | grep -E "mingw|msys" >/dev/null && echo 1)
SDL2_CONFIG := $(shell command -v sdl2-config 2>/dev/null)
PKG_CONFIG := $(shell command -v pkg-config 2>/dev/null)

LAUNCHER_SOURCES := $(wildcard $(SRC_DIR)/*.c)
BRIDGE_SOURCES := $(wildcard $(BRIDGE_DIR)/*.c)
SOURCES := $(LAUNCHER_SOURCES) $(BRIDGE_SOURCES)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/launcher/%.o,$(LAUNCHER_SOURCES)) \
           $(patsubst $(BRIDGE_DIR)/%.c,$(BUILD_DIR)/bridge/%.o,$(BRIDGE_SOURCES))
RESOURCE := $(BUILD_DIR)/resources.res
TARGET := $(DIST_DIR)/$(APP).exe

CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
CFLAGS += -DAPP_NAME=\"PET\ Retro\ Mini\" -I$(SRC_DIR) -I$(BRIDGE_DIR)
LDFLAGS ?=
LDLIBS ?=

ifeq ($(MINGW_CC),1)
  PLATFORM := windows
  CFLAGS += -D_WIN32_WINNT=0x0A00
  ifneq ($(SDL2_CONFIG),)
    CFLAGS += $(shell sdl2-config --cflags)
    LDLIBS += $(shell sdl2-config --static-libs 2>/dev/null || sdl2-config --libs)
  else ifneq ($(PKG_CONFIG),)
    CFLAGS += $(shell pkg-config --cflags sdl2 2>/dev/null)
    LDLIBS += $(shell pkg-config --static --libs sdl2 2>/dev/null || pkg-config --libs sdl2 2>/dev/null)
  else
    CFLAGS += -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main
    LDLIBS += -LC:/msys64/mingw64/lib -lmingw32 -lSDL2main -lSDL2
  endif
  LDFLAGS += -mwindows -static -static-libgcc
  LDLIBS += -lcomdlg32 -lole32 -luuid -lwinmm -limm32 -lsetupapi -lversion
else
  PLATFORM := host
  ifneq ($(SDL2_CONFIG),)
    CFLAGS += $(shell sdl2-config --cflags)
    LDLIBS += $(shell sdl2-config --libs)
  else ifneq ($(PKG_CONFIG),)
    CFLAGS += $(shell pkg-config --cflags sdl2 2>/dev/null)
    LDLIBS += $(shell pkg-config --libs sdl2 2>/dev/null)
  else
    CFLAGS += -I/usr/include/SDL2 -D_REENTRANT
    LDLIBS += -lSDL2
  endif
endif

.PHONY: all clean dist info

all: $(TARGET)

info:
	@echo "platform=$(PLATFORM)"
	@echo "cc=$(CC)"
	@echo "sdl2_config=$(SDL2_CONFIG)"
	@echo "pkg_config=$(PKG_CONFIG)"

$(TARGET): $(OBJECTS) $(if $(filter windows,$(PLATFORM)),$(RESOURCE))
	@mkdir -p $(DIST_DIR)
	$(CC) $(OBJECTS) $(if $(filter windows,$(PLATFORM)),$(RESOURCE)) $(LDFLAGS) -o $@ $(LDLIBS)

$(BUILD_DIR)/launcher/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/*.h $(BRIDGE_DIR)/*.h
	@mkdir -p $(BUILD_DIR)/launcher
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bridge/%.o: $(BRIDGE_DIR)/%.c $(BRIDGE_DIR)/*.h
	@mkdir -p $(BUILD_DIR)/bridge
	$(CC) $(CFLAGS) -c $< -o $@

$(RESOURCE): $(SRC_DIR)/resources.rc $(SRC_DIR)/app.manifest
	@mkdir -p $(BUILD_DIR)
	$(WINDRES) $< -O coff -o $@

dist: all
	@mkdir -p $(DIST_DIR)
	@cd $(DIST_DIR) && zip -q $(APP)-v0.1.zip $(APP).exe

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
