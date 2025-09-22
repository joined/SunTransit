#!/usr/bin/env bash

# Detect target - priority: command line arg > IDF_TARGET env var > sdkconfig fallback
if [ $# -gt 0 ]; then
    TARGET="$1"
    echo "Using target from command line: $TARGET"
elif [ -n "$IDF_TARGET" ]; then
    TARGET="$IDF_TARGET"
    echo "Using target from IDF_TARGET environment variable: $TARGET"
else
    echo "Error: No target specified. Use:"
    echo "  $0 <target>           # Command line argument"
    echo "  IDF_TARGET=<target>   # Environment variable"
    echo ""
    echo "Supported targets: esp32, esp32s3"
    exit 1
fi

# Set parameters based on target
case "$TARGET" in
    "esp32")
        CHIP="esp32"
        FLASH_MODE="qio"
        FLASH_FREQ="80m"
        FLASH_SIZE="4MB"
        APP_OFFSET="0x10000"
        WWW_OFFSET="0x300000"
        ;;
    "esp32s3")
        CHIP="esp32s3"
        FLASH_MODE="qio"
        FLASH_FREQ="80m"
        FLASH_SIZE="16MB"
        APP_OFFSET="0x10000"
        WWW_OFFSET="0xEE0000"
        ;;
    *)
        echo "Error: Unsupported target $TARGET"
        exit 1
        ;;
esac

echo "Building merged firmware for $TARGET..."
echo "  Flash mode: $FLASH_MODE"
echo "  Flash freq: $FLASH_FREQ"
echo "  Flash size: $FLASH_SIZE"

esptool.py --chip "$CHIP" merge_bin \
    -o ../build/merged-firmware.bin \
    --flash_mode "$FLASH_MODE" \
    --flash_freq "$FLASH_FREQ" \
    --flash_size "$FLASH_SIZE" \
    0x1000 ../build/bootloader/bootloader.bin \
    0x8000 ../build/partition_table/partition-table.bin \
    "$APP_OFFSET" ../build/suntransit.bin \
    "$WWW_OFFSET" ../build/www.bin
