SHELL := /bin/bash
# Makefile ThermoNode firmware — cibles : build test coverage static cross size-check sim clean
CC      ?= gcc
CROSS   ?= arm-none-eabi-
SRC     := src/convert.c src/filter.c src/threshold.c src/fault.c src/ringbuf.c src/frame.c src/node.c
TESTS   := $(wildcard test/test_*.c)
UNITY   := test/unity/unity.c
CFLAGS  := -std=c99 -Wall -Wextra -Werror -Wconversion -Isrc -Itest -Itest/unity -DUNITY_INCLUDE_DOUBLE
COVFLAGS:= --coverage -O0 -g
BUILD   := build
REPORTS := ../reports
ifeq ($(SAN),1)
CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g
endif

.PHONY: build test coverage static cross size-check clean sim

build: $(BUILD)/libtn.a
$(BUILD)/libtn.a: $(SRC)
	@mkdir -p $(BUILD) $(REPORTS)
	@for f in $(SRC); do echo "CC $$f"; $(CC) $(CFLAGS) -c $$f -o $(BUILD)/$$(basename $${f%.c}).o || exit 1; done
	ar rcs $@ $(BUILD)/*.o

# Un exécutable de test par fichier test_*.c ; rapport JUnit agrégé dans ../reports/junit-fw.xml
test:
	@mkdir -p $(BUILD) $(REPORTS); rm -f $(BUILD)/*.gcda; status=0; \
	for t in $(TESTS); do n=$$(basename $${t%.c}); \
	  $(CC) $(CFLAGS) $(COVFLAGS) $(SRC) test/hal_double.c $(UNITY) $$t -o $(BUILD)/$$n || exit 1; \
	  ./$(BUILD)/$$n | tee $(BUILD)/$$n.log; rc=$${PIPESTATUS[0]}; [ $$rc -eq 0 ] || status=1; done; \
	python3 ../tools/unity2junit.py $(BUILD)/*.log > $(REPORTS)/junit-fw.xml; exit $$status

coverage: test
	gcovr -r . --object-directory $(BUILD) --txt-metric branch --xml -o $(REPORTS)/coverage-fw.xml \
	      --html-details $(REPORTS)/coverage-fw.html -e 'test/.*' -e 'hal/.*' --print-summary

static:
	@mkdir -p $(REPORTS)
	cppcheck --enable=warning,style,performance,portability --std=c99 --inline-suppr --error-exitcode=0 \
	         --xml --xml-version=2 --suppressions-list=misra-suppressions.txt -Isrc src/ 2> $(REPORTS)/cppcheck.xml
	@echo "constats : $$(grep -c '<error ' $(REPORTS)/cppcheck.xml || true)"

# Compilation croisée des modules applicatifs (mesure de taille sans la bibliothèque STM32Cube)
cross:
	@mkdir -p $(BUILD) $(REPORTS)
	$(CROSS)gcc -std=c99 -Os -mcpu=cortex-m0plus -mthumb -Wall -Wextra -Werror -Isrc -c $(SRC) hal/hal_stm32_stub.c
	@mv *.o $(BUILD)/ 2>/dev/null || true
	$(CROSS)gcc -mcpu=cortex-m0plus -mthumb -nostartfiles -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs \
	    $(BUILD)/convert.o $(BUILD)/filter.o $(BUILD)/threshold.o $(BUILD)/fault.o $(BUILD)/ringbuf.o \
	    $(BUILD)/frame.o $(BUILD)/node.o $(BUILD)/hal_stm32_stub.o -o $(BUILD)/thermonode-core.elf
	$(CROSS)size $(BUILD)/thermonode-core.elf | tee $(REPORTS)/size.txt

size-check: cross
	python3 ../tools/check_size.py $(REPORTS)/size.txt --flash-max 49152 --ram-max 8192

# Simulateur hôte (publie en MQTT réel) : ./build/node-sim
sim:
	@mkdir -p $(BUILD)
	$(CC) -std=gnu99 -Wall -Wextra -O2 -DTN_SIM -Isrc $(SRC) src/main.c hal/hal_sim.c -o $(BUILD)/node-sim

clean:
	rm -rf $(BUILD) $(REPORTS)/junit-fw.xml $(REPORTS)/coverage-fw.* *.gcda *.gcno
