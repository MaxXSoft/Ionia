# C++ compiler
CXXFLAGS := $(DEBUG_ARG) $(OPT_ARG) $(INCLUDE_ARG)
CXXFLAGS += -c -Wall -Werror -std=c++17
export CXX = clang++ $(CXXFLAGS)

# C compiler
CFLAGS := $(DEBUG_ARG) $(OPT_ARG) $(INCLUDE_ARG)
CFLAGS += -c -Wall -Werror
export CC = clang $(CFLAGS)

# linker
LDFLAGS := $(LIBRARY_ARG)
export LD = clang++ $(LDFLAGS)
