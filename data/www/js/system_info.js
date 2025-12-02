// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

async function getFirmwareVersion() {
  try {
    const response = await fetch("/firmware");
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json();

    document.getElementById("deviceInfo").textContent =
      data.device || "Unknown";
    document.getElementById("firmwareVersion").textContent =
      data.version || "Unknown";
    document.getElementById("buildDate").textContent =
      data.buildDate || "Unknown";
  } catch (error) {
    console.error("Error fetching system information:", error);
    document.getElementById("deviceInfo").textContent = "Error loading";
    document.getElementById("firmwareVersion").textContent = "Error loading";
    document.getElementById("buildDate").textContent = "Error loading";
  }
}

document.addEventListener("DOMContentLoaded", getFirmwareVersion);
