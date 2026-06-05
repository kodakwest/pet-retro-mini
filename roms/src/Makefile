CA65 ?= ca65
LD65 ?= ld65
MSBASIC_DIR ?= /tmp/msbasic
OPEN_DIR := ../open
BUILD_DIR := build

.PHONY: all clean basic check-tools

all: check-tools basic $(OPEN_DIR)/kernal.bin $(OPEN_DIR)/editor.bin $(OPEN_DIR)/chargen.bin

check-tools:
	@command -v $(CA65) >/dev/null 2>&1 || { echo "missing ca65; install cc65"; exit 1; }
	@command -v $(LD65) >/dev/null 2>&1 || { echo "missing ld65; install cc65"; exit 1; }

basic: check-tools
	@if [ ! -d "$(MSBASIC_DIR)" ]; then \
		git clone https://github.com/mist64/msbasic.git "$(MSBASIC_DIR)"; \
	fi
	@cd "$(MSBASIC_DIR)" && ./make.sh
	@mkdir -p $(OPEN_DIR)
	cp "$(MSBASIC_DIR)/tmp/cbmbasic1.bin" "$(OPEN_DIR)/basic1.bin"
	cp "$(MSBASIC_DIR)/tmp/cbmbasic2.bin" "$(OPEN_DIR)/basic2.bin"
	@test "$$(wc -c < "$(OPEN_DIR)/basic1.bin")" -eq 8192
	@test "$$(wc -c < "$(OPEN_DIR)/basic2.bin")" -eq 8192

$(OPEN_DIR)/kernal.bin: kernal.s keyboard.s pet-kernal.cfg
	@mkdir -p $(OPEN_DIR) $(BUILD_DIR)
	$(CA65) kernal.s -o $(BUILD_DIR)/kernal.o
	$(LD65) -o $@ $(BUILD_DIR)/kernal.o -C pet-kernal.cfg
	@test "$$(wc -c < "$@")" -eq 4096

$(OPEN_DIR)/editor.bin: editor.s pet-editor.cfg
	@mkdir -p $(OPEN_DIR) $(BUILD_DIR)
	$(CA65) editor.s -o $(BUILD_DIR)/editor.o
	$(LD65) -o $@ $(BUILD_DIR)/editor.o -C pet-editor.cfg
	@test "$$(wc -c < "$@")" -eq 2048

$(OPEN_DIR)/chargen.bin: chargen.s pet-chargen.cfg
	@mkdir -p $(OPEN_DIR) $(BUILD_DIR)
	$(CA65) chargen.s -o $(BUILD_DIR)/chargen.o
	$(LD65) -o $@ $(BUILD_DIR)/chargen.o -C pet-chargen.cfg
	@test "$$(wc -c < "$@")" -eq 2048

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OPEN_DIR)/basic1.bin $(OPEN_DIR)/basic2.bin
	rm -f $(OPEN_DIR)/kernal.bin $(OPEN_DIR)/editor.bin $(OPEN_DIR)/chargen.bin
