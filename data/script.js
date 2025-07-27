/**
 * @file script.js
 * @brief Client-side JavaScript for the HMI web interface.
 *
 * This script handles:
 * - Establishing and maintaining a WebSocket connection to the ESP32.
 * - Receiving live status updates and rendering them.
 * - Receiving the initial configuration and building the UI.
 * - Reading user input from the config forms and sending it back to the ESP32.
 * - Managing the tabbed interface and dynamic form elements.
 */

"use strict";

// --- GLOBAL VARIABLES & CONSTANTS ---
let websocket;
const wsStatusElem = document.getElementById("ws-status");

// Dictionaries to map enum values from the firmware to human-readable strings
const TRIGGER_TYPE_MAP = {
  0: "Machine is ON",
  1: "In AUTO Mode",
  2: "In MDI Mode",
  3: "In JOG Mode",
  4: "Program is Running",
  5: "Program is Paused",
  6: "On Home Position",
  16: "Spindle is ON",
  17: "Spindle at Speed",
  18: "Mist Coolant is ON",
  19: "Flood Coolant is ON",
};
const ACTION_TYPE_MAP = {
  0: "No Action",
  1: "Enable Joystick 1",
  2: "Disable Joystick 1",
};
const BINDING_TYPE_MAP = {
  0: "Unbound",
  1: "Bound to Button",
  2: "Bound to LCNC",
};

// --- INITIALIZATION ---
window.addEventListener("load", onLoad);

/**
 * @brief Main function called when the page is fully loaded.
 */
function onLoad(event) {
  initWebSocket();
  initUI();
}

/**
 * @brief Initializes the WebSocket connection and sets up event handlers.
 */
function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  const gateway = `ws://${window.location.hostname}/ws`;
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

/**
 * @brief Sets up static UI event listeners.
 */
function initUI() {
  document
    .getElementById("save-config-btn")
    .addEventListener("click", saveConfiguration);
  createMatrixGrid("button-matrix-live", "btn-live");
  createMatrixGrid("led-matrix-live", "led-live");
}

// --- WEBSOCKET EVENT HANDLERS ---

/**
 * @brief Called when the WebSocket connection is successfully established.
 */
function onOpen(event) {
  console.log("Connection opened");
  wsStatusElem.textContent = "Connected";
  wsStatusElem.className = "status-connected";
}

/**
 * @brief Called when the WebSocket connection is closed.
 * Attempts to reconnect after a delay.
 */
function onClose(event) {
  console.log("Connection closed");
  wsStatusElem.textContent = "Disconnected";
  wsStatusElem.className = "status-disconnected";
  setTimeout(initWebSocket, 2000); // Attempt to reconnect every 2 seconds
}

/**
 * @brief Called when a message is received from the ESP32 via WebSocket.
 * Distinguishes between an initial configuration message and a live status update.
 * @param {MessageEvent} event The message event from the server.
 */
function onMessage(event) {
  const data = JSON.parse(event.data);

  if (data.type === "initialConfig") {
    console.log("Received initial configuration.");
    buildConfigUI(data.payload);
  } else if (data.type === "liveStatus") {
    updateLiveStatus(data.payload);
  }
}

// --- UI RENDERING FUNCTIONS ---

/**
 * @brief Updates the live status grids (buttons and LEDs) based on a status payload.
 * @param {object} status The payload containing button and LED state arrays.
 */
function updateLiveStatus(status) {
  if (status.buttons) {
    for (let r = 0; r < 8; r++) {
      for (let c = 0; c < 8; c++) {
        const cell = document.getElementById(`btn-live-${r}-${c}`);
        if (cell) {
          if ((status.buttons[r] >> c) & 1) {
            cell.classList.add("button-pressed");
          } else {
            cell.classList.remove("button-pressed");
          }
        }
      }
    }
  }
  if (status.leds) {
    for (let r = 0; r < 8; r++) {
      for (let c = 0; c < 8; c++) {
        const cell = document.getElementById(`led-live-${r}-${c}`);
        if (cell) {
          if ((status.leds[r] >> c) & 1) {
            cell.classList.add("led-on");
          } else {
            cell.classList.remove("led-on");
          }
        }
      }
    }
  }
}

/**
 * @brief Creates an 8x8 grid of divs for the live status displays.
 * @param {string} containerId The ID of the parent element.
 * @param {string} prefix A prefix for the individual cell IDs.
 */
function createMatrixGrid(containerId, prefix) {
  const container = document.getElementById(containerId);
  if (!container) return;
  container.innerHTML = ""; // Clear previous content
  for (let r = 0; r < 8; r++) {
    for (let c = 0; c < 8; c++) {
      const cell = document.createElement("div");
      cell.className = "matrix-cell";
      cell.id = `${prefix}-${r}-${c}`;
      container.appendChild(cell);
    }
  }
}

/**
 * @brief Populates all configuration forms based on the config object from the ESP32.
 * @param {object} config The full configuration object.
 */
function buildConfigUI(config) {
  if (!config) return;

  // --- Build Button Config Table ---
  if (config.buttons) {
    const container = document.getElementById("button-config-table");
    let html = `<div class="config-table-header"><div>#</div><div>Name</div><div>Toggle</div><div>Radio Grp</div></div>`;
    config.buttons.forEach((btn, index) => {
      html += `<div class="config-table-row">
                <div>${index}</div>
                <div><input type="text" id="btn-name-${index}" value="${
        btn.name
      }"></div>
                <div><input type="checkbox" id="btn-toggle-${index}" ${
        btn.is_toggle ? "checked" : ""
      }></div>
                <div><input type="number" id="btn-radio-${index}" value="${
        btn.radio_group_id
      }" min="0"></div>
            </div>`;
    });
    container.innerHTML = html;
  }

  // --- Build LED Config Table ---
  if (config.leds) {
    const container = document.getElementById("led-config-table");
    let html = `<div class="config-table-header"><div>#</div><div>Name</div><div>Binding Type</div><div>Bound To Button #</div><div>LCNC State Bit #</div></div>`;
    config.leds.forEach((led, index) => {
      let bindingOptions = "";
      for (const [key, value] of Object.entries(BINDING_TYPE_MAP)) {
        bindingOptions += `<option value="${key}" ${
          led.binding_type == key ? "selected" : ""
        }>${value}</option>`;
      }
      html += `<div class="config-table-row">
                <div>${index}</div>
                <div><input type="text" id="led-name-${index}" value="${led.name}"></div>
                <div><select id="led-binding-${index}" onchange="toggleLedInputs(${index})">${bindingOptions}</select></div>
                <div id="led-bound-button-wrapper-${index}"><input type="number" id="led-bound-button-${index}" value="${led.bound_button_index}" min="-1"></div>
                <div id="led-lcnc-bit-wrapper-${index}"><input type="number" id="led-lcnc-bit-${index}" value="${led.lcnc_state_bit}" min="-1"></div>
            </div>`;
    });
    container.innerHTML = html;
    config.leds.forEach((led, index) => toggleLedInputs(index));
  }

  // --- Build Joystick Config Table ---
  if (config.joysticks) {
    const container = document.getElementById("joystick-config-table");
    let html = ``;
    config.joysticks.forEach((joy, index) => {
      html += `<h3>${joy.name}</h3>
                <div class="config-table-header"><div>Axis</div><div>Invert</div><div>Sensitivity</div><div>Deadzone</div></div>`;
      joy.axes.forEach((axis, axis_idx) => {
        const axis_name = ["X", "Y", "Z"][axis_idx];
        html += `<div class="config-table-row">
                    <div><b>${axis_name}</b></div>
                    <div><input type="checkbox" id="joy-${index}-axis-${axis_idx}-inverted" ${
          axis.is_inverted ? "checked" : ""
        }></div>
                    <div><input type="number" step="0.1" id="joy-${index}-axis-${axis_idx}-sensitivity" value="${
          axis.sensitivity
        }"></div>
                    <div><input type="number" id="joy-${index}-axis-${axis_idx}-deadzone" value="${
          axis.center_deadzone
        }"></div>
                </div>`;
      });
    });
    container.innerHTML = html;
  }

  // --- Build Action Binding Table ---
  if (config.bindings) {
    const container = document.getElementById("action-binding-table");
    let html = `<div class="config-table-header"><div>Active</div><div>IF (Machine State Is...)</div><div>THEN (Perform Action...)</div></div>`;
    config.bindings.forEach((binding, index) => {
      let triggerOptions = "";
      for (const [key, value] of Object.entries(TRIGGER_TYPE_MAP)) {
        triggerOptions += `<option value="${key}" ${
          binding.trigger == key ? "selected" : ""
        }>${value}</option>`;
      }
      let actionOptions = "";
      for (const [key, value] of Object.entries(ACTION_TYPE_MAP)) {
        actionOptions += `<option value="${key}" ${
          binding.action == key ? "selected" : ""
        }>${value}</option>`;
      }
      html += `<div class="config-table-row">
                <div><input type="checkbox" id="binding-active-${index}" ${
        binding.is_active ? "checked" : ""
      }></div>
                <div><select id="binding-trigger-${index}">${triggerOptions}</select></div>
                <div><select id="binding-action-${index}">${actionOptions}</select></div>
            </div>`;
    });
    container.innerHTML = html;
  }
}

/**
 * @brief Shows or hides conditional input fields for the LED configuration based on the selected binding type.
 * @param {number} index The row index of the LED.
 */
function toggleLedInputs(index) {
  const bindingType = document.getElementById(`led-binding-${index}`).value;
  const buttonWrapper = document.getElementById(
    `led-bound-button-wrapper-${index}`
  );
  const lcncWrapper = document.getElementById(`led-lcnc-bit-wrapper-${index}`);

  buttonWrapper.style.display = bindingType === "1" ? "block" : "none";
  lcncWrapper.style.display = bindingType === "2" ? "block" : "none";
}

// --- EVENT HANDLERS & LOGIC ---

/**
 * @brief Reads all values from the configuration forms, builds a JSON object,
 * and sends it to the ESP32 to be saved.
 */
function saveConfiguration() {
  const config = { buttons: [], leds: [], joysticks: [], bindings: [] };

  // --- Read button config from UI ---
  for (let i = 0; document.getElementById(`btn-name-${i}`); i++) {
    config.buttons.push({
      name: document.getElementById(`btn-name-${i}`).value,
      is_toggle: document.getElementById(`btn-toggle-${i}`).checked,
      radio_group_id: parseInt(document.getElementById(`btn-radio-${i}`).value),
    });
  }

  // --- Read LED config from UI ---
  for (let i = 0; document.getElementById(`led-name-${i}`); i++) {
    config.leds.push({
      name: document.getElementById(`led-name-${i}`).value,
      binding_type: parseInt(document.getElementById(`led-binding-${i}`).value),
      bound_button_index: parseInt(
        document.getElementById(`led-bound-button-${i}`).value
      ),
      lcnc_state_bit: parseInt(
        document.getElementById(`led-lcnc-bit-${i}`).value
      ),
    });
  }

  // --- Read Joystick config from UI ---
  for (let i = 0; document.getElementById(`joy-${i}-axis-0-sensitivity`); i++) {
    const joy_data = { axes: [] };
    for (let j = 0; j < 3; j++) {
      // Assuming 3 axes
      joy_data.axes.push({
        is_inverted: document.getElementById(`joy-${i}-axis-${j}-inverted`)
          .checked,
        sensitivity: parseFloat(
          document.getElementById(`joy-${i}-axis-${j}-sensitivity`).value
        ),
        center_deadzone: parseInt(
          document.getElementById(`joy-${i}-axis-${j}-deadzone`).value
        ),
      });
    }
    config.joysticks.push(joy_data);
  }

  // --- Read Action Bindings config from UI ---
  for (let i = 0; document.getElementById(`binding-active-${i}`); i++) {
    config.bindings.push({
      is_active: document.getElementById(`binding-active-${i}`).checked,
      trigger: parseInt(document.getElementById(`binding-trigger-${i}`).value),
      action: parseInt(document.getElementById(`binding-action-${i}`).value),
    });
  }

  console.log("Sending config to save:", config);
  websocket.send(JSON.stringify({ command: "saveConfig", payload: config }));
  alert("Configuration sent to HMI!");
}

/**
 * @brief Handles the logic for switching between configuration tabs.
 * @param {Event} evt The click event.
 * @param {string} tabName The ID of the tab content to display.
 */
function openTab(evt, tabName) {
  const tabcontent = document.getElementsByClassName("tab-content");
  for (let i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }
  const tablinks = document.getElementsByClassName("tab-link");
  for (let i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}
