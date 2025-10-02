package main

import (
	_ "embed"
	"flag"
	"fmt"
	"os"
	"runtime"
)

//go:embed bootloader.bin
var bootloaderBin []byte

//go:embed partitions.bin
var partitionsBin []byte

//go:embed boot_app0.bin
var bootApp0Bin []byte

//go:embed firmware.bin
var firmwareBin []byte

//go:embed LittleFS.bin
var littleFSBin []byte

func main() {
	flag.BoolVar(&verbose, "v", false, "Enable verbose output with detailed debugging information")
	flag.BoolVar(&verbose, "verbose", false, "Enable verbose output with detailed debugging information")
	flag.Parse()

	// Detect product type from embedded firmware
	detectProduct(firmwareBin)

	fmt.Printf("\033[1;36m=== %s Firmware Updater ===\033[0m\n", productDisplayName)
	fmt.Println("")

	installEsptool()
	espPort := findSerialPort()

	if runtime.GOOS == "linux" {
		checkLinuxSerialPermissions(espPort)
	}

	if verbose {
		fmt.Println("\033[1;32mStarting firmware update...\033[0m")
	} else {
		fmt.Println("\033[1;32mFlashing firmware...\033[0m")
	}
	tryFlash(espPort)

	fmt.Println("\033[1;32m✓ Firmware update completed successfully!\033[0m")
	fmt.Println("\033[1;32m✓ Device is now running the new firmware.\033[0m")
	fmt.Println("\033[1;36mNote: Manually power cycle the device before use.\033[0m")

	logVerbose("\033[1;33mCleaning up temporary files...\033[0m\n")
	os.Remove("bootloader.bin")
	os.Remove("partitions.bin")
	os.Remove("boot_app0.bin")
	os.Remove("firmware.bin")
	os.Remove("LittleFS.bin")
	cleanupLogFiles()

	fmt.Println("")
	os.Stdout.Sync()
}
