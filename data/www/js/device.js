// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

var connection = new WebSocket("ws://" + location.hostname + ":81/");

function eraseCards() {
  if (confirm("Are you sure you want to erase the stored card data?")) {
    alert("Erasing stored card data...");
    let cardData = JSON.stringify({ WIPE_CARDS: true });
    connection.send(cardData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function resetCard() {
  if (
    confirm(
      "Are you sure you want to reset the Reset Card to the default values?"
    )
  ) {
    alert("Reseting Reset Card values to BL = 35, FC = 111, CN = 4444...");
    let resetData = JSON.stringify({ RESET_CARD: true });
    connection.send(resetData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function restoreNotificationsConfig() {
  if (
    confirm(
      "Are you sure you want to restore the notification settings to the default values?"
    )
  ) {
    alert("Clearing stored notification settings...");
    let configData = JSON.stringify({ WIPE_CONFIG: true });
    connection.send(configData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function resetWireless() {
  if (
    confirm(
      "Are you sure you want to reset the wireless credentials to the default values?"
    )
  ) {
    alert("Reseting wireless credentials and rebooting the device...");
    let wirelessCreds = JSON.stringify({ RESET_WIRELESS: true });
    connection.send(wirelessCreds);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function fullReset() {
  if (confirm("Are you sure you want to perform a full factory reset?")) {
    alert(
      "Clearing stored card data, Reset Card values, Notification/GPIO settings, and resetting wireless credentials..."
    );
    let restoreData = JSON.stringify({ RESET_DEVICE: true });
    connection.send(restoreData);

    window.location.reload();
  } else {
  }
}
