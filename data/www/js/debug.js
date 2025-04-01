// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

var connection = new WebSocket("ws://" + location.hostname + ":81/");

// Ensure debug button always has correct styling
document.addEventListener("DOMContentLoaded", function () {
  const debugButton = document.getElementById("debug-submit-button");
  if (debugButton) {
    // Ensure button has the correct classes
    debugButton.classList.add("update-button", "centered");
  }
});

function processDebugForm() {
  const debugSelect = document.getElementById("debug-select");
  if (!debugSelect) {
    console.error("Debug select element not found");
    return;
  }

  // Get the selected option's text content instead of value
  const selectedOption = debugSelect.options[debugSelect.selectedIndex];
  const debugEnabled = selectedOption.text === "Enable";
  console.log("Selected option:", selectedOption.text);
  console.log("Setting debug mode to:", debugEnabled); // Debug log

  const formData = {
    DEBUG: debugEnabled,
  };

  const debugConfig = JSON.stringify(formData);
  console.log("Sending debug config:", debugConfig); // Debug log
  connection.send(debugConfig);

  if (debugEnabled) {
    window.alert(
      "Debug settings have been updated.\n\n" +
        "To connect to the device, use:\n" +
        "macOS:\n" +
        "screen /dev/cu.usbserial-XXXX 115200\n\n" +
        "Linux:\n" +
        "screen /dev/ttyUSBX 115200\n" +
        "or\n" +
        "screen /dev/ttyACMX 115200\n\n" +
        "Note: Debug mode will slightly impact battery life. Remember to disable debugging prior to deployment.\n\n" +
        "Rebooting..."
    );
  } else {
    window.alert(
      "Debug mode has been disabled.\n\n" +
        "The device will no longer output debug messages over serial.\n\n" +
        "Rebooting..."
    );
  }

  setTimeout(function () {
    window.location.reload();
  }, 2000);
}

// Add WebSocket message handler to confirm state changes
connection.onmessage = function (event) {
  console.log("WebSocket message received:", event.data);
};

connection.onerror = function (error) {
  console.error("WebSocket error:", error);
};

connection.onopen = function () {
  console.log("WebSocket connection established");
};

// Function to update the current debug mode display
function updateDebugMode() {
  fetch("debug_config.json")
    .then((response) => response.json())
    .then((data) => {
      // Convert boolean to string status with capitalized first letter
      const status = data.DEBUG ? "Enabled" : "Disabled";
      updateDebugStatus(status);
    })
    .catch((error) => {
      console.error("Error fetching debug config:", error);
      document.getElementById("current-debug-mode").textContent =
        "Error loading";
    });
}

// Initialize debug mode display when page loads
document.addEventListener("DOMContentLoaded", updateDebugMode);

function updateDebugSelect(currentMode) {
  const debugSelect = document.getElementById("debug-select");
  debugSelect.innerHTML = ""; // Clear existing options

  // Add only the opposite option of current state
  const option = document.createElement("option");
  if (currentMode === "Enabled") {
    option.value = "Disabled";
    option.textContent = "Disable";
  } else {
    option.value = "Enabled";
    option.textContent = "Enable";
  }
  debugSelect.appendChild(option);
}

// Call this when loading current debug state
function updateDebugStatus(status) {
  const debugStatus = document.getElementById("current-debug-mode");
  // Capitalize first letter for display
  debugStatus.textContent = status.charAt(0).toUpperCase() + status.slice(1);
  updateDebugSelect(debugStatus.textContent);
}
