NAME = libtpp
BUILD_DIR = build

.PHONY: all re clean fclean

all:
	@cmake -B$(BUILD_DIR) -S .
	@cmake --build $(BUILD_DIR) -- -s

re: fclean all

clean:
	@$(MAKE) -C $(BUILD_DIR) clean

fclean:
	@rm -rf $(BUILD_DIR)
