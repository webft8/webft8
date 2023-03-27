

DEBUG ?= 0


SDK_PATH=$(HOME)/bin/emsdk

BASE_OUT_DIR=./dist
OUTPUT_NAME=webft8_ft8_decode_js
FUNCTIONS=_free,_malloc,_webft8_ft8_decode_js,_webft8_buffer_create,_webft8_buffer_size,_webft8_buffer_pos,_webft8_buffer_data,_webft8_buffer_write,_webft8_buffer_destroy,_webft8_buffer_dump
RUNTIME_FUNCTIONS=ccall,cwrap,setValue,getValue,UTF8ToString,stringToUTF8,intArrayFromString,intArrayToString

LIB_FT8_DEPS=./ft8_lib/common/*.c \
		 ./ft8_lib/ft8/*.c \
		 ./ft8_lib/fft/*.c

LIB_FT8_DEPS_NATIVE=../../ft8_lib/common/*.c \
		 ../../ft8_lib/ft8/*.c \
		 ../../ft8_lib/fft/*.c

NATIVE_FILES=webft8_js.cpp \
		webft8_decode.cpp \
		webft8_encode.cpp \
		$(LIB_FT8_DEPS)

TOTAL_MEM = 128MB
TOTAL_STACK = 64MB
 
DEF_WARN_FLAGS= -Wfatal-errors -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-const-variable

HAS_VALGRIND := $(shell command -v valgrind 2> /dev/null)

ifeq ($(DEBUG), 1)
 $(info DEBUG BUILD)
 EMCC_DEBUG_FLAGS= -g3 -gsource-map  --source-map-base https://localhost:8443/webft8/ $(DEF_WARN_FLAGS) -Wno-limited-postlink-optimizations
 DEBUG_FLAGS= -g $(DEF_WARN_FLAGS)
 OPT_LEVEL= -O0
 EMCC_OPT_LEVEL= -O0
else
 $(info RELEASE BUILD)
 EMCC_DEBUG_FLAGS= -g3 -gsource-map  --source-map-base https://localhost:8443/webft8/ $(DEF_WARN_FLAGS) -Wno-limited-postlink-optimizations
 DEBUG_FLAGS= -g $(DEF_WARN_FLAGS)
 OPT_LEVEL= -O3
 EMCC_OPT_LEVEL= -Os
endif

all: test_telemetry mk_test_data cli cli_test node node_test web

full:
	DEBUG=1 make all cli_valgrind

clean:
	rm -rvf *.o webft8_cli ./dist/

cli:
	rm -rf $(BASE_OUT_DIR)/cli/
	mkdir -p $(BASE_OUT_DIR)/cli/
	cd $(BASE_OUT_DIR)/cli/ && \
		$(CC)  $(DEBUG_FLAGS) $(OPT_LEVEL)  -I../../ft8_lib/ \
		 -c -Wall -Wno-unused-variable -Wno-unused-function \
		$(LIB_FT8_DEPS_NATIVE)
	$(CXX) $(DEBUG_FLAGS) $(OPT_LEVEL) -std=c++17 -D_POSIX_C_SOURCE=199309L -I./ft8_lib/ webft8.cpp webft8_decode.cpp webft8_encode.cpp \
		-Wfatal-errors -Wall -Wno-unused-variable -Wno-unused-function -O0 -g $(BASE_OUT_DIR)/cli/*.o -o $(BASE_OUT_DIR)/cli/webft8_cli

# -std=gnu11 -D_POSIX_C_SOURCE=199309L

cli_valgrind:
ifndef HAS_VALGRIND
		$(info No valgrind in PATH, consider doing apt-get install valgrind on linux!)
endif
ifdef HAS_VALGRIND
		valgrind $(BASE_OUT_DIR)/cli/./webft8_cli
endif

cli_test:
	$(BASE_OUT_DIR)/cli/./webft8_cli -t
	$(BASE_OUT_DIR)/cli/./webft8_cli -d -f extra_test_data/2023-03-23T18_14_26.815Z.wav
	$(BASE_OUT_DIR)/cli/./webft8_cli -d -f extra_test_data/telemetry_7FFFFFFFFFFFFFFFFF.wav 
	# help: $(BASE_OUT_DIR)/cli/./webft8_cli -h
	$(BASE_OUT_DIR)/cli/./webft8_cli -e -i 7FFFFFFFFFFFFFFFFF

web: 
	rm -rf $(BASE_OUT_DIR)/web/
	mkdir -p $(BASE_OUT_DIR)/web/
	cp index.html $(BASE_OUT_DIR)/web/
	cp *.js $(BASE_OUT_DIR)/web/
	bash -c "source $(SDK_PATH)/emsdk_env.sh && \
		emcc -I. $(EMCC_DEBUG_FLAGS) $(EMCC_OPT_LEVEL) $(NATIVE_FILES) \
		-DTARGET_WEB=1 \
		-I./ft8_lib/ \
		-o $(BASE_OUT_DIR)/web/$(OUTPUT_NAME).js \
		-s EXPORTED_FUNCTIONS=$(FUNCTIONS) \
		-s EXPORTED_RUNTIME_METHODS=$(RUNTIME_FUNCTIONS) \
		-s ASSERTIONS=1 \
		-s EXPORT_NAME=webft8_ft8_decode_js -s EXPORT_ES6=1 -s MODULARIZE=1  \
		-s ENVIRONMENT=web -s TOTAL_MEMORY=$(TOTAL_MEM) -s TOTAL_STACK=$(TOTAL_STACK) "

not_used:
	# -s ALLOW_MEMORY_GROWTH=1  -s INITIAL_MEMORY=500MB -s MAXIMUM_MEMORY=500MB -s TOTAL_MEMORY=128MB  -s TOTAL_STACK=128MB
	
web_host:
	cd $(BASE_OUT_DIR)/web/ && python3 -m http.server 8080

node:
	rm -rf $(BASE_OUT_DIR)/node/
	mkdir -p $(BASE_OUT_DIR)/node/
	cp webft8_ft8_decode.js $(BASE_OUT_DIR)/node/
	cp test_data.js $(BASE_OUT_DIR)/node/
	cp webft8_node.mjs $(BASE_OUT_DIR)/node/webft8_node.mjs
	cp package.json $(BASE_OUT_DIR)/node/
	bash -c "source $(SDK_PATH)/emsdk_env.sh && \
		emcc $(EMCC_DEBUG_FLAGS) $(EMCC_OPT_LEVEL) \
		-I./ft8_lib/ \
		$(NATIVE_FILES) \
		-o $(BASE_OUT_DIR)/node/$(OUTPUT_NAME).js \
		-s EXPORTED_FUNCTIONS=$(FUNCTIONS) \
		-s EXPORTED_RUNTIME_METHODS=$(RUNTIME_FUNCTIONS) \
		-s ASSERTIONS=1 -s MODULARIZE=1 -s EXPORT_ES6 -s ENVIRONMENT=node \
		-s TOTAL_MEMORY=$(TOTAL_MEM) -s TOTAL_STACK=$(TOTAL_STACK) "
	#cd $(BASE_OUT_DIR)/node/ && node webft8_node.js

node_test:
	cd $(BASE_OUT_DIR)/node/ && node webft8_node.mjs

tools:
	rm -rf $(BASE_OUT_DIR)/tools/
	mkdir -p $(BASE_OUT_DIR)/tools/
	gcc ./scripts/bin2c.c -o $(BASE_OUT_DIR)/tools/bin2c

mk_test_data: tools
	$(BASE_OUT_DIR)/tools/bin2c ./ft8_lib/test/wav/websdr_test6.wav $(BASE_OUT_DIR)/tools/websdr_test6.wav.h websdr_test6_wav

test_telemetry:
	CC='$(CC) -std=gnu11 -D_POSIX_C_SOURCE=199309L' ./test_telemetry.sh

submodule_update:
	git submodule update --init --recursive

submodule_update_to_tip:
	git submodule update --recursive --remote