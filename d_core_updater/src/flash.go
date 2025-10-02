package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"
	"time"

	"github.com/briandowns/spinner"
)

func extractEmbeddedFiles() {
	if verbose {
		fmt.Printf("\033[1;33mExtracting embedded firmware files...\033[0m\n")
	} else {
		fmt.Printf("\033[1;33mExtracting firmware files...\033[0m\n")
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

// applyOffsetsFromIdeData reads PlatformIO idedata.json and updates offsets
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

	// Update offsets based on idedata.json
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

// flashAllSegments writes all images in a single esptool transaction.
// If eraseFlash is true, it performs a full chip erase first (recovery mode).
func flashAllSegments(espPort string, eraseFlash bool, flashMode string, flashFreq string, baud string, useStub bool) {
	esptoolCmd := getEsptoolCommand()
	logVerbose("\033[1;36mUsing esptool command: %s\033[0m\n", esptoolCmd)

	// Verify files exist
	needed := []string{"bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin", "LittleFS.bin"}
	for _, f := range needed {
		if !fileExists(f) {
			panic(fmt.Sprintf("Required file missing: %s", f))
		}
	}

	// Get correct syntax based on esptool version
	beforeReset, afterReset, writeFlash, eraseFlashCmd, flashModeArg, flashFreqArg, flashSizeArg := getEsptoolArgs(esptoolCmd, false)
	logVerbose("\033[1;36mUsing esptool args: --before %s --after %s %s\033[0m\n", beforeReset, afterReset, writeFlash)

	baseArgs := []string{
		"--chip", "esp32s3",
		"--port", espPort,
		"--baud", baud,
		"--before", beforeReset,
		"--after", afterReset,
	}

	if eraseFlash {
		fmt.Printf("\033[1;33mErasing entire flash (recovery) ...\033[0m\n")
		eraseArgs := append([]string{}, baseArgs...)
		eraseArgs = append(eraseArgs, eraseFlashCmd)
		logVerbose("\033[1;36mExecuting: %s %s\033[0m\n", esptoolCmd, strings.Join(eraseArgs, " "))
		cmdErase := exec.Command(esptoolCmd, eraseArgs...)
		outErase, err := cmdErase.CombinedOutput()
		fmt.Printf("\033[1;90m%s\033[0m\n", string(outErase))
		if err != nil {
			panic(fmt.Sprintf("%s failed: %v", eraseFlashCmd, err))
		}
	}

	// Single transaction write-flash with all segments in correct order/offsets
	args := append([]string{}, baseArgs...)
	if !useStub {
		args = append(args, "--no-stub")
	}
	args = append(args, writeFlash)
	args = append(args, "--"+flashModeArg, flashMode)
	args = append(args, "--"+flashFreqArg, flashFreq)
	args = append(args, "--"+flashSizeArg, "8MB")
	args = append(args, "-z")
	args = append(args,
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
		fmt.Printf("\033[1;90m[ESPTOOL] %s\033[0m\n", line)
	}
	s.Stop()
	if err := cmd.Wait(); err != nil {
		// Only retry with safer settings if we're not already using them
		if flashMode != "dio" || flashFreq != "40m" || baud != "115200" || useStub {
			fmt.Printf("\033[1;31mwrite-flash failed: %v\033[0m\n", err)
			fmt.Println("\033[1;33mRetrying with safer settings (DIO/40m, lower baud, --no-stub) ...\033[0m")
			// Retry path: switch to DIO/40m and 115200 baud, no stub
			safeMode := "dio"
			safeFreq := "40m"
			safeBaud := "115200"
			flashAllSegments(espPort, false, safeMode, safeFreq, safeBaud, false)
			return
		}
		// If we're already using safe settings and still failing, propagate the error
		panic(fmt.Sprintf("write-flash failed with safe settings: %v", err))
	}
}
