#!/bin/bash

# Flash Application Script
# This script flashes the application to STM32F401 using GDB + OpenOCD

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if application binary exists
if [ ! -f "application/build/application.elf" ]; then
    print_error "Application ELF not found. Please build first:"
    echo "  ./build.sh application"
    exit 1
fi

print_info "Flashing Application to STM32F401..."
echo ""

# Flash application using OpenOCD directly
openocd -f tools/openocd_stm32f4.cfg -c "program application/build/application.elf verify reset exit"

if [ $? -eq 0 ]; then
    print_info "Application flashed successfully!"
    echo ""
    print_info "Reset the board to run the application."
    echo "The bootloader will jump to the application automatically."
    echo ""
    echo "To debug, run:"
    echo "  ./tools/debug_application.sh"
else
    print_error "Flashing failed!"
    exit 1
fi
