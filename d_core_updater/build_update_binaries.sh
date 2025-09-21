#!/bin/bash
# Doppelgänger Core Firmware Builder
# Created: September 21, 2025
# Version: 2.0.0
# Author: @tweathers-sec

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

# Resolve paths so the script works from any $PWD
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Verbose logging
log_verbose() {
  if [ "$VERBOSE" = true ]; then
    echo -e "$@"
  fi
}

# Regular output
log_info() {
  echo -e "$@"
}

if [ "$CLEANUP" = true ]; then
  log_info "\033[1;36m=== Cleaning Build Artifacts ===\033[0m"
  log_verbose "\033[1;33mRemoving build directories and files...\033[0m"
  
  rm -rf build/
  rm -rf bin_files_*
  rm -rf enc_files_*
  rm -rf installers_*
  rm -f *.bin
  rm -f *.enc
  rm -f go.mod go.sum
  rm -f .serial_log_*.log
  rm -f test_updater
  rm -f bindata.go
  rm -f main.go.bak
  
  log_info "\033[1;32m✓ Cleanup completed!\033[0m"
  exit 0
fi

if [ "$HELP" = true ]; then
  echo -e "\n\033[1;36mDoppelgänger Core Firmware Builder\033[0m"
  echo -e "\033[1;33mUsage:\033[0m"
  echo -e "  $0                    # Build firmware (version from src/version_config.cpp)"
  echo -e "  $0 -v, --verbose     # Build with detailed output"
  echo -e "  $0 clean             # Clean all build artifacts"
  echo -e "  $0 -h, --help        # Show this help"
  echo ""
  exit 0
fi

log_info "\033[1;32m=== Doppelgänger Core Build Process ===\033[0m"
log_verbose "\033[1;33mExtracting version information from src/version_config.cpp...\033[0m"

VERSION=$(grep 'const char \*version = ' "$PROJECT_ROOT/src/version_config.cpp" | cut -d'"' -f2)
DEVICE=$(grep 'const char \*device = ' "$PROJECT_ROOT/src/version_config.cpp" | cut -d'"' -f2)
BUILDDATE=$(grep 'const char \*builddate = ' "$PROJECT_ROOT/src/version_config.cpp" | cut -d'"' -f2)

if [ -z "$VERSION" ] || [ -z "$DEVICE" ]; then
  log_info "\033[1;31mError: Could not extract version information from src/main.cpp\033[0m"
  exit 1
fi

log_info "\033[1;32mDevice: $DEVICE | Version: $VERSION | Build: $BUILDDATE\033[0m"
log_verbose "\033[1;36mSimplified Build - No Encryption Required\033[0m"
log_verbose ""

export PATH=$PATH:~/.platformio/penv/bin
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

log_verbose "\033[1;32mCopying binary files...\033[0m"
cp "$PROJECT_ROOT"/.pio/build/esp32-s3-devkitc-1/*.bin .
cp $HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin .

if [ -f "littlefs.bin" ] && [ ! -f "LittleFS.bin" ]; then
  log_verbose "\033[1;33mNormalizing littlefs.bin -> LittleFS.bin\033[0m"
  cp littlefs.bin LittleFS.bin
fi

REQUIRED_FILES=("bootloader.bin" "partitions.bin" "boot_app0.bin" "firmware.bin" "LittleFS.bin")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "\033[1;31mError: Required file $file not found!\033[0m"
        exit 1
    fi
done

if [ ! -f go.mod ]; then
  log_info "\033[1;32mInitializing Go module...\033[0m"
  go mod init d_core_builder 2>/dev/null
  NEED_DEPS=true
else
  # Check if dependencies are already present
  if ! grep -q "github.com/jacobsa/go-serial" go.mod || ! grep -q "github.com/briandowns/spinner" go.mod; then
    NEED_DEPS=true
  else
    NEED_DEPS=false
  fi
fi

if [ "$NEED_DEPS" = true ]; then
  log_info "\033[1;32mFetching Go dependencies...\033[0m"
  if [ "$VERBOSE" = true ]; then
    go get github.com/jacobsa/go-serial/serial
    go get github.com/briandowns/spinner
    go mod tidy
  else
    go get github.com/jacobsa/go-serial/serial >/dev/null 2>&1
    go get github.com/briandowns/spinner >/dev/null 2>&1
    go mod tidy >/dev/null 2>&1
  fi
else
  log_verbose "\033[1;33mGo dependencies already present, skipping fetch\033[0m"
fi

TIME=$(date +%Y%m%d%H%M%S)
log_verbose "\033[1;32mCreating directories...\033[0m"
mkdir -p "$SCRIPT_DIR"/bin_files_$TIME
mkdir -p "$SCRIPT_DIR"/installers_${VERSION}_$TIME

log_verbose "\033[1;32mVerifying required files for embedding...\033[0m"
if [ ! -f "bootloader.bin" ] || [ ! -f "partitions.bin" ] || [ ! -f "firmware.bin" ] || [ ! -f "LittleFS.bin" ] || [ ! -f "boot_app0.bin" ]; then
  log_info "\033[1;31mMissing required binary files! Cannot build installer.\033[0m"
  log_info "\033[1;33mFound files:\033[0m"
  ls -la *.bin 2>/dev/null || echo "No .bin files found"
  exit 1
fi

log_verbose "\033[1;32m✓ All required files present:\033[0m"
if [ "$VERBOSE" = true ]; then
  ls -la *.bin
fi

log_verbose "\033[1;32mArchiving binary files...\033[0m"
cp *.bin "$SCRIPT_DIR"/bin_files_$TIME/

log_info "\033[1;32mBuilding installers...\033[0m"

platforms=("darwin_amd64" "darwin_arm64" "linux_amd64" "linux_arm" "linux_arm64" "windows_amd64" "windows_arm64")

for platform in "${platforms[@]}"; do
  IFS='_' read -r os arch <<< "$platform"
  log_verbose "\033[1;33mBuilding for $os/$arch...\033[0m"
  
  if [ "$os" = "windows" ]; then
    output_file="${DEVICE}_firmware_${VERSION}_${os}_${arch}.exe"
  else
    output_file="${DEVICE}_firmware_${VERSION}_${os}_${arch}"
  fi
  
  if [ "$VERBOSE" = true ]; then
    env GOOS=$os GOARCH=$arch go build -ldflags="-s -w" -o "$output_file" main.go
  else
    env GOOS=$os GOARCH=$arch go build -ldflags="-s -w" -o "$output_file" main.go >/dev/null 2>&1
  fi
  
  if [ $? -eq 0 ]; then
    log_verbose "\033[1;32m✓ Built $output_file\033[0m"
  else
    log_info "\033[1;31m✗ Failed to build for $os/$arch\033[0m"
  fi
done

log_verbose "\033[1;32mMoving installers to final directory...\033[0m"
mv ${DEVICE}_firmware_* "$SCRIPT_DIR"/installers_${VERSION}_$TIME/

# Create final build structure
log_verbose "\033[1;32mOrganizing build artifacts...\033[0m"
mkdir -p "$SCRIPT_DIR"/build/artifacts
mv "$SCRIPT_DIR"/bin_files_$TIME "$SCRIPT_DIR"/build/artifacts/
mv "$SCRIPT_DIR"/installers_${VERSION}_$TIME "$SCRIPT_DIR"/build/

# Create a README for the installers
cat > "$SCRIPT_DIR"/build/installers_${VERSION}_$TIME/README.txt << EOF
Doppelgänger Core Firmware Updater
Device: $DEVICE
Version: $VERSION
Build Date: $BUILDDATE
Compiled: $(date)

INSTALLATION INSTRUCTIONS:
1. Connect your ESP32 device via USB
2. Run the appropriate installer for your platform:
   - macOS Intel: ${DEVICE}_firmware_${VERSION}_darwin_amd64
   - macOS Apple Silicon: ${DEVICE}_firmware_${VERSION}_darwin_arm64
   - Linux x64: ${DEVICE}_firmware_${VERSION}_linux_amd64
   - Linux ARM: ${DEVICE}_firmware_${VERSION}_linux_arm
   - Linux ARM64: ${DEVICE}_firmware_${VERSION}_linux_arm64
   - Windows x64: ${DEVICE}_firmware_${VERSION}_windows_amd64.exe
   - Windows ARM64: ${DEVICE}_firmware_${VERSION}_windows_arm64.exe
3. Select your ESP32 port when prompted (auto-suggested)
4. Device will automatically reboot and update

REQUIREMENTS:
- esptool (will be auto-installed on macOS/Linux)
- USB drivers for ESP32 (usually auto-installed)

For support, contact: support@mayweathergroup.com
EOF

# Clean up temporary files
log_verbose "\033[1;32mCleaning up temporary files...\033[0m"
rm -f *.bin

log_info "\033[1;32m✓ Build completed successfully!\033[0m"
log_info ""
log_info "\033[1;36mBuild Summary:\033[0m"
log_info "Device: $DEVICE | Version: $VERSION | Build: $BUILDDATE"
log_verbose "Compiled: $(date)"
log_info ""
log_info "\033[1;32mInstallers: $SCRIPT_DIR/build/installers_${VERSION}_$TIME\033[0m"
