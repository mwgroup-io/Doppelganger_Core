// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Use the existing connection from device.js
var connection =
  connection || new WebSocket("ws://" + location.hostname + ":81/");

function resetGPIO() {
  if (
    confirm("Are you sure you want to reset GPIO settings to default values?")
  ) {
    alert("Resetting GPIO settings to factory defaults...");
    const formData = JSON.stringify({ reset_gpio: true });
    connection.send(formData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  }
}

// WebSocket event handlers
connection.onmessage = function (event) {
  console.log("WebSocket message received:", event.data);
  const response = JSON.parse(event.data);

  if (response.status === "success") {
    alert("GPIO settings have been reset to default values");
  } else {
    alert("Error resetting GPIO settings");
  }
};
