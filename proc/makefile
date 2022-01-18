NEOK_DIR = ${HOME}/neok/neok_lib
BUILD_CONFIG = default.config
BOARD_CONFIG = armA32.config

include ${NEOK_DIR}/config/${BUILD_CONFIG}
include ${NEOK_DIR}/config/${BOARD_CONFIG}

CFLAGS += -g -march=$(ARCH)$(VERSION)
CFLAGS += $(BOARD_FLAGS)

INCLUDES = -I. -I${NEOK_DIR}/public/

all: main con proc io rfs
	$(CC) -nostartfiles -T ${NEOK_DIR}/src/_start/lscript.ld \
	${NEOK_DIR}/bin/armv7-a_neoklib.a *.o -o proc.elf
	rm *.o
	@echo 'Finished building'

main:
	$(CC) $(CFLAGS) main.c $(INCLUDES) -o main.o

rfs:
	$(CC) $(CFLAGS) rfs.c $(INCLUDES) -o rfs.o

proc:
	$(CC) $(CFLAGS) proc.c $(INCLUDES) -o proc.o

io:
	$(CC) $(CFLAGS) proc_io.c $(INCLUDES) -o proc_io.o

con:
	$(CC) $(CFLAGS) connection.c $(INCLUDES) -o connection.o