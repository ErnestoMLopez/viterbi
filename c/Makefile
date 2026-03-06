# Directorios
SRC_DIR      := src
TEST_DIR     := test
BUILD_DIR    := build
OBJ_DIR      := $(BUILD_DIR)/obj
LIB_DIR      := $(BUILD_DIR)/lib
BIN_DIR      := $(BUILD_DIR)/bin
LIB_NAME     := libviterbi.a
LIB_PATH     := $(LIB_DIR)/$(LIB_NAME)
TEST_BIN_DIR := $(BIN_DIR)/test

# Fuentes y objetos
SRC      := $(SRC_DIR)/viterbi.c
SRC_HDR  := $(SRC_DIR)/viterbi.h
SRC_OBJ  := $(OBJ_DIR)/viterbi.o
TEST_SRC := $(TEST_DIR)/test_viterbi.c
TEST_OBJ := $(OBJ_DIR)/test_viterbi.o

# Herramientas y flags
CC := gcc
AR := ar

CFLAGS  := -Wall -Wextra -O2 -g -I$(SRC_DIR)
ARFLAGS := rcs
LDFLAGS :=

.PHONY: all viterbi test clean dirs

all: viterbi test

# Creación de la librería estática
viterbi: dirs $(LIB_PATH)

$(LIB_PATH): $(SRC_OBJ)
	@echo "Creating static library: $@"
	$(AR) $(ARFLAGS) $@ $^

# Compilación del test enlazando con la librería estática
test: viterbi dirs $(TEST_BIN_DIR)

$(TEST_BIN_DIR): $(TEST_OBJ) $(LIB_PATH)
	@echo "Linking test executable: $@"
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) $(LIB_PATH) $(LDFLAGS)

# Compilación genérica de archivos fuente
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilación del archivo fuente de test
$(OBJ_DIR)/test_viterbi.o: $(TEST_DIR)/test_viterbi.c $(SRC_HDR) | dirs
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Creación de directorios
dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# Limpieza de artefactos
clean:
	@echo "Cleaning build..."
	@rm -rf $(BUILD_DIR)