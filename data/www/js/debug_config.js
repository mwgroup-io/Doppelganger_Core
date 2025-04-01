// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Ensure DOM is loaded before running scripts
document.addEventListener("DOMContentLoaded", function () {
  // Fetch debug configuration
  fetch("/debug_config.json")
    .then((response) => response.json())
    .then((data) => {
      console.log("Received debug config:", data); // Debug log
      updateDebugDisplay(data);
    })
    .catch((error) => {
      console.error("Error loading debug configuration:", error);
      const debugStatus = document.getElementById("current-debug-mode");
      if (debugStatus) {
        debugStatus.textContent = "Error loading status";
      }
    });
});

function updateDebugDisplay(data) {
  const debugModeSpan = document.getElementById("current-debug-mode");
  if (!debugModeSpan) {
    console.error("Debug mode span element not found");
    return;
  }

  // Get debug state directly from the config
  const isEnabled = Boolean(data.DEBUG);
  console.log("Debug state:", isEnabled); // Debug log

  // Update the status text
  debugModeSpan.textContent = isEnabled ? "Enabled" : "Disabled";

  // Update the select element
  const debugSelect = document.getElementById("debug-select");
  if (debugSelect) {
    // Clear existing options
    debugSelect.innerHTML = "";

    // Add options based on current state
    const options = [
      { value: "disabled", text: "Disable" },
      { value: "enabled", text: "Enable" },
    ];

    options.forEach((option) => {
      const opt = document.createElement("option");
      opt.value = option.value;
      opt.text = option.text;
      if (
        (option.value === "enabled" && isEnabled) ||
        (option.value === "disabled" && !isEnabled)
      ) {
        opt.selected = true;
      }
      debugSelect.appendChild(opt);
    });
  }
}
