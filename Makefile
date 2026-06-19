CC ?= cc
CXX ?= c++
CPPFLAGS ?= -Ilib/core-net -Ilib/core -Ilib/misc -Iexample/controller
CFLAGS ?= -Wall -Wextra -O2
CXXFLAGS ?= $(CFLAGS) -std=c++11
LDFLAGS ?=

TARGET := mini_express
MAIN_SRC := example/main.c
SRC := lib/core/mini_express.c lib/misc/hash_map.c \
	lib/core-net/epoll_server.c lib/misc/json_parser.c lib/misc/static_files.c \
	lib/roles/roles.c lib/roles/http1.c lib/roles/http2.c \
	lib/roles/http3.c lib/roles/ws.c example/controller/cache_controller.c
OBJ := main.o $(SRC:.c=.o)
WS_FRAGMENT_TEST := tests/ws_fragment_test
UNAME_S := $(shell uname -s)

.PHONY: all run debug test test-js test-ws-fragments clean unsupported

test: test-ws-fragments

test-ws-fragments: tests/ws_fragment_test.c lib/roles/ws.c lib/roles/ws.h
	$(CC) $(CFLAGS) tests/ws_fragment_test.c lib/roles/ws.c -o $(WS_FRAGMENT_TEST)
	./$(WS_FRAGMENT_TEST)

test-js:
	node tests/ws_handshake_test.js

ifeq ($(UNAME_S),Darwin)
all run debug: unsupported

unsupported:
	@echo "This project uses Linux epoll and must be built on Linux."
	@echo "Try building inside a Linux VM/container or on a Linux host."
	@exit 1
else
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

main.o: $(MAIN_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -x c++ -c $(MAIN_SRC) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

run-80: $(TARGET)
	sudo ./$(TARGET) 80

debug: CFLAGS := -Wall -Wextra -g -O0
debug: clean $(TARGET)
endif

clean:
	rm -f $(TARGET) $(OBJ) $(WS_FRAGMENT_TEST)
