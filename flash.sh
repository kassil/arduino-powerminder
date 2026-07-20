#!/bin/sh
# Flash script for Arduino projects
# Automatically detects board and programmer from cmake build configuration
# 
# Usage: $0 [SERIAL_PORT]
#   SERIAL_PORT: Serial port device or symlink (e.g., /dev/ttyUSB0, /dev/ttyACM0, or /dev/serial/by-id/...)

set -e  # Fail fast on any error

# Source cmake-generated config
BUILD_CONFIG="$(dirname "$0")/build/config.sh"
if [ -f "$BUILD_CONFIG" ]; then
    . "$BUILD_CONFIG"
    # Validate that config was properly sourced
    if [ -z "$BOARD" ] || [ -z "$MCU" ]; then
        echo "Error: Build config incomplete (BOARD='$BOARD', MCU='$MCU')" >&2
        exit 1
    fi
else
    # Fallback defaults if config.sh doesn't exist
    BOARD="${BOARD:-uno}"
    MCU="${MCU:-atmega328p}"
fi

# Serial port from command line or board-specific default
case "$BOARD" in
    leonardo)
        DEFAULT_PORT="/dev/serial/by-id/usb-Arduino_LLC_Arduino_Leonardo-if00"
        ;;
    uno)
        DEFAULT_PORT="/dev/ttyUSB0"
        ;;
esac

SER_PORT="${1:-$DEFAULT_PORT}"
case "$BOARD" in
    leonardo)
        PROGRAMMER="avr109"    # Leonardo uses avr109 protocol (not arduino)
        BAUD=57600             # Caterina bootloader speed
        ;;
    uno)
        PROGRAMMER="arduino"   # Uno uses arduino protocol (optiboot)
        BAUD=115200            # optiboot speed
        ;;
    *)
        echo "Error: Unknown board '$BOARD'" >&2
        exit 1
        ;;
esac

echo "Flashing board: $BOARD (MCU: $MCU)"
echo "Programmer: $PROGRAMMER"
echo "Serial port: $SER_PORT"
echo "Baud rate: $BAUD"
echo "Hex file: build/ArduinoProject.hex"
echo ""

avrdude -p "$MCU" -c "$PROGRAMMER" -P "$SER_PORT" -b "$BAUD" -U flash:w:build/ArduinoProject.hex:i
