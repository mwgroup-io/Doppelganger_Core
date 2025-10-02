package main

import (
	"fmt"
	"os"
	"os/exec"
	"regexp"
	"runtime"
	"strconv"
	"strings"
)

func checkLinuxSerialPermissions(espPort string) {
	fmt.Printf("\033[1;33mChecking serial port permissions for %s...\033[0m\n", espPort)

	file, err := os.Open(espPort)
	if err != nil {
		if strings.Contains(err.Error(), "permission denied") {
			fmt.Printf("\033[1;31mPermission denied accessing %s\033[0m\n", espPort)
			fmt.Println("\033[1;33mYou need to be in the dialout group to access serial ports\033[0m")
			fmt.Println("\033[1;33mThe installer has already added you to the dialout group, but the changes\033[0m")
			fmt.Println("\033[1;33mneed to take effect. Please run one of the following:\033[0m")
			fmt.Println("")
			fmt.Println("\033[1;36mOption 1 (Recommended):\033[0m")
			fmt.Println("\033[1;33m  newgrp dialout\033[0m")
			fmt.Printf("\033[1;33m  ./%s_firmware_%s_linux_arm64\033[0m\n", strings.ToLower(productName), currentFirmwareVersion)
			fmt.Println("")
			fmt.Println("\033[1;36mOption 2:\033[0m")
			fmt.Println("\033[1;33m  Log out and log back in, then run the installer again\033[0m")
			os.Exit(1)
		} else {
			fmt.Printf("\033[1;31mError accessing %s: %v\033[0m\n", espPort, err)
			os.Exit(1)
		}
	}
	file.Close()

	fmt.Println("\033[1;32mSerial port permissions OK\033[0m")
}

func isLikelyESP32Port(port string) bool {
	portLower := strings.ToLower(port)
	for _, pattern := range esp32Patterns {
		if strings.Contains(portLower, pattern) {
			return true
		}
	}
	return false
}

func findSerialPort() string {
	clearScreen()
	var serialPorts []string

	// List serial ports based on the OS
	switch runtime.GOOS {
	case "linux":
		serialPorts = listSerialPortsLinux()
	case "darwin":
		serialPorts = listSerialPortsDarwin()
	case "windows":
		serialPorts = listSerialPortsWindows()
	default:
		fmt.Println("\033[1;31mUnsupported OS\033[0m")
		os.Exit(1)
	}

	if len(serialPorts) == 0 {
		fmt.Println("\033[1;31mNo serial ports found.\033[0m")
		fmt.Println("\033[1;33mPlease ensure your ESP32 device is connected via USB.\033[0m")
		os.Exit(1)
	}

	var esp32Ports []string
	var otherPorts []string
	var portDetails map[string]WindowsSerialPort
	var linuxPortDetails map[string]LinuxSerialPort

	if runtime.GOOS == "windows" {
		windowsPorts := getWindowsSerialPortDetails()
		portDetails = make(map[string]WindowsSerialPort)
		for _, winPort := range windowsPorts {
			portDetails[winPort.DeviceID] = winPort
			if winPort.IsESP32 {
				esp32Ports = append(esp32Ports, winPort.DeviceID)
			} else {
				otherPorts = append(otherPorts, winPort.DeviceID)
			}
		}
	} else if runtime.GOOS == "linux" {
		linuxPorts := getLinuxSerialPortDetails()
		linuxPortDetails = make(map[string]LinuxSerialPort)
		for _, linuxPort := range linuxPorts {
			linuxPortDetails[linuxPort.DevicePath] = linuxPort
			if linuxPort.IsESP32 {
				esp32Ports = append(esp32Ports, linuxPort.DevicePath)
			} else {
				otherPorts = append(otherPorts, linuxPort.DevicePath)
			}
		}
	} else {
		for _, port := range serialPorts {
			if isLikelyESP32Port(port) {
				esp32Ports = append(esp32Ports, port)
			} else {
				otherPorts = append(otherPorts, port)
			}
		}
	}

	fmt.Println("\033[1;36mAvailable serial ports:\033[0m")
	fmt.Println("")

	allPorts := append(esp32Ports, otherPorts...)
	for i, port := range allPorts {
		if runtime.GOOS == "windows" {
			if winPort, exists := portDetails[port]; exists {
				if winPort.IsESP32 {
					fmt.Printf("%d: %s \033[1;32m(ESP32 Device", i+1, port)

					isNativeUSB := (winPort.VendorID == "303A") ||
						(strings.Contains(strings.ToLower(winPort.Description), "usb serial device") && winPort.VendorID == "")

					if isNativeUSB {
						if winPort.VendorID == "303A" {
							fmt.Printf(" - Native USB")
							if winPort.ProductID != "" {
								fmt.Printf(" VID:303A PID:%s", winPort.ProductID)
							}
						} else {
							fmt.Printf(" - Likely ESP32-S3 Native USB")
						}
					} else if winPort.VendorID != "" && winPort.ProductID != "" {
						fmt.Printf(" - VID:%s PID:%s", winPort.VendorID, winPort.ProductID)
					}

					if winPort.Description != "" && winPort.Description != "Serial Port" && !isNativeUSB {
						fmt.Printf(" - %s", winPort.Description)
					}
					fmt.Printf(")\033[0m\n")
				} else {
					fmt.Printf("%d: %s", i+1, port)
					if winPort.Description != "" && winPort.Description != "Serial Port" {
						fmt.Printf(" (%s)", winPort.Description)
					}
					fmt.Printf("\n")
				}
			} else {
				fmt.Printf("%d: %s\n", i+1, port)
			}
		} else if runtime.GOOS == "linux" {
			if linuxPort, exists := linuxPortDetails[port]; exists {
				if linuxPort.IsESP32 {
					fmt.Printf("%d: %s \033[1;32m(ESP32 Device", i+1, port)

					isNativeUSB := (linuxPort.VendorID == "303A") ||
						(strings.Contains(strings.ToLower(linuxPort.Description), "usb serial") && linuxPort.VendorID == "")

					if isNativeUSB {
						if linuxPort.VendorID == "303A" {
							fmt.Printf(" - Native USB")
							if linuxPort.ProductID != "" {
								fmt.Printf(" VID:303A PID:%s", linuxPort.ProductID)
							}
						} else {
							fmt.Printf(" - Likely ESP32-S3 Native USB")
						}
					} else if linuxPort.VendorID != "" && linuxPort.ProductID != "" {
						fmt.Printf(" - VID:%s PID:%s", linuxPort.VendorID, linuxPort.ProductID)
					}

					if linuxPort.Description != "" && !isNativeUSB {
						fmt.Printf(" - %s", linuxPort.Description)
					}
					fmt.Printf(")\033[0m\n")
				} else {
					fmt.Printf("%d: %s", i+1, port)
					if linuxPort.Description != "" {
						fmt.Printf(" (%s)", linuxPort.Description)
					}
					fmt.Printf("\n")
				}
			} else {
				fmt.Printf("%d: %s\n", i+1, port)
			}
		} else {
			if isLikelyESP32Port(port) {
				fmt.Printf("%d: %s \033[1;32m(Likely ESP32)\033[0m\n", i+1, port)
			} else {
				fmt.Printf("%d: %s\n", i+1, port)
			}
		}
	}

	fmt.Println("")
	defaultSelection := 1
	if len(esp32Ports) > 0 {
		fmt.Printf("\033[1;32mRecommended: Port %d appears to be an ESP32 device\033[0m\n", defaultSelection)
		fmt.Println("")
		fmt.Printf("\033[1;33mSelect your ESP32 [%d]: \033[0m", defaultSelection)
	} else {
		fmt.Print("\033[1;33mSelect your ESP32: \033[0m")
	}

	var input string
	fmt.Scanln(&input)

	var selection int
	if input == "" && len(esp32Ports) > 0 {
		selection = defaultSelection
	} else {
		var err error
		selection, err = strconv.Atoi(input)
		if err != nil {
			fmt.Println("\033[1;31mInvalid input. Please enter a number.\033[0m")
			os.Exit(1)
		}
	}

	if selection < 1 || selection > len(allPorts) {
		fmt.Println("\033[1;31mInvalid selection.\033[0m")
		os.Exit(1)
	}

	clearScreen()

	return allPorts[selection-1]
}

// LinuxSerialPort represents a Linux serial port
type LinuxSerialPort struct {
	DevicePath  string
	VendorID    string
	ProductID   string
	Description string
	IsESP32     bool
}

func listSerialPortsLinux() []string {
	// Get detailed information about serial ports on Linux
	linuxPorts := getLinuxSerialPortDetails()

	var ports []string
	for _, port := range linuxPorts {
		ports = append(ports, port.DevicePath)
	}

	return ports
}

func getLinuxSerialPortDetails() []LinuxSerialPort {
	var ports []LinuxSerialPort

	// Method 1: Try the standard shell command
	output, err := exec.Command("sh", "-c", "ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null").Output()
	if err == nil {
		devicePaths := strings.Fields(string(output))
		for _, devicePath := range devicePaths {
			port := analyzeLinuxPort(devicePath)
			ports = append(ports, port)
		}
		if len(ports) > 0 {
			return ports
		}
	}

	// Method 2: Try with bash instead of sh
	output, err = exec.Command("bash", "-c", "ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null").Output()
	if err == nil {
		devicePaths := strings.Fields(string(output))
		for _, devicePath := range devicePaths {
			port := analyzeLinuxPort(devicePath)
			ports = append(ports, port)
		}
		if len(ports) > 0 {
			return ports
		}
	}

	// Method 3: Direct file system check (fallback)
	commonPorts := []string{"/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"}
	for _, devicePath := range commonPorts {
		if _, err := os.Stat(devicePath); err == nil {
			port := analyzeLinuxPort(devicePath)
			ports = append(ports, port)
		}
	}

	return ports
}

func analyzeLinuxPort(devicePath string) LinuxSerialPort {
	port := LinuxSerialPort{
		DevicePath: devicePath,
		IsESP32:    false,
	}

	// Try to get USB device information using udevadm
	cmd := exec.Command("udevadm", "info", "-q", "property", "-n", devicePath)
	output, err := cmd.Output()
	if err == nil {
		lines := strings.Split(string(output), "\n")
		for _, line := range lines {
			if strings.HasPrefix(line, "ID_VENDOR_ID=") {
				port.VendorID = strings.TrimPrefix(line, "ID_VENDOR_ID=")
			} else if strings.HasPrefix(line, "ID_MODEL_ID=") {
				port.ProductID = strings.TrimPrefix(line, "ID_MODEL_ID=")
			} else if strings.HasPrefix(line, "ID_SERIAL_SHORT=") {
				port.Description = strings.TrimPrefix(line, "ID_SERIAL_SHORT=")
			}
		}
	}

	// Check if this is likely an ESP32 device
	port.IsESP32 = isLikelyESP32DeviceLinux(port.VendorID, port.ProductID, port.Description)

	return port
}

func isLikelyESP32DeviceLinux(vendorID, productID, description string) bool {
	// Known ESP32 VID/PID combinations
	esp32VIDs := []string{"303A", "10C4", "0403", "1A86"}

	// Check VID
	for _, vid := range esp32VIDs {
		if strings.EqualFold(vendorID, vid) {
			return true
		}
	}

	// Check for ESP32 keywords in description
	esp32Keywords := []string{"esp32", "espressif", "cp210", "ch340", "ftdi"}
	descriptionLower := strings.ToLower(description)
	for _, keyword := range esp32Keywords {
		if strings.Contains(descriptionLower, keyword) {
			return true
		}
	}

	// ESP32-S3 specific detection (native USB CDC)
	if strings.EqualFold(vendorID, "303A") || strings.Contains(strings.ToLower(description), "usb serial") {
		return true
	}

	return false
}

func listSerialPortsDarwin() []string {
	output, err := exec.Command("sh", "-c", "ls /dev/tty.* 2>/dev/null").Output()
	if err != nil {
		return []string{}
	}
	return strings.Fields(string(output))
}

// WindowsSerialPort represents a serial port with additional Windows-specific information
type WindowsSerialPort struct {
	DeviceID    string
	Description string
	VendorID    string
	ProductID   string
	IsESP32     bool
}

func listSerialPortsWindows() []string {
	// Get detailed information about serial ports on Windows
	windowsPorts := getWindowsSerialPortDetails()

	var ports []string
	for _, port := range windowsPorts {
		ports = append(ports, port.DeviceID)
	}

	return ports
}

func getWindowsSerialPortDetails() []WindowsSerialPort {
	var ports []WindowsSerialPort

	// Use PowerShell to get detailed COM port information including VID/PID
	psScript := `
	Get-CimInstance -ClassName Win32_SerialPort | ForEach-Object {
		$pnpEntity = Get-CimInstance -ClassName Win32_PnPEntity | Where-Object { $_.Name -eq $_.Description }
		$hardwareID = ""
		if ($pnpEntity) {
			$hardwareID = $pnpEntity.HardwareID[0]
		}
		
		# Try to get hardware ID from DeviceID if not found
		if (-not $hardwareID) {
			$pnpDevice = Get-CimInstance -ClassName Win32_PnPEntity | Where-Object { $_.DeviceID -like "*$($_.DeviceID)*" }
			if ($pnpDevice) {
				$hardwareID = $pnpDevice.HardwareID[0]
			}
		}
		
		Write-Output "$($_.DeviceID)|$($_.Description)|$hardwareID"
	}`

	cmd := exec.Command("powershell", "-Command", psScript)
	output, err := cmd.Output()
	if err != nil {
		// Fallback to basic method
		basicCmd := exec.Command("powershell", "-Command", "Get-CimInstance -ClassName Win32_SerialPort | Select-Object -ExpandProperty DeviceID")
		basicOutput, basicErr := basicCmd.Output()
		if basicErr != nil {
			return ports
		}

		// Create basic port entries
		deviceIDs := strings.Fields(string(basicOutput))
		for _, deviceID := range deviceIDs {
			ports = append(ports, WindowsSerialPort{
				DeviceID:    deviceID,
				Description: "Serial Port",
				VendorID:    "",
				ProductID:   "",
				IsESP32:     false,
			})
		}
		return ports
	}

	lines := strings.Split(strings.TrimSpace(string(output)), "\n")
	for _, line := range lines {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}

		parts := strings.Split(line, "|")
		if len(parts) >= 2 {
			deviceID := strings.TrimSpace(parts[0])
			description := strings.TrimSpace(parts[1])
			hardwareID := ""
			if len(parts) >= 3 {
				hardwareID = strings.TrimSpace(parts[2])
			}

			// Extract VID and PID from hardware ID
			vendorID, productID := extractVidPid(hardwareID)

			// Determine if this is likely an ESP32 device
			isESP32 := isLikelyESP32Device(description, vendorID, productID)

			ports = append(ports, WindowsSerialPort{
				DeviceID:    deviceID,
				Description: description,
				VendorID:    vendorID,
				ProductID:   productID,
				IsESP32:     isESP32,
			})
		}
	}

	return ports
}

// extractVidPid extracts Vendor ID and Product ID from Windows hardware ID
func extractVidPid(hardwareID string) (string, string) {
	// Hardware ID format: USB\VID_10C4&PID_EA60\0001
	// Or: USB\VID_1A86&PID_7523\5&2E2A2B1B&0&2

	vidRegex := regexp.MustCompile(`VID_([0-9A-Fa-f]{4})`)
	pidRegex := regexp.MustCompile(`PID_([0-9A-Fa-f]{4})`)

	vidMatch := vidRegex.FindStringSubmatch(hardwareID)
	pidMatch := pidRegex.FindStringSubmatch(hardwareID)

	vendorID := ""
	productID := ""

	if len(vidMatch) > 1 {
		vendorID = strings.ToUpper(vidMatch[1])
	}

	if len(pidMatch) > 1 {
		productID = strings.ToUpper(pidMatch[1])
	}

	return vendorID, productID
}

// isLikelyESP32Device determines if a Windows serial port is likely an ESP32 device
func isLikelyESP32Device(description, vendorID, productID string) bool {
	// Common ESP32 VID/PID combinations
	esp32VidPids := map[string][]string{
		"10C4": {"EA60", "EA61"},                         // Silicon Labs CP210x (very common for ESP32 dev boards)
		"1A86": {"7523", "55D4"},                         // CH340/CH341 (common on cheap ESP32 boards)
		"0403": {"6001", "6010"},                         // FTDI (some ESP32 boards)
		"303A": {"1001", "1002", "0002", "0003", "4001"}, // Espressif native USB (ESP32-S2/S3/C3/C6/H2)
		"2E8A": {"0005"},                                 // Raspberry Pi Pico (sometimes used with ESP32)
	}

	// Check VID/PID combinations
	if pids, exists := esp32VidPids[vendorID]; exists {
		for _, pid := range pids {
			if pid == productID {
				return true
			}
		}
	}

	// Check description for ESP32-related keywords
	descLower := strings.ToLower(description)
	esp32Keywords := []string{
		"esp32", "esp-32", "espressif",
		"silicon labs cp210", "cp2102", "cp2104",
		"ch340", "ch341", "ch9102",
		"usb-serial", "uart bridge",
		"usb serial device",                 // Generic USB serial (could be ESP32-S3 native)
		"cdc", "communication device class", // Native USB CDC
	}

	for _, keyword := range esp32Keywords {
		if strings.Contains(descLower, keyword) {
			return true
		}
	}

	// Special case: If we see "USB Serial Device" with no specific VID/PID,
	// it could be an ESP32-S3 with native USB CDC
	if strings.Contains(descLower, "usb serial device") && vendorID == "" {
		return true
	}

	return false
}
