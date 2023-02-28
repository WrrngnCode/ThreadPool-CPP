
BUILD_DIR = build
EXE_SUBFOLDER = out/Debug
EXECUTABLE_NAME ?= test_basic.exe

#GENERATOR = "Unix Makefiles"
GENERATOR = "Ninja Multi-Config"
CMAKE_CXX_COMPILER = "C:/msys64/mingw64/bin/g++.exe"
CMAKE_C_COMPILER = "C:/msys64/mingw64/bin/gcc.exe"

CMAKE_CXX_COMPILER_MSYS = "/mingw64/bin/g++"
CMAKE_C_COMPILER_MSYS = "/c/msys2/mingw64/bin/gcc"

VERBOSE ?= OFF
MAKEFLAGS += $(if $(ifeq($(VERBOSE),ON)),,--no-print-directory)

VERBOSE_FLAG += $(if $(ifeq($(VERBOSE),ON)),-v,)

.PHONY: execute clean generate build distclean

all: generate build test
	@echo Finished generating, building and testing the project

generate:
	cmake -S . -B build -DENABLE_TESTING=1 -DCMAKE_VERBOSE_MAKEFILE:BOOL=$(VERBOSE) -DCMAKE_CXX_COMPILER:FILEPATH=$(CMAKE_CXX_COMPILER) -DCMAKE_C_COMPILER:FILEPATH=$(CMAKE_C_COMPILER) -DTHREADPOOL_DEBUG:STRING=ON  -G $(GENERATOR)

build-debug:
	cmake --build build --config Debug --target all -j 12 $(VERBOSE_FLAG)

build-release:
	cmake --build build --config Release --target all -j 12 $(VERBOSE_FLAG)

build: build-release

execute:
	./$(BUILD_DIR)/$(EXE_SUBFOLDER)/$(EXECUTABLE_NAME)

build-debug-execute: build-debug execute

test: build-release
	ctest --test-dir build -N
	ctest --test-dir build -C Release --timeout 90 -V

test-debug: build-debug
	ctest --test-dir build -N
	ctest --test-dir build -C Debug --timeout 90 -V
	
rerun-failed:
	ctest --test-dir build -C Release --timeout 90 -V --rerun-failed --output-on-failure
	
install: build-release
	cmake --install build --config Release --prefix "C:/msys64/mingw64" $(VERBOSE_FLAG)

#coverage:
#	@echo Work in Progress. TO DO: COVERAGE
#	cmake -S . -B build -DENABLE_COVERAGE=1 -DCMAKE_BUILD_TYPE=Coverage -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_VERBOSE_MAKEFILE:BOOL=$(VERBOSE) -DCMAKE_CXX_COMPILER:FILEPATH=$(CMAKE_CXX_COMPILER) -DCMAKE_C_COMPILER:FILEPATH=$(CMAKE_C_COMPILER) -DTHREADPOOL_DEBUG:STRING=ON  -G $(GENERATOR) 
#	cmake --build build --target ThreadPool-C_coverage -j4 -v

clean:
	cmake --build build --config Debug --target clean -j 12 -v
	cmake --build build --config Release --target clean -j 12 -v
# make -C build clean
# ninja -C build -f build-Debug.ninja clean $(VERBOSE_FLAG)
# ninja -C build -f build-Release.ninja clean $(VERBOSE_FLAG)


distclean:
	rm -rf build/*.o
	rm -rf build/.cmake
	rm -rf build/Makefile
	rm -rf build
	mkdir build









