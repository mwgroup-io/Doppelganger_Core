async function loadNetworkInfo() {
  try {
    const response = await fetch("/network");
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json();

    const configIP = document.getElementById("networkIP");
    const configHostname = document.getElementById("networkHostname");

    if (data.ip && data.ip !== "Not Connected") {
      if (configIP) {
        configIP.textContent = data.ip;
      }

      if (configHostname) {
        configHostname.textContent = data.mdns || data.hostname || "N/A";
      }
    } else {
      if (configIP) {
        configIP.textContent = "Not Connected";
      }
      if (configHostname) {
        configHostname.textContent = "N/A";
      }
    }
  } catch (error) {
    console.error("Error fetching network information:", error);
    const configIP = document.getElementById("networkIP");
    if (configIP) {
      configIP.textContent = "Error";
    }
  }
}

document.addEventListener("DOMContentLoaded", loadNetworkInfo);
