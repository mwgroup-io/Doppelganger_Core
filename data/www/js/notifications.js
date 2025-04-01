// Fetch and display current notification settings
async function fetchNotificationSettings() {
  try {
    const response = await fetch("/notifications");
    const data = await response.json();

    document.getElementById("current-notification-mode").textContent =
      data.enable_email === "true" ? "Enabled" : "Disabled";

    // Populate form fields with current values
    document.getElementById("enable_email").value = data.enable_email;
    document.getElementById("smtp_host").value = data.smtp_host || "";
    document.getElementById("smtp_port").value = data.smtp_port || "";
    document.getElementById("smtp_user").value = data.smtp_user || "";
    document.getElementById("smtp_recipient").value = data.smtp_recipient || "";
    // Don't populate password for security
  } catch (error) {
    console.error("Error fetching notification settings:", error);
  }
}

// Process notification settings form submission
async function processNotificationsForm() {
  const formData = {
    enable_email: document.getElementById("enable_email").value,
    smtp_host: document.getElementById("smtp_host").value,
    smtp_port: document.getElementById("smtp_port").value,
    smtp_user: document.getElementById("smtp_user").value,
    smtp_pass: document.getElementById("smtp_pass").value,
    smtp_recipient: document.getElementById("smtp_recipient").value,
  };

  try {
    const ws = new WebSocket(`ws://${window.location.hostname}:81`);

    ws.onopen = () => {
      ws.send(JSON.stringify(formData));
    };

    ws.onmessage = (event) => {
      console.log("Message from server:", event.data);
      // Refresh the notification status after update
      fetchNotificationSettings();
    };

    ws.onerror = (error) => {
      console.error("WebSocket error:", error);
    };

    // Hide the form after submission
    document.querySelector(".notificationsForm").style.display = "none";
  } catch (error) {
    console.error("Error updating notification settings:", error);
  }
}

// Initialize notification settings on page load
document.addEventListener("DOMContentLoaded", fetchNotificationSettings);
