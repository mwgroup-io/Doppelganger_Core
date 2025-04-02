// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Function to update notification status display
function updateNotificationStatus() {
  fetch("/notifications")
    .then((response) => response.json())
    .then((data) => {
      const statusElement = document.getElementById(
        "current-notification-mode"
      );
      if (statusElement) {
        statusElement.textContent =
          data.enable_email === "true" ? "Enabled" : "Disabled";
      }

      // Update form fields if they exist
      const formFields = {
        enable_email: data.enable_email,
        smtp_host: data.smtp_host || "",
        smtp_port: data.smtp_port || "",
        smtp_user: data.smtp_user || "",
        smtp_recipient: data.smtp_recipient || "",
      };

      Object.entries(formFields).forEach(([id, value]) => {
        const element = document.getElementById(id);
        if (element) {
          element.value = value;
        }
      });
    })
    .catch((error) => {
      console.error("Error fetching notification settings:", error);
      const statusElement = document.getElementById(
        "current-notification-mode"
      );
      if (statusElement) {
        statusElement.textContent = "Error";
      }
    });
}

// Function to show the notifications form
function showNotificationsForm() {
  const form = document.querySelector(".notificationsForm");
  if (form) {
    form.classList.add("visible");
  }
}

// Function to hide the notifications form
function hideNotificationsForm() {
  const form = document.querySelector(".notificationsForm");
  if (form) {
    form.classList.remove("visible");
    // Clear password field for security
    const passField = document.getElementById("smtp_pass");
    if (passField) {
      passField.value = "";
    }
  }
}

// Function to toggle notifications form visibility
function toggleNotificationsVisibility() {
  const form = document.querySelector(".notificationsForm");
  if (form) {
    if (form.classList.contains("visible")) {
      hideNotificationsForm();
    } else {
      showNotificationsForm();
    }
  }
}

// Function to process the notifications form
function processNotificationsForm() {
  const formData = {
    source: "notifications",
    enable_email: document.getElementById("enable_email")?.value || "false",
    smtp_host: document.getElementById("smtp_host")?.value || "",
    smtp_port: document.getElementById("smtp_port")?.value || "",
    smtp_user: document.getElementById("smtp_user")?.value || "",
    smtp_pass: document.getElementById("smtp_pass")?.value || "",
    smtp_recipient: document.getElementById("smtp_recipient")?.value || "",
  };

  if (formData.enable_email === "true") {
    alert("Notifications will be sent to: " + formData.smtp_recipient);
  } else {
    formData.smtp_host = "smtp.<domain>.com";
    formData.smtp_port = "465";
    formData.smtp_user = "<sender_email>@<domain>.com";
    formData.smtp_pass = "AppPassword";
    formData.smtp_recipient = "<phonenumber>@<carrierdomain>.com";
    alert("Notifications have been disabled.");
  }

  // Send data and ensure UI is updated
  sendData(formData);
  hideNotificationsForm();

  // Update the status after a short delay to allow the server to process
  setTimeout(updateNotificationStatus, 500);
}

// Register notification message handler
registerHandler("notifications", function (data) {
  if (data.status === "success") {
    updateNotificationStatus();
    hideNotificationsForm();
    window.alert("Notification settings have been updated.");
  } else {
    console.error("Error updating notification settings:", data);
    window.alert("Error updating notification settings.");
  }
});

// Make functions globally available
window.updateNotificationStatus = updateNotificationStatus;
window.hideNotificationsForm = hideNotificationsForm;
window.showNotificationsForm = showNotificationsForm;
window.toggleNotificationsVisibility = toggleNotificationsVisibility;
window.processNotificationsForm = processNotificationsForm;

// Initialize when the page loads
document.addEventListener("DOMContentLoaded", function () {
  updateNotificationStatus();
});
