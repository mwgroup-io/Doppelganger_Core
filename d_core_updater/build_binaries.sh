#!/bin/bash
# Doppelgänger Firmware Builder
# Version: 2.0.0
# 
# Cross-platform firmware build system for ESP32-S3 devices
# Supports: Doppelgänger Stealth v2 and Doppelgänger Core

# Global verbose flag
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -v|--verbose)
      VERBOSE=true
      shift
      ;;
    clean|cleanup)
      CLEANUP=true
      shift
      ;;
    -h|--help|help)
      HELP=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Helper function for verbose logging
log_verbose() {
  if [ "$VERBOSE" = true ]; then
    echo -e "$@"
  fi
}

# Helper function for regular output
log_info() {
  echo -e "$@"
}

# Handle cleanup option
if [ "$CLEANUP" = true ]; then
  log_info "\033[1;36m=== Cleaning Build Artifacts ===\033[0m"
  log_verbose "\033[1;33mRemoving build directories and files...\033[0m"
  
  rm -rf build/
  rm -rf bin_files_*
  rm -rf enc_files_*
  rm -rf installers_*
  rm -f *.bin
  rm -f *.enc
  rm -f src/*.bin
  rm -f src/go.mod src/go.sum
  rm -f .serial_log_*.log
  rm -f test_updater
  rm -f bindata.go
  rm -f main.go.bak
  
  log_info "\033[1;32m✓ Cleanup completed!\033[0m"
  exit 0
fi

# Check for help
if [ "$HELP" = true ]; then
  echo -e "\n\033[1;36mDoppelgänger Firmware Builder\033[0m"
  echo -e "\033[1;33mUsage:\033[0m"
  echo -e "  $0                    # Build firmware (version from src/main.cpp)"
  echo -e "  $0 -v, --verbose     # Build with detailed output"
  echo -e "  $0 clean             # Clean all build artifacts"
  echo -e "  $0 -h, --help        # Show this help"
  echo ""
  echo -e "\033[1;33mSupported Products:\033[0m"
  echo -e "  - Doppelgänger Stealth v2"
  echo -e "  - Doppelgänger Core"
  echo -e "  (Product auto-detected from src/main.cpp)"
  echo ""
  exit 0
fi

# Determine script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

log_info "\033[1;32m=== Doppelgänger Firmware Build Process ===\033[0m"
log_verbose "\033[1;33mExtracting version information...\033[0m"

# Try main.cpp first (Stealth v2 format)
VERSION=$(grep 'const char \*version = ' "$PROJECT_ROOT/src/main.cpp" 2>/dev/null | cut -d'"' -f2)
DEVICE=$(grep 'const char \*device = ' "$PROJECT_ROOT/src/main.cpp" 2>/dev/null | cut -d'"' -f2)
BUILDDATE=$(grep 'const char \*builddate = ' "$PROJECT_ROOT/src/main.cpp" 2>/dev/null | cut -d'"' -f2)

# If not found, try version_config.cpp (Core format)
if [ -z "$VERSION" ] || [ -z "$DEVICE" ]; then
  log_verbose "\033[1;33mTrying src/version_config.cpp...\033[0m"
  VERSION=$(grep 'const char \*version = ' "$PROJECT_ROOT/src/version_config.cpp" 2>/dev/null | cut -d'"' -f2)
  DEVICE=$(grep 'const char \*device = ' "$PROJECT_ROOT/src/version_config.cpp" 2>/dev/null | cut -d'"' -f2)
  BUILDDATE=$(grep 'const char \*builddate = ' "$PROJECT_ROOT/src/version_config.cpp" 2>/dev/null | cut -d'"' -f2)
fi

if [ -z "$VERSION" ] || [ -z "$DEVICE" ]; then
  log_info "\033[1;31mError: Could not extract version information\033[0m"
  log_info "\033[1;33mChecked files:\033[0m"
  log_info "  - src/main.cpp"
  log_info "  - src/version_config.cpp"
  log_info "\033[1;33mExpected format:\033[0m"
  log_info "  const char *version = \"1.1.0\";"
  log_info "  const char *device = \"Core\";"
  log_info "  const char *builddate = \"21SEP2025\";"
  exit 1
fi

log_info "\033[1;32mProduct: Doppelgänger $DEVICE | Version: $VERSION | Build: $BUILDDATE\033[0m"
log_verbose ""
export PATH=$PATH:~/.platformio/penv/bin

# Change to project root for PlatformIO build
cd "$PROJECT_ROOT"

log_info "\033[1;32mBuilding firmware...\033[0m"
if [ "$VERBOSE" = true ]; then
  platformio run --environment esp32-s3-devkitc-1
else
  platformio run --environment esp32-s3-devkitc-1 --silent
fi

log_info "\033[1;32mBuilding filesystem...\033[0m"
if [ "$VERBOSE" = true ]; then
  platformio run --target buildfs --environment esp32-s3-devkitc-1
else
  platformio run --target buildfs --environment esp32-s3-devkitc-1 --silent
fi

cd "$SCRIPT_DIR"
log_verbose "\033[1;32mCopying binary files to src directory...\033[0m"
cp "$PROJECT_ROOT/.pio/build/esp32-s3-devkitc-1"/*.bin src/
cp "$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin" src/

# Verify required files exist
REQUIRED_FILES=("bootloader.bin" "partitions.bin" "boot_app0.bin" "firmware.bin" "LittleFS.bin")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "src/$file" ]; then
        echo -e "\033[1;31mError: Required file $file not found!\033[0m"
        exit 1
    fi
done

# Initialize Go module if needed
if [ ! -f src/go.mod ]; then
  log_info "\033[1;32mInitializing Go module...\033[0m"
  (cd src && go mod init d_stealth_builder) 2>/dev/null
  NEED_DEPS=true
else
  if ! grep -q "github.com/jacobsa/go-serial" src/go.mod || ! grep -q "github.com/briandowns/spinner" src/go.mod; then
    NEED_DEPS=true
  else
    NEED_DEPS=false
  fi
fi

# Fetch Go dependencies if needed
if [ "$NEED_DEPS" = true ]; then
  log_info "\033[1;32mFetching Go dependencies...\033[0m"
  if [ "$VERBOSE" = true ]; then
    (cd src && go get github.com/jacobsa/go-serial/serial)
    (cd src && go get github.com/briandowns/spinner)
    (cd src && go mod tidy)
  else
    (cd src && go get github.com/jacobsa/go-serial/serial) >/dev/null 2>&1
    (cd src && go get github.com/briandowns/spinner) >/dev/null 2>&1
    (cd src && go mod tidy) >/dev/null 2>&1
  fi
else
  log_verbose "\033[1;33mGo dependencies already present, skipping fetch\033[0m"
fi

# Create build directories
TIME=$(date +%Y%m%d%H%M%S)
log_verbose "\033[1;32mCreating directories...\033[0m"
mkdir -p bin_files_$TIME
mkdir -p installers_${VERSION}_$TIME

# Verify binary files for embedding
log_verbose "\033[1;32mVerifying required files for embedding...\033[0m"
if [ ! -f "src/bootloader.bin" ] || [ ! -f "src/partitions.bin" ] || [ ! -f "src/firmware.bin" ] || [ ! -f "src/LittleFS.bin" ] || [ ! -f "src/boot_app0.bin" ]; then
  log_info "\033[1;31mMissing required binary files! Cannot build installer.\033[0m"
  log_info "\033[1;33mFound files:\033[0m"
  ls -la src/*.bin 2>/dev/null || echo "No .bin files found"
  exit 1
fi

log_verbose "\033[1;32m✓ All required files present:\033[0m"
if [ "$VERBOSE" = true ]; then
  ls -la src/*.bin
fi

# Archive binary files
log_verbose "\033[1;32mArchiving binary files...\033[0m"
cp src/*.bin bin_files_$TIME/

log_info "\033[1;32mBuilding installers...\033[0m"
platforms=("darwin_amd64" "darwin_arm64" "linux_amd64" "linux_arm" "linux_arm64" "windows_amd64" "windows_arm64")

for platform in "${platforms[@]}"; do
  IFS='_' read -r os arch <<< "$platform"
  log_verbose "\033[1;33mBuilding for $os/$arch...\033[0m"
  
  # Set file extension for Windows and convert device name to lowercase
  DEVICE_LOWER=$(echo "$DEVICE" | tr '[:upper:]' '[:lower:]')
  if [ "$os" = "windows" ]; then
    output_file="${DEVICE_LOWER}_firmware_${VERSION}_${os}_${arch}.exe"
  else
    output_file="${DEVICE_LOWER}_firmware_${VERSION}_${os}_${arch}"
  fi
  
  if [ "$VERBOSE" = true ]; then
    (cd src && env GOOS=$os GOARCH=$arch go build -ldflags="-s -w" -o "../$output_file" .)
  else
    (cd src && env GOOS=$os GOARCH=$arch go build -ldflags="-s -w" -o "../$output_file" .) >/dev/null 2>&1
  fi
  
  if [ $? -eq 0 ]; then
    log_verbose "\033[1;32m✓ Built $output_file\033[0m"
  else
    log_info "\033[1;31m✗ Failed to build for $os/$arch\033[0m"
  fi
done

log_verbose "\033[1;32mMoving installers to final directory...\033[0m"
DEVICE_LOWER=$(echo "$DEVICE" | tr '[:upper:]' '[:lower:]')
mv ${DEVICE_LOWER}_firmware_* installers_${VERSION}_$TIME/

# Organize build artifacts
log_verbose "\033[1;32mOrganizing build artifacts...\033[0m"
mkdir -p build/artifacts
mv bin_files_$TIME build/artifacts/
mv installers_${VERSION}_$TIME build/

# Generate installer documentation
cat > build/installers_${VERSION}_$TIME/README.txt << EOF
Doppelgänger $DEVICE Firmware Updater
Product: Doppelgänger $DEVICE
Version: $VERSION
Build Date: $BUILDDATE
Compiled: $(date)

INSTALLATION INSTRUCTIONS:
1. Connect your ESP32-S3 device via USB
2. Run the appropriate installer for your platform:
   - macOS Intel: ${DEVICE_LOWER}_firmware_${VERSION}_darwin_amd64
   - macOS Apple Silicon: ${DEVICE_LOWER}_firmware_${VERSION}_darwin_arm64
   - Linux x64: ${DEVICE_LOWER}_firmware_${VERSION}_linux_amd64
   - Linux ARM: ${DEVICE_LOWER}_firmware_${VERSION}_linux_arm
   - Linux ARM64: ${DEVICE_LOWER}_firmware_${VERSION}_linux_arm64
   - Windows x64: ${DEVICE_LOWER}_firmware_${VERSION}_windows_amd64.exe
   - Windows ARM64: ${DEVICE_LOWER}_firmware_${VERSION}_windows_arm64.exe
3. Select your ESP32 port when prompted (auto-suggested)
4. Device will automatically reboot and update

REQUIREMENTS:
- esptool (will be auto-installed on macOS/Linux)
- USB drivers for ESP32-S3 (usually auto-installed)

SUPPORTED PRODUCTS:
- Doppelgänger Stealth v2 (ESP32-S3 based RFID tool)
- Doppelgänger Core (ESP32-S3 based RFID tool)

For support, contact: support@mayweathergroup.com
EOF

# Clean up temporary files
log_verbose "\033[1;32mCleaning up temporary files...\033[0m"
rm -f *.bin
rm -f src/*.bin

log_info "\033[1;32m✓ Build completed successfully!\033[0m"
log_info ""
log_info "\033[1;36mBuild Summary:\033[0m"
log_info "Product: Doppelgänger $DEVICE | Version: $VERSION | Build: $BUILDDATE"
log_verbose "Compiled: $(date)"
log_info ""
log_info "\033[1;32mInstallers: $SCRIPT_DIR/build/installers_${VERSION}_$TIME\033[0m"
