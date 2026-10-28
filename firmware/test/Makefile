# Test sul PC (non serve l'ESP32):   make -C firmware/test
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror
BUILD := build
CTL := ../controller/src
NODE := ../sensor_node/src

SHARED := radiator_packet.h ds18b20.h ds18b20.cpp

.PHONY: all test shared-files compile-check clean

all: test shared-files compile-check

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/test_logic: test_logic.cpp $(CTL)/control_logic.cpp $(CTL)/control_logic.h \
                     $(CTL)/sensor_math.cpp $(CTL)/sensor_math.h $(CTL)/ds18b20.cpp \
                     $(CTL)/ds18b20.h $(CTL)/radiator_packet.h $(NODE)/ntc.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ test_logic.cpp $(CTL)/control_logic.cpp $(CTL)/sensor_math.cpp $(CTL)/ds18b20.cpp

test: $(BUILD)/test_logic
	./$(BUILD)/test_logic

# I file condivisi tra centralina e nodo devono essere identici.
shared-files:
	@for f in $(SHARED); do \
	  cmp -s $(CTL)/$$f $(NODE)/$$f || { echo "ERRORE: $$f diverso tra controller e sensor_node"; exit 1; }; \
	done; echo "[ok]   file condivisi identici"

# Compila tutto il firmware contro intestazioni finte (stubs/) per trovare
# errori di sintassi e di tipo senza la toolchain ESP32. Non sostituisce la
# compilazione vera con PlatformIO o Arduino IDE.
STUB_FLAGS := -std=gnu++17 -fsyntax-only -Wall -Wextra -Wno-unused-parameter -Werror -DARDUINO=10819 -Istubs
compile-check:
	@for v in "" "-DPIN_LED=-1 -DPIN_SERVO_POWER=27"; do \
	  for f in $(CTL)/*.cpp; do $(CXX) $(STUB_FLAGS) $$v -I$(CTL) $$f || exit 1; done; \
	done
	@for v in "" "-DARDUINO_XIAO_ESP32C6" "-DARDUINO_XIAO_ESP32C6 -DNODE_SENSOR=2" "-DNODE_DEBUG=1"; do \
	  for f in $(NODE)/*.cpp; do $(CXX) $(STUB_FLAGS) $$v -I$(NODE) $$f || exit 1; done; \
	done
	@echo "[ok]   compilazione di controllo (stub) riuscita"

clean:
	rm -rf $(BUILD)
