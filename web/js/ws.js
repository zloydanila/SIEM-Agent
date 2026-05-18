let ws;
let wsConnected = false;

function connectWebSocket() {
    const protocol = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`${protocol}://${location.hostname}:8080`);
    ws.onopen = () => {
        wsConnected = true;
        updateConnectionStatus();
    };
    ws.onclose = () => {
        wsConnected = false;
        updateConnectionStatus();
    };
    ws.onmessage = (event) => {
        const msg = JSON.parse(event.data);
        handleWsMessage(msg);
    };
}

function handleWsMessage(msg) {
    if (msg.eventType) {
        if (typeof eventsPage !== 'undefined' && eventsPage.autoRefresh) {
            eventsPage.prependEvent(msg);
        }
        if (typeof dashboardPage !== 'undefined') {
            dashboardPage.updateStats();
        }
    }
    if (msg.type === 'alert_update') {
        if (typeof alertsPage !== 'undefined') alertsPage.refresh();
        if (typeof dashboardPage !== 'undefined') dashboardPage.updateStats();
    }
    if (msg.type === 'stats') {
        if (typeof dashboardPage !== 'undefined') dashboardPage.updateStats();
    }
    if (msg.type === 'user_update' && typeof settingsPage !== 'undefined') {
        settingsPage.refreshUsers();
    }
}

function updateConnectionStatus() {
    const el = document.getElementById('ws-status');
    if (el) {
        el.textContent = wsConnected ? 'Работает' : 'Остановлен';
        el.style.color = wsConnected ? 'var(--success)' : 'var(--danger)';
    }
}