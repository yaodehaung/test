CC ?= cc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=

TARGET := mini_express
SRC := main.c
UNAME_S := $(shell uname -s)

.PHONY: all run debug clean unsupported

ifeq ($(UNAME_S),Darwin)
all run debug: unsupported

unsupported:
	@echo "This project uses Linux epoll and must be built on Linux."
	@echo "Try building inside a Linux VM/container or on a Linux host."
	@exit 1
else
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS := -Wall -Wextra -g -O0
debug: clean $(TARGET)
endif

clean:
	rm -f $(TARGET)
