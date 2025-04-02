// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Reset Card Configuration
let resetCardConfig = {
  RBL: "",
  RFC: "",
  RCN: "",
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
      };

      Object.entries(formFields).forEach(([id, value]) => {
        const element = document.getElementById(id);
        if (element) {
          element.value = value;
        }
      });
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
    document.querySelector(".resetCardForm").classList.remove("visible");

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
    form.classList.add("visible");
  }
}

function hideResetCardForm() {
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.classList.remove("visible");
  }
}

function toggleResetCardVisibility() {
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.classList.toggle("visible");
  }
}

// Make functions globally available
window.updateResetCardTable = updateResetCardTable;
window.hideResetCardForm = hideResetCardForm;
window.showResetCardForm = showResetCardForm;
window.toggleResetCardVisibility = toggleResetCardVisibility;
window.processResetCardForm = processResetCardForm;

// Initialize when the page loads
document.addEventListener("DOMContentLoaded", function () {
  // Ensure we have a WebSocket connection
  ensureWebSocket();
  updateResetCardTable();

  // Ensure form is hidden initially
  const form = document.querySelector(".resetCardForm");
  if (form) {
    form.classList.remove("visible");
  }
});
