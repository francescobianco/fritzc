# Convenience front-end for the CMake build and the analysis tools.
#
#   make start      build and open the port window (what is reconstructed so far)
#   make compare    same, with the original DOSBox capture: Tab = original, D = diff
#   make dump       render the port headless to build/native.png
#   make test       pixel regression test against DOSBox captures
#   make original   run the original Fritz 2.51 in DOSBox
#   make unpack     unpack FRITZ2.EXE into build/re/FRITZ2.UNP.EXE
#   make engine-demo the original FRITZ2.BIN plays itself in a CPU emulator
#   make engine-test C engine routines vs the original ones (differential)
#   make clean      remove the CMake build directory
#
# Variables: DATA (original files, default dos/fritz251), SCALE (window
# scale, default 2), BUILD (CMake build dir).

DATA  ?= dos/fritz251
SCALE ?= 2
BUILD ?= build/cmake
SDL_PREFIX := build/deps/prefix
FRITZ := $(BUILD)/fritz

# Use the locally built SDL3 when present; otherwise CMake finds or fetches it.
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug
ifneq ($(wildcard $(SDL_PREFIX)/lib/cmake/SDL3),)
CMAKE_FLAGS += -DCMAKE_PREFIX_PATH=$(abspath $(SDL_PREFIX))
endif

CAPTURE := build/captures/test/main/fritz2_000.png

.PHONY: all build start compare dump test original unpack clean check-data engine-demo engine-test

all: build

VENV := build/venv
$(VENV)/bin/python:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install -q unicorn capstone

engine-demo: $(VENV)/bin/python check-data
	$(VENV)/bin/python tools/engine/harness.py $(DATA)/FRITZ2.BIN 4 10

engine-test: build $(VENV)/bin/python check-data
	$(VENV)/bin/python tests/engine/diff.py $(DATA)/FRITZ2.BIN $(BUILD)/libfritzengine.so

$(BUILD)/CMakeCache.txt:
	cmake -S . -B $(BUILD) $(CMAKE_FLAGS)

build: $(BUILD)/CMakeCache.txt
	cmake --build $(BUILD) -j

check-data:
	@test -f $(DATA)/FRITZ2.EXE || { \
	    echo "Original Fritz 2.51 files not found in $(DATA) (set DATA=...)"; exit 1; }

start: build check-data
	$(FRITZ) --data $(DATA) --scale $(SCALE)

compare: build check-data $(CAPTURE)
	python3 -c "from PIL import Image; Image.open('$(CAPTURE)').save('build/reference.bmp')"
	$(FRITZ) --data $(DATA) --scale $(SCALE) --reference build/reference.bmp

$(CAPTURE): unpack
	tests/gui/screens.sh $(BUILD) >/dev/null || true

dump: build check-data
	$(FRITZ) --data $(DATA) --dump build/native.bmp
	python3 -c "from PIL import Image; Image.open('build/native.bmp').convert('RGB').save('build/native.png')"
	@echo "build/native.png"

test: build check-data unpack
	tests/gui/screens.sh $(BUILD)

original: check-data unpack
	FRITZ_EXE=build/re/FRITZ2.UNP.EXE tools/compare/dosbox-run.sh build/captures/manual 600

unpack: build/re/FRITZ2.UNP.EXE

build/re/FRITZ2.UNP.EXE: $(DATA)/FRITZ2.EXE
	@mkdir -p build/re
	python3 -I tools/inspect/unexepack.py $< $@

clean:
	rm -rf $(BUILD)
