// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

// Global WebSocket instance
let ws = null;
let isInitializing = false;

// Message handlers
const handlers = {};

function registerHandler(type, handler) {
  if (!handlers[type]) {
    handlers[type] = [];
  }
  handlers[type].push(handler);
}

function ensureWebSocket() {
  if (isInitializing) {
    return ws;
  }

  if (!ws || ws.readyState !== WebSocket.OPEN) {
    isInitializing = true;
    ws = new WebSocket("ws://" + window.location.hostname + ":81");

    ws.onopen = () => {
      isInitializing = false;
    };

    ws.onmessage = (event) => {
      if (event.data.startsWith("Connected")) {
        return;
      }

      try {
        const data = JSON.parse(event.data);
        if (data.source && handlers[data.source]) {
          handlers[data.source].forEach((handler) => handler(data));
        }
      } catch (error) {
        console.error("Error parsing WebSocket message:", error);
      }
    };

    ws.onerror = (error) => {
      console.error("WebSocket error:", error);
      isInitializing = false;
    };

    ws.onclose = () => {
      ws = null;
      isInitializing = false;
      setTimeout(ensureWebSocket, 2000);
    };
  }
  return ws;
}

function sendData(data) {
  const ws = ensureWebSocket();
  if (ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(data));
  } else {
    console.error("WebSocket is not open. Current state:", ws.readyState);
  }
}

window.sendData = sendData;
window.ensureWebSocket = ensureWebSocket;
window.registerHandler = registerHandler;

document.addEventListener("DOMContentLoaded", ensureWebSocket);
