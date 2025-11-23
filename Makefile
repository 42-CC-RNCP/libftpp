NAME = libtpp
BUILD_DIR = build

.PHONY: all re clean fclean test

all:
	@if [ ! -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "[cmake] configure..."; \
		cmake -B$(BUILD_DIR) -S .; \
	else \
		echo "[cmake] reuse existing build dir"; \
	fi
	@cmake --build $(BUILD_DIR) -- -s
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

re: fclean all

clean:
	@$(MAKE) -C $(BUILD_DIR) clean

fclean:
	@rm -f compile_commands.json
	@cmake -E remove_directory $(BUILD_DIR) || true

test: all
	@cmake --build $(BUILD_DIR) --target libftpp_tests -- -s
	@cd $(BUILD_DIR) && ctest --output-on-failure
	