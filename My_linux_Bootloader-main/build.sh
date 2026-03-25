#!/bin/bash

# STM32 Bootloader Build Script
# This script builds the bootloader and/or application for STM32F401

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to check if toolchain is installed
check_toolchain() {
    print_info "Checking toolchain..."
    
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        print_error "arm-none-eabi-gcc not found. Please install the toolchain:"
        echo "  sudo apt-get install gcc-arm-none-eabi"
        exit 1
    fi
    
    if ! command -v arm-none-eabi-objcopy &> /dev/null; then
        print_error "arm-none-eabi-objcopy not found."
        exit 1
    fi
    
    print_info "Toolchain OK: $(arm-none-eabi-gcc --version | head -n1)"
}

# Function to clean build directories
clean() {
    print_info "Cleaning build directories..."
    rm -rf bootloader/build
    rm -rf application/build
    print_info "Clean complete."
}

# Function to build bootloader
build_bootloader() {
    print_info "Building Bootloader..."
    cd bootloader
    make clean
    if make all; then
        print_info "Bootloader build successful!"
        print_info "Output: bootloader/build/bootloader.bin"
        ls -lh build/bootloader.bin 2>/dev/null || print_warning "Binary file not found"
        cd ..
    else
        print_error "Bootloader build failed!"
        cd ..
        exit 1
    fi
}

# Function to build application
build_application() {
    print_info "Building Application..."
    cd application
    make clean
    if make all; then
        print_info "Application build successful!"
        print_info "Output: application/build/application.bin"
        ls -lh build/application.bin 2>/dev/null || print_warning "Binary file not found"
        cd ..
    else
        print_error "Application build failed!"
        cd ..
        exit 1
    fi
}

# Function to show sizes
show_sizes() {
    print_info "Binary sizes:"
    echo ""
    echo "Bootloader:"
    arm-none-eabi-size bootloader/build/bootloader.elf 2>/dev/null || echo "  Not built"
    echo ""
    echo "Application:"
    arm-none-eabi-size application/build/application.elf 2>/dev/null || echo "  Not built"
    echo ""
}

# Main script
main() {
    case "${1:-all}" in
        clean)
            clean
            ;;
        bootloader)
            check_toolchain
            build_bootloader
            show_sizes
            ;;
        application)
            check_toolchain
            build_application
            show_sizes
            ;;
        all)
            check_toolchain
            build_bootloader
            build_application
            show_sizes
            print_info "Build complete!"
            ;;
        *)
            echo "Usage: $0 {all|bootloader|application|clean}"
            echo ""
            echo "Options:"
            echo "  all         - Build both bootloader and application (default)"
            echo "  bootloader  - Build only bootloader"
            echo "  application - Build only application"
            echo "  clean       - Clean all build artifacts"
            exit 1
            ;;
    esac
}

main "$@"
