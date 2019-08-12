OS := $(shell uname -s | tr A-Z a-z)
CFLAGS := -DNDEBUG -O2 -std=c11
CXXFLAGS := -DNDEBUG -O2 -std=c++17
ARFLAGS := -rcs
LDFLAGS := -fPIC

# lib/libxchg.so & lib/libxchg_static.a

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -o $@ -c $<

.PHONY: all shared static test fuzz examples docs clean

all: shared static

shared: $(OBJ)
ifeq ($(OS),darwin)
	$(CC) $(LDFLAGS) -dynamiclib -o lib/libxchg.dylib $^
else
	$(CC) $(LDFLAGS) -shared -o lib/libxchg.so $^
endif

static: $(OBJ)
	$(AR) $(ARFLAGS) lib/libxchg_static.a $^

# bin/xchg_tests

TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(TEST_SRC:.cpp=.o)

TEST_CFLAGS ?= $(CFLAGS)
TEST_CXXFLAGS ?= $(CXXFLAGS)
TEST_LDFLAGS ?= $(LDFLAGS)

test: CFLAGS := $(TEST_CFLAGS)
test: CXXFLAGS := $(TEST_CXXFLAGS)
test: LDFLAGS := $(TEST_LDFLAGS)
test: $(TEST_OBJ) | all
	$(CXX) $(CXXFLAGS) -Iinclude $(LDFLAGS) -Llib -lxchg_static -o bin/xchg_tests $^
	@bin/xchg_tests -s -r compact

# bin/xchg_fuzz

FUZZ_SRC := $(wildcard tests/fuzz/*.cpp)
FUZZ_OBJ := $(FUZZ_SRC:.cpp=.o)

AFL_CC := afl-clang
AFL_CXX := afl-clang++
AFL_CFLAGS := -DDEBUG -O0 -std=c11
AFL_CXXFLAGS := -DDEBUG -O0 -std=c++17
AFL_LDFLAGS ?= $(LDFLAGS)

fuzz: CC := $(AFL_CC)
fuzz: CXX := $(AFL_CXX)
fuzz: CFLAGS := $(AFL_CFLAGS)
fuzz: CXXFLAGS := $(AFL_CXXFLAGS)
fuzz: LDFLAGS := $(AFL_LDFLAGS)
fuzz: export AFL_HARDEN=1
fuzz: $(FUZZ_OBJ) | all
	$(CXX) $(CXXFLAGS) -Iinclude $(LDFLAGS) -Llib -lxchg_static -o bin/xchg_fuzz $^
	@afl-fuzz -i tests/fuzz/testcases -o tests/fuzz/findings -- bin/xchg_fuzz

# bin/examples_serialization

examples:
	$(CC) $(CFLAGS) -Iinclude $(LDFLAGS) -Llib -lxchg_static -o bin/xchg_examples_serialization examples/serialization.c

# misc

docs:
	doxygen

clean:
	@rm -rf bin/*
	@rm -rf lib/*
	@rm -f src/*.o
	@rm -f tests/*.o
	@rm -f tests/fuzz/*.o
	@rm -rf tests/fuzz/findings/*
