CC ?= cc
CXX ?= c++
CFLAGS ?= -Wall -Wextra -O2
CXXFLAGS ?= $(CFLAGS) -std=c++11
LDFLAGS ?=

TARGET := mini_express
MAIN_SRC := main.c
SRC := lib/core/mini_express.c lib/misc/hash_map.c \
	lib/core-net/epoll_server.c lib/misc/json_parser.c lib/misc/static_files.c \
	lib/roles/roles.c lib/roles/http1.c lib/roles/http2.c \
	lib/roles/http3.c lib/roles/ws.c
OBJ := main.o $(SRC:.c=.o)
UNAME_S := $(shell uname -s)

.PHONY: all run debug test-js clean unsupported

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
	$(CXX) $(CXXFLAGS) -x c++ -c $(MAIN_SRC) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

run-80: $(TARGET)
	sudo ./$(TARGET) 80

test-js:
	node tests/ws_handshake_test.js

debug: CFLAGS := -Wall -Wextra -g -O0
debug: clean $(TARGET)
endif

clean:
	rm -f $(TARGET) $(OBJ)
