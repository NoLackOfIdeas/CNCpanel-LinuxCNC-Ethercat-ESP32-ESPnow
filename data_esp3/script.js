"use strict";

document.addEventListener("DOMContentLoaded", () => {
  const appState = {
    config: {},
    websocket: null,
    isConnecting: false,
  };

  const DOM = {
    wsIndicator: document.getElementById("ws-indicator"),
    wsText: document.getElementById("ws-text"),
    statusMessage: document.getElementById("status-message"),
    saveBtn: document.getElementById("save-config-btn"),
    updateBtn: document.getElementById("fw-update-btn"),
    tabs: document.querySelector(".tab-bar"),
    tabContentContainer: document.getElementById("tab-content-container"),
    liveButtonMatrix: document.getElementById("button-matrix-live"),
    liveAxisSelector: document.getElementById("live-axis-selector"),
    liveStepSelector: document.getElementById("live-step-selector"),
    liveHandwheelPos: document.getElementById("live-handwheel-pos"),
  };

  // --- WebSocket Management ---
  function initWebSocket() {
    if (appState.websocket || appState.isConnecting) return;
    appState.isConnecting = true;

    const wsUrl = `ws://${window.location.hostname}/ws`;
    appState.websocket = new WebSocket(wsUrl);

    appState.websocket.onopen = () => {
      console.log("WebSocket connected.");
      updateWsStatus(true);
      appState.isConnecting = false;
    };

    appState.websocket.onclose = () => {
      console.log("WebSocket disconnected. Reconnecting...");
      updateWsStatus(false);
      appState.websocket = null;
      appState.isConnecting = false;
      setTimeout(initWebSocket, 2000);
    };

    appState.websocket.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        handleWsMessage(msg);
      } catch (e) {
        console.error("Invalid JSON from server:", event.data);
      }
    };

    appState.websocket.onerror = (error) => {
      console.error("WebSocket Error:", error);
      appState.isConnecting = false;
    };
  }

  function updateWsStatus(isConnected) {
    DOM.wsIndicator.classList.toggle("bg-green-500", isConnected);
    DOM.wsIndicator.classList.toggle("bg-yellow-500", !isConnected);
    DOM.wsIndicator.classList.toggle("animate-pulse", !isConnected);
    DOM.wsText.textContent = isConnected ? "Connected" : "Disconnected";
  }

  function handleWsMessage(msg) {
    switch (msg.type) {
      case "initialConfig":
        appState.config = msg.payload;
        buildFullUI();
        break;
      case "liveStatus":
        updateLiveStatusUI(msg.payload);
        break;
      default:
        console.warn("Unknown message type:", msg.type);
    }
  }

  // --- UI Building ---
  function buildFullUI() {
    buildButtonMatrix();
    buildButtonConfigUI();
    buildLedConfigUI();
    buildMacroConfigUI();
    buildSettingsUI();
  }

  function buildButtonMatrix() {
    DOM.liveButtonMatrix.innerHTML = "";
    for (let i = 0; i < 25; i++) {
      // Assuming 5x5 matrix
      const cell = document.createElement("div");
      cell.className = "matrix-cell";
      cell.id = `live-btn-${i}`;
      DOM.liveButtonMatrix.appendChild(cell);
    }
  }

  function buildButtonConfigUI() {
    /* ... Implementation ... */
  }
  function buildLedConfigUI() {
    /* ... Implementation ... */
  }
  function buildMacroConfigUI() {
    /* ... Implementation ... */
  }
  function buildSettingsUI() {
    /* ... Implementation ... */
  }

  // --- Live UI Updates ---
  function updateLiveStatusUI(status) {
    if (typeof status.buttons !== "number") return;
    for (let i = 0; i < 25; i++) {
      const cell = document.getElementById(`live-btn-${i}`);
      if (cell) {
        cell.classList.toggle("pressed", (status.buttons >> i) & 1);
      }
    }
    DOM.liveAxisSelector.textContent = status.axis ?? "--";
    DOM.liveStepSelector.textContent = status.step ?? "--";
    DOM.liveHandwheelPos.textContent = status.handwheel ?? "0";
  }

  // --- Event Handling ---
  function setupEventListeners() {
    DOM.tabs.addEventListener("click", handleTabClick);
    DOM.saveBtn.addEventListener("click", saveConfiguration);
    DOM.updateBtn.addEventListener(
      "click",
      () => (window.location.href = "/update")
    );
    // Use event delegation for dynamic content
    DOM.tabContentContainer.addEventListener(
      "change",
      handleDynamicInputChange
    );
  }

  function handleTabClick(e) {
    if (!e.target.matches(".tab-link")) return;

    document
      .querySelectorAll(".tab-link")
      .forEach((tab) => tab.classList.remove("active"));
    document
      .querySelectorAll(".tab-content")
      .forEach((content) => content.classList.remove("active"));

    e.target.classList.add("active");
    document.getElementById(e.target.dataset.tab).classList.add("active");
  }

  function handleDynamicInputChange(e) {
    // Logic to handle changes in dynamically generated inputs
    // e.g., update appState.config in real-time
    console.log(`Input changed: ${e.target.id} = ${e.target.value}`);
  }

  // --- API & State Management ---
  async function saveConfiguration() {
    showStatus("Saving...", false);
    DOM.saveBtn.disabled = true;

    try {
      // Here you would gather the current state from the UI into a payload
      const payload = gatherConfigFromUI();

      if (
        appState.websocket &&
        appState.websocket.readyState === WebSocket.OPEN
      ) {
        appState.websocket.send(
          JSON.stringify({ command: "saveConfig", payload })
        );
        showStatus("Configuration saved successfully!", true);
      } else {
        throw new Error("WebSocket is not connected.");
      }
    } catch (error) {
      console.error("Save failed:", error);
      showStatus(`Error: ${error.message}`, false);
    } finally {
      DOM.saveBtn.disabled = false;
    }
  }

  function gatherConfigFromUI() {
    // This function would read all the values from the input fields
    // and build the configuration object to send to the ESP32.
    // This is a placeholder for the full implementation.
    console.log("Gathering configuration from UI elements...");
    return appState.config; // For now, just send back the last known config
  }

  function showStatus(message, isSuccess) {
    DOM.statusMessage.textContent = message;
    DOM.statusMessage.classList.toggle("text-green-400", isSuccess);
    DOM.statusMessage.classList.toggle("text-red-400", !isSuccess);
    setTimeout(() => (DOM.statusMessage.textContent = ""), 3000);
  }

  // --- Initializer ---
  function init() {
    setupEventListeners();
    initWebSocket();
  }

  init();
});
