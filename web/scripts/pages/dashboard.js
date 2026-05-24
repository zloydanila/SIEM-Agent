import { showToast } from "../components/toast.js";
import { createUser, updateUser, deleteUser } from "../api.js";
import {
    createModal,
    openModal,
    closeModal,
    setModalBody,
    setModalFooter,
    ensureModalHost,
    openConfirmModal
} from "../components/modal.js";

const KPI_COLORS = {
    accent: "var(--accent)",
    danger: "var(--danger)",
    warning: "var(--warning)",
    success: "var(--success)"
};

export function renderDashboard({
    stats = {},
    activity = [],
    topDevices = [],
    users = [],
    currentUser = {},
    currentUserRole = "",
    wsConnected = false,
    canManage = false,
    isAdmin = false,
    isViewer = false,
    onRefresh,
    onLogout,
    onCreateUser
} = {}) {
    const root = document.createElement("div");
    root.className = "page-inner dashboard-page";

    const safeActivity = Array.isArray(activity) ? activity : [];
    const safeTopDevices = Array.isArray(topDevices) ? topDevices : [];
    const safeUsers = Array.isArray(users) ? users : [];
    const role = String(currentUserRole || currentUser.role || "").toLowerCase();

    root.innerHTML = `
        <div class="dashboard-shell">
            <header class="dashboard-page-header">
                <div class="dashboard-header-left">
                    <div class="dashboard-brand-icon">S</div>
                    <div>
                        <div class="dashboard-title">SIEM Dashboard</div>
                        <div class="dashboard-subtitle">${esc(currentUser.fullName || currentUser.username || "")}  |  ${esc(role.toUpperCase())}</div>
                    </div>
                </div>

                <div class="dashboard-header-center">
                    <span class="ws-status-dot ${wsConnected ? "online" : "offline"}"></span>
                    <span class="ws-status-label ${wsConnected ? "online" : "offline"}">${wsConnected ? "Online" : "Offline"}</span>
                </div>

                <div class="dashboard-header-right">
                    ${!canManage && isViewer ? `<span class="viewer-only-badge">Только просмотр</span>` : ""}
                    ${isAdmin ? `<button type="button" class="btn success" id="dashAddUserBtn">+ Add User</button>` : ""}
                    <button type="button" class="btn ghost" id="dashboardRefreshBtn">Refresh</button>
                    <button type="button" class="btn ghost dash-logout-btn" id="dashLogoutBtn">Logout</button>
                </div>
            </header>

            <div class="dashboard-kpi-row">
                ${[
                    ["Всего событий", stats.totalEvents ?? 0, "accent"],
                    ["Открытых алертов", stats.openAlerts ?? 0, "danger"],
                    ["Критических", stats.criticalCount ?? 0, "danger"],
                    ["Высоких", stats.highCount ?? 0, "warning"],
                    ["Средних", stats.mediumCount ?? 0, "accent"],
                    ["Низких", stats.lowCount ?? 0, "success"]
                ].map(([label, value, color]) => `
                    <div class="dashboard-kpi">
                        <div class="kpi-accent-bar" style="background:${KPI_COLORS[color] || KPI_COLORS.accent}"></div>
                        <div class="kpi-label">${label}</div>
                        <div class="kpi-value" style="color:${KPI_COLORS[color] || KPI_COLORS.accent}">${Number(value) || 0}</div>
                    </div>
                `).join("")}
            </div>

            <div class="section-shell charts-row">
                <section class="card">
                    <div class="card-header"><h3>Активность событий (последние 7 часов)</h3></div>
                    <div class="card-body">
                        <div class="db-bar-chart" id="activityChart"></div>
                    </div>
                </section>

                <section class="card threat-card">
                    <div class="card-header"><h3>По уровню угрозы</h3></div>
                    <div class="card-body severity-wrap">
                        <canvas id="donutCanvas" width="110" height="110"></canvas>
                        <div class="severity-legend">
                            ${[
                                ["Критично", stats.criticalCount ?? 0, "#f85149"],
                                ["Высокий", stats.highCount ?? 0, "#d29922"],
                                ["Средний", stats.mediumCount ?? 0, "#58a6ff"],
                                ["Низкий", stats.lowCount ?? 0, "#3fb950"]
                            ].map(([label, val, color]) => `
                                <div class="legend-row">
                                    <span class="legend-dot" style="background:${color}"></span>
                                    <span class="legend-label">${label}</span>
                                    <span class="legend-value">${Number(val) || 0}</span>
                                </div>
                            `).join("")}
                        </div>
                    </div>
                </section>
            </div>

            <section class="card dashboard-top-devices">
                <div class="card-header"><h3>Топ устройств по событиям</h3></div>
                <div class="card-body content-gap">
                    ${safeTopDevices.length
                        ? safeTopDevices.slice(0, 5).map((dev) => `
                            <div class="top-device-block">
                                <div class="device-row">
                                    <span class="device-name">${esc(dev.name ?? dev.deviceName ?? "")}</span>
                                    <span class="device-count">${Number(dev.count ?? 0)}</span>
                                </div>
                                <div class="progress-track">
                                    <div class="progress-fill" style="width:${topDeviceBarWidth(safeTopDevices, dev)}%"></div>
                                </div>
                            </div>
                        `).join("")
                        : '<div class="empty-state"><div>Данные появятся после получения событий</div></div>'}
                </div>
            </section>

            <section class="card dashboard-users-card">
                <div class="card-header dashboard-users-header">
                    <h3>Пользователи</h3>
                    <span class="badge muted">${safeUsers.length}</span>
                </div>

                <div class="card-body table-wrap" style="padding:0;">
                    <table class="table users-table">
                        <thead>
                            <tr>
                                <th class="col-username">USERNAME</th>
                                <th class="col-fullname">FULL NAME</th>
                                <th class="col-email">EMAIL</th>
                                <th class="col-role">ROLE</th>
                                <th class="col-status">STATUS</th>
                                <th class="col-actions">ACTIONS</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${safeUsers.length
                                ? safeUsers.map((u) => `
                                    <tr>
                                        <td>
                                            <div style="display:flex;align-items:center;gap:8px;">
                                                <div class="role-avatar ${roleClass(u.role)}">${esc((u.username || "?").charAt(0).toUpperCase())}</div>
                                                <span>${esc(u.username || "")}</span>
                                            </div>
                                        </td>
                                        <td>${esc(u.fullName || "—")}</td>
                                        <td style="color:var(--text-muted);">${esc(u.email || "—")}</td>
                                        <td><span class="badge ${roleBadgeClass(u.role)}">${roleLabel(u.role)}</span></td>
                                        <td><span class="badge ${u.isActive ? "success" : "muted"}">${u.isActive ? "Active" : "Inactive"}</span></td>
                                        <td>${
                                            isAdmin
                                                ? `<div style="display:flex;gap:6px;">
                                                    <button class="btn mini ghost" data-edit="${u.id}">Edit</button>
                                                    ${String(u.id) === String(currentUser.id)
                                                        ? '<span class="badge muted">Self</span>'
                                                        : `<button class="btn mini danger" data-del="${u.id}" data-name="${esc(u.username || "")}">Delete</button>`}
                                                  </div>`
                                                : "—"
                                        }</td>
                                    </tr>
                                `).join("")
                                : '<tr><td colspan="6" style="text-align:center;padding:20px;color:var(--text-muted);">No users</td></tr>'}
                        </tbody>
                    </table>
                </div>
            </section>
        </div>
    `;

    requestAnimationFrame(() => {
        drawActivityChart(root.querySelector("#activityChart"), safeActivity);
        const canvas = root.querySelector("#donutCanvas");
        if (canvas) drawDonut(canvas, stats);
    });

    root.querySelector("#dashboardRefreshBtn")?.addEventListener("click", () => onRefresh?.());
    root.querySelector("#dashLogoutBtn")?.addEventListener("click", () => onLogout?.());
    root.querySelector("#dashAddUserBtn")?.addEventListener("click", () => {
        if (typeof onCreateUser === "function") onCreateUser();
        else openCreateUserModal(onRefresh);
    });

    root.querySelectorAll("[data-edit]").forEach((btn) => {
        btn.addEventListener("click", () => {
            const user = safeUsers.find((u) => String(u.id) === btn.dataset.edit);
            if (user) openEditUserModal(user, onRefresh, currentUser);
        });
    });

    root.querySelectorAll("[data-del]").forEach((btn) => {
        const id = btn.dataset.del;
        const name = btn.dataset.name;

        if (String(id) === String(currentUser.id)) {
            btn.style.display = "none";
            return;
        }

        btn.addEventListener("click", () => {
            openConfirmModal("Удалить пользователя?", `Удалить пользователя "${name}"? Это действие необратимо.`, async () => {
                try {
                    await deleteUser(id);
                    showToast("Пользователь удалён", "success");
                    await onRefresh?.();
                } catch (e) {
                    showToast(e.message || "Ошибка удаления пользователя", "danger");
                }
            });
        });
    });

    return root;
}

function openCreateUserModal(onRefresh) {
    ensureModalHost();
    const modalId = "createUserModal";
    document.getElementById(modalId)?.remove();

    const modal = createModal({
        id: modalId,
        title: "Добавить пользователя",
        width: "460px"
    });

    setModalBody(modal, `
        <div class="form-grid">
            <div class="form-group">
                <label class="form-label">ИМЯ ПОЛЬЗОВАТЕЛЯ</label>
                <input class="input" id="createUsername" />
            </div>
            <div class="form-group">
                <label class="form-label">ПОЛНОЕ ИМЯ</label>
                <input class="input" id="createFullName" />
            </div>
            <div class="form-group">
                <label class="form-label">EMAIL</label>
                <input class="input" id="createEmail" />
            </div>
            <div class="form-group">
                <label class="form-label">ПАРОЛЬ</label>
                <input class="input" id="createPassword" type="password" />
            </div>
            <div class="form-group">
                <label class="form-label">РОЛЬ</label>
                <select class="input" id="createRole">
                    <option value="viewer">Наблюдатель</option>
                    <option value="operator">Оператор</option>
                    <option value="admin">Администратор</option>
                </select>
            </div>
            <div class="form-group">
                <label class="form-label" style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                    <span class="toggle on" id="createActiveToggle"><span class="toggle-knob"></span></span>
                    <span id="createActiveLabel">Активен</span>
                </label>
            </div>
            <div class="form-group">
                <label class="form-label" style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                    <span class="toggle on" id="createMustChangeToggle"><span class="toggle-knob"></span></span>
                    <span id="createMustChangeLabel">Сменить пароль при первом входе</span>
                </label>
            </div>
            <div class="error-box" id="createError"></div>
        </div>
    `);

    setModalFooter(modal, `
        <button class="btn ghost" id="createCancelBtn">Отмена</button>
        <button class="btn success" id="createSaveBtn">Создать</button>
    `);

    document.getElementById("modalHost")?.appendChild(modal);

    let isActive = true;
    let mustChangePassword = true;

    const activeToggle = modal.querySelector("#createActiveToggle");
    activeToggle?.addEventListener("click", () => {
        isActive = !isActive;
        activeToggle.classList.toggle("on", isActive);
        const label = modal.querySelector("#createActiveLabel");
        if (label) label.textContent = isActive ? "Активен" : "Отключён";
    });

    const mustChangeToggle = modal.querySelector("#createMustChangeToggle");
    mustChangeToggle?.addEventListener("click", () => {
        mustChangePassword = !mustChangePassword;
        mustChangeToggle.classList.toggle("on", mustChangePassword);
        const label = modal.querySelector("#createMustChangeLabel");
        if (label) {
            label.textContent = mustChangePassword
                ? "Сменить пароль при первом входе"
                : "Не требовать смену пароля";
        }
    });

    modal.querySelector("#createCancelBtn")?.addEventListener("click", () => closeModal(modalId));

    modal.querySelector("#createSaveBtn")?.addEventListener("click", async () => {
        const errBox = modal.querySelector("#createError");
        errBox.classList.remove("visible");
        errBox.textContent = "";

        const payload = {
            username: modal.querySelector("#createUsername").value.trim(),
            fullName: modal.querySelector("#createFullName").value.trim(),
            email: modal.querySelector("#createEmail").value.trim(),
            password: modal.querySelector("#createPassword").value,
            role: modal.querySelector("#createRole").value,
            isActive,
            mustChangePassword
        };

        if (!payload.username) return showModalError(errBox, "Введите имя пользователя");
        if (!payload.fullName) return showModalError(errBox, "Введите полное имя");
        if (!payload.password) return showModalError(errBox, "Введите пароль");
        if (payload.password.length < 6) return showModalError(errBox, "Пароль должен быть не менее 6 символов");

        try {
            await createUser(payload);
            showToast("Пользователь создан", "success");
            closeModal(modalId);
            await onRefresh?.();
        } catch (e) {
            showModalError(errBox, e.message || "Ошибка создания пользователя");
        }
    });

    openModal(modalId);
}

function drawActivityChart(container, activity) {
    if (!container) return;

    const buckets = Array.isArray(activity)
        ? activity.map((item) => {
            if (item && typeof item === "object" && Object.prototype.hasOwnProperty.call(item, "hour")) {
                return { label: String(item.hour || ""), count: Number(item.count || 0) };
            }
            return null;
        }).filter(Boolean)
        : [];

    if (!buckets.length) {
        container.innerHTML = `
            <div class="empty-state" style="height:100%;display:flex;align-items:center;justify-content:center;">
                <div>Активность появится после получения событий.</div>
            </div>
        `;
        return;
    }

    const max = Math.max(1, ...buckets.map(b => b.count));

    container.innerHTML = buckets.map(b => {
        const h = Math.max(4, Math.round((b.count / max) * 100));
        return `
            <div class="db-bar-item">
                <div class="db-bar-wrap">
                    <div class="db-bar-tooltip">${b.count}</div>
                    <div class="db-bar-fill" style="height:${h}%"></div>
                </div>
                <div class="db-bar-label">${esc(b.label)}</div>
            </div>
        `;
    }).join("");
}

function drawDonut(canvas, stats) {
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const cx = 55;
    const cy = 55;
    const r = 46;
    const ir = 28;

    const vals = [
        stats.criticalCount ?? 0,
        stats.highCount ?? 0,
        stats.mediumCount ?? 0,
        stats.lowCount ?? 0
    ];

    const colors = ["#f85149", "#d29922", "#58a6ff", "#3fb950"];
    const total = vals.reduce((a, b) => a + b, 0);

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (total === 0) {
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.arc(cx, cy, ir, 0, Math.PI * 2, true);
        ctx.fillStyle = "#2a2a2a";
        ctx.fill();
    } else {
        let angle = -Math.PI / 2;

        vals.forEach((v, i) => {
            if (!v) return;
            const sweep = (v / total) * Math.PI * 2;

            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.arc(cx, cy, r, angle, angle + sweep);
            ctx.arc(cx, cy, ir, angle + sweep, angle, true);
            ctx.closePath();
            ctx.fillStyle = colors[i];
            ctx.fill();

            angle += sweep;
        });
    }

    ctx.beginPath();
    ctx.arc(cx, cy, ir - 2, 0, Math.PI * 2);
    ctx.fillStyle = "#161b22";
    ctx.fill();

    ctx.fillStyle = "#e6edf3";
    ctx.font = "bold 14px sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(total, cx, cy);
}

function openEditUserModal(user, onRefresh, currentUser) {
    ensureModalHost();
    const modalId = "editUserModal";
    document.getElementById(modalId)?.remove();

    const modal = createModal({ id: modalId, title: "Редактировать пользователя", width: "460px" });

    setModalBody(modal, `
        <div class="form-grid">
            <div class="form-group">
                <label class="form-label">ИМЯ ПОЛЬЗОВАТЕЛЯ</label>
                <input class="input" id="editUsername" value="${esc(user.username || "")}" />
            </div>
            <div class="form-group">
                <label class="form-label">ПОЛНОЕ ИМЯ</label>
                <input class="input" id="editFullName" value="${esc(user.fullName || "")}" />
            </div>
            <div class="form-group">
                <label class="form-label">EMAIL</label>
                <input class="input" id="editEmail" value="${esc(user.email || "")}" />
            </div>
            <div class="form-group">
                <label class="form-label">РОЛЬ</label>
                <select class="input" id="editRole">
                    <option value="viewer" ${user.role === "viewer" ? "selected" : ""}>Наблюдатель</option>
                    <option value="operator" ${user.role === "operator" ? "selected" : ""}>Оператор</option>
                    <option value="admin" ${user.role === "admin" ? "selected" : ""}>Администратор</option>
                </select>
            </div>
            <div class="form-group">
                <label class="form-label" style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                    <span class="toggle ${user.isActive ? "on" : ""}" id="editActiveToggle"><span class="toggle-knob"></span></span>
                    <span id="editActiveLabel">${user.isActive ? "Активен" : "Отключён"}</span>
                </label>
            </div>
            <div class="error-box" id="editError"></div>
        </div>
    `);

    setModalFooter(modal, `
        <button class="btn ghost" id="editCancelBtn">Отмена</button>
        <button class="btn primary" id="editSaveBtn">Сохранить</button>
    `);

    document.getElementById("modalHost")?.appendChild(modal);

    let isActive = !!user.isActive;
    const toggle = modal.querySelector("#editActiveToggle");

    toggle?.addEventListener("click", () => {
        isActive = !isActive;
        toggle.classList.toggle("on", isActive);
        const label = modal.querySelector("#editActiveLabel");
        if (label) label.textContent = isActive ? "Активен" : "Отключён";
    });

    modal.querySelector("#editSaveBtn")?.addEventListener("click", async () => {
        const errBox = modal.querySelector("#editError");
        errBox.classList.remove("visible");
        errBox.textContent = "";

        const payload = {
            username: modal.querySelector("#editUsername").value.trim(),
            fullName: modal.querySelector("#editFullName").value.trim(),
            email: modal.querySelector("#editEmail").value.trim(),
            role: modal.querySelector("#editRole").value,
            isActive
        };

        if (!payload.username) return showModalError(errBox, "Введите имя пользователя");
        if (!payload.fullName) return showModalError(errBox, "Введите полное имя");

        try {
            await updateUser(user.id, payload);
            showToast("Пользователь обновлён", "success");
            closeModal(modalId);
            await onRefresh?.();
        } catch (e) {
            showModalError(errBox, e.message || "Ошибка обновления пользователя");
        }
    });

    modal.querySelector("#editCancelBtn")?.addEventListener("click", () => closeModal(modalId));
    openModal(modalId);
}

function showModalError(errBox, text) {
    if (!errBox) return;
    errBox.textContent = text;
    errBox.classList.add("visible");
}

function topDeviceBarWidth(devices, dev) {
    const max = Math.max(1, ...devices.map(d => Number(d.count ?? 0)));
    const count = Number(dev?.count ?? 0);
    return Math.max(2, Math.round((count / max) * 100));
}

function roleClass(role) {
    const r = String(role || "").toLowerCase();
    if (r === "admin") return "admin";
    if (r === "operator") return "operator";
    return "viewer";
}

function roleBadgeClass(role) {
    const r = String(role || "").toLowerCase();
    if (r === "admin") return "danger";
    if (r === "operator") return "warning";
    return "accent";
}

function roleLabel(role) {
    const r = String(role || "").toLowerCase();
    if (r === "admin") return "Admin";
    if (r === "operator") return "Operator";
    return "Viewer";
}

function esc(v) {
    const s = String(v ?? "");
    return s
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;");
}