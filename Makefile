
TARGET_EXEC := final_program
BUILD_DIR := ./build
SRC_DIRS := ./

SRCS := $(shell find $(SRC_DIRS) -name '*.c' \
    ! -path './utils/algorithms/*/*.c' \
    ! -path './utils/queues/fifo/queuef.c' \
    ! -path './utils/queues/priority/priority_queue.c' \
    ! -path './utils/gannt/format.c' \
    ! -path './utils/algorithms/useful/useful.c')

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

ALGORITHMS_DIR := utils/algorithms

ALGORITHM_DIRS := $(shell find $(ALGORITHMS_DIR) -maxdepth 1 -type d \
    ! -name "algorithms" ! -name "useful" ! -name "registry" ! -name ".")


define find_algo_source
$(firstword \
    $(wildcard $(1)/$(notdir $(1)).c) \
    $(wildcard $(1)/$(shell echo $(notdir $(1)) | tr '[:upper:]' '[:lower:]').c) \
    $(wildcard $(1)/*.c))
endef


ALGO_PAIRS :=
$(foreach dir,$(ALGORITHM_DIRS), \
    $(eval _algo := $(notdir $(dir))) \
    $(eval _src := $(call find_algo_source,$(dir))) \
    $(if $(_src), \
        $(eval ALGO_PAIRS += $(_algo):$(_src))))


ALGORITHM_NAMES := $(foreach pair,$(ALGO_PAIRS),$(firstword $(subst :, ,$(pair))))


ALGORITHM_SOS := $(addprefix $(BUILD_DIR)/algorithms/,$(addsuffix .so,$(ALGORITHM_NAMES)))



INC_DIRS := $(shell find $(SRC_DIRS) -type d) $(ALGORITHM_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

GTK3_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK3_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)
CJSON_CFLAGS := $(shell pkg-config --cflags libcjson)
CJSON_LDFLAGS := $(shell pkg-config --libs libcjson)

CPPFLAGS := $(INC_FLAGS) $(GTK3_CFLAGS) $(CJSON_CFLAGS) -MMD -MP
LDFLAGS := $(GTK3_LDFLAGS) $(CJSON_LDFLAGS) -ldl -Wl,-export-dynamic

ALGO_CPPFLAGS := $(INC_FLAGS) -MMD -MP
CFLAGS_SHARED := -fPIC
LDFLAGS_SHARED := -shared



UTIL_OBJS := \
    $(BUILD_DIR)/utils/queues/fifo/queuef.c.o \
    $(BUILD_DIR)/utils/queues/priority/priority_queue.c.o \
    $(BUILD_DIR)/utils/gannt/format.c.o \
    $(BUILD_DIR)/utils/algorithms/useful/useful.c.o



$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(UTIL_OBJS)
	@echo "Linking main program..."
	$(CC) $(OBJS) $(UTIL_OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/utils/queues/fifo/queuef.c.o: utils/queues/fifo/queuef.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $< -o $@

$(BUILD_DIR)/utils/queues/priority/priority_queue.c.o: utils/queues/priority/priority_queue.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $< -o $@

$(BUILD_DIR)/utils/gannt/format.c.o: utils/gannt/format.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $< -o $@

$(BUILD_DIR)/utils/algorithms/useful/useful.c.o: utils/algorithms/useful/useful.c
	@mkdir -p $(dir $@)
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $< -o $@



$(BUILD_DIR)/algorithms/%.so: $(UTIL_OBJS)
	@mkdir -p $(dir $@)
	
	$(eval ALGO_SOURCE := $(filter $*:%,$(ALGO_PAIRS)))
	$(eval ALGO_SOURCE := $(lastword $(subst :, ,$(ALGO_SOURCE))))
	@if [ -z "$(ALGO_SOURCE)" ]; then \
		echo "Error: Algorithm $* has no source file"; \
		exit 1; \
	fi
	@echo "Building plugin: $*"
	$(CC) $(ALGO_CPPFLAGS) $(CFLAGS) $(CFLAGS_SHARED) -c $(ALGO_SOURCE) -o $(BUILD_DIR)/algorithms/$*.o
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
