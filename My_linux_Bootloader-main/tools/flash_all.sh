#!/bin/bash

# Flash Both Bootloader and Application
# This script flashes both bootloader and application to STM32F401

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

print_info "=========================================="
print_info "Flashing Bootloader + Application"
print_info "=========================================="
echo ""

# Flash bootloader
print_info "Step 1/2: Flashing Bootloader..."
if ! ./tools/flash_bootloader.sh; then
    print_error "Flashing Bootloader failed!"
    exit 1
fi
print_info "Bootloader flashed successfully!"

sleep 2

# Flash application
print_info "Step 2/2: Flashing Application..."
if ! ./tools/flash_application.sh; then
    print_error "Flashing Application failed!"
    exit 1
fi
print_info "Application flashed successfully!"

print_info "=========================================="
print_info "Flash Complete!"
print_info "=========================================="
echo ""
print_info "What to expect:"
echo "  1. Reset the board (press NRST button)"
echo "  2. LED will blink FAST (2Hz) - Bootloader running"
echo "  3. After ~1 second, LED will blink SLOW (0.5Hz) - Application running"
echo ""
print_info "LED Behavior Guide:"
echo "  - Fast blink (2Hz):     Bootloader is running, checking application"
echo "  - Slow blink (0.5Hz):   Application is running successfully"
echo "  - Solid on:            Error - No valid application found"
echo "  - Off:                 Hardware/Power issue"
echo ""
