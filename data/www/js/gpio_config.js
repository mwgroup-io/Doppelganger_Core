// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// GPIO Configuration
let gpioConfig = {
  pin35_enabled: false,
  pin36_enabled: false,
  pin35_default_high: false,
  pin36_default_high: false,
  pin35_pulse_duration: 1000,
  pin36_pulse_duration: 1000,
};

// Register GPIO message handler
registerHandler("gpio", function (data) {
  updateGPIOStatus();
});

// Function to update the GPIO status display
function updateGPIOStatus() {
  fetch("gpio_settings.json")
    .then((response) => response.json())
    .then((data) => {
      // Update status display
      document.getElementById("pin35-status").textContent = data.pin35_enabled
        ? "Enabled"
        : "Disabled";
      document.getElementById("pin36-status").textContent = data.pin36_enabled
        ? "Enabled"
        : "Disabled";
      document.getElementById("pin35-duration").textContent =
        data.pin35_pulse_duration;
      document.getElementById("pin36-duration").textContent =
        data.pin36_pulse_duration;

      // Update form fields
      document.getElementById("pin35_status").value = data.pin35_enabled
        ? "enabled"
        : "disabled";
      document.getElementById("pin36_status").value = data.pin36_enabled
        ? "enabled"
        : "disabled";
      document.getElementById("pin35_pulse_duration").value =
        data.pin35_pulse_duration;
      document.getElementById("pin36_pulse_duration").value =
        data.pin36_pulse_duration;
    })
    .catch((error) => {
      console.error("Error fetching GPIO config:", error);
      document.getElementById("pin35-status").textContent = "Error";
      document.getElementById("pin36-status").textContent = "Error";
      document.getElementById("pin35-duration").textContent = "Error";
      document.getElementById("pin36-duration").textContent = "Error";
    });
}

function validateGPIOInput(
  pin35Status,
  pin35Duration,
  pin36Status,
  pin36Duration
) {
  const errors = [];
  const durationMin = 100;
  const durationMax = 10000;

  if (pin35Duration < durationMin || pin35Duration > durationMax) {
    errors.push(
      `GPIO 35 duration must be between ${durationMin} and ${durationMax} ms`
    );
  }

  if (pin36Duration < durationMin || pin36Duration > durationMax) {
    errors.push(
      `GPIO 36 duration must be between ${durationMin} and ${durationMax} ms`
    );
  }

  return errors;
}

function processGPIOForm() {
  const pin35Status =
    document.getElementById("pin35_status").value === "enabled";
  const pin35Duration = parseInt(
    document.getElementById("pin35_pulse_duration").value
  );
  const pin36Status =
    document.getElementById("pin36_status").value === "enabled";
  const pin36Duration = parseInt(
    document.getElementById("pin36_pulse_duration").value
  );

  const errors = validateGPIOInput(
    pin35Status,
    pin35Duration,
    pin36Status,
    pin36Duration
  );

  if (errors.length === 0) {
    const formData = {
      pin35_enabled: pin35Status,
      pin35_pulse_duration: pin35Duration,
      pin36_enabled: pin36Status,
      pin36_pulse_duration: pin36Duration,
    };

    sendData(formData);
    window.alert("GPIO settings have been updated.");

    // Hide the form
    document.querySelector(".gpioForm").classList.remove("visible");

    // Update the status display after a short delay
    setTimeout(updateGPIOStatus, 1000);
  } else {
    const errorMessage =
      "Invalid input parameters. Please check the following issues:\n" +
      errors.map((error) => `• ${error}`).join("\n");
    window.alert(errorMessage);
  }
}

function toggleGPIOVisibility() {
  const form = document.querySelector(".gpioForm");
  if (form) {
    form.classList.toggle("visible");
  }
}

// Initialize when the page loads
document.addEventListener("DOMContentLoaded", function () {
  // Ensure we have a WebSocket connection
  ensureWebSocket();
  updateGPIOStatus();

  // Ensure form is hidden initially
  const form = document.querySelector(".gpioForm");
  if (form) {
    form.classList.remove("visible");
  }
});
