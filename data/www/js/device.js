// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

document.addEventListener("DOMContentLoaded", function () {
  ensureWebSocket();

  registerHandler("cards", function (data) {
    document
      .querySelector('button[onclick="eraseCards()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("Card data has been erased successfully.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error erasing card data.");
    }
  });

  registerHandler("reset_card", function (data) {
    document
      .querySelector('button[onclick="resetCard()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("Reset card has been restored to default values.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error resetting card values.");
    }
  });

  registerHandler("notifications", function (data) {
    document
      .querySelector('button[onclick="restoreNotificationsConfig()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("Notification settings have been restored to defaults.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error resetting notification settings.");
    }
  });

  registerHandler("gpio", function (data) {
    document
      .querySelector('button[onclick="resetGPIO()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("GPIO settings have been reset to defaults.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error resetting GPIO settings.");
    }
  });

  registerHandler("wireless", function (data) {
    document
      .querySelector('button[onclick="resetWireless()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("Wireless credentials have been reset. Device will reboot.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error resetting wireless credentials.");
    }
  });

  registerHandler("system", function (data) {
    document
      .querySelector('button[onclick="fullReset()"]')
      .classList.remove("active");
    if (data.status === "success") {
      alert("Full device reset completed. Device will reboot.");
      setTimeout(function () {
        window.location.reload();
      }, 2000);
    } else {
      alert("Error performing full device reset.");
    }
  });
});

function eraseCards() {
  if (confirm("Are you sure you want to erase the stored card data?")) {
    document
      .querySelector('button[onclick="eraseCards()"]')
      .classList.add("active");
    const cardData = { source: "cards", WIPE_CARDS: true };
    sendData(cardData);
  }
}

function resetCard() {
  if (
    confirm(
      "Are you sure you want to reset the Reset Card to the default values?"
    )
  ) {
    document
      .querySelector('button[onclick="resetCard()"]')
      .classList.add("active");
    const resetData = { source: "reset_card", RESET_CARD: true };
    sendData(resetData);
  }
}

function restoreNotificationsConfig() {
  if (
    confirm(
      "Are you sure you want to restore the notification settings to the default values?"
    )
  ) {
    document
      .querySelector('button[onclick="restoreNotificationsConfig()"]')
      .classList.add("active");
    const configData = { source: "notifications", WIPE_CONFIG: true };
    sendData(configData);
  }
}

function resetGPIO() {
  if (
    confirm("Are you sure you want to reset GPIO settings to default values?")
  ) {
    document
      .querySelector('button[onclick="resetGPIO()"]')
      .classList.add("active");
    const formData = { source: "gpio", reset_gpio: true };
    sendData(formData);
  }
}

function resetWireless() {
  if (
    confirm(
      "Are you sure you want to reset the wireless credentials to the default values?"
    )
  ) {
    document
      .querySelector('button[onclick="resetWireless()"]')
      .classList.add("active");
    const wirelessCreds = { source: "wireless", RESET_WIRELESS: true };
    sendData(wirelessCreds);
  }
}

function fullReset() {
  if (confirm("Are you sure you want to perform a full factory reset?")) {
    document
      .querySelector('button[onclick="fullReset()"]')
      .classList.add("active");
    const restoreData = { source: "system", RESET_DEVICE: true };
    sendData(restoreData);
  }
}
