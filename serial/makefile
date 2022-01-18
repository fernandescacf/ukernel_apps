NEOK_DIR = ${HOME}/neok/neok_lib
BUILD_CONFIG = default.config
BOARD_CONFIG = sunxi-h3.config

include ${NEOK_DIR}/config/${BUILD_CONFIG}
include ${NEOK_DIR}/config/${BOARD_CONFIG}

CFLAGS += -g -march=$(ARCH)$(VERSION)
CFLAGS += $(BOARD_FLAGS)

INCLUDES = -I. -I${NEOK_DIR}/public/

all: uart main
	@mkdir -p out/$(BOARD)
	$(CC) -nostartfiles -T ${NEOK_DIR}/src/_start/lscript.ld \
	${NEOK_DIR}/bin/armv7-a_neoklib.a *.o -o out/$(BOARD)/serial.elf
	rm *.o
	@echo 'Finished building'

main:
	$(CC) $(CFLAGS) main.c $(INCLUDES) -o main.o

uart:
	$(CC) $(CFLAGS) $(VARIANT) $(BOARD)/uart.c $(INCLUDES) -o uart.o
