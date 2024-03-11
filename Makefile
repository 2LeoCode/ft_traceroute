NAME=ft_traceroute

SRC_DIR=src
BUILD_DIR=.build
INC_DIRS=inc

SRC_EXT=c
OBJ_EXT=o
DEP_EXT=d

CC=gcc
CSTD=gnu17
CFLAGS=-Wall -Wextra -Werror -std=$(CSTD) -MMD $(addprefix -I,$(INC_DIRS))

SRC=$(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJ=$(SRC:$(SRC_DIR)/%.$(SRC_EXT)=$(BUILD_DIR)/%.$(OBJ_EXT))
DEP=$(OBJ:.$(OBJ_EXT)=.$(DEP_EXT))


all: $(BUILD_DIR) $(NAME)

-include $(DEP)

$(BUILD_DIR):
	mkdir -p $@

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.$(SRC_EXT)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
