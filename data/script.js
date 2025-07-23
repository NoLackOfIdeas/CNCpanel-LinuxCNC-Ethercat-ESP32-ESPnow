let websocket;
const wsStatusElem = document.getElementById('ws-status');

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initUI();
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    const gateway = `ws://${window.location.hostname}/ws`;
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    wsStatusElem.textContent = 'Verbunden';
    wsStatusElem.className = 'status-connected';
    websocket.send(JSON.stringify({ command: 'getInitialState' }));
}

function onClose(event) {
    console.log('Connection closed');
    wsStatusElem.textContent = 'Getrennt';
    wsStatusElem.className = 'status-disconnected';
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log('Received:', event.data);
    const data = JSON.parse(event.data);

    if (data.type === 'liveStatus') {
        updateLiveStatus(data.payload);
    } else if (data.type === 'initialState') {
        buildConfigUI(data.payload.config);
        updateLiveStatus(data.payload.status);
    }
}

function initUI() {
    document.getElementById('save-config-btn').addEventListener('click', saveConfiguration);
    createMatrixGrid('button-matrix-live', 'btn-live');
    createMatrixGrid('led-matrix-live', 'led-live');
}

function createMatrixGrid(containerId, prefix) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    for (let r = 0; r < 8; r++) {
        for (let c = 0; c < 8; c++) {
            const cell = document.createElement('div');
            cell.className = 'matrix-cell';
            cell.id = `${prefix}-${r}-${c}`;
            container.appendChild(cell);
        }
    }
}

function updateLiveStatus(status) {
    // Update button matrix
    for (let i = 0; i < 8; i++) {
        for (let j = 0; j < 8; j++) {
            const cell = document.getElementById(`btn-live-${i}-${j}`);
            if (cell) {
                const buttonIndex = i * 8 + j;
                if ((BigInt(status.buttons) >> BigInt(buttonIndex)) & 1n) {
                    cell.classList.add('button-pressed');
                } else {
                    cell.classList.remove('button-pressed');
                }
            }
        }
    }
    // Update LED matrix
    for (let i = 0; i < 8; i++) {
        for (let j = 0; j < 8; j++) {
            const cell = document.getElementById(`led-live-${i}-${j}`);
            if (cell) {
                const ledIndex = i * 8 + j;
                if ((BigInt(status.leds) >> BigInt(ledIndex)) & 1n) {
                    cell.classList.add('led-on');
                } else {
                    cell.classList.remove('led-on');
                }
            }
        }
    }
}

function buildConfigUI(config) {
    // Build button config table
    const btnContainer = document.getElementById('button-config-table');
    btnContainer.innerHTML = `
        <div class="config-table-header">
            <div>#</div><div>Name</div><div>Toggle</div><div>Radio Grp</div><div>Position</div>
        </div>
    `;
    config.buttons.forEach((btn, index) => {
        const row = document.createElement('div');
        row.className = 'config-table-row';
        const r = Math.floor(index / 8);
        const c = index % 8;
        row.innerHTML = `
            <div>${index}</div>
            <div><input type="text" id="btn-name-${index}" value="${btn.name}" title="Button Name"></div>
            <div><input type="checkbox" id="btn-toggle-${index}" ${btn.is_toggle? 'checked' : ''} title="Toggle Mode"></div>
            <div><input type="number" id="btn-radio-${index}" value="${btn.radio_group_id}" min="0" title="Radio Group ID (0 for none)"></div>
            <div><span title="Row: ${r}, Col: ${c}">Pos: ${r},${c}</span></div>
        `;
        btnContainer.appendChild(row);
    });
    // Build LED config table similarly...
}

function saveConfiguration() {
    const config = {
        buttons:,
        leds:
    };
    // Read button config from UI
    for (let i = 0; i < 64; i++) { // Assuming MAX_BUTTONS
        const nameEl = document.getElementById(`btn-name-${i}`);
        if (!nameEl) break;
        config.buttons.push({
            name: nameEl.value,
            is_toggle: document.getElementById(`btn-toggle-${i}`).checked,
            radio_group_id: parseInt(document.getElementById(`btn-radio-${i}`).value)
        });
    }
    // Read LED config from UI...

    console.log('Sending config to save:', config);
    websocket.send(JSON.stringify({ command: 'saveConfig', payload: config }));
    alert('Konfiguration gesendet!');
}

function openTab(evt, tabName) {
    let i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tab-content");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }
    tablinks = document.getElementsByClassName("tab-link");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}