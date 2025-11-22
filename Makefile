TARGET_EXEC := final_program
BUILD_DIR := ./build
SRC_DIRS := ./


SRCS := $(shell find $(SRC_DIRS) -name '*.c' -not -path './utils/algorithms/*/*.c')

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)


ALGORITHMS_DIR := utils/algorithms


ALGORITHM_DIRS := $(shell find $(ALGORITHMS_DIR) -maxdepth 1 -type d ! -name "algorithms" ! -name "useful" ! -name "registry" ! -name "wrappers")
ALGORITHM_NAMES := $(notdir $(ALGORITHM_DIRS))


ALGORITHM_EXISTING := $(foreach algo,$(ALGORITHM_NAMES),\
    $(if $(or \
        $(wildcard $(ALGORITHMS_DIR)/$(algo)/$(algo).c),\
        $(wildcard $(ALGORITHMS_DIR)/$(algo)/$(shell echo $(algo) | tr '[:upper:]' '[:lower:]').c),\
        $(wildcard $(ALGORITHMS_DIR)/$(algo)/*.c)\
    ),$(algo)))

ALGORITHM_NAMES := $(ALGORITHM_EXISTING)
ALGORITHM_SOS := $(addprefix $(BUILD_DIR)/algorithms/,$(addsuffix .so,$(ALGORITHM_NAMES)))


INC_DIRS := $(shell find $(SRC_DIRS) -type d) $(ALGORITHM_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))


GTK3_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK3_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)
CJSON_CFLAGS := $(shell pkg-config --cflags libcjson)
CJSON_LDFLAGS := $(shell pkg-config --libs libcjson)

CPPFLAGS := $(INC_FLAGS) $(GTK3_CFLAGS) $(CJSON_CFLAGS) -MMD -MP
LDFLAGS := $(GTK3_LDFLAGS) $(CJSON_LDFLAGS) -ldl


CFLAGS_SHARED := -fPIC
LDFLAGS_SHARED := -shared


ALGO_CPPFLAGS := $(INC_FLAGS) -MMD -MP


INSTALL_DIR := /usr/local/bin
ALGORITHMS_INSTALL_DIR := /usr/local/lib/scheduler


UTIL_OBJS := \
	$(BUILD_DIR)/utils/queues/fifo/queuef.c.o \
	$(BUILD_DIR)/utils/queues/priority/priority_queue.c.o \
	$(BUILD_DIR)/utils/gannt/format.c.o \
	$(BUILD_DIR)/utils/algorithms/useful/useful.c.o


$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo "Linking main program..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Main program built: $@"


$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/utils/queues/fifo/queuef.c.o: utils/queues/fifo/queuef.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils/queues/priority/priority_queue.c.o: utils/queues/priority/priority_queue.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils/gannt/format.c.o: utils/gannt/format.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils/algorithms/useful/useful.c.o: utils/algorithms/useful/useful.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/algorithms/%.so: $(UTIL_OBJS)
	@mkdir -p $(dir $@)
	@echo "Building algorithm plugin: $*"
	@# Find the actual .c file for this algorithm
	$(eval ALGO_C_FILE := $(firstword \
		$(wildcard $(ALGORITHMS_DIR)/$*/$*.c) \
		$(wildcard $(ALGORITHMS_DIR)/$*/$(shell echo $* | tr '[:upper:]' '[:lower:]').c) \
		$(wildcard $(ALGORITHMS_DIR)/$*/*.c) \
	))
	@if [ -z "$(ALGO_C_FILE)" ]; then \
		echo "Warning: No .c file found for algorithm $* - skipping"; \
		exit 0; \
	fi
	@echo "Using source file: $(ALGO_C_FILE)"
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $(ALGO_C_FILE) -o $(BUILD_DIR)/algorithms/$*.o
	$(CC) $(BUILD_DIR)/algorithms/$*.o $(UTIL_OBJS) -o $@ $(LDFLAGS_SHARED) -lc


.PHONY: all
all: $(BUILD_DIR)/$(TARGET_EXEC) algorithms
	@echo "Build completed successfully!"

.PHONY: algorithms
algorithms: $(ALGORITHM_SOS)
	@echo "Discovered $(words $(ALGORITHM_NAMES)) algorithms: $(ALGORITHM_NAMES)"
	@echo "Built $(words $(ALGORITHM_SOS)) algorithm plugins"
	@mkdir -p ./algorithms
	@cp $(ALGORITHM_SOS) ./algorithms/
	@echo "Plugins copied to ./algorithms/ directory"

.PHONY: list-algorithms
list-algorithms:
	@echo "Available algorithm directories:"
	@for algo in $(ALGORITHM_NAMES); do \
		echo "  - $$algo"; \
	done
	@echo "Total: $(words $(ALGORITHM_NAMES)) algorithms"

.PHONY: debug-files
debug-files:
	@echo "=== Algorithm File Discovery Debug ==="
	@echo "ALGORITHM_NAMES (with .c files): $(ALGORITHM_NAMES)"
	@echo ""
	@echo "Checking each algorithm:"
	@for algo in FIFO SJF SRT RR priority priority_np multilevel; do \
		if [ -n "$(filter $$algo,$(ALGORITHM_NAMES))" ]; then \
			echo "  ✓ $$algo: WILL BE BUILT (.c file exists)"; \
		else \
			echo "  ✗ $$algo: WILL BE SKIPPED (.c file missing)"; \
		fi \
	done
	@echo "=== End Debug ==="

.PHONY: debug-discovery
debug-discovery:
	@echo "=== Algorithm Discovery Debug ==="
	@echo "ALGORITHM_DIRS: $(ALGORITHM_DIRS)"
	@echo "ALGORITHM_NAMES: $(ALGORITHM_NAMES)"
	@echo "ALGORITHM_SOS: $(ALGORITHM_SOS)"
	@echo ""
	@echo "Checking each algorithm directory:"
	@for algo in FIFO SJF SRT RR priority priority_np multilevel; do \
		if [ -f "utils/algorithms/$$algo/$$algo.c" ]; then \
			echo "  ✓ $$algo: $$algo.c FOUND"; \
		else \
			echo "  ✗ $$algo: $$algo.c MISSING"; \
		fi \
	done
	@echo "=== End Debug ==="

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./algorithms

.PHONY: clean-algorithms
clean-algorithms:
	rm -rf ./algorithms
	rm -rf $(BUILD_DIR)/algorithms	

.PHONY: install
install: all
	@if [ -w $(INSTALL_DIR) ]; then \
		cp $(BUILD_DIR)/$(TARGET_EXEC) $(INSTALL_DIR)/scheduler; \
		chmod +x $(INSTALL_DIR)/scheduler; \
		mkdir -p $(ALGORITHMS_INSTALL_DIR); \
		cp ./algorithms/*.so $(ALGORITHMS_INSTALL_DIR)/; \
		echo "Installation completed. Use: scheduler g"; \
	else \
		echo "No permission. Try: sudo make install"; \
		exit 1; \
	fi

-include $(DEPS)