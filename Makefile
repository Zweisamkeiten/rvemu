# default rule
default: makedir all

# tool macros
CC := clang
CFLAGS := -O3 -Wall -Werror -MMD
LINKLIB := -lm
DBGFLAGS := -g -fsanitize=address

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := debug

WORK_DIR  = $(shell pwd)

INC_PATH := $(WORK_DIR)/include $(INC_PATH)
INCLUDES = $(addprefix -I, $(INC_PATH))
CFLAGS := $(INCLUDES) $(CFLAGS)

# compile macros
TARGET_NAME := rvemu
RUN := $(WORK_DIR)/$(TARGET_NAME)
RUN_DEBUG := $(WORK_DIR)/$(TARGET_NAME)_debug
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

# clean files list
DISTCLEAN_LIST := $(OBJ) \
                  $(OBJ_DEBUG)
CLEAN_LIST := $(RUN) \
				$(RUN_DEBUG) \
				$(TARGET) \
			  $(TARGET_DEBUG) \
			  $(DISTCLEAN_LIST)

# Depencies
-include $(OBJ:.o=.d)

# non-phony targets
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LINKLIB) -o $@ $(OBJ)
	cp $@ $(RUN)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c*
	@echo + CC $<
	$(CC) $(CFLAGS) -c $< -o $@

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c*
	@echo + CC $<
	$(CC) $(CFLAGS) -c $(DBGFLAGS) -o $@ $<
	$(CC) -E $(INCLUDES) $< | \
		grep -ve '^#' | \
		clang-format - > $(basename $@).i

$(TARGET_DEBUG): $(OBJ_DEBUG)
	@echo + LD $@
	$(CC) $(CFLAGS) $(DBGFLAGS) $(OBJ_DEBUG) -o $@
	cp $@ $(RUN_DEBUG)

MAIN_EXEC := $(RUN)
ARGS :=
DEBUG_EXEC := $(RUN_DEBUG)

# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH)

.PHONY: all
all: $(TARGET)

.PHONY: run
run: all
	$(MAIN_EXEC) $(ARGS)

.PHONY: test
test: all
	riscv64-elf-gcc -nostdlib -fno-builtin -march=rv64g -mabi=lp64 -O0 ./playground/decode_test.S -o ./playground/a.out
	make run ARGS="./playground/a.out" 2>&1 | grep inst | sed 's/inst: inst_\(.*\)/\1/' | head -n -1 > ./playground/output
	tail -n +5 ./playground/decode_test.S | head -n -2 | awk '{print $$1}' > ./playground/diff
	diff ./playground/diff ./playground/output

.PHONY: debug
debug: $(TARGET_DEBUG)
	gdb $(DEBUG_EXEC) --args $(DEBUG_EXEC) $(ARGS)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)
