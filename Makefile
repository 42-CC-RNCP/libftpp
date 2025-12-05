NAME      = libtpp
BUILD_DIR = build

MODULES = data_structures design_patterns tpp_iostream mathematics network threading

TEST_TARGETS = \
    test_data_structures \
    test_memento \
    test_observer \
    test_singleton \
    test_state_machine \
    test_threading \
	test_network

.PHONY: all re clean fclean test cmake_configure help \
        $(MODULES) $(TEST_TARGETS)

all: cmake_configure
	@cmake --build $(BUILD_DIR) -- -q >/dev/null 2>&1; \
	if [ $$? -eq 0 ]; then \
		echo "make: Nothing to be done for 'all'."; \
	else \
		cmake --build $(BUILD_DIR) -- -s; \
	fi
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

$(MODULES): cmake_configure
	@echo "[cmake] build module $@"
	@cmake --build $(BUILD_DIR) --target $@ -- -s

re: fclean all

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(MAKE) -C $(BUILD_DIR) clean || true; \
	fi

fclean:
	@rm -f compile_commands.json
	@cmake -E remove_directory $(BUILD_DIR) || true


# ---- test by module ----
$(TEST_TARGETS): cmake_configure
	@echo "[cmake] build test target $@"
	@cmake --build $(BUILD_DIR) --target $@ -- -s
	@echo "[ctest] run tests with prefix $@"
	@cd $(BUILD_DIR) && ctest -R "^$@"

help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "Main targets:"
	@echo "  all              - Configure (if needed) and build all libraries"
	@echo "  test             - Build and run ALL tests"
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
	@echo "  make threading           # build threading module"
	@echo "  make test_threading      # build and run threading-related tests"
	@echo "  make test_data_structures"

cmake_configure:
	@if [ ! -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "[cmake] configure..."; \
		cmake -B$(BUILD_DIR) -S .; \
	else \
		echo "[cmake] reuse existing build dir"; \
	fi
