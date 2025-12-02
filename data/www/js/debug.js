// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

var connection = new WebSocket("ws://" + location.hostname + ":81/");

document.addEventListener("DOMContentLoaded", function () {
  const debugButton = document.getElementById("debug-submit-button");
  if (debugButton) {
    debugButton.classList.add("update-button", "centered");
  }
});

function processDebugForm() {
  const debugSelect = document.getElementById("debug-select");
  if (!debugSelect) {
    console.error("Debug select element not found");
    return;
  }

  const selectedOption = debugSelect.options[debugSelect.selectedIndex];
  const debugEnabled = selectedOption.text === "Enable";
  console.log("Selected option:", selectedOption.text);
  console.log("Setting debug mode to:", debugEnabled);

  const formData = {
    DEBUG: debugEnabled,
  };

  const debugConfig = JSON.stringify(formData);
  console.log("Sending debug config:", debugConfig);
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

connection.onmessage = function (event) {
  console.log("WebSocket message received:", event.data);
};

connection.onerror = function (error) {
  console.error("WebSocket error:", error);
};

connection.onopen = function () {
  console.log("WebSocket connection established");
};

function updateDebugMode() {
  fetch("debug_config.json")
    .then((response) => response.json())
    .then((data) => {
      const status = data.DEBUG ? "Enabled" : "Disabled";
      updateDebugStatus(status);
    })
    .catch((error) => {
      console.error("Error fetching debug config:", error);
      document.getElementById("current-debug-mode").textContent =
        "Error loading";
    });
}

document.addEventListener("DOMContentLoaded", updateDebugMode);

function updateDebugSelect(currentMode) {
  const debugSelect = document.getElementById("debug-select");
  debugSelect.innerHTML = "";

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

function updateDebugStatus(status) {
  const debugStatus = document.getElementById("current-debug-mode");
  debugStatus.textContent = status.charAt(0).toUpperCase() + status.slice(1);
  updateDebugSelect(debugStatus.textContent);
}
