# external parameters
export DEBUG = 1
export OPT_LEVEL = 3
export LLVM_DIR = /usr/local/opt/llvm

# judge if is debug mode
ifeq ($(DEBUG), 0)
	DEBUG_ARG = 
	OPT_ARG = -O$(OPT_LEVEL)
else
	DEBUG_ARG = -DDEBUG -g
	OPT_ARG = 
endif

# directories
export TOP_DIR = $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
TARGET_DIR = $(TOP_DIR)/build
OBJ_DIR = $(TOP_DIR)/build/obj
PL01_OBJ_DIR = $(OBJ_DIR)/pl01
TEST_OBJ_DIR = $(OBJ_DIR)/test
LIB_OBJ_DIR = $(OBJ_DIR)/lib
SRC_DIR = $(TOP_DIR)/src
TEST_DIR = $(TOP_DIR)/test
LIB_DIR = $(TOP_DIR)/lib
INCLUDE_ARG := -I$(SRC_DIR)/inc -I$(TEST_DIR) -I$(LIB_DIR) -I$(LLVM_DIR)/include
LIBRARY_ARG := -L$(LLVM_DIR)/lib `$(LLVM_DIR)/bin/llvm-config --system-libs --libs all`

# files
PL01_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(PL01_OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.cpp))
PL01_OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(PL01_OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/**/*.cpp))
PL01_OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(PL01_OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/**/**/*.cpp))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp, $(TEST_OBJ_DIR)/%.o, $(wildcard $(TEST_DIR)/*.cpp))
TEST_OBJS += $(patsubst $(TEST_DIR)/%.cpp, $(TEST_OBJ_DIR)/%.o, $(wildcard $(TEST_DIR)/**/*.cpp))
TEST_OBJS += $(filter-out $(PL01_OBJ_DIR)/main.o, $(PL01_OBJS))
LIB_OBJS := $(patsubst $(LIB_DIR)/%.c, $(LIB_OBJ_DIR)/%.o, $(wildcard $(LIB_DIR)/*.c))
LIB_OBJS += $(patsubst $(LIB_DIR)/%.c, $(LIB_OBJ_DIR)/%.o, $(wildcard $(LIB_DIR)/**/*.c))
TEST_OBJS += $(LIB_OBJS)
PL01_TARGET := $(TARGET_DIR)/pl01
TEST_TARGET := $(TARGET_DIR)/test
LIB_TARGET := $(TARGET_DIR)/libpl01rt.a

# compiler & linker
# NOTE: only support macOS & Debian
OS := $(shell uname -s)
ifeq ($(OS), Darwin)
	include make/macos.make
else
	include make/debian.make
endif

# archiver
ARFLAGS := rcs
export AR = ar $(ARFLAGS)

.PHONY: all pl01 test lib clean

all: pl01 test lib

pl01: $(TARGET_DIR) $(PL01_OBJ_DIR) $(PL01_TARGET)

test: $(TARGET_DIR) $(TEST_OBJ_DIR) $(TEST_TARGET)

lib: $(TARGET_DIR) $(LIB_OBJ_DIR) $(LIB_TARGET)

clean:
	-rm -rf $(PL01_OBJ_DIR)
	-rm -rf $(TEST_OBJ_DIR)
	-rm -rf $(LIB_OBJ_DIR)
	-rm -f $(PL01_TARGET)
	-rm -f $(TEST_TARGET)
	-rm -f $(LIB_TARGET)

# directories
$(TARGET_DIR):
	mkdir $(TARGET_DIR)

$(PL01_OBJ_DIR):
	mkdir -p $(PL01_OBJ_DIR)

$(TEST_OBJ_DIR):
	mkdir -p $(TEST_OBJ_DIR)

$(LIB_OBJ_DIR):
	mkdir -p $(LIB_OBJ_DIR)

# PL/0.1
$(PL01_TARGET): $(PL01_OBJS)
	$(LD) -o $@ $^

$(PL01_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	-mkdir -p $(dir $@)
	$(CXX) -o $@ $^

# unit test
$(TEST_TARGET): $(TEST_OBJS)
	$(LD) -o $@ $^

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	-mkdir -p $(dir $@)
	$(CXX) -o $@ $^

# PL/0.1 runtime library
$(LIB_TARGET): $(LIB_OBJS)
	$(AR) $@ $^

$(LIB_OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	-mkdir -p $(dir $@)
	$(CC) -o $@ $^
