#!/bin/bash

make -C ../neok_lib/
make -C proc/
make -C cmd/
make -C echo/
make -C ls/
make -C cat/
make -C sloader/
make -C serial/ BOARD_CONFIG=sunxi-h3.config
make -C timer/ BOARD_CONFIG=sunxi-h3.config
make -C gpio/ BOARD_CONFIG=sunxi-h3.config

./../rfs_generator/rfs_generator ../rfs_generator/script_sunxi.txt ../rfs_generator/rfs.bin
