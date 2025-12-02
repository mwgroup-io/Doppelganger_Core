package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"
)

// isPythonEsptool detects Python vs standalone esptool for syntax compatibility
func isPythonEsptool(esptoolCmd string) bool {
	if strings.Contains(esptoolCmd, "esptool.py") || strings.HasSuffix(esptoolCmd, ".py") {
		return true
	}

	if runtime.GOOS == "windows" && (strings.HasSuffix(esptoolCmd, "esptool.exe") || strings.HasSuffix(esptoolCmd, "esptool")) {
		return true
	}

	if runtime.GOOS == "darwin" {
		if esptoolCmd == "esptool" || strings.HasSuffix(esptoolCmd, "/esptool") {
			return false
		}
		return true
	}

	return false
}

// installHomebrewAndEsptool installs Homebrew and esptool
func installHomebrewAndEsptool() {
	fmt.Println("\033[1;33mInstalling Homebrew...\033[0m")
	fmt.Println("\033[1;33mNote: This may take several minutes and will require your password.\033[0m")

	cmd := exec.Command("/bin/bash", "-c", `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin

	if err := cmd.Run(); err != nil {
		fmt.Printf("\033[1;31mFailed to install Homebrew: %v\033[0m\n", err)
		fmt.Println("\033[1;33mPlease install Homebrew manually: https://brew.sh/\033[0m")
		os.Exit(1)
	} else {
		fmt.Println("\033[1;32mHomebrew installed successfully!\033[0m")
		fmt.Println("\033[1;33mSetting up Homebrew PATH and installing esptool...\033[0m")

		homebrewPath := "/opt/homebrew/bin"
		if runtime.GOARCH == "amd64" {
			homebrewPath = "/usr/local/bin"
		}

		currentPath := os.Getenv("PATH")
		newPath := homebrewPath + ":" + currentPath
		os.Setenv("PATH", newPath)

		brewEnvCmd := exec.Command(homebrewPath+"/brew", "shellenv")
		envOutput, err := brewEnvCmd.Output()
		if err == nil {
			lines := strings.Split(string(envOutput), "\n")
			for _, line := range lines {
				if strings.HasPrefix(line, "export ") {
					envLine := strings.TrimPrefix(line, "export ")
					if parts := strings.SplitN(envLine, "=", 2); len(parts) == 2 {
						key := parts[0]
						value := strings.Trim(parts[1], "\"")
						os.Setenv(key, value)
					}
				}
			}
		}

		brewCmd := exec.Command(homebrewPath+"/brew", "install", "esptool")
		brewCmd.Stdout = os.Stdout
		brewCmd.Stderr = os.Stderr
		if err := brewCmd.Run(); err != nil {
			fmt.Printf("\033[1;31mFailed to install esptool via Homebrew: %v\033[0m\n", err)
			fmt.Println("\033[1;33mHomebrew was installed but esptool installation failed.\033[0m")
			fmt.Println("\033[1;33mYou may need to restart your terminal and run: brew install esptool\033[0m")
			os.Exit(1)
		} else {
			fmt.Println("\033[1;32mEsptool installed successfully via Homebrew!\033[0m")
			fmt.Println("\033[1;33mNote: You may need to restart your terminal for the PATH changes to take effect permanently.\033[0m")
		}
	}
}

func getEsptoolArgs(esptoolCmd string, useHyphens bool) (beforeReset, afterReset, writeFlash, eraseFlash, flashMode, flashFreq, flashSize string) {
	if isPythonEsptool(esptoolCmd) {
		return "default_reset", "hard_reset", "write_flash", "erase_flash", "flash_mode", "flash_freq", "flash_size"
	} else {
		return "default-reset", "hard-reset", "write-flash", "erase-flash", "flash-mode", "flash-freq", "flash-size"
	}
}

func getEsptoolCommand() string {
	switch runtime.GOOS {
	case "linux":
		if _, err := exec.LookPath("esptool"); err == nil {
			return "esptool"
		}
		homeDir := os.Getenv("HOME")
		localBinPath := filepath.Join(homeDir, ".local", "bin", "esptool")
		if _, err := os.Stat(localBinPath); err == nil {
			return localBinPath
		}
		commonPaths := []string{"/usr/bin/esptool", "/usr/local/bin/esptool", "/bin/esptool"}
		for _, path := range commonPaths {
			if _, err := os.Stat(path); err == nil {
				return path
			}
		}
		return "esptool" // fallback
	case "darwin":
		// Try esptool (Homebrew) first, then esptool.py (pip)
		if _, err := exec.LookPath("esptool"); err == nil {
			return "esptool"
		} else if _, err := exec.LookPath("esptool.py"); err == nil {
			return "esptool.py"
		} else {
			homebrewPaths := []string{
				"/opt/homebrew/bin/esptool", // Apple Silicon
				"/usr/local/bin/esptool",    // Intel
			}
			for _, path := range homebrewPaths {
				if _, err := os.Stat(path); err == nil {
					return path
				}
			}
			return "esptool.py" // fallback
		}
	case "windows":
		// Try esptool.exe first, then esptool.py
		if _, err := exec.LookPath("esptool.exe"); err == nil {
			return "esptool.exe"
		} else if _, err := exec.LookPath("esptool.py"); err == nil {
			return "esptool.py"
		} else {
			// If not in PATH, search all Python installations
			pythonPaths := getAllPythonPaths()
			for _, pythonPath := range pythonPaths {
				if _, err := os.Stat(pythonPath); err == nil {
					scriptsDir := filepath.Dir(pythonPath) + "\\Scripts"
					esptoolPath := filepath.Join(scriptsDir, "esptool.exe")
					if _, err := os.Stat(esptoolPath); err == nil {
						return esptoolPath
					}
				}
			}

			return "esptool.exe"
		}
	default:
		return "esptool"
	}
}

func installEsptool() {
	esptoolCmd := getEsptoolCommand()
	if esptoolCmd != "" && esptoolCmd != "esptool" {
		// Try to run esptool version to see if it works
		cmd := exec.Command(esptoolCmd, "version")
		cmd.Stdout = nil
		cmd.Stderr = nil
		if err := cmd.Run(); err != nil {
			if repairEsptoolInstallation() {
				return
			}
		} else {
			return // esptool is working
		}
	}

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

func repairEsptoolInstallation() bool {
	if runtime.GOOS == "linux" {
		return repairEsptoolInstallationLinux()
	} else if runtime.GOOS == "windows" {
		return repairEsptoolInstallationWindows()
	} else {
		// macOS - try pip install
		if _, err := exec.LookPath("pip"); err == nil {
			fmt.Println("\033[1;33mInstalling esptool via pip to get complete installation...\033[0m")
			cmd := exec.Command("pip", "install", "--upgrade", "esptool")
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			if err := cmd.Run(); err != nil {
				fmt.Printf("\033[1;33mPip installation failed: %v\033[0m\n", err)
				return false
			}
			fmt.Println("\033[1;32mEsptool installed via pip successfully!\033[0m")
			return true
		}
		return false
	}
}

func repairEsptoolInstallationLinux() bool {
	// Determine which pip command to use - try full paths first
	var pipCmd string

	// Try common pip3 paths first
	commonPipPaths := []string{
		"/usr/bin/pip3",
		"/usr/local/bin/pip3",
		"/bin/pip3",
	}

	for _, pipPath := range commonPipPaths {
		if _, err := os.Stat(pipPath); err == nil {
			pipCmd = pipPath
			break
		}
	}

	// If not found in common paths, try PATH lookup
	if pipCmd == "" {
		if _, err := exec.LookPath("pip"); err == nil {
			pipCmd = "pip"
		} else if _, err := exec.LookPath("pip3"); err == nil {
			pipCmd = "pip3"
		}
	}

	if pipCmd == "" {
		return false
	}

	// First, install all the required dependencies individually
	fmt.Println("\033[1;33mInstalling esptool dependencies...\033[0m")
	deps := []string{"pyserial", "cryptography", "reedsolo", "PyYAML", "intelhex", "click", "bitstring"}

	for _, dep := range deps {
		fmt.Printf("\033[1;36mInstalling %s...\033[0m\n", dep)
		cmd := exec.Command(pipCmd, "install", "--upgrade", dep)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		cmd.Run() // Ignore errors, try anyway
	}

	// Now install esptool
	fmt.Println("\033[1;33mInstalling esptool...\033[0m")
	cmd := exec.Command(pipCmd, "install", "--upgrade", "esptool")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		fmt.Printf("\033[1;31mFailed to install esptool: %v\033[0m\n", err)
		return false
	}
	return true
}

func repairEsptoolInstallationWindows() bool {
	pythonPath := ""

	// Try to find Python installation that has esptool
	pythonPaths := getAllPythonPaths()
	for _, path := range pythonPaths {
		if isValidPythonInstallation(path) {
			scriptsDir := filepath.Dir(path) + "\\Scripts"
			esptoolPath := filepath.Join(scriptsDir, "esptool.exe")
			if _, err := os.Stat(esptoolPath); err == nil {
				pythonPath = path
				fmt.Printf("\033[1;32mFound Python with esptool: %s\033[0m\n", path)
				break
			}
		}
	}

	if pythonPath == "" {
		fmt.Println("\033[1;31mCould not find Python installation with esptool\033[0m")
		return false
	}

	// Install missing dependencies
	deps := []string{"pyserial", "cryptography", "reedsolo", "PyYAML", "intelhex", "click"}

	fmt.Println("\033[1;33mInstalling missing dependencies...\033[0m")
	for _, dep := range deps {
		fmt.Printf("\033[1;36mInstalling %s...\033[0m\n", dep)
		cmd := exec.Command(pythonPath, "-m", "pip", "install", dep)
		if err := cmd.Run(); err != nil {
			fmt.Printf("\033[1;33mFailed to install %s: %v\033[0m\n", dep, err)
		}
	}

	// Test if esptool works now
	fmt.Println("\033[1;33mTesting repaired esptool installation...\033[0m")
	esptoolCmd := getEsptoolCommand()
	if esptoolCmd == "" {
		return false
	}

	cmd := exec.Command(esptoolCmd, "version")
	if err := cmd.Run(); err != nil {
		fmt.Printf("\033[1;31mEsptool still not working after repair: %v\033[0m\n", err)
		return false
	}

	fmt.Println("\033[1;32mEsptool repair successful!\033[0m")
	return true
}

func installEsptoolLinux() {
	_, err := exec.LookPath("esptool")
	if err != nil {
		// Ask for permission before installing
		fmt.Println("\033[1;33mEsptool is not installed and is required for firmware flashing.\033[0m")
		fmt.Println("\033[1;33mThis installer can automatically install Python, pip, and esptool (requires sudo).\033[0m")
		fmt.Println("")

		if !askUserConfirmation("Would you like to automatically install esptool? (y/N): ") {
			fmt.Println("\033[1;33mAutomatic installation declined. Manual installation required:\033[0m")
			fmt.Println("\033[1;33m  1. Install Python and pip: sudo apt install python3 python3-pip\033[0m")
			fmt.Println("\033[1;33m  2. Install esptool: pip3 install esptool\033[0m")
			fmt.Println("\033[1;33m  3. Add user to dialout group: sudo usermod -a -G dialout $USER\033[0m")
			fmt.Println("\033[1;33m  4. Log out and log back in, or run: newgrp dialout\033[0m")
			fmt.Println("\033[1;33m  5. Run this installer again\033[0m")
			fmt.Println("")
			os.Exit(1)
		}

		fmt.Println("\033[1;33mInstalling Python, pip, and esptool...\033[0m")
		fmt.Println("")

		// Install Python and pip first if not available
		fmt.Println("\033[1;36mStep 1: Updating package list...\033[0m")
		cmdPython := exec.Command("sudo", "apt", "update")
		cmdPython.Stdout = os.Stdout
		cmdPython.Stderr = os.Stderr
		cmdPython.Run() // Ignore errors

		fmt.Println("\033[1;36mStep 2: Installing Python and pip...\033[0m")
		cmdPython = exec.Command("sudo", "apt", "install", "-y", "python3", "python3-pip", "python3-venv")
		cmdPython.Stdout = os.Stdout
		cmdPython.Stderr = os.Stderr
		cmdPython.Run()

		// Setup user groups
		fmt.Println("\033[1;36mStep 3: Setting up user permissions...\033[0m")
		setupLinuxUserGroups()

		// Install esptool via pip
		fmt.Println("\033[1;36mStep 4: Installing esptool...\033[0m")
		installEsptoolLinuxWithPip()

		fmt.Println("")
		fmt.Println("\033[1;32m✓ Installation completed successfully!\033[0m")
		fmt.Println("")
	}
	checkEsptoolVersion("esptool")
}

func installEsptoolLinuxWithPip() bool {
	// Determine which pip command to use - try full paths first
	var pipCmd string

	// Try common pip3 paths first
	commonPipPaths := []string{
		"/usr/bin/pip3",
		"/usr/local/bin/pip3",
		"/bin/pip3",
	}

	for _, pipPath := range commonPipPaths {
		if _, err := os.Stat(pipPath); err == nil {
			pipCmd = pipPath
			break
		}
	}

	// If not found in common paths, try PATH lookup
	if pipCmd == "" {
		if _, err := exec.LookPath("pip3"); err == nil {
			pipCmd = "pip3"
		} else if _, err := exec.LookPath("pip"); err == nil {
			pipCmd = "pip"
		}
	}

	if pipCmd == "" {
		return false
	}

	// Install esptool with --break-system-packages to bypass externally-managed-environment
	fmt.Printf("\033[1;33mRunning: %s install --break-system-packages esptool\033[0m\n", pipCmd)
	cmd := exec.Command(pipCmd, "install", "--break-system-packages", "esptool")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		fmt.Printf("\033[1;31mFailed to install esptool: %v\033[0m\n", err)
		return false
	}

	// Add ~/.local/bin to PATH for this session
	homeDir := os.Getenv("HOME")
	localBinPath := filepath.Join(homeDir, ".local", "bin")
	currentPath := os.Getenv("PATH")
	newPath := localBinPath + ":" + currentPath
	os.Setenv("PATH", newPath)

	fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")
	return true
}

func setupLinuxUserGroups() {
	// Check if user is already in dialout group
	cmd := exec.Command("groups")
	output, err := cmd.Output()
	if err != nil {
		fmt.Printf("\033[1;33mCould not check user groups: %v\033[0m\n", err)
		return
	}

	groups := string(output)
	if strings.Contains(groups, "dialout") {
		return
	}

	// Add user to dialout group automatically
	username := os.Getenv("USER")
	if username == "" {
		username = os.Getenv("USERNAME")
	}
	if username == "" {
		fmt.Println("\033[1;31mCould not determine username\033[0m")
		return
	}

	fmt.Printf("\033[1;33mAdding user %s to dialout group for serial port access...\033[0m\n", username)
	cmd = exec.Command("sudo", "usermod", "-a", "-G", "dialout", username)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		fmt.Printf("\033[1;31mFailed to add user to dialout group: %v\033[0m\n", err)
		fmt.Println("\033[1;33mYou may need to run: sudo usermod -a -G dialout $USER\033[0m")
		fmt.Println("\033[1;33mThen log out and log back in, or run: newgrp dialout\033[0m")
	} else {
		fmt.Println("\033[1;32mUser added to dialout group successfully!\033[0m")
		fmt.Println("\033[1;33mNote: You may need to log out and log back in for changes to take effect\033[0m")
		fmt.Println("\033[1;33mAlternatively, run: newgrp dialout\033[0m")
	}
}

func installEsptoolDarwin() {
	// Check if esptool is already installed (either version)
	if _, err := exec.LookPath("esptool"); err == nil {
		checkEsptoolVersion("esptool")
		return
	}
	if _, err := exec.LookPath("esptool.py"); err == nil {
		checkEsptoolVersion("esptool.py")
		return
	}

	homebrewPaths := []string{
		"/opt/homebrew/bin/esptool", // Apple Silicon
		"/usr/local/bin/esptool",    // Intel
	}
	for _, path := range homebrewPaths {
		if _, err := os.Stat(path); err == nil {
			checkEsptoolVersion(path)
			return
		}
	}

	// Esptool not found, proceed with installation
	fmt.Println("\033[1;33mEsptool is not installed and is required for firmware flashing.\033[0m")

	_, brewErr := exec.LookPath("brew")
	if brewErr == nil {
		fmt.Println("\033[1;33mThis installer can automatically install esptool using Homebrew.\033[0m")
		fmt.Println("")

		if !askUserConfirmation("Would you like to automatically install esptool via Homebrew? (y/N): ") {
			fmt.Println("\033[1;33mAutomatic installation declined. Manual installation options:\033[0m")
			fmt.Println("\033[1;33m  1. Homebrew: brew install esptool\033[0m")
			fmt.Println("\033[1;33m  2. pip: pip install esptool\033[0m")
			fmt.Println("\033[1;33m  3. Download binary: https://github.com/espressif/esptool/releases\033[0m")
			fmt.Println("\033[1;33m  4. Run this installer again after installation\033[0m")
			os.Exit(1)
		}

		fmt.Println("\033[1;33mInstalling esptool using Homebrew...\033[0m")
		cmd := exec.Command("brew", "install", "esptool")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		err := cmd.Run()
		if err != nil {
			fmt.Printf("\033[1;31mError installing esptool using Homebrew: %v\033[0m\n", err)
			fmt.Println("\033[1;33mPlease install esptool manually and run this installer again.\033[0m")
			os.Exit(1)
		}
		fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")
	} else {
		// Homebrew is not installed, offer to install it automatically
		fmt.Println("\033[1;33mHomebrew is not installed and is required for installing esptool on macOS.\033[0m")
		fmt.Println("\033[1;33mThis installer can automatically install Homebrew and then esptool.\033[0m")
		fmt.Println("")

		if askUserConfirmation("Would you like to automatically install Homebrew? (y/N): ") {
			installHomebrewAndEsptool()
			return
		}

		// Automatic installation declined
		fmt.Println("\033[1;33mAutomatic installation declined. Manual installation required:\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mRecommended method - Install Homebrew:\033[0m")
		fmt.Println("\033[1;33m  1. Install Homebrew: https://brew.sh/\033[0m")
		fmt.Println("\033[1;33m  2. Then run: brew install esptool\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mAlternative method - Download binary:\033[0m")
		fmt.Println("\033[1;33m  • https://github.com/espressif/esptool/releases\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mRun this installer again after installing esptool.\033[0m")
		os.Exit(1)
	}

	// This should not be reached, but just in case
	checkEsptoolVersion(getEsptoolCommand())
}

func installEsptoolWindows() {
	// Check if esptool is already available
	if _, err := exec.LookPath("esptool.exe"); err == nil {
		checkEsptoolVersion("esptool.exe")
		return
	}

	// Check if esptool.py is available (Python installation)
	if _, err := exec.LookPath("esptool.py"); err == nil {
		checkEsptoolVersion("esptool.py")
		return
	}

	fmt.Println("\033[1;33mEsptool is not installed and is required for firmware flashing.\033[0m")
	fmt.Println("\033[1;33mThis installer can attempt automatic installation using available package managers.\033[0m")
	fmt.Println("")

	if !askUserConfirmation("Would you like to attempt automatic installation of esptool? (y/N): ") {
		fmt.Println("\033[1;33mAutomatic installation declined. Manual installation required:\033[0m")
		fmt.Println("\033[1;33mOption 1 - Install Python and esptool:\033[0m")
		fmt.Println("\033[1;33m  1. Download Python from: https://python.org/downloads/\033[0m")
		fmt.Println("\033[1;33m  2. During installation, check 'Add Python to PATH'\033[0m")
		fmt.Println("\033[1;33m  3. Open Command Prompt and run: pip install esptool\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mOption 2 - Install package manager first:\033[0m")
		fmt.Println("\033[1;33m  • Winget (usually pre-installed on Windows 10/11)\033[0m")
		fmt.Println("\033[1;33m  • Chocolatey: https://chocolatey.org/install\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mOption 3 - Download esptool binary:\033[0m")
		fmt.Println("\033[1;33m  • https://github.com/espressif/esptool/releases\033[0m")
		fmt.Println("")
		fmt.Println("\033[1;33mAfter installation, run this installer again.\033[0m")
		os.Exit(1)
	}

	fmt.Println("\033[1;33mAttempting automatic installation...\033[0m")

	// First check if Python is already installed but not in PATH
	fmt.Println("\033[1;33mChecking for existing Python installations...\033[0m")
	if installEsptoolAfterPythonInstall() {
		return
	}

	// Try to install via pip if Python is available in PATH
	if pythonPath, err := exec.LookPath("python"); err == nil {
		// Check if it's a Windows Store stub
		if strings.Contains(pythonPath, "WindowsApps") {
			fmt.Println("\033[1;33mPython found in PATH is a Windows Store placeholder, skipping...\033[0m")
		} else {
			fmt.Println("\033[1;33mPython found in PATH. Upgrading pip and installing esptool...\033[0m")
			// Upgrade pip first
			upgradeCmd := exec.Command("python", "-m", "pip", "install", "--upgrade", "pip")
			upgradeCmd.Run() // Don't show errors for pip upgrade

			cmd := exec.Command("python", "-m", "pip", "install", "esptool")
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			if err := cmd.Run(); err != nil {
				fmt.Println("\033[1;33mPython installation appears to be incomplete, trying alternative methods...\033[0m")
			} else {
				fmt.Println("\033[1;32mEsptool installed successfully via pip!\033[0m")
				checkEsptoolVersion("esptool.py")
				return
			}
		}
	}

	// Try to install via pip3 if Python3 is available in PATH
	if python3Path, err := exec.LookPath("python3"); err == nil {
		// Check if it's a Windows Store stub
		if strings.Contains(python3Path, "WindowsApps") {
			fmt.Println("\033[1;33mPython3 found in PATH is a Windows Store placeholder, skipping...\033[0m")
		} else {
			fmt.Println("\033[1;33mPython3 found in PATH. Upgrading pip and installing esptool...\033[0m")
			// Upgrade pip first
			upgradeCmd := exec.Command("python3", "-m", "pip", "install", "--upgrade", "pip")
			upgradeCmd.Run() // Don't show errors for pip upgrade

			cmd := exec.Command("python3", "-m", "pip", "install", "esptool")
			cmd.Stdout = os.Stdout
			cmd.Stderr = os.Stderr
			if err := cmd.Run(); err != nil {
				fmt.Println("\033[1;33mPython3 installation appears to be incomplete, trying alternative methods...\033[0m")
			} else {
				fmt.Println("\033[1;32mEsptool installed successfully via pip3!\033[0m")
				checkEsptoolVersion("esptool.py")
				return
			}
		}
	}

	// Try winget (Windows Package Manager)
	if _, err := exec.LookPath("winget"); err == nil {
		fmt.Println("\033[1;33mTrying to install Python via winget...\033[0m")
		cmd := exec.Command("winget", "install", "Python.Python.3.12", "--accept-source-agreements", "--accept-package-agreements")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		err := cmd.Run()

		// Check if Python is already installed or was just installed
		if err != nil {
			// Check if the error is because Python is already installed
			if strings.Contains(err.Error(), "0x8a15002b") || strings.Contains(err.Error(), "already installed") {
				fmt.Println("\033[1;32mPython is already installed. Attempting to install esptool...\033[0m")
			} else {
				fmt.Printf("\033[1;33mWinget installation failed: %v\033[0m\n", err)
			}
		} else {
			fmt.Println("\033[1;32mPython installed via winget. Now installing esptool...\033[0m")
		}

		// Try to install esptool regardless of winget result (Python might already be installed)
		if installEsptoolAfterPythonInstall() {
			return
		}
	}

	// Try chocolatey
	if _, err := exec.LookPath("choco"); err == nil {
		fmt.Println("\033[1;33mTrying to install Python via chocolatey...\033[0m")
		cmd := exec.Command("choco", "install", "python", "-y")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			fmt.Printf("\033[1;33mChocolatey installation failed: %v\033[0m\n", err)
		} else {
			fmt.Println("\033[1;32mPython installed via chocolatey. Now installing esptool...\033[0m")
		}

		// Try to install esptool regardless of chocolatey result (Python might already be installed)
		if installEsptoolAfterPythonInstall() {
			return
		}
	}

	// If all automatic methods fail, provide manual instructions
	fmt.Println("\033[1;31mAll automatic installation methods failed.\033[0m")
	fmt.Println("\033[1;33mPlease install esptool manually using one of these options:\033[0m")
	fmt.Println("")
	fmt.Println("\033[1;33mOption 1 - Install Python and esptool:\033[0m")
	fmt.Println("\033[1;33m  1. Download Python from: https://python.org/downloads/\033[0m")
	fmt.Println("\033[1;33m  2. During installation, check 'Add Python to PATH'\033[0m")
	fmt.Println("\033[1;33m  3. Open Command Prompt and run: pip install esptool\033[0m")
	fmt.Println("")
	fmt.Println("\033[1;33mOption 2 - Install package manager first:\033[0m")
	fmt.Println("\033[1;33m  • Winget (usually pre-installed on Windows 10/11)\033[0m")
	fmt.Println("\033[1;33m  • Chocolatey: https://chocolatey.org/install\033[0m")
	fmt.Println("")
	fmt.Println("\033[1;33mOption 3 - Download esptool binary:\033[0m")
	fmt.Println("\033[1;33m  • https://github.com/espressif/esptool/releases\033[0m")
	fmt.Println("")
	fmt.Println("\033[1;33mAfter installation, run this installer again.\033[0m")
	os.Exit(1)
}

// installEsptoolAfterPythonInstall attempts to install esptool after Python has been freshly installed
// installEsptoolWindows handles Windows esptool installation
func installEsptoolAfterPythonInstall() bool {
	fmt.Println("\033[1;33mRefreshing environment and searching for Python...\033[0m")

	pythonPaths := getAllPythonPaths()

	for _, pythonPath := range pythonPaths {

		if _, err := os.Stat(pythonPath); err == nil {
			fmt.Printf("\033[1;32mFound Python at: %s\033[0m\n", pythonPath)

			if !isValidPythonInstallation(pythonPath) {
				fmt.Printf("\033[1;33mSkipping %s (appears to be a Windows Store stub)\033[0m\n", pythonPath)
				continue
			}

			fmt.Println("\033[1;33mInstalling esptool...\033[0m")

			fmt.Println("\033[1;36mUpgrading pip to latest version...\033[0m")
			upgradeCmd := exec.Command(pythonPath, "-m", "pip", "install", "--upgrade", "pip")
			upgradeCmd.Run()

			if runtime.GOOS == "windows" && runtime.GOARCH == "arm64" {
				fmt.Println("\033[1;33mUsing Windows ARM64 compatible installation method...\033[0m")

				fmt.Println("\033[1;33mInstalling dependencies then compatible version...\033[0m")

				deps := []string{"pyserial", "cryptography", "reedsolo", "PyYAML", "intelhex", "click"}
				allDepsInstalled := true

				for _, dep := range deps {
					fmt.Printf("\033[1;36mInstalling %s...\033[0m\n", dep)
					cmdDep := exec.Command(pythonPath, "-m", "pip", "install", dep)
					cmdDep.Stdout = os.Stdout
					cmdDep.Stderr = os.Stderr
					if err := cmdDep.Run(); err != nil {
						fmt.Printf("\033[1;33mFailed to install %s: %v\033[0m\n", dep, err)
						allDepsInstalled = false
					}
				}

				if !allDepsInstalled {
					fmt.Println("\033[1;33mSome dependencies failed, trying esptool anyway...\033[0m")
				}

				// Now install esptool 4.7.0 without dependencies (to avoid bitarray)
				fmt.Println("\033[1;33mInstalling esptool 4.7.0 without problematic dependencies...\033[0m")
				cmdCompat := exec.Command(pythonPath, "-m", "pip", "install", "esptool==4.7.0", "--no-deps", "--force-reinstall")
				cmdCompat.Stdout = os.Stdout
				cmdCompat.Stderr = os.Stderr

				if err := cmdCompat.Run(); err != nil {
					fmt.Printf("\033[1;31mEsptool installation failed with %s: %v\033[0m\n", pythonPath, err)
					continue
				}
			} else {
				// For non-Windows ARM64, try normal installation first
				cmd := exec.Command(pythonPath, "-m", "pip", "install", "esptool")
				cmd.Stdout = os.Stdout
				cmd.Stderr = os.Stderr

				if err := cmd.Run(); err != nil {
					fmt.Printf("\033[1;33mNormal installation failed: %v\033[0m\n", err)
					continue
				}
			}

			fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")

			// Check if esptool is now available
			if _, err := exec.LookPath("esptool.exe"); err == nil {
				checkEsptoolVersion("esptool.exe")
				return true
			}
			if _, err := exec.LookPath("esptool.py"); err == nil {
				checkEsptoolVersion("esptool.py")
				return true
			}

			// Try to find esptool in Python Scripts directory
			scriptsDir := filepath.Dir(pythonPath) + "\\Scripts"
			esptoolPath := scriptsDir + "\\esptool.exe"
			if _, err := os.Stat(esptoolPath); err == nil {
				fmt.Printf("\033[1;32mFound esptool at: %s\033[0m\n", esptoolPath)
				checkEsptoolVersionWithPath(esptoolPath)
				return true
			}

			fmt.Println("\033[1;32mEsptool installed but may require PATH refresh or new terminal session\033[0m")
			return true
		}
	}

	fmt.Printf("\033[1;33mNo Python installations found in %d checked paths\033[0m\n", len(pythonPaths))

	// If direct path checking fails, try refreshing PATH and checking again
	fmt.Println("\033[1;33mTrying PATH refresh...\033[0m")

	// Try to refresh environment variables (Windows-specific)
	if err := refreshWindowsPath(); err == nil {
		// Check if Python is now available after PATH refresh
		if pythonCmd, err := exec.LookPath("python"); err == nil {
			fmt.Printf("\033[1;32mPython found in PATH after refresh: %s\033[0m\n", pythonCmd)

			// Validate this is not a Windows Store stub
			if !isValidPythonInstallation(pythonCmd) {
				fmt.Printf("\033[1;33mSkipping %s (appears to be a Windows Store stub)\033[0m\n", pythonCmd)
			} else {
				fmt.Println("\033[1;33mInstalling esptool via refreshed PATH...\033[0m")
				cmd := exec.Command("python", "-m", "pip", "install", "esptool")
				cmd.Stdout = os.Stdout
				cmd.Stderr = os.Stderr

				if err := cmd.Run(); err == nil {
					fmt.Println("\033[1;32mEsptool installed successfully!\033[0m")
					if _, err := exec.LookPath("esptool.py"); err == nil {
						checkEsptoolVersion("esptool.py")
						return true
					}
					return true
				} else {
					fmt.Printf("\033[1;33mFailed to install esptool via PATH: %v\033[0m\n", err)
				}
			}
		}
	} else {
		fmt.Printf("\033[1;33mFailed to refresh PATH: %v\033[0m\n", err)
	}

	fmt.Println("\033[1;33mCould not install esptool with detected Python installation, trying other methods...\033[0m")
	return false
}

// refreshWindowsPath attempts to refresh the PATH environment variable on Windows
func refreshWindowsPath() error {
	// Try to read the updated PATH from the registry
	cmd := exec.Command("powershell", "-Command", "[Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')")
	output, err := cmd.Output()
	if err != nil {
		return err
	}

	newPath := strings.TrimSpace(string(output))
	if newPath != "" {
		os.Setenv("PATH", newPath)
		fmt.Println("\033[1;32mPATH refreshed from registry\033[0m")
	}

	return nil
}

// isValidPythonInstallation checks if a Python path is a real installation, not a Windows Store stub
func isValidPythonInstallation(pythonPath string) bool {
	// Skip Windows Store stubs
	if strings.Contains(pythonPath, "WindowsApps") {
		return false
	}

	// Try to run python --version to verify it's a real Python installation
	cmd := exec.Command(pythonPath, "--version")
	output, err := cmd.CombinedOutput()
	if err != nil {
		return false
	}

	versionOutput := string(output)

	// Check if output contains "Python" and a version number
	if strings.Contains(versionOutput, "Python") && (strings.Contains(versionOutput, "3.") || strings.Contains(versionOutput, "2.")) {
		return true
	}

	return false
}

// getAllPythonPaths returns all possible Python installation paths (static + dynamic detection)
func getAllPythonPaths() []string {
	var allPaths []string

	// Static common paths
	commonPaths := []string{
		// Standard Python.org installer paths
		os.Getenv("LOCALAPPDATA") + "\\Programs\\Python\\Python312\\python.exe",
		os.Getenv("LOCALAPPDATA") + "\\Programs\\Python\\Python311\\python.exe",
		os.Getenv("LOCALAPPDATA") + "\\Programs\\Python\\Python310\\python.exe",
		os.Getenv("LOCALAPPDATA") + "\\Programs\\Python\\Python39\\python.exe",
		// System-wide installations
		"C:\\Python312\\python.exe",
		"C:\\Python311\\python.exe",
		"C:\\Python310\\python.exe",
		"C:\\Python39\\python.exe",
		// Winget installation paths (newer versions)
		os.Getenv("LOCALAPPDATA") + "\\Microsoft\\WinGet\\Packages\\Python.Python.3.12_Microsoft.Winget.Source_8wekyb3d8bbwe\\python.exe",
		os.Getenv("LOCALAPPDATA") + "\\Microsoft\\WinGet\\Packages\\Python.Python.3.11_Microsoft.Winget.Source_8wekyb3d8bbwe\\python.exe",
		// Program Files installations
		"C:\\Program Files\\Python312\\python.exe",
		"C:\\Program Files\\Python311\\python.exe",
		"C:\\Program Files\\Python310\\python.exe",
		"C:\\Program Files (x86)\\Python312\\python.exe",
		"C:\\Program Files (x86)\\Python311\\python.exe",
		"C:\\Program Files (x86)\\Python310\\python.exe",
	}

	allPaths = append(allPaths, commonPaths...)

	// Dynamic detection using 'where' command
	cmd := exec.Command("where", "python.exe")
	output, err := cmd.Output()
	if err == nil {
		lines := strings.Split(strings.TrimSpace(string(output)), "\n")
		for _, line := range lines {
			line = strings.TrimSpace(line)
			if line != "" && !strings.Contains(line, "WindowsApps") {
				allPaths = append(allPaths, line)
			}
		}
	}

	// Try to find Python via directory scanning
	scannedPaths := scanForPythonDirectories()
	allPaths = append(allPaths, scannedPaths...)

	// Try to find Python via registry (Windows-specific)
	registryPaths := findPythonInRegistry()
	allPaths = append(allPaths, registryPaths...)

	// Remove duplicates
	seen := make(map[string]bool)
	var uniquePaths []string
	for _, path := range allPaths {
		if !seen[path] {
			seen[path] = true
			uniquePaths = append(uniquePaths, path)
		}
	}

	return uniquePaths
}

// findPythonInRegistry searches for Python installations in Windows registry
func findPythonInRegistry() []string {
	var paths []string

	// Try to query registry for Python installations
	cmd := exec.Command("reg", "query", "HKLM\\SOFTWARE\\Python\\PythonCore", "/s", "/f", "InstallPath", "/k")
	output, err := cmd.Output()
	if err != nil {
		return paths
	}

	lines := strings.Split(string(output), "\n")
	for _, line := range lines {
		line = strings.TrimSpace(line)
		if strings.Contains(line, "InstallPath") {
			// Try to get the actual path value
			parts := strings.Split(line, "\\")
			if len(parts) > 0 {
				keyPath := strings.Join(parts, "\\")
				valueCmd := exec.Command("reg", "query", keyPath, "/ve")
				valueOutput, err := valueCmd.Output()
				if err == nil {
					valueLines := strings.Split(string(valueOutput), "\n")
					for _, valueLine := range valueLines {
						if strings.Contains(valueLine, "REG_SZ") {
							parts := strings.Split(valueLine, "REG_SZ")
							if len(parts) > 1 {
								installPath := strings.TrimSpace(parts[1])
								pythonExe := filepath.Join(installPath, "python.exe")
								if _, err := os.Stat(pythonExe); err == nil {
									paths = append(paths, pythonExe)
								}
							}
						}
					}
				}
			}
		}
	}

	return paths
}

// scanForPythonDirectories scans common directories for Python installations
func scanForPythonDirectories() []string {
	var paths []string

	// Directories to scan
	scanDirs := []string{
		os.Getenv("LOCALAPPDATA") + "\\Programs\\Python",
		os.Getenv("LOCALAPPDATA") + "\\Microsoft\\WinGet\\Packages",
		"C:\\Program Files",
		"C:\\Program Files (x86)",
		"C:\\",
	}

	for _, scanDir := range scanDirs {
		entries, err := os.ReadDir(scanDir)
		if err != nil {
			continue
		}

		for _, entry := range entries {
			if entry.IsDir() {
				dirName := entry.Name()

				// Look for Python-like directory names
				if strings.Contains(strings.ToLower(dirName), "python") {
					pythonExePath := filepath.Join(scanDir, dirName, "python.exe")
					if _, err := os.Stat(pythonExePath); err == nil {
						paths = append(paths, pythonExePath)
					}

					// Also check Scripts subdirectory for python.exe
					scriptsPath := filepath.Join(scanDir, dirName, "Scripts", "python.exe")
					if _, err := os.Stat(scriptsPath); err == nil {
						paths = append(paths, scriptsPath)
					}
				}
			}
		}
	}

	return paths
}

// checkEsptoolVersionWithPath checks esptool version using a specific path
func checkEsptoolVersionWithPath(esptoolPath string) {
	cmdVersion := exec.Command(esptoolPath, "version")
	versionOutput, err := cmdVersion.CombinedOutput()
	if err != nil {
		fmt.Printf("\033[1;31mError checking esptool version: %v\033[0m\nOutput: %s\n", err, string(versionOutput))
		return
	}
	version := strings.TrimSpace(string(versionOutput))
	if !strings.Contains(version, "esptool") || !isVersionCompatible(version, "4.5.0") {
		fmt.Printf("\033[1;31mEsptool version is %s. Version 4.5.0 or later is required.\033[0m\n", version)
		return
	}
	fmt.Printf("\033[1;32mEsptool version verified: %s\033[0m\n", version)
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
