# C++ compiler
CXXFLAGS := $(DEBUG_ARG) $(OPT_ARG) $(INCLUDE_ARG)
CXXFLAGS += -c -Wall -Werror -std=c++17
export CXX = g++ $(CXXFLAGS)

# C compiler
CFLAGS := $(DEBUG_ARG) $(OPT_ARG) $(INCLUDE_ARG)
CFLAGS += -c -Wall -Werror
export CC = gcc $(CFLAGS)

# linker
LDFLAGS := $(LIBRARY_ARG)
export LD = g++ $(LDFLAGS)
