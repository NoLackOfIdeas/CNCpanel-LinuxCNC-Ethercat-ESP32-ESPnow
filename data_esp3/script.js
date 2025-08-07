/**
 * @file script.js
 * @brief Client-side JavaScript for the HMI Handheld Pendant web interface.
 */
"use strict";

// --- SCHEMA VERSION FOR UI/FIRMWARE MISMATCH CHECK ---
const SCHEMA_VERSION = 1;

// --- CONSTANTS & DOM REFERENCES ---
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
  3: "Enable Handwheel",
  4: "Disable Handwheel",
};

const BINDING_TYPE_MAP = {
  0: "Unbound",
  1: "Bound to Button",
  2: "Bound to LCNC",
};

let websocket;
const wsStatusElem = document.getElementById("ws-status");

// --- ENTRY POINT ---
document.addEventListener("DOMContentLoaded", () => {
  initWebSocket();
  initUI();
});

// --- INITIALIZATION ---

function initWebSocket() {
  const wsUrl = `ws://${window.location.hostname}/ws`;
  websocket = new WebSocket(wsUrl);

  websocket.addEventListener("open", () => {
    wsStatusElem.textContent = "Connected";
    wsStatusElem.className = "status-connected";
  });

  websocket.addEventListener("close", () => {
    wsStatusElem.textContent = "Disconnected";
    wsStatusElem.className = "status-disconnected";
    setTimeout(initWebSocket, 2000);
  });

  websocket.addEventListener("message", onWsMessage);
}

function initUI() {
  // Save Configuration
  document
    .getElementById("save-config-btn")
    .addEventListener("click", saveConfiguration);

  // Firmware Update
  document.getElementById("fw-update-btn").addEventListener("click", () => {
    window.location.href = "/update";
  });

  // Tab Navigation
  document.querySelectorAll(".tab-link").forEach((btn) => {
    btn.addEventListener("click", onTabClick);
  });

  // Live Button Grid (5×5)
  createMatrixGrid("button-matrix-live", "btn-live", 5, 5);
}

// --- WEBSOCKET HANDLER ---

function onWsMessage(evt) {
  let msg;
  try {
    msg = JSON.parse(evt.data);
  } catch (e) {
    console.error("Invalid JSON from server:", e);
    return;
  }

  switch (msg.type) {
    case "initialConfig":
      // Warn if schema versions differ
      if (msg.payload.schemaVersion !== SCHEMA_VERSION) {
        alert(
          `Config schema version mismatch!\nUI: ${SCHEMA_VERSION}, Firmware: ${msg.payload.schemaVersion}`
        );
      }
      buildConfigUI(msg.payload);
      break;

    case "liveStatus":
      updateLiveStatus(msg.payload);
      break;

    default:
      console.warn("Unknown message type:", msg.type);
  }
}

// --- LIVE-STATUS RENDERING ---

function createMatrixGrid(containerId, prefix, rows, cols) {
  const container = document.getElementById(containerId);
  container.innerHTML = "";
  container.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;

  for (let r = 0; r < rows; r++) {
    for (let c = 0; c < cols; c++) {
      const cell = document.createElement("div");
      cell.className = "matrix-cell";
      cell.id = `${prefix}-${r}-${c}`;
      container.appendChild(cell);
    }
  }
}

function updateLiveStatus(status) {
  if (typeof status.buttons !== "number") return;

  for (let r = 0; r < 5; r++) {
    for (let c = 0; c < 5; c++) {
      const idx = r * 5 + c;
      const cell = document.getElementById(`btn-live-${r}-${c}`);
      if (!cell) continue;

      const pressed = ((status.buttons >> idx) & 1) === 1;
      cell.classList.toggle("button-pressed", pressed);
    }
  }
}

// --- BUILD CONFIG UIs ---

function buildConfigUI(cfg) {
  buildButtonConfig(cfg.buttons || []);
  buildLedConfig(cfg.leds || []);
  buildSelectorConfig(cfg.selectors || {});
  buildActionBindings(cfg.bindings || []);
}

function buildButtonConfig(buttons) {
  const container = document.getElementById("button-config-table");
  let html = `
    <div class="config-table-header">
      <div>#</div><div>Name</div><div>Toggle</div><div>Radio Grp</div>
    </div>`;

  buttons.forEach((btn, i) => {
    html += `
      <div class="config-table-row">
        <div>${i}</div>
        <div>
          <input type="text" id="btn-name-${i}" value="${btn.name}" />
        </div>
        <div>
          <input type="checkbox" id="btn-toggle-${i}" ${
      btn.is_toggle ? "checked" : ""
    } />
        </div>
        <div>
          <input type="number" id="btn-radio-${i}" value="${
      btn.radio_group_id
    }" min="0" />
        </div>
      </div>`;
  });

  container.innerHTML = html;
}

function buildLedConfig(leds) {
  const container = document.getElementById("led-config-table");
  let html = `
    <div class="config-table-header">
      <div>#</div><div>Name</div>
      <div>Binding Type</div><div>Button #</div><div>LCNC Bit #</div>
    </div>`;

  leds.forEach((led, i) => {
    let options = "";
    for (const [key, label] of Object.entries(BINDING_TYPE_MAP)) {
      const sel = led.binding_type == key ? "selected" : "";
      options += `<option value="${key}" ${sel}>${label}</option>`;
    }

    html += `
      <div class="config-table-row">
        <div>${i}</div>
        <div><input type="text" id="led-name-${i}" value="${led.name}" /></div>
        <div><select id="led-binding-${i}">${options}</select></div>
        <div id="led-bound-button-wrapper-${i}">
          <input type="number" id="led-bound-button-${i}" value="${led.bound_button_index}" min="-1" />
        </div>
        <div id="led-lcnc-bit-wrapper-${i}">
          <input type="number" id="led-lcnc-bit-${i}" value="${led.lcnc_state_bit}" min="-1" />
        </div>
      </div>`;
  });

  container.innerHTML = html;

  leds.forEach((_, i) => {
    const sel = document.getElementById(`led-binding-${i}`);
    sel.addEventListener("change", () => toggleLedInputs(i));
    toggleLedInputs(i);
  });
}

function buildSelectorConfig(selectors) {
  const container = document.getElementById("selector-config-table");

  // 1) Add DRO-axes count dropdown
  let html = `
    <div class="config-table-row">
      <div>Number of DRO Axes</div>
      <div>
        <select id="axes-count">
          <option value="1" ${
            selectors.num_dro_axes === 1 ? "selected" : ""
          }>1 Axis</option>
          <option value="2" ${
            selectors.num_dro_axes === 2 ? "selected" : ""
          }>2 Axes</option>
          <option value="3" ${
            selectors.num_dro_axes === 3 ? "selected" : ""
          }>3 Axes</option>
          <option value="4" ${
            selectors.num_dro_axes === 4 ? "selected" : ""
          }>4 Axes</option>
          <option value="5" ${
            selectors.num_dro_axes === 5 ? "selected" : ""
          }>5 Axes</option>
          <option value="6" ${
            selectors.num_dro_axes === 6 ? "selected" : ""
          }>6 Axes</option>
        </select>
      </div>
    </div>
    <h3>Axis Selector Names</h3>
  `;

  // 2) Existing axis-name inputs
  (selectors.axis_names || []).forEach((name, i) => {
    html += `
      <div class="config-table-row">
        <div>Position ${i + 1}</div>
        <div><input type="text" id="axis-name-${i}" value="${name}" /></div>
      </div>`;
  });

  // 3) Existing step-name inputs
  html += `<h3>Step Selector Names</h3>`;
  (selectors.step_names || []).forEach((name, i) => {
    html += `
      <div class="config-table-row">
        <div>Position ${i + 1}</div>
        <div><input type="text" id="step-name-${i}" value="${name}" /></div>
      </div>`;
  });

  container.innerHTML = html;
}

function buildActionBindings(bindings) {
  const container = document.getElementById("action-binding-table");
  let html = `
    <div class="config-table-header">
      <div>Active</div><div>IF (Machine State)</div><div>THEN (Action)</div>
    </div>`;

  bindings.forEach((b, i) => {
    let trigOpts = "";
    for (const [key, label] of Object.entries(TRIGGER_TYPE_MAP)) {
      const sel = b.trigger == key ? "selected" : "";
      trigOpts += `<option value="${key}" ${sel}>${label}</option>`;
    }

    let actOpts = "";
    for (const [key, label] of Object.entries(ACTION_TYPE_MAP)) {
      const sel = b.action == key ? "selected" : "";
      actOpts += `<option value="${key}" ${sel}>${label}</option>`;
    }

    html += `
      <div class="config-table-row">
        <div>
          <input type="checkbox" id="binding-active-${i}" ${
      b.is_active ? "checked" : ""
    } />
        </div>
        <div><select id="binding-trigger-${i}">${trigOpts}</select></div>
        <div><select id="binding-action-${i}">${actOpts}</select></div>
      </div>`;
  });

  container.innerHTML = html;

  // Confirmation dialog for Handwheel enable/disable bindings
  bindings.forEach((b, i) => {
    const actionSelect = document.getElementById(`binding-action-${i}`);
    actionSelect.dataset.prev = b.action.toString();
    actionSelect.addEventListener("change", (e) => {
      const newVal = e.target.value;
      if (newVal === "3" || newVal === "4") {
        const ok = confirm(
          "Binding Handwheel enable/disable can affect safety. Proceed?"
        );
        if (!ok) {
          e.target.value = e.target.dataset.prev;
          return;
        }
      }
      e.target.dataset.prev = e.target.value;
    });
  });
}

// --- INTERACTION LOGIC ---

function toggleLedInputs(idx) {
  const type = document.getElementById(`led-binding-${idx}`).value;
  document.getElementById(`led-bound-button-wrapper-${idx}`).style.display =
    type === "1" ? "block" : "none";
  document.getElementById(`led-lcnc-bit-wrapper-${idx}`).style.display =
    type === "2" ? "block" : "none";
}

function loadSelectorConfig() {
  const container = document.getElementById("selector-config");
  container.innerHTML = "<p>Loading selector options…</p>";

  fetch("/selectors")
    .then((res) => {
      if (!res.ok) throw new Error("Failed to load selectors");
      return res.json();
    })
    .then((data) => {
      // Build your UI; here’s an example structure:
      const html = data
        .map(
          (s) => `
        <div class="form-group">
          <label for="sel-${s.id}">${s.name}</label>
          <select id="sel-${s.id}">
            ${s.options
              .map(
                (opt) => `<option value="${opt.value}">${opt.label}</option>`
              )
              .join("")}
          </select>
        </div>
      `
        )
        .join("");
      container.innerHTML = html;
    })
    .catch((err) => {
      container.innerHTML = `<p class="error">${err.message}</p>`;
    });
}

function onTabClick(evt) {
  const target = evt.currentTarget.dataset.target;

  // hide all panels / deactivate all tabs
  document.querySelectorAll(".tab-content").forEach((panel) => {
    panel.style.display = "none";
  });
  document.querySelectorAll(".tab-link").forEach((btn) => {
    btn.classList.remove("active");
  });

  // show the one we clicked
  document.getElementById(target).style.display = "block";
  evt.currentTarget.classList.add("active");

  // *** when the Selectors panel is shown, kick off dynamic load ***
  if (target === "selectors") {
    loadSelectorConfig();
  }
}

// --- CONFIG VALIDATION & SAVE ---

function validateConfig(cfg) {
  // Buttons: radio_group_id ≥ 0
  for (let i = 0; i < cfg.buttons.length; i++) {
    const rg = cfg.buttons[i].radio_group_id;
    if (isNaN(rg) || rg < 0) {
      alert(`Button #${i} radio group must be 0 or higher.`);
      return false;
    }
  }
  // LEDs: bound_button_index ≥ -1, lcnc_state_bit ≥ -1
  for (let i = 0; i < cfg.leds.length; i++) {
    const bi = cfg.leds[i].bound_button_index;
    const lb = cfg.leds[i].lcnc_state_bit;
    if (isNaN(bi) || bi < -1) {
      alert(`LED #${i} bound button index must be -1 or higher.`);
      return false;
    }
    if (isNaN(lb) || lb < -1) {
      alert(`LED #${i} LCNC bit must be -1 or higher.`);
      return false;
    }
  }
  // Bindings: valid trigger & action codes
  for (let i = 0; i < cfg.bindings.length; i++) {
    const t = cfg.bindings[i].trigger;
    const a = cfg.bindings[i].action;
    if (!(t in TRIGGER_TYPE_MAP)) {
      alert(`Binding #${i} has invalid trigger code.`);
      return false;
    }
    if (!(a in ACTION_TYPE_MAP)) {
      alert(`Binding #${i} has invalid action code.`);
      return false;
    }
  }
  return true;
}

function saveConfiguration() {
  const cfg = {
    schemaVersion: SCHEMA_VERSION,
    buttons: [],
    leds: [],
    selectors: { axis_names: [], step_names: [] },
    bindings: [],
  };

  // Buttons
  for (let i = 0; ; i++) {
    const nameEl = document.getElementById(`btn-name-${i}`);
    if (!nameEl) break;
    cfg.buttons.push({
      name: nameEl.value,
      is_toggle: document.getElementById(`btn-toggle-${i}`).checked,
      radio_group_id: parseInt(
        document.getElementById(`btn-radio-${i}`).value,
        10
      ),
    });
  }

  // LEDs
  for (let i = 0; ; i++) {
    const nameEl = document.getElementById(`led-name-${i}`);
    if (!nameEl) break;
    cfg.leds.push({
      name: nameEl.value,
      binding_type: parseInt(
        document.getElementById(`led-binding-${i}`).value,
        10
      ),
      bound_button_index: parseInt(
        document.getElementById(`led-bound-button-${i}`).value,
        10
      ),
      lcnc_state_bit: parseInt(
        document.getElementById(`led-lcnc-bit-${i}`).value,
        10
      ),
    });
  }

  // Selectors: axis & step names (unchanged)…
  for (let i = 0; ; i++) {
    const el = document.getElementById(`axis-name-${i}`);
    if (!el) break;
    cfg.selectors.axis_names.push(el.value);
  }
  for (let i = 0; ; i++) {
    const el = document.getElementById(`step-name-${i}`);
    if (!el) break;
    cfg.selectors.step_names.push(el.value);
  }

  // 4) Capture the new DRO-axes count
  const axesEl = document.getElementById("axes-count");
  if (axesEl) {
    cfg.selectors.num_dro_axes = parseInt(axesEl.value, 10);
  }

  // Bindings
  for (let i = 0; ; i++) {
    const activeEl = document.getElementById(`binding-active-${i}`);
    if (!activeEl) break;
    cfg.bindings.push({
      is_active: activeEl.checked,
      trigger: parseInt(
        document.getElementById(`binding-trigger-${i}`).value,
        10
      ),
      action: parseInt(
        document.getElementById(`binding-action-${i}`).value,
        10
      ),
    });
  }

  // Validate and send
  if (!validateConfig(cfg)) return;
  websocket.send(JSON.stringify({ command: "saveConfig", payload: cfg }));
  alert("Pendant configuration sent!");
}
