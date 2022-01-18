NEOK_DIR = ${HOME}/neok/neok_lib
BUILD_CONFIG = default.config
BOARD_CONFIG = sunxi-h3.config

include ${NEOK_DIR}/config/${BUILD_CONFIG}
include ${NEOK_DIR}/config/${BOARD_CONFIG}

CFLAGS += -g -march=$(ARCH)$(VERSION)
CFLAGS += $(BOARD_FLAGS)

INCLUDES = -I. -I${NEOK_DIR}/public/

all: out gpio main
	$(CC) -nostartfiles -T ${NEOK_DIR}/src/_start/lscript.ld \
	${NEOK_DIR}/bin/armv7-a_neoklib.a *.o -o out/$(BOARD)/gpio.elf
	rm *.o
	@echo 'Finished building'

out:
	@mkdir -p out/$(BOARD)

main:
	$(CC) $(CFLAGS) main.c $(INCLUDES) -o main.o

gpio:
	$(CC) $(CFLAGS) $(VARIANT) $(BOARD)/gpio.c $(INCLUDES) -o gpio.o
