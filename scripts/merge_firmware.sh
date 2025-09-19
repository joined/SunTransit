#!/usr/bin/env bash

esptool.py --chip esp32 merge_bin \
    -o ../build/merged-firmware.bin \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 4MB \
    0x1000 ../build/bootloader/bootloader.bin \
    0x8000 ../build/partition_table/partition-table.bin \
    0x20000 ../build/suntransit.bin \
    0x300000 ../build/www.bin \
