// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Reset Card Configuration
let resetCardConfig = {
  RBL: "",
  RFC: "",
  RCN: "",
  PAXTON_RESET_HEX: "",
};

// Register reset card message handler
registerHandler("reset_card", function (data) {
  if (data.status === "success") {
    // 1. Show alert
    window.alert("Reset card settings have been updated.");

    // 2. Hide form
    hideResetCardForm();

    // 3. Update table
    updateResetCardTable();
  } else {
    window.alert("Error updating reset card settings.");
  }
});

// Function to update the reset card table
function updateResetCardTable() {
  fetch("reset_card.json")
    .then((response) => response.json())
    .then((data) => {
      // Update table display
      const tableBody = document.getElementById("cardTable");
      if (tableBody) {
        tableBody.innerHTML = `
          <tr>
            <td>${data.RBL || ""}</td>
            <td>${data.RFC || ""}</td>
            <td>${data.RCN || ""}</td>
          </tr>
        `;
      }

      // Update form fields
      const formFields = {
        bit_length: data.RBL || "",
        facility_code: data.RFC || "",
        card_number: data.RCN || "",
        paxton_reset_hex: data.PAXTON_RESET_HEX || "0000001337",
      };

      Object.entries(formFields).forEach(([id, value]) => {
        const element = document.getElementById(id);
        if (element) {
          element.value = value;
        }
      });

      // Update Paxton Reset Card display
      const paxtonHex = data.PAXTON_RESET_HEX || "0000001337";
      const paxtonTable = document.getElementById("paxtonResetTable");
      if (paxtonTable) {
        paxtonTable.innerHTML = `
          <tr>
            <td>${paxtonHex.toUpperCase()}</td>
          </tr>
        `;
      }
    })
    .catch((error) => {
      console.error("Error fetching reset card settings:", error);
      const tableBody = document.getElementById("cardTable");
      if (tableBody) {
        tableBody.innerHTML = `
          <tr>
            <td colspan="3">Error loading data</td>
          </tr>
        `;
      }
    });
}

function validateResetCardInput(bitLength, facilityCode, cardNumber) {
  const errors = [];
  const digitRegex = /^\d+$/;

  if (
    !digitRegex.test(bitLength) ||
    (bitLength !== "26" && bitLength !== "35")
  ) {
    errors.push("Reset Cards must be either 26 or 35 bit cards");
  }

  if (
    !digitRegex.test(facilityCode) ||
    facilityCode < 1 ||
    facilityCode > 255
  ) {
    errors.push("Facility Code must be a numeric value between 1 and 255");
  }

  cardNumber = cardNumber.replace(/,/g, "");
  if (!digitRegex.test(cardNumber) || cardNumber < 1 || cardNumber > 65535) {
    errors.push("Card Number must be a numeric value between 1 and 65,535");
  }

  return errors;
}

function processResetCardForm() {
  const bitLength = document.getElementById("bit_length").value;
  const facilityCode = document.getElementById("facility_code").value;
  const cardNumber = document.getElementById("card_number").value;

  const errors = validateResetCardInput(bitLength, facilityCode, cardNumber);

  if (errors.length === 0) {
    const formData = {
      source: "reset_card",
      RBL: parseInt(bitLength),
      RFC: parseInt(facilityCode),
      RCN: parseInt(cardNumber),
    };

    // Send data and update UI immediately
    sendData(formData);
    window.alert("Reset card settings have been updated.");

    // Hide the form
    const form = document.querySelector(".resetCardForm");
    if (form) {
      form.style.display = "none";
    }

    // Update the table after a short delay
    setTimeout(updateResetCardTable, 1000);
  } else {
    const errorMessage =
      "Invalid input parameters. Please check the following issues:\n" +
      errors.map((error) => `• ${error}`).join("\n");
    window.alert(errorMessage);
  }
}

function showResetCardForm() {
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.style.display = "block";
  }
}

function hideResetCardForm() {
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.style.display = "none";
  }
}

function toggleResetCardVisibility() {
  const form = document.querySelector(".resetCardForm");
  if (form) {
    if (form.style.display === "none") {
      form.style.display = "block";
    } else {
      form.style.display = "none";
    }
  }
}

// Function to validate Paxton reset card hex input
function validatePaxtonResetInput(hexValue) {
  var errors = [];

  // Check if hex value is exactly 10 characters
  if (hexValue.length !== 10) {
    errors.push("HEX value must be exactly 10 characters");
  }

  // Check if hex value contains only valid hex characters (0-9, A-F)
  var hexRegex = /^[0-9A-Fa-f]+$/;
  if (!hexRegex.test(hexValue)) {
    errors.push(
      "HEX value must contain only hexadecimal characters (0-9, A-F)"
    );
  }

  return errors;
}

// Function to toggle Paxton reset card form visibility
function togglePaxtonResetVisibility() {
  var form = document.querySelector(".paxtonResetForm");
  if (form.style.display === "none") {
    form.style.display = "block";
  } else {
    form.style.display = "none";
  }
}

// Function to process Paxton reset card form
function processPaxtonResetForm() {
  // Get form input
  var hexValue = document
    .getElementById("paxton_reset_hex")
    .value.toUpperCase();

  // Validate input
  var errors = validatePaxtonResetInput(hexValue);

  if (errors.length === 0) {
    // Prepare data object
    var formData = {
      PAXTON_RESET_HEX: hexValue,
    };

    // Send data via WebSocket
    sendData(formData);

    // Display success message
    var successMessage = "Paxton Reset Card has been updated to: " + hexValue;
    window.alert(successMessage);

    // Wait a moment for the file to be written, then update the display
    setTimeout(function () {
      updateResetCardTable();
      var paxtonTable = document.getElementById("paxtonResetTable");
      if (paxtonTable) {
        paxtonTable.innerHTML = `
          <tr>
            <td>${hexValue}</td>
          </tr>
        `;
      }
    }, 1000);

    // Hide the form
    var form = document.querySelector(".paxtonResetForm");
    if (form) {
      form.style.display = "none";
    }
  } else {
    var errorMessage =
      "Invalid Paxton HEX value. Please check the following issues:\n";
    errorMessage += errors.map((error) => `• ${error}`).join("\n");
    window.alert(errorMessage);
  }
}

// Make functions globally available
window.updateResetCardTable = updateResetCardTable;
window.hideResetCardForm = hideResetCardForm;
window.showResetCardForm = showResetCardForm;
window.toggleResetCardVisibility = toggleResetCardVisibility;
window.processResetCardForm = processResetCardForm;
window.togglePaxtonResetVisibility = togglePaxtonResetVisibility;
window.processPaxtonResetForm = processPaxtonResetForm;

// Initialize when the page loads
document.addEventListener("DOMContentLoaded", function () {
  // Ensure we have a WebSocket connection
  ensureWebSocket();
  updateResetCardTable();

  // Ensure forms are hidden initially
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.style.display = "none";
  }
  const paxtonForm = document.querySelector(".paxtonResetForm");
  if (paxtonForm) {
    paxtonForm.style.display = "none";
  }
});
