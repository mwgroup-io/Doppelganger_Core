package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
)

// Flash memory addresses for ESP32-S3
var bootloader = "0x0000"
var partitions = "0x8000"
var bootapp0 = "0xe000"
var firmware = "0x10000"
var littlefs = "0x670000"

var logFile string
var verbose bool

const currentFirmwareVersion = "1.1.0"

var productName string
var productDisplayName string

var flashMessages = map[string]string{
	"bootloader.bin": "Uploading the bootloader...",
	"partitions.bin": "Setting up the partitions...",
	"boot_app0.bin":  "Uploading app0...",
	"firmware.bin":   "Applying the firmware update...",
	"LittleFS.bin":   "Uploading the filesystem...",
}

var esp32Patterns = []string{
	"usbmodem", "usbserial", "ttyUSB", "ttyACM", "cu.wchusbserial", "cu.SLAB_USBtoUART", "cu.usbserial",
}

// logVerbose prints output only in verbose mode
func logVerbose(format string, args ...interface{}) {
	if verbose {
		fmt.Printf(format, args...)
	}
}

// cleanupLogFiles removes temporary log files
func cleanupLogFiles() {
	matches, err := filepath.Glob(".serial_log_*.log")
	if err == nil {
		for _, match := range matches {
			os.Remove(match)
		}
	}

	matches, err = filepath.Glob(".post_flash_log_*.log")
	if err == nil {
		for _, match := range matches {
			os.Remove(match)
		}
	}
}

// fileExists checks if a file exists
func fileExists(filename string) bool {
	_, err := os.Stat(filename)
	return !os.IsNotExist(err)
}

// askUserConfirmation prompts for yes/no confirmation
func askUserConfirmation(prompt string) bool {
	fmt.Print(prompt)
	var response string
	fmt.Scanln(&response)
	response = strings.ToLower(strings.TrimSpace(response))
	return response == "y" || response == "yes"
}

// clearScreen clears the terminal
func clearScreen() {
	var cmd *exec.Cmd
	switch runtime.GOOS {
	case "windows":
		cmd = exec.Command("cmd", "/c", "cls")
	default:
		cmd = exec.Command("clear")
	}
	cmd.Stdout = os.Stdout
	cmd.Run()
}

// detectProduct analyzes the firmware binary to determine the product type
func detectProduct(firmwareData []byte) {
	// Default to Stealth for backward compatibility
	productName = "Stealth"
	productDisplayName = "Doppelgänger Stealth v2"

	// Check if firmware contains Core device identifier
	firmwareStr := string(firmwareData)

	// Debug: Show what we're looking for
	logVerbose("Searching for Core identifiers in firmware binary...\n")

	// Only look for the specific device declaration to avoid false positives
	if strings.Contains(firmwareStr, "device = \"Core\"") {
		logVerbose("Found: device = \"Core\"\n")
		productName = "Core"
		productDisplayName = "Doppelgänger Core"
	} else {
		logVerbose("Core device identifier not found, defaulting to Stealth\n")
	}

	logVerbose("Detected product: %s (analyzed %d bytes)\n", productDisplayName, len(firmwareData))
}
