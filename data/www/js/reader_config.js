// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

var connection = new WebSocket("ws://" + location.hostname + ":81/");

function loadReaderConfig() {
  fetch("reader_config.json")
    .then((response) => response.json())
    .then((config) => {
      const readerType = config.READER_TYPE || "HID";
      document.getElementById("current-reader-type").textContent = readerType;
      document.getElementById("reader-select").value = readerType;
    })
    .catch((error) => {
      console.error("Error loading reader config:", error);
      document.getElementById("current-reader-type").textContent = "HID";
    });
}

function processReaderForm() {
  const readerType = document.getElementById("reader-select").value;
  
  if (readerType !== "HID" && readerType !== "PAXTON") {
    alert("Invalid reader type selected");
    return;
  }

  const formData = {
    READER_TYPE: readerType
  };

  const readerConfig = JSON.stringify(formData);

  connection.send(readerConfig);

  alert("Reader configuration has been updated.");

  setTimeout(() => {
    loadReaderConfig();
  }, 1000);
}

// Load reader config when page loads
document.addEventListener("DOMContentLoaded", loadReaderConfig);

