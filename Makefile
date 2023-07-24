# C/C++ Makefile v2.2.2 2021-Set-17 Jeisson Hidalgo ECCI-UCR CC-BY 4.0

# Compiler and tool flags
CC=gcc
XC=g++
DEFS=-DB_CWPX_CONTEXT
FLAGS=$(strip -Wall -Wextra -Wconversion -fPIC $(DEFS))
FLAGC=$(FLAGS) -std=gnu11
FLAGX=$(FLAGS) -std=gnu++11
LIBS=-lm -ldl
LINTF=-build/header_guard,-build/include_subdir
LINTC=$(LINTF),-readability/casting
LINTX=$(LINTF),-build/c++11,-runtime/references
ARGS=

# Directories
LIB_DIR=lib
#OBJ_DIR=build
OBJ_DIR=lib
DOC_DIR=doc
SRC_DIR=src
TST_DIR=tests
EXP_DIR=examples
prefix=/usr/lib

# If src/ dir does not exist, use current directory .
ifeq "$(wildcard $(SRC_DIR) )" ""
	SRC_DIR=.
endif

# Add another place for finding the libcwpx.so library
CWPXLIB=-Wl,-R$(shell pwd)/lib

# Files
# Enr: Added "" (quotes) in "$(SRC_DIR)" for the find command to work on Windows
DIRS=$(shell find "$(SRC_DIR)" -type d)
APPNAME=$(shell basename $(shell pwd))
HEADERC=$(wildcard $(DIRS:%=%/*.h))
HEADERX=$(wildcard $(DIRS:%=%/*.hpp))
#SOURCEC=$(wildcard $(DIRS:%=%/*.c))
#SOURCEC = $(filter-out src/cwpx_cc.c, $(wildcard src/*.c))
SOURCEC= $(SRC_DIR)/cwpx_misc.c $(SRC_DIR)/cwpx_queue.c $(SRC_DIR)/cwpx_reqparser.c $(SRC_DIR)/cwpx_reqhandler.c $(SRC_DIR)/cwpx_resphandler.c $(SRC_DIR)/cwpx_memfile.c $(SRC_DIR)/cwpx_context.c
SOURCEX=$(wildcard $(DIRS:%=%/*.cpp))
INPUTFC=$(strip $(HEADERC) $(SOURCEC))
INPUTFX=$(strip $(HEADERX) $(SOURCEX))
INPUTCX=$(strip $(INPUTFC) $(INPUTFX))
OBJECTC=$(SOURCEC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJECTX=$(SOURCEX:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJECTS=$(strip $(OBJECTC) $(OBJECTX))
TESTINF=$(wildcard $(TST_DIR)/input*.txt)
TESTOUT=$(TESTINF:$(TST_DIR)/input%.txt=$(OBJ_DIR)/output%.txt)
INCLUDE=-Iinclude
DEPENDS=$(OBJECTS:%.o=%.d)
IGNORES=$(LIB_DIR) $(OBJ_DIR) $(DOC_DIR)
LIBFILE=$(LIB_DIR)/libcwpx.so
EXEARGS=$(strip $(LIBFILE) $(ARGS))
LD=$(if $(SOURCEC),$(CC),$(XC))


# Check OS for changing extension from .so to .dll
# https://stackoverflow.com/questions/714100/os-detecting-makefile
ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Windows)
    LIBFILE=$(LIB_DIR)/libcwpx.dll
endif


# Targets
default: debug
all: doc lint memcheck helgrind test
debug: FLAGS += -g
debug: $(LIBFILE)
release: FLAGS += -O3 -DNDEBUG
release: $(LIBFILE)
asan: FLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: debug
msan: FLAGS += -fsanitize=memory
msan: CC = clang
msan: XC = clang++
msan: debug
tsan: FLAGS += -fsanitize=thread
tsan: debug
ubsan: FLAGS += -fsanitize=undefined
ubsan: debug

-include $(DEPENDS)
.SECONDEXPANSION:

# Linker call
$(LIBFILE): $(OBJECTS) | $$(@D)/.
	$(LD) -shared $(FLAGS) $(INCLUDE) $^ -o $@ $(LIBS)

# Compile C source file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $$(@D)/.
	$(CC) -c $(FLAGC) $(INCLUDE) -MMD $< -o $@

# Compile C++ source file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(@D)/.
	$(XC) -c $(FLAGX) $(INCLUDE) -MMD $< -o $@

# Create a subdirectory if not exists
.PRECIOUS: %/.
%/.:
	mkdir -p $(dir $@)

# Test cases
.PHONY: test
test: $(LIBFILE) $(TESTOUT)

$(OBJ_DIR)/output%.txt: SHELL:=/bin/bash
$(OBJ_DIR)/output%.txt: $(TST_DIR)/input%.txt $(TST_DIR)/output%.txt
	icdiff --no-headers $(word 2,$^) <($(EXEARGS) < $<)

# Documentation
doc: $(INPUTCX)
	doxygen

# Utility rules
.PHONY: lint run memcheck helgrind gitignore clean instdeps install

lint:
ifneq ($(INPUTFC),)
	cpplint --filter=$(LINTC) $(INPUTFC)
endif
ifneq ($(INPUTFX),)
	cpplint --filter=$(LINTX) $(INPUTFX)
endif

run: $(LIBFILE)
	$(EXEARGS)

memcheck: $(LIBFILE)
	valgrind --tool=memcheck $(EXEARGS)

helgrind: $(LIBFILE)
	valgrind --quiet --tool=helgrind $(EXEARGS)

gitignore:
	echo $(IGNORES) | tr " " "\n" > .gitignore

clean:
	rm -rf $(IGNORES)

# Install dependencies (Debian)
instdeps:
	sudo apt install build-essential clang valgrind icdiff doxygen graphviz \
	python3-gpg && sudo pip3 install cpplint

install: $(LIBFILE)
	sudo cp -p $< $(prefix)


.PHONY: icwpx
icwpx: lib/libicwpx.so
lib/libicwpx.so: include/icwpx.h src/cwpx_icwpx.c
	$(CC) -c src/cwpx_icwpx.c -Iinclude -fPIC -o lib/cwpx_icwpx.o
	$(CC) -Wall -Wextra -Wconversion  lib/cwpx_misc.o lib/cwpx_icwpx.o -shared -o lib/libicwpx.so $(LIBS)


.PHONY: cwpxcc
cwpxcc: bin/cwpxcc
bin/cwpxcc: include/cwpx_globals.h src/cwpx_cc.c
	$(CC) -Iinclude src/cwpx_cc.c -o bin/cwpxcc


.PHONY: example
example: bin/examples/helloworld.cwpx
bin/examples/helloworld.cwpx: include/cwpx.h examples/helloworld.c $(LIBFILE)
	$(CC) -Iinclude examples/helloworld.c -o bin/examples/helloworld.cwpx -Llib -lcwpx $(CWPXLIB)


#.PHONY: scripttests
#scripttests: bin/tests/example_tests
#bin/tests/example_tests: tests/example_tests.c
#	$(CC) tests/example_tests.c -o bin/tests/example_tests


help:
	@echo "Usage make [-jN] [VAR=value] [target]"
	@echo "  -jN       Compile N files simultaneously [N=1]"
	@echo "  VAR=value Overrides a variable, e.g CC=mpicc DEFS=-DGUI"
	@echo "  all       Run targets: doc lint [memcheck helgrind] test"
	@echo "  asan      Build for detecting memory leaks and invalid accesses"
	@echo "  clean     Remove generated directories and files"
	@echo "  debug     Build an executable for debugging [default]"
	@echo "  doc       Generate documentation from sources with Doxygen"
	@echo "  gitignore Generate a .gitignore file"
	@echo "  helgrind  Run executable for detecting thread errors with Valgrind"
	@echo "  instdeps  Install needed packages on Debian-based distributions"
	@echo "  lint      Check code style conformance using Cpplint"
	@echo "  memcheck  Run executable for detecting memory errors with Valgrind"
	@echo "  msan      Build for detecting uninitialized memory usage"
	@echo "  release   Build an optimized executable"
	@echo "  run       Run executable using ARGS value as arguments"
	@echo "  test      Run executable against test cases in folder tests/"
	@echo "  tsan      Build for detecting thread errors, e.g race conditions"
	@echo "  ubsan     Build for detecting undefined behavior"

