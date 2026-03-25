#!/bin/bash

# Debug Application Script
# This script starts GDB for debugging the application

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

# Check if application ELF exists
if [ ! -f "application/build/application.elf" ]; then
    print_error "Application ELF not found. Please build first:"
    echo "  ./build.sh application"
    exit 1
fi

print_info "Starting GDB for Application debugging..."
echo ""
print_info "Make sure OpenOCD is running in another terminal:"
echo "  openocd -f tools/openocd_stm32f4.cfg"
echo ""
print_info "Useful GDB commands:"
echo "  load              - Load the ELF file"
echo "  break main        - Set breakpoint at main"
echo "  continue          - Continue execution"
echo "  step/next         - Step through code"
echo "  info registers    - Show registers"
echo "  print variable    - Print variable value"
echo "  x/10x 0x08010000  - Examine memory at application start"
echo "  quit              - Quit GDB"
echo ""
print_info "Note: Application starts at 0x08010000"
echo ""

# Start GDB
arm-none-eabi-gdb application/build/application.elf \
    -ex "target extended-remote :3333" \
    -ex "monitor reset halt"
