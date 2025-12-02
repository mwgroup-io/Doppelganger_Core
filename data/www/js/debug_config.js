// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

document.addEventListener("DOMContentLoaded", function () {
  fetch("/debug_config.json")
    .then((response) => response.json())
    .then((data) => {
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

  const isEnabled = Boolean(data.DEBUG);

  debugModeSpan.textContent = isEnabled ? "Enabled" : "Disabled";

  const debugSelect = document.getElementById("debug-select");
  if (debugSelect) {
    debugSelect.innerHTML = "";

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
