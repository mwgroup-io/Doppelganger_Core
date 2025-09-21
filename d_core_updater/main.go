// Doppelgänger Core Firmware Updater
// Created: September 21, 2025
// Version: 2.0.0
// Author: @tweathers-sec
package main

import (
	"bufio"
	_ "embed"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"regexp"
	"runtime"
	"strconv"
	"strings"
	"time"

	"github.com/briandowns/spinner"
	"github.com/jacobsa/go-serial/serial"
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

// Flash memory addresses for ESP32-S3
var bootloader = "0x0000"
var partitions = "0x8000"
var bootapp0 = "0xe000"
var firmware = "0x10000"
var littlefs = "0x670000"

var logFile string
var verbose bool

func logVerbose(format string, args ...interface{}) {
	if verbose {
		fmt.Printf(format, args...)
	}
}

var validVersionStrings = []string{"Core", "D_Core"}

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

func installEsptool() {
	switch runtime.GOOS {
	case "linux":
		installEsptoolLinux()
	case "darwin":
		installEsptoolDarwin()
	case "windows":
		installEsptoolWindows()
	default:
		fmt.Println("\033[1;31mUnsupported OS\033[0m")
		os.Exit(1)
	}
}

func installEsptoolLinux() {
	_, err := exec.LookPath("esptool")
	if err != nil {
		fmt.Println("\033[1;33mEsptool is not installed. Attempting to install using apt...\033[0m")

		cmdUpdate := exec.Command("sudo", "apt", "update")
		cmdUpdate.Stdout = os.Stdout
		cmdUpdate.Stderr = os.Stderr
		err := cmdUpdate.Run()
		if err != nil {
			fmt.Printf("\033[1;31mError updating package list: %v\033[0m\n", err)
			os.Exit(1)
		}

		cmdInstall := exec.Command("sudo", "apt", "install", "-y", "esptool")
		cmdInstall.Stdout = os.Stdout
		cmdInstall.Stderr = os.Stderr
		err = cmdInstall.Run()
		if err != nil {
			fmt.Printf("\033[1;31mError installing esptool using apt: %v\033[0m\n", err)
			os.Exit(1)
		}
		fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")
	}
	checkEsptoolVersion("esptool")
}

func installEsptoolDarwin() {
	_, err := exec.LookPath("esptool.py")
	if err != nil {
		fmt.Println("\033[1;33mEsptool is not installed. Attempting to install using Homebrew...\033[0m")
		_, err := exec.LookPath("brew")
		if err == nil {
			cmd := exec.Command("brew", "install", "esptool")
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			err := cmd.Run()
			if err != nil {
				fmt.Printf("\033[1;31mError installing esptool using Homebrew: %v\033[0m\n", err)
				os.Exit(1)
			}
			fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")
		} else {
			fmt.Println("\033[1;31mHomebrew is not installed. Please install Homebrew first.\033[0m")
			fmt.Println("\033[1;31mInstall Homebrew: https://brew.sh/\033[0m")
			fmt.Printf("\033[1;31mAlternatively, you can install esptool directly from GitHub: https://github.com/espressif/esptool/releases\033[0m\n")
			os.Exit(1)
		}
	}
	checkEsptoolVersion("esptool.py")
}

func installEsptoolWindows() {
	_, err := exec.LookPath("esptool.exe")
	if err != nil {
		fmt.Println("\033[1;33mEsptool is not installed on Windows.\033[0m")
		fmt.Println("\033[1;33mPlease install Python and esptool:\033[0m")
		fmt.Println("\033[1;33m1. Install Python from https://python.org\033[0m")
		fmt.Println("\033[1;33m2. Run: pip install esptool\033[0m")
		fmt.Println("\033[1;33mOr download esptool binary from: https://github.com/espressif/esptool/releases\033[0m")
		os.Exit(1)
	}
	checkEsptoolVersion("esptool.exe")
}

func checkEsptoolVersion(command string) {
	cmdVersion := exec.Command(command, "version")
	versionOutput, err := cmdVersion.CombinedOutput()
	if err != nil {
		fmt.Printf("\033[1;31mError checking esptool version: %v\033[0m\nOutput: %s\n", err, string(versionOutput))
		os.Exit(1)
	}
	version := strings.TrimSpace(string(versionOutput))
	if !strings.Contains(version, "esptool") || !isVersionCompatible(version, "4.5.0") {
		fmt.Printf("\033[1;31mEsptool version is %s. Version 4.5.0 or later is required.\033[0m\n", version)
		os.Exit(1)
	}
}

func isVersionCompatible(currentVersion, requiredVersion string) bool {
	// Extract version number from esptool output
	re := regexp.MustCompile(`v?(\d+\.\d+\.\d+)`)
	matches := re.FindStringSubmatch(currentVersion)
	if len(matches) < 2 {
		return false
	}

	currentParts := strings.Split(matches[1], ".")
	requiredParts := strings.Split(requiredVersion, ".")

	for i := 0; i < len(requiredParts); i++ {
		if i >= len(currentParts) {
			return false
		}
		currentPart, _ := strconv.Atoi(currentParts[i])
		requiredPart, _ := strconv.Atoi(requiredParts[i])
		if currentPart < requiredPart {
			return false
		} else if currentPart > requiredPart {
			return true
		}
	}
	return true
}

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

	for _, port := range serialPorts {
		if isLikelyESP32Port(port) {
			esp32Ports = append(esp32Ports, port)
		} else {
			otherPorts = append(otherPorts, port)
		}
	}

	fmt.Println("\033[1;36mAvailable serial ports:\033[0m")
	fmt.Println("")

	allPorts := append(esp32Ports, otherPorts...)
	for i, port := range allPorts {
		if isLikelyESP32Port(port) {
			fmt.Printf("%d: %s \033[1;32m(Likely ESP32)\033[0m\n", i+1, port)
		} else {
			fmt.Printf("%d: %s\n", i+1, port)
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

	return allPorts[selection-1]
}

func listSerialPortsLinux() []string {
	output, err := exec.Command("sh", "-c", "ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null").Output()
	if err != nil {
		return []string{}
	}
	return strings.Fields(string(output))
}

func listSerialPortsDarwin() []string {
	output, err := exec.Command("sh", "-c", "ls /dev/tty.* 2>/dev/null").Output()
	if err != nil {
		return []string{}
	}
	return strings.Fields(string(output))
}

func listSerialPortsWindows() []string {
	cmd := exec.Command("powershell", "-Command", "Get-CimInstance -ClassName Win32_SerialPort | Select-Object -ExpandProperty DeviceID")
	output, err := cmd.Output()
	if err != nil {
		return []string{}
	}
	return strings.Fields(string(output))
}

func triggerReboot(espPort string) {
	fmt.Printf("\033[1;33mTriggering device reboot...\033[0m\n")

	var esptoolCmd string
	switch runtime.GOOS {
	case "linux":
		esptoolCmd = "esptool"
	case "darwin":
		esptoolCmd = "esptool.py"
	case "windows":
		esptoolCmd = "esptool.exe"
	}

	logVerbose("\033[1;36mExecuting: %s --chip esp32s3 --port %s chip_id\033[0m\n", esptoolCmd, espPort)
	cmd := exec.Command(esptoolCmd, "--chip", "esp32s3", "--port", espPort, "chip_id")
	output, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Printf("\033[1;33mReboot command output: %s\033[0m\n", string(output))
		fmt.Printf("\033[1;33mReboot command error (expected): %v\033[0m\n", err)
	} else {
		logVerbose("\033[1;32mReboot successful: %s\033[0m\n", string(output))
	}

	fmt.Printf("\033[1;33mWaiting 3 seconds for device to reboot...\033[0m\n")
	time.Sleep(3 * time.Second)
}

func openSerialConsole(espPort string) {
	clearScreen()
	logFile = fmt.Sprintf(".serial_log_%s.log", time.Now().Format("20060102_150405"))

	triggerReboot(espPort)

	fmt.Printf("\033[1;33mConnecting to %s... Device is rebooting automatically.\033[0m\n\n", espPort)
	log, err := os.Create(logFile)
	if err != nil {
		fmt.Printf("\033[1;31mError creating log file: %v\033[0m\n", err)
		os.Exit(1)
	}
	defer log.Close()

	mode := serial.OpenOptions{
		PortName:        espPort,
		BaudRate:        115200,
		DataBits:        8,
		StopBits:        1,
		MinimumReadSize: 4,
	}

	ser, err := serial.Open(mode)
	if err != nil {
		fmt.Printf("\033[1;31mError opening serial port: %v\033[0m\n", err)
		os.Exit(1)
	}
	defer ser.Close()

	timer := time.NewTimer(30 * time.Second)
	done := make(chan bool)

	go func() {
		writtenLines := 0
		reader := io.TeeReader(ser, log)
		buf := make([]byte, 1024)
		var lineBuffer []byte

		for {
			n, err := reader.Read(buf)
			if n > 0 {
				logVerbose("\033[1;90m[SERIAL] %s\033[0m", string(buf[:n]))

				for _, b := range buf[:n] {
					lineBuffer = append(lineBuffer, b)
					if b == '\n' {
						line := string(lineBuffer)
						logVerbose("\033[1;36m[LINE %d] %s\033[0m", writtenLines+1, line)
						lineBuffer = nil
						writtenLines++
						if writtenLines >= 34 {
							done <- true
							return
						}
					}
				}
			}
			if err != nil {
				if err != io.EOF {
					fmt.Printf("\033[1;31mSerial read error: %v\033[0m\n", err)
					fmt.Printf("\033[1;33mThis may be normal during device reset\033[0m\n")
				}
				done <- true
				return
			}
		}
	}()

	select {
	case <-timer.C:
		fmt.Println("\033[1;33mTimeout reached. Please restart the application.\033[0m")
	case <-done:
	}
}

func checkLogForValidVersion(logFile string) bool {
	logVerbose("\033[1;36mAnalyzing device output from %s...\033[0m\n", logFile)

	file, err := os.Open(logFile)
	if err != nil {
		fmt.Printf("\033[1;31mError opening log file: %v\033[0m\n", err)
		os.Exit(1)
	}
	defer file.Close()

	var allContent strings.Builder
	buf := make([]byte, 1024)

	for {
		n, err := file.Read(buf)
		if n > 0 {
			content := string(buf[:n])
			allContent.WriteString(content)

			for _, str := range validVersionStrings {
				if strings.Contains(content, str) {
					logVerbose("\033[1;32mFound validation string: '%s'\033[0m\n", str)
					return true
				}
			}
		}
		if err != nil {
			break
		}
	}

	if verbose {
		fmt.Printf("\033[1;33m=== Full Device Output ===\033[0m\n")
		fmt.Printf("\033[1;37m%s\033[0m\n", allContent.String())
		fmt.Printf("\033[1;33m=== End Device Output ===\033[0m\n")
	}

	fmt.Printf("\033[1;31mNo valid device strings found. Looking for: %v\033[0m\n", validVersionStrings)
	return false
}

func extractEmbeddedFiles() {
	if !verbose {
		fmt.Printf("\033[1;33mExtracting firmware files...\033[0m\n")
	} else {
		fmt.Printf("\033[1;33mExtracting embedded firmware files...\033[0m\n")
	}

	embeddedFiles := map[string][]byte{
		"bootloader.bin": bootloaderBin,
		"partitions.bin": partitionsBin,
		"boot_app0.bin":  bootApp0Bin,
		"firmware.bin":   firmwareBin,
		"LittleFS.bin":   littleFSBin,
	}

	for filename, data := range embeddedFiles {
		if len(data) == 0 {
			fmt.Printf("\033[1;31m✗ Embedded file %s is empty or missing\033[0m\n", filename)
			fmt.Printf("\033[1;33mThis installer may have been built incorrectly.\033[0m\n")
			os.Exit(1)
		}

		err := os.WriteFile(filename, data, 0644)
		if err != nil {
			fmt.Printf("\033[1;31mFailed to extract %s: %v\033[0m\n", filename, err)
			os.Exit(1)
		}

		logVerbose("\033[1;32m✓ Extracted %s (%d bytes)\033[0m\n", filename, len(data))
	}
}

func tryFlash(espPort string) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Printf("\033[1;31mError flashing firmware: %v\033[0m\n", r)
			os.Exit(1)
		}
	}()

	extractEmbeddedFiles()

	_ = applyOffsetsFromIdeData()
	flashAllSegments(espPort, false, "dio", "40m", "115200", false)

	fmt.Fprint(os.Stdout, "\033[1;32mFlashing completed successfully!\033[0m\n")
	os.Stdout.Sync()
}

func applyOffsetsFromIdeData() bool {
	buildDir := "../.pio/build/esp32-s3-devkitc-1/idedata.json"
	f, err := os.ReadFile(buildDir)
	if err != nil {
		logVerbose("\033[1;33midedata.json not found, using default offsets\033[0m\n")
		return false
	}
	var data struct {
		Extra struct {
			FlashImages []struct {
				Offset string `json:"offset"`
				Path   string `json:"path"`
			} `json:"flash_images"`
			ApplicationOffset string `json:"application_offset"`
		} `json:"extra"`
	}
	if err := json.Unmarshal(f, &data); err != nil {
		logVerbose("\033[1;33mUnable to parse idedata.json, using default offsets: %v\033[0m\n", err)
		return false
	}

	for _, img := range data.Extra.FlashImages {
		switch {
		case strings.HasSuffix(img.Path, "bootloader.bin"):
			bootloader = img.Offset
		case strings.HasSuffix(img.Path, "partitions.bin"):
			partitions = img.Offset
		case strings.HasSuffix(img.Path, "boot_app0.bin"):
			bootapp0 = img.Offset
		}
	}
	if data.Extra.ApplicationOffset != "" {
		firmware = data.Extra.ApplicationOffset
	}

	logVerbose("\033[1;36mUsing offsets from idedata.json:\033[0m bootloader=%s partitions=%s app0=%s app=%s fs=%s\n",
		bootloader, partitions, bootapp0, firmware, littlefs)
	return true
}

func flashAllSegments(espPort string, eraseFlash bool, flashMode string, flashFreq string, baud string, useStub bool) {
	var esptoolCmd string
	switch runtime.GOOS {
	case "linux":
		esptoolCmd = "esptool"
	case "darwin":
		esptoolCmd = "esptool.py"
	case "windows":
		esptoolCmd = "esptool.exe"
	}

	needed := []string{"bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin", "LittleFS.bin"}
	for _, f := range needed {
		if !fileExists(f) {
			panic(fmt.Sprintf("Required file missing: %s", f))
		}
	}

	baseArgs := []string{
		"--chip", "esp32s3",
		"--port", espPort,
		"--baud", baud,
		"--before", "default_reset",
		"--after", "hard_reset",
	}

	if eraseFlash {
		fmt.Printf("\033[1;33mErasing entire flash (recovery) ...\033[0m\n")
		eraseArgs := append([]string{}, baseArgs...)
		eraseArgs = append(eraseArgs, "erase_flash")
		logVerbose("\033[1;36mExecuting: %s %s\033[0m\n", esptoolCmd, strings.Join(eraseArgs, " "))
		cmdErase := exec.Command(esptoolCmd, eraseArgs...)
		outErase, err := cmdErase.CombinedOutput()
		logVerbose("\033[1;90m%s\033[0m\n", string(outErase))
		if err != nil {
			panic(fmt.Sprintf("erase_flash failed: %v", err))
		}
	}

	args := append([]string{}, baseArgs...)
	if !useStub {
		args = append(args, "--no-stub")
	}
	args = append(args,
		"write_flash",
		"-z",
		"--flash_mode", flashMode,
		"--flash_freq", flashFreq,
		"--flash_size", "8MB",
		bootloader, "bootloader.bin",
		partitions, "partitions.bin",
		bootapp0, "boot_app0.bin",
		firmware, "firmware.bin",
		littlefs, "LittleFS.bin",
	)

	logVerbose("\033[1;36mExecuting: %s %s\033[0m\n", esptoolCmd, strings.Join(args, " "))
	cmd := exec.Command(esptoolCmd, args...)
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	if err := cmd.Start(); err != nil {
		panic(fmt.Sprintf("Failed to start esptool: %v", err))
	}

	s := spinner.New(spinner.CharSets[14], 100*time.Millisecond)
	s.Start()
	scanner := bufio.NewScanner(io.MultiReader(stdout, stderr))
	for scanner.Scan() {
		line := scanner.Text()
		logVerbose("\033[1;90m[ESPTOOL] %s\033[0m\n", line)
	}
	s.Stop()
	if err := cmd.Wait(); err != nil {
		fmt.Printf("\033[1;31mwrite_flash failed: %v\033[0m\n", err)
		fmt.Println("\033[1;33mRetrying with safer settings (DIO/40m, lower baud, --no-stub) ...\033[0m")
		safeMode := "dio"
		safeFreq := "40m"
		safeBaud := "115200"
		flashAllSegments(espPort, false, safeMode, safeFreq, safeBaud, false)
		return
	}
}

func performRecoveryReflash(espPort string) {
	fmt.Println("\033[1;33mStarting recovery: full chip erase + reflash (DIO/40m, 115200, --no-stub)\033[0m")
	_ = applyOffsetsFromIdeData()
	flashAllSegments(espPort, true, "dio", "40m", "115200", false)
	fmt.Println("\033[1;32mRecovery reflash completed. Verifying...\033[0m")
	res := checkDeviceAfterFlash(espPort)
	if res == 1 {
		fmt.Println("\033[1;32mDevice booted successfully after recovery.\033[0m")
	} else {
		fmt.Println("\033[1;31mDevice still not booting after recovery.\033[0m")
		fmt.Println("\033[1;33mVerify hardware, strapping pins, and try flashing with PlatformIO once to baseline.\033[0m")
	}
}

func flashCmd(espPort string, addr string, fileName string) {
	if message, exists := flashMessages[fileName]; exists {
		fmt.Fprintf(os.Stdout, "\033[1;32m%s\033[0m\n", message)
	} else {
		fmt.Fprintf(os.Stdout, "\033[1;32mFlashing %s to address %s...\033[0m\n", fileName, addr)
	}
	os.Stdout.Sync()

	var cmd *exec.Cmd
	var esptoolCmd string

	switch runtime.GOOS {
	case "linux":
		esptoolCmd = "esptool"
	case "darwin":
		esptoolCmd = "esptool.py"
	case "windows":
		esptoolCmd = "esptool.exe"
	}

	if _, err := os.Stat(fileName); os.IsNotExist(err) {
		panic(fmt.Sprintf("File %s does not exist", fileName))
	}

	args := []string{
		"--chip", "esp32s3",
		"--port", espPort,
		"--baud", "460800",
		"--before", "default_reset",
		"--after", "hard_reset",
		"write_flash",
		"-z",
		"--flash_mode", "qio",
		"--flash_freq", "80m",
		"--flash_size", "8MB",
		addr, fileName}

	logVerbose("\033[1;36mExecuting: %s %s\033[0m\n", esptoolCmd, strings.Join(args, " "))
	cmd = exec.Command(esptoolCmd, args...)

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		panic(fmt.Sprintf("Failed to get stdout pipe: %v", err))
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		panic(fmt.Sprintf("Failed to get stderr pipe: %v", err))
	}

	err = cmd.Start()
	if err != nil {
		panic(fmt.Sprintf("Failed to start flashing %s: %v", fileName, err))
	}

	s := spinner.New(spinner.CharSets[14], 100*time.Millisecond)
	s.Start()

	scanner := bufio.NewScanner(io.MultiReader(stdout, stderr))
	progressRegex := regexp.MustCompile(`Writing at .* \((\d+)%\)`)
	var allOutput strings.Builder

	go func() {
		for scanner.Scan() {
			line := scanner.Text()
			allOutput.WriteString(line + "\n")

			logVerbose("\033[1;90m[ESPTOOL] %s\033[0m\n", line)

			if matches := progressRegex.FindStringSubmatch(line); matches != nil {
				percentage := matches[1]
				s.Suffix = " " + percentage + "%"
			}
		}
	}()

	err = cmd.Wait()
	s.Stop()
	if err != nil {
		fmt.Printf("\033[1;31mFlashing failed for %s\033[0m\n", fileName)
		fmt.Printf("\033[1;31mError: %v\033[0m\n", err)
		fmt.Printf("\033[1;33mCommand: %s %s\033[0m\n", esptoolCmd, strings.Join(args, " "))
		fmt.Printf("\033[1;33mFile exists: %v\033[0m\n", fileExists(fileName))
		if fileExists(fileName) {
			info, _ := os.Stat(fileName)
			fmt.Printf("\033[1;33mFile size: %d bytes\033[0m\n", info.Size())
		}
		fmt.Printf("\033[1;33m=== Full esptool output ===\033[0m\n")
		fmt.Printf("\033[1;37m%s\033[0m\n", allOutput.String())
		fmt.Printf("\033[1;33m=== End esptool output ===\033[0m\n")
		panic(fmt.Sprintf("Failed to flash %s: %v", fileName, err))
	}
}

func fileExists(filename string) bool {
	_, err := os.Stat(filename)
	return !os.IsNotExist(err)
}

func askUserConfirmation(prompt string) bool {
	fmt.Print(prompt)
	var response string
	_, err := fmt.Scanln(&response)
	if err != nil {
		return false
	}
	response = strings.ToLower(strings.TrimSpace(response))
	return response == "y" || response == "yes"
}

func checkDeviceAfterFlash(espPort string) int {
	fmt.Printf("\033[1;33mRebooting device and checking status...\033[0m\n")

	time.Sleep(2 * time.Second)

	triggerReboot(espPort)
	postFlashLog := fmt.Sprintf(".post_flash_log_%s.log", time.Now().Format("20060102_150405"))

	mode := serial.OpenOptions{
		PortName:        espPort,
		BaudRate:        115200,
		DataBits:        8,
		StopBits:        1,
		MinimumReadSize: 4,
	}

	ser, err := serial.Open(mode)
	if err != nil {
		fmt.Printf("\033[1;33mCould not open serial port for verification: %v\033[0m\n", err)
		return -1
	}
	defer ser.Close()

	logFile, err := os.Create(postFlashLog)
	if err != nil {
		fmt.Printf("\033[1;33mCould not create verification log: %v\033[0m\n", err)
		return -1
	}
	defer logFile.Close()
	defer os.Remove(postFlashLog)

	timer := time.NewTimer(20 * time.Second)
	done := make(chan int)
	bootLoopCount := 0
	var allOutput strings.Builder

	go func() {
		reader := io.TeeReader(ser, logFile)
		buf := make([]byte, 1024)

		for {
			n, err := reader.Read(buf)
			if n > 0 {
				content := string(buf[:n])
				allOutput.WriteString(content)

				logVerbose("\033[1;90m[VERIFY] %s\033[0m", content)

				if strings.Contains(content, "ESP-ROM:esp32s3") {
					bootLoopCount++
					logVerbose("\033[1;33m[BOOT ATTEMPT %d]\033[0m\n", bootLoopCount)

					if bootLoopCount >= 3 {
						fmt.Printf("\033[1;31m[BOOT LOOP DETECTED]\033[0m\n")
						done <- 0
						return
					}
				}

				if strings.Contains(content, "Doppelgänger") && strings.Contains(content, "Core") {
					fmt.Printf("\033[1;32m[CORE DEVICE DETECTED]\033[0m\n")
					done <- 1 // Success
					return
				}

				if strings.Contains(content, "WEBSERVER") || strings.Contains(content, "WiFi Access Point") {
					fmt.Printf("\033[1;32m[DEVICE STARTUP DETECTED]\033[0m\n")
					done <- 1 // Success
					return
				}

				if strings.Contains(content, "Guru Meditation") || strings.Contains(content, "abort()") {
					fmt.Printf("\033[1;31m[CRITICAL ERROR DETECTED]\033[0m\n")
					done <- 0 // Error
					return
				}
			}
			if err != nil {
				done <- -1
				return
			}
		}
	}()

	select {
	case <-timer.C:
		fmt.Printf("\033[1;33mVerification timeout after 20 seconds\033[0m\n")
		fmt.Printf("\033[1;33m=== Device Output ===\033[0m\n")
		fmt.Printf("\033[1;37m%s\033[0m\n", allOutput.String())
		fmt.Printf("\033[1;33m=== End Output ===\033[0m\n")
		return -1
	case result := <-done:
		return result
	}
}

func main() {
	flag.BoolVar(&verbose, "v", false, "Enable verbose output with detailed debugging information")
	flag.BoolVar(&verbose, "verbose", false, "Enable verbose output with detailed debugging information")
	flag.Parse()

	fmt.Println("\033[1;36m=== Doppelgänger Core Firmware Updater ===\033[0m")
	if verbose {
		fmt.Println("\033[1;36mSimplified Build - No Encryption Required\033[0m")
	}
	fmt.Println("")

	installEsptool()
	espPort := findSerialPort()
	logFile = fmt.Sprintf(".serial_log_%s.log", time.Now().Format("20060102_150405"))
	openSerialConsole(espPort)

	if checkLogForValidVersion(logFile) {
		os.Remove(logFile)
		if verbose {
			fmt.Println("\033[1;32mDevice validation successful! Starting firmware update...\033[0m")
		} else {
			fmt.Println("\033[1;32mDevice validated. Flashing firmware...\033[0m")
		}
		tryFlash(espPort)

		if verbose {
			fmt.Println("\033[1;33mVerifying firmware installation...\033[0m")
		} else {
			fmt.Println("\033[1;33mVerifying installation...\033[0m")
		}
		flashResult := checkDeviceAfterFlash(espPort)

		switch flashResult {
		case 1:
			fmt.Println("\033[1;32mFirmware update completed successfully!\033[0m")
			fmt.Println("\033[1;32mDevice is running normally.\033[0m")
		case 0:
			fmt.Println("\033[1;31mFirmware flashed but device is in boot loop!\033[0m")
			fmt.Println("\033[1;33mThis is unexpected since safe flash parameters were used.\033[0m")
			fmt.Println("\033[1;33mTry full chip erase and reflash as recovery option.\033[0m")
			if askUserConfirmation("Proceed with recovery reflash (full erase)? (y/N): ") {
				performRecoveryReflash(espPort)
			} else {
				fmt.Println("\033[1;33mRecovery skipped by user.\033[0m")
			}
		case -1:
			fmt.Println("\033[1;33mFirmware flashed - device status unknown\033[0m")
			fmt.Println("\033[1;33mDevice may be working but not responding to serial\033[0m")
			fmt.Println("\033[1;33mCheck device manually or try power cycling\033[0m")
		}

		logVerbose("\033[1;33mCleaning up temporary files...\033[0m\n")
		os.Remove("bootloader.bin")
		os.Remove("partitions.bin")
		os.Remove("boot_app0.bin")
		os.Remove("firmware.bin")
		os.Remove("LittleFS.bin")
		os.Stdout.Sync()
	} else {
		clearScreen()
		fmt.Println("\033[1;31mDevice validation failed - device may not be a Core Reader.\033[0m")
		fmt.Println("\033[1;33mSupported devices: Core, D_Core\033[0m")
		fmt.Println("")

		if askUserConfirmation("Do you want to proceed with flashing anyway? (y/N): ") {
			fmt.Println("\033[1;33mProceeding with firmware update...\033[0m")
			tryFlash(espPort)

			fmt.Println("\033[1;33mVerifying firmware installation...\033[0m")
			flashResult := checkDeviceAfterFlash(espPort)

			switch flashResult {
			case 1:
				fmt.Println("\033[1;32mFirmware update completed successfully!\033[0m")
				fmt.Println("\033[1;32mDevice converted to Core Reader successfully!\033[0m")
			case 0:
				fmt.Println("\033[1;31mFirmware flashed but device is in boot loop!\033[0m")
				fmt.Println("\033[1;33mThis is unexpected since safe flash parameters were used.\033[0m")
				fmt.Println("\033[1;33mDevice may be incompatible hardware or have flash issues.\033[0m")
				if askUserConfirmation("Proceed with recovery reflash (full erase)? (y/N): ") {
					performRecoveryReflash(espPort)
				} else {
					fmt.Println("\033[1;33mRecovery skipped by user.\033[0m")
				}
			case -1:
				fmt.Println("\033[1;33mFirmware flashed - device status unknown\033[0m")
				fmt.Println("\033[1;33mThis may be expected if converting from different firmware.\033[0m")
			}
		} else {
			fmt.Println("\033[1;33mOperation cancelled by user.\033[0m")
		}

		logVerbose("\033[1;33mCleaning up temporary files...\033[0m\n")
		os.Remove("bootloader.bin")
		os.Remove("partitions.bin")
		os.Remove("boot_app0.bin")
		os.Remove("firmware.bin")
		os.Remove("LittleFS.bin")
		os.Remove(logFile)
	}
}
