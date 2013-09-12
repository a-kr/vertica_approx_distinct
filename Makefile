############################
# Vertica Analytic Database
#
# Makefile to build example user defined functions
#
# Copyright (c) 2005 - 2012 Vertica, an HP company
############################

## Set to the location of the SDK installation
SDK_HOME?=/opt/vertica/sdk

CXX=g++
CXXFLAGS=-I $(SDK_HOME)/include -g -Wall -Wno-unused-value -shared -fPIC 
CXX_ADDL_FLAGS= 

ifdef OPTIMIZE
## UDLs should be compiled with compiler optimizations in release builds
CXX_ADDL_FLAGS-=$(CXX_ADDL_FLAGS) -O3
endif

## Set to the desired destination directory for .so output files
BUILD_DIR?=build

## Set to a valid temporary directory
TMPDIR?=/tmp

## Set to the path to 
BOOST_INCLUDE ?= /usr/include
CURL_INCLUDE ?= /usr/include
ZLIB_INCLUDE ?= /usr/include
BZIP_INCLUDE ?= /usr/include

.PHONEY: run

all: AggregateFunctions

$(BUILD_DIR)/.exists:
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	touch $(BUILD_DIR)/.exists

###
# Aggregate Functions
###
AggregateFunctions: $(BUILD_DIR)/AggregateFunctions.so

FUNC_LIB_SOURCES=src/EstimateCountDistinct.cpp src/MurmurHash3.cpp src/CardinalityEstimators.cpp

$(BUILD_DIR)/AggregateFunctions.so: $(FUNC_LIB_SOURCES) $(SDK_HOME)/include/Vertica.cpp $(SDK_HOME)/include/BuildInfo.h $(BUILD_DIR)/.exists
	$(CXX) $(CXXFLAGS) $(CXX_ADDL_FLAGS) -o $@ $(FUNC_LIB_SOURCES) $(SDK_HOME)/include/Vertica.cpp

TEST_MAIN_SOURCES=src/test_main.cpp src/MurmurHash3.cpp src/CardinalityEstimators.cpp

test_main: $(TEST_MAIN_SOURCES)
	$(CXX) -g -Wall -Werror -o $@ $(TEST_MAIN_SOURCES)


clean:
	rm -rf $(TMPDIR)/libcsv-3.0.1
	rm -f $(BUILD_DIR)/*.so 
	-rmdir $(BUILD_DIR) >/dev/null 2>&1 || true
