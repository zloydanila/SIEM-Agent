let currentUser = null;
let currentPage = 'login';
let dashboardPage, eventsPage, alertsPage, settingsPage;

document.addEventListener('DOMContentLoaded', () => {
    if (authToken) {
        try {
            const payload = JSON.parse(atob(authToken.split('.')[0]));
            currentUser = payload;
            showMain();
        } catch (e) { logout(); }
    } else {
        showLogin();
    }
    setupMenu();
});

function showLogin() {
    document.getElementById('content').innerHTML = `
        <div class="login-form card" style="max-width:400px; margin:100px auto;">
            <h2>Вход в систему</h2>
            <input id="username" class="input" placeholder="Имя пользователя">
            <input id="password" class="input" type="password" placeholder="Пароль">
            <div id="error" style="color:var(--danger); margin-bottom:12px;"></div>
            <button id="login-btn" class="btn-primary">Войти</button>
        </div>`;
    document.getElementById('login-btn').onclick = async () => {
        const u = document.getElementById('username').value;
        const p = document.getElementById('password').value;
        try {
            const res = await api.login(u, p);
            authToken = res.token;
            localStorage.setItem('authToken', authToken);
            currentUser = res.user;
            showMain();
        } catch (e) { document.getElementById('error').innerText = 'Ошибка входа'; }
    };
    document.getElementById('sidebar').style.display = 'none';
}

function showMain() {
    document.getElementById('user-name').innerText = currentUser.fullName || currentUser.username;
    document.getElementById('user-avatar').innerText = (currentUser.fullName || currentUser.username).charAt(0).toUpperCase();
    document.getElementById('sidebar').style.display = 'flex';
    currentPage = 'dashboard';
    renderPage();
    connectWebSocket();
}

function setupMenu() {
    document.querySelectorAll('.menu-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            document.querySelectorAll('.menu-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            currentPage = btn.dataset.page;
            renderPage();
        });
    });
    document.getElementById('logout-btn').onclick = logout;
}

function renderPage() {
    const container = document.getElementById('content');
    switch (currentPage) {
        case 'dashboard':
            if (!dashboardPage) dashboardPage = new DashboardPage(container);
            else dashboardPage.render(container);
            break;
        case 'events':
            if (!eventsPage) eventsPage = new EventsPage(container);
            else eventsPage.render(container);
            break;
        case 'alerts':
            if (!alertsPage) alertsPage = new AlertsPage(container);
            else alertsPage.render(container);
            break;
        case 'settings':
            if (!settingsPage) settingsPage = new SettingsPage(container);
            else settingsPage.render(container);
            break;
    }
}

function logout() {
    authToken = '';
    localStorage.removeItem('authToken');
    currentUser = null;
    currentPage = 'login';
    document.getElementById('sidebar').style.display = 'none';
    dashboardPage = eventsPage = alertsPage = settingsPage = null;
    showLogin();
}

// ===================== Страница Дашборд =====================
class DashboardPage {
    constructor(container) { this.container = container; this.render(); }
    async render() {
        this.container.innerHTML = `<h2>Дашборд</h2><div id="dash-content">Загрузка...</div>`;
        await this.updateStats();
    }
    async updateStats() {
        try {
            const data = await api.getDashboard();
            const div = document.getElementById('dash-content');
            div.innerHTML = `
                <div class="stats-grid">
                    <div class="card">Всего событий: <b>${data.totalEvents}</b></div>
                    <div class="card">Всего алертов: <b>${data.totalAlerts}</b></div>
                    <div class="card">Открытых: <b>${data.openAlerts}</b></div>
                    <div class="card">Критичных: <b style="color:var(--danger)">${data.criticalCount}</b></div>
                    <div class="card">Высоких: <b style="color:var(--warning)">${data.highCount}</b></div>
                    <div class="card">Средних: <b style="color:var(--accent)">${data.mediumCount}</b></div>
                    <div class="card">Низких: <b style="color:var(--success)">${data.lowCount}</b></div>
                </div>
                <h3>Топ устройств</h3>
                <ul>${(data.topDevices || []).map(d => `<li>${d.name}: ${d.count}</li>`).join('')}</ul>
            `;
        } catch (e) { console.error(e); }
    }
}

// ===================== Страница События =====================
class EventsPage {
    constructor(container) { this.container = container; this.autoRefresh = true; this.render(); }
    async render() {
        this.container.innerHTML = `<h2>События</h2><div id="events-list">Загрузка...</div>`;
        await this.loadEvents();
    }
    async loadEvents() {
        try {
            const data = await api.getEvents();
            const list = document.getElementById('events-list');
            list.innerHTML = (data.events || []).map(e => `
                <div class="card event-card">
                    <div><b>${e.deviceName}</b> — ${e.action}</div>
                    <div>${e.severity} | ${e.timestamp}</div>
                    <pre>${e.rawLog}</pre>
                </div>`).join('');
        } catch (e) { console.error(e); }
    }
    prependEvent(event) {
        const list = document.getElementById('events-list');
        if (!list) return;
        const card = document.createElement('div');
        card.className = 'card event-card';
        card.innerHTML = `
            <div><b>${event.deviceName}</b> — ${event.action}</div>
            <div>${event.severity} | ${event.timestamp}</div>
            <pre>${event.rawLog}</pre>`;
        list.prepend(card);
    }
}

// ===================== Страница Алерты =====================
class AlertsPage {
    constructor(container) { this.container = container; this.render(); }
    async render() {
        this.container.innerHTML = `<h2>Алерты</h2><div id="alerts-list">Загрузка...</div>`;
        await this.refresh();
    }
    async refresh() {
        try {
            const data = await api.getAlerts();
            const list = document.getElementById('alerts-list');
            list.innerHTML = (data.alerts || []).map(a => `
                <div class="card alert-card">
                    <div><b>${a.title}</b> (${a.severity})</div>
                    <div>Статус: ${a.status}</div>
                    <div>${a.description}</div>
                    <div>${a.deviceName} | ${a.triggeredAt}</div>
                    ${currentUser.role !== 'viewer' ? `
                        <select data-id="${a.id}" class="status-select">
                            <option ${a.status === 'open' ? 'selected' : ''} value="open">Открыт</option>
                            <option ${a.status === 'investigating' ? 'selected' : ''} value="investigating">Изучается</option>
                            <option ${a.status === 'closed' ? 'selected' : ''} value="closed">Закрыт</option>
                        </select>` : ''}
                </div>`).join('');
            document.querySelectorAll('.status-select').forEach(sel => {
                sel.onchange = async (e) => {
                    await api.updateAlertStatus(sel.dataset.id, sel.value);
                    this.refresh();
                };
            });
        } catch (e) { console.error(e); }
    }
}

// ===================== Страница Настройки =====================
class SettingsPage {
    constructor(container) { this.container = container; this.render(); }
    async render() {
        this.container.innerHTML = `
            <h2>Настройки</h2>
            <div class="card">
                <h3>WebSocket</h3>
                <div>Статус: <span id="ws-status">${wsConnected ? 'Работает' : 'Остановлен'}</span></div>
                <div>Порт: 8080</div>
            </div>
            <div class="card">
                <h3>Смена пароля</h3>
                <input id="old-password" class="input" type="password" placeholder="Текущий пароль">
                <input id="new-password" class="input" type="password" placeholder="Новый пароль">
                <button id="change-pw-btn" class="btn-primary">Сменить пароль</button>
            </div>
            <div class="card" id="users-card">
                <h3>Пользователи</h3>
                <div id="users-list"></div>
                <button id="add-user-btn" class="btn-primary" style="margin-top:10px;">+ Добавить</button>
            </div>
            <div class="card">
                <h3>Правила корреляции</h3>
                <div id="rules-list"></div>
                <button id="add-rule-btn" class="btn-primary">+ Добавить правило</button>
            </div>
            <div class="card">
                <h3>Экспорт</h3>
                <button class="btn-primary" id="export-events">События CSV</button>
                <button class="btn-primary" id="export-alerts">Алерты CSV</button>
                <button class="btn-primary" id="export-report">Отчёт JSON</button>
            </div>
        `;

        document.getElementById('change-pw-btn').onclick = async () => {
            const oldPw = document.getElementById('old-password').value;
            const newPw = document.getElementById('new-password').value;
            try {
                await api.changePassword(oldPw, newPw);
                alert('Пароль изменён');
            } catch (e) { alert(e.message); }
        };

        if (currentUser.role === 'admin') {
            document.getElementById('add-user-btn').style.display = 'block';
            document.getElementById('add-rule-btn').style.display = 'block';
            this.refreshUsers();
            this.refreshRules();
        } else {
            document.getElementById('add-user-btn').style.display = 'none';
            document.getElementById('add-rule-btn').style.display = 'none';
            document.getElementById('users-card').innerHTML += '<p>Только администратор управляет пользователями</p>';
        }

        document.getElementById('add-user-btn').onclick = () => {
            const name = prompt('Имя пользователя:');
            const pass = prompt('Пароль:');
            const role = prompt('Роль (admin/operator/viewer):', 'viewer');
            if (name && pass && role) {
                api.createUser({ username: name, password: pass, role, fullName: name, email: '' })
                   .then(() => this.refreshUsers());
            }
        };

        document.getElementById('add-rule-btn').onclick = () => {
            const rule = {
                name: prompt('Название:'),
                ruleType: prompt('Тип (threshold/correlation):', 'threshold'),
                matchEventType: prompt('Тип события:'),
                threshold: parseInt(prompt('Порог:', '1')),
                windowSeconds: parseInt(prompt('Окно (сек):', '60')),
                cooldownSeconds: parseInt(prompt('Кулдаун (сек):', '60')),
                alertSeverity: prompt('Серьёзность (low/medium/high/critical):', 'high'),
                alertTitle: prompt('Заголовок алерта:'),
                alertDescription: prompt('Описание:', '')
            };
            if (rule.name && rule.matchEventType) {
                api.createRule(rule).then(() => this.refreshRules());
            }
        };

        document.getElementById('export-events').onclick = () => api.exportEventsCsv().then(d => alert(`Файл: ${d.file}`));
        document.getElementById('export-alerts').onclick = () => api.exportAlertsCsv().then(d => alert(`Файл: ${d.file}`));
        document.getElementById('export-report').onclick = () => api.exportReportJson().then(d => alert(`Файл: ${d.file}`));
    }

    async refreshUsers() {
        const data = await api.getUsers();
        const list = document.getElementById('users-list');
        list.innerHTML = (data.users || []).map(u => `
            <div class="user-row">
                ${u.username} (${u.role}) [${u.isActive ? 'Active' : 'Inactive'}]
                ${currentUser.role === 'admin' ? `<button data-id="${u.id}" class="btn-danger del-user">Удалить</button>` : ''}
            </div>`).join('');
        document.querySelectorAll('.del-user').forEach(btn => {
            btn.onclick = async () => {
                await api.deleteUser(btn.dataset.id);
                this.refreshUsers();
            };
        });
    }

    async refreshRules() {
        const data = await api.getRules();
        const list = document.getElementById('rules-list');
        list.innerHTML = (data.rules || []).map(r => `
            <div class="rule-row">
                <span>${r.name} (${r.ruleType})</span>
                <span>${r.matchEventType} > порог ${r.threshold}</span>
                <span style="color:${r.isEnabled ? 'var(--success)' : 'var(--danger)'}">${r.isEnabled ? 'Вкл' : 'Выкл'}</span>
                <button data-id="${r.id}" class="btn-secondary toggle-rule">Переключить</button>
                <button data-id="${r.id}" class="btn-danger del-rule">Удалить</button>
            </div>`).join('');
        document.querySelectorAll('.toggle-rule').forEach(btn => {
            btn.onclick = async () => {
                const id = btn.dataset.id;
                const rule = data.rules.find(r => r.id === id);
                await api.toggleRule(id, !rule.isEnabled);
                this.refreshRules();
            };
        });
        document.querySelectorAll('.del-rule').forEach(btn => {
            btn.onclick = async () => {
                await api.deleteRule(btn.dataset.id);
                this.refreshRules();
            };
        });
    }
}