// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

var connection = new WebSocket("ws://" + location.hostname + ":81/");

// Function to validate input parameters
function validateResetCardInput(bitLength, facilityCode, cardNumber) {
  // Define error messages
  var errors = [];

  // Regular expression to match only digits
  var digitRegex = /^\d+$/;

  // Check bit_length
  if (
    !digitRegex.test(bitLength) ||
    (bitLength !== "26" && bitLength !== "35")
  ) {
    errors.push("Reset Cards must be either 26 or 35 bit cards");
  }

  // Check facility_code
  if (
    !digitRegex.test(facilityCode) ||
    facilityCode < 1 ||
    facilityCode > 255
  ) {
    errors.push("Facility Code must be a numeric value between 1 and 255");
  }

  // Check card_number
  // Strip commas and then validate
  cardNumber = cardNumber.replace(/,/g, ""); // Remove commas
  if (!digitRegex.test(cardNumber) || cardNumber < 1 || cardNumber > 65535) {
    errors.push("Card Number must be a numeric value between 1 and 65,535");
  }

  // Check for unauthorized characters
  if (
    /[^\d]/.test(bitLength) ||
    /[^\d]/.test(facilityCode) ||
    /[^\d]/.test(cardNumber)
  ) {
    errors.push("Only numeric values are accepted.");
  }

  return errors;
}

// Function to update the stealth card table
function updateResetCardTable() {
  fetch("reset_card.json")
    .then((response) => response.json())
    .then((jsonData) => {
      const table = document.getElementById("cardTable");
      table.innerHTML = `<tr>
        <td>${jsonData.RBL}</td>
        <td>${jsonData.RFC}</td>
        <td>${jsonData.RCN}</td>
      </tr>`;
    })
    .catch((error) => {
      console.error("Error updating reset card table:", error);
    });
}

// Function to process the form input and send data via WebSocket
function processResetCardForm() {
  // Get form inputs
  var bitLength = document.getElementById("bit_length").value;
  var facilityCode = document.getElementById("facility_code").value;
  var cardNumber = document.getElementById("card_number").value;

  // Validate input parameters
  var errors = validateResetCardInput(bitLength, facilityCode, cardNumber);

  // Display the result using a single pop-up window for all errors
  if (errors.length === 0) {
    // Prepare data object
    var formData = {
      RBL: bitLength,
      RFC: facilityCode,
      RCN: cardNumber,
    };

    // Convert data to JSON
    var resetCard = JSON.stringify(formData);

    // Send data via WebSocket
    connection.send(resetCard);

    // Display success message
    var successMessage = "Reset Card has been updated.";
    window.alert(successMessage);

    // Wait a moment for the file to be written, then update the table
    setTimeout(updateResetCardTable, 1000);

    // Hide the form
    var form = document.querySelector(".resetCardForm");
    form.style.display = "none";
  } else {
    var errorMessage =
      "Invalid input parameters. Please check the following issues:\n";
    errorMessage += errors.map((error) => `• ${error}`).join("\n");
    window.alert(errorMessage);
  }
}

// Function to toggle the visibility of the Reset Card form
function toggleResetCardVisibility() {
  var form = document.querySelector(".resetCardForm");
  form.style.display =
    form.style.display === "none" || form.style.display === ""
      ? "block"
      : "none";
}
