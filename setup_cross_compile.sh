#!/bin/bash
# PDFSlipPrint v1.0 cross-compile bash script on Ubuntu 22.04
# Set base directory to the script's location
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PDFIUM_DIR="$BASE_DIR/pdfium"
BUILD_DIR="$BASE_DIR/build"
SRC_FILE="$BASE_DIR/PDFSlipPrint.cpp"
OUTPUT_EXE="$BUILD_DIR/PDFSlipPrint.exe"

# Set MinGW compiler
CXX=x86_64-w64-mingw32-g++

# Ensure required tools are installed
if ! command -v $CXX &> /dev/null; then
    echo "Error: MinGW-w64 not installed. Installing it..."
    sudo apt install -y g++-mingw-w64-x86-64-posix   
    exit 1
fi

# Check and install UPX if missing
if ! command -v upx &>/dev/null; then
    echo "UPX is not installed. Installing upx-ucl..."
    sudo apt update && sudo apt install -y upx-ucl
fi


# Create necessary directories
mkdir -p "$PDFIUM_DIR" "$BUILD_DIR"

# Download and extract PDFium if not already present
if [ ! -d "$PDFIUM_DIR/lib" ]; then
    echo "Downloading PDFium..."
    PDFIUM_URL="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x64.tgz"
    wget -O "$PDFIUM_DIR/pdfium-win-x64.tgz" "$PDFIUM_URL"
    
    echo "Extracting PDFium..."
    tar -xvzf "$PDFIUM_DIR/pdfium-win-x64.tgz" -C "$PDFIUM_DIR" 
fi

# Verify PDFium library
if [ ! -f "$PDFIUM_DIR/lib/pdfium.dll.lib" ]; then
    echo "Error: PDFium library not found!"
    exit 1
fi

# Compile the printpdf C++ program
echo "Compiling PDFSlipPrint..."
$CXX "$SRC_FILE" -o "$OUTPUT_EXE" \
    -I"$PDFIUM_DIR/include" \
    -L"$PDFIUM_DIR/lib" \
    -lgdi32 -luser32 -lkernel32 -lwinpthread   \
    -lwinspool -lpdfium.dll \
    -mconsole 

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo "Executable: $OUTPUT_EXE"
    
    # Compress with UPX
    # echo "Compressing with UPX..."
    #upx --fast  "$OUTPUT_EXE"
    #if [ $? -eq 0 ]; then
        #echo "✅ UPX compression successful!"
    #else
        #echo "❌ UPX compression failed!"
        #exit 1
    #fi    
    
else
    echo "❌ Compilation failed!"
    exit 1
fi


