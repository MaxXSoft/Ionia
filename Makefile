# external parameters
export DEBUG = 1
export OPT_LEVEL = 3

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
IONIA_OBJ_DIR = $(OBJ_DIR)/ionia
SRC_DIR = $(TOP_DIR)/src
INCLUDE_ARG := -I$(SRC_DIR)
LIBRARY_ARG :=

# files
IONIA_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(IONIA_OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.cpp))
IONIA_OBJS += $(patsubst $(SRC_DIR)/%.cpp, $(IONIA_OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/**/*.cpp))
IONIA_TARGET := $(TARGET_DIR)/ionia

# compiler & linker
# NOTE: only support macOS & Debian
OS := $(shell uname -s)
ifeq ($(OS), Darwin)
	include make/macos.make
else
	include make/debian.make
endif

.PHONY: all ionia clean

all: ionia

ionia: $(TARGET_DIR) $(IONIA_OBJ_DIR) $(IONIA_TARGET)

clean:
	-rm -rf $(IONIA_OBJ_DIR)
	-rm -f $(IONIA_TARGET)

# directories
$(TARGET_DIR):
	mkdir $(TARGET_DIR)

$(IONIA_OBJ_DIR):
	mkdir -p $(IONIA_OBJ_DIR)

# Ionia
$(IONIA_TARGET): $(IONIA_OBJS)
	$(LD) -o $@ $^

$(IONIA_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	-mkdir -p $(dir $@)
	$(CXX) -o $@ $^
