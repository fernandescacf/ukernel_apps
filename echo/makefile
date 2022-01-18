NEOK_DIR = ${HOME}/neok/neok_lib
BUILD_CONFIG = ${NEOK_DIR}/config/default.config
BOARD_CONFIG = ${NEOK_DIR}/config/ve-a9.config

include ${BUILD_CONFIG}
include ${BOARD_CONFIG}

CFLAGS += -O2 -march=$(ARCH)$(VERSION)
CFLAGS += $(BOARD_FLAGS)

INCLUDES = -I. -I${NEOK_DIR}/public/

.PHONY: api

all: main
	$(CC) -nostartfiles -T ${NEOK_DIR}/src/_start/lscript.ld \
	${NEOK_DIR}/bin/armv7-a_neoklib.a main.o -o echo.elf
	rm *.o
	@echo 'Finished building'

main:
	$(CC) $(CFLAGS) main.c $(INCLUDES) -o main.o

