NAME = libtpp
BUILD_DIR = build
MODULES = data_structures design_patterns tpp_iostream mathematics network threading
MODULE_LIBS = \
	$(BUILD_DIR)/src/data_structures/libdata_structures.a \
	$(BUILD_DIR)/src/design_patterns/libdesign_patterns.a \
	$(BUILD_DIR)/src/iostream/libtpp_iostream.a \
	$(BUILD_DIR)/src/mathematics/libmathematics.a \
	$(BUILD_DIR)/src/network/libnetwork.a \
	$(BUILD_DIR)/src/threading/libthreading.a
LIB_PATH = $(BUILD_DIR)/$(NAME).a
TEST_TARGETS = \
    test_data_structures \
    test_memento \
    test_observer \
    test_singleton \
    test_state_machine \
    test_threading \
	test_network \
	test_mathematics
TEST_JOBS ?= $(shell nproc)

.PHONY: all
all: $(LIB_PATH)
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

.PHONY: $(LIB_PATH)
$(LIB_PATH): cmake_configure
	@echo "[cmake] build all module libraries"
	@cmake --build $(BUILD_DIR) --target $(MODULES) -- -s
	@echo "[ar] merge module archives -> $(LIB_PATH)"
	@{ \
		echo "create $(NAME).a"; \
		echo "addlib src/data_structures/libdata_structures.a"; \
		echo "addlib src/design_patterns/libdesign_patterns.a"; \
		echo "addlib src/iostream/libtpp_iostream.a"; \
		echo "addlib src/mathematics/libmathematics.a"; \
		echo "addlib src/network/libnetwork.a"; \
		echo "addlib src/threading/libthreading.a"; \
		echo "save"; \
		echo "end"; \
	} | (cd $(BUILD_DIR) && ar -M)
	@ranlib $(LIB_PATH)
	@cp -f $(LIB_PATH) ./$(NAME).a
	@echo "[ar] library $(NAME) created successfully"

.PHONY: $(MODULES)
$(MODULES): cmake_configure
	@echo "[cmake] build module $@"
	@cmake --build $(BUILD_DIR) --target $@ -- -s

.PHONY: re
re: fclean all

.PHONY: clean
clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(MAKE) -C $(BUILD_DIR) clean || true; \
	fi

.PHONY: fclean
fclean:
	@rm -f compile_commands.json
	@cmake -E remove_directory $(BUILD_DIR) || true

.PHONY: tests
tests: cmake_configure
	@echo "[cmake] build all test targets"
	@cmake --build $(BUILD_DIR) --target libftpp_tests -- -s >/tmp/libtpp-build-tests.log 2>&1 || { \
		echo "[cmake] build failed, details:"; \
		cat /tmp/libtpp-build-tests.log; \
		exit 1; \
	}
	@echo "[ctest] run test modules in parallel (module-jobs=$(TEST_JOBS))"
	@python3 scripts/run_tests_pretty.py \
		--build-dir $(BUILD_DIR) \
		--jobs $(TEST_JOBS) \
		$(TEST_TARGETS)
	

# ---- test by module ----
.PHONY: $(TEST_TARGETS)
$(TEST_TARGETS): cmake_configure
	@echo "[cmake] build test target $@"
	@cmake --build $(BUILD_DIR) --target $@ -- -s
	@echo "[ctest] run tests with prefix $@"
	@cd $(BUILD_DIR) && ctest -R "^$@"

.PHONY: help
help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "Main targets:"
	@echo "  all              - Configure (if needed), build modules, and pack $(NAME)"
	@echo "  tests            - Build tests, run modules in parallel, show . / x progress"
	@echo "  clean            - Clean build artifacts inside $(BUILD_DIR)"
	@echo "  fclean           - Remove $(BUILD_DIR) and compile_commands.json"
	@echo "  re               - fclean + all"
	@echo ""
	@echo "Module build targets:"
	@$(foreach m,$(MODULES),echo "  $(m)$(shell printf '\n')";)
	@echo ""
	@echo "Test targets (per group):"
	@$(foreach t,$(TEST_TARGETS),echo "  $(t)$(shell printf '\n')";)
	@echo ""
	@echo "Examples:"
	@echo "  make tests TEST_JOBS=8  # run all tests with 8 parallel jobs"
	@echo "  make threading           # build threading module"
	@echo "  make test_threading      # build and run threading-related tests"
	@echo "  make test_data_structures"
	@echo "  make test_mathematics"

.PHONY: cmake_configure
cmake_configure:
	@if [ ! -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "[cmake] configure..."; \
	else \
		echo "[cmake] refresh configure..."; \
	fi
	@cmake -B$(BUILD_DIR) -S .
