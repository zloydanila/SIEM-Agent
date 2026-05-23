import { showToast } from "../components/toast.js";
import { updateUser, deleteUser } from "../api.js";
import { createModal, openModal, closeModal, setModalBody, setModalFooter, ensureModalHost, openConfirmModal } from "../components/modal.js";

export function renderDashboard({
    stats = {},
    activity = [],
    topDevices = [],
    users = [],
    events = [],
    alerts = [],
    currentUser = {},
    currentUserRole = "",
    wsConnected = false,
    canManage = false,
    onRefresh
} = {}) {
    const root = document.createElement("div");
    root.className = "page-inner dashboard-page";

    const safeActivity = Array.isArray(activity) ? activity : [];
    const safeTopDevices = Array.isArray(topDevices) ? topDevices : [];
    const safeUsers = Array.isArray(users) ? users : [];

    root.innerHTML = `
        <div class="dashboard-shell">
            <section class="card dashboard-header-card">
                <div class="card-body dashboard-header-body">
                    <div class="dashboard-header-left">
                        <div class="dashboard-brand-icon">S</div>
                        <div>
                            <div class="section-title">SIEM Dashboard</div>
                            <div class="section-subtitle">${esc(currentUser.fullName || currentUser.username || "")}  |  ${esc(String(currentUserRole).toUpperCase())}</div>
                        </div>
                    </div>
                    <div class="dashboard-header-actions">
                        <span class="status-dot ${wsConnected ? "online" : "offline"}"></span>
                        <span class="status-text ${wsConnected ? "online" : "offline"}">${wsConnected ? "Online" : "Offline"}</span>
                        <button class="btn ghost" id="dashboardRefreshBtn">Refresh</button>
                    </div>
                </div>
            </section>
            <section class="card">
                <div class="card-body">
                    <div class="grid-kpis">
                        ${[
                            ["Всего событий", stats.totalEvents ?? 0, "accent"],
                            ["Открытых алертов", stats.openAlerts ?? 0, "danger"],
                            ["Критических", stats.criticalCount ?? 0, "danger"],
                            ["Высоких", stats.highCount ?? 0, "warning"],
                            ["Средних", stats.mediumCount ?? 0, "accent"],
                            ["Низких", stats.lowCount ?? 0, "success"]
                        ].map(([label, value, color]) => `
                            <div class="card kpi-card ${color}">
                                <div class="kpi-label">${label}</div>
                                <div class="kpi-value" style="color:var(--${color});">${Number(value) || 0}</div>
                            </div>
                        `).join("")}
                    </div>
                </div>
            </section>

            <div class="section-shell">
                <section class="card">
                    <div class="card-header">
                        <h3>Активность событий (последние 7 часов)</h3>
                    </div>
                    <div class="card-body">
                        <div class="db-bar-chart" id="activityChart"></div>
                    </div>
                </section>

                <section class="card">
                    <div class="card-header">
                        <h3>По уровню угрозы</h3>
                    </div>
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

            <div class="section-shell">
                <section class="card">
                    <div class="card-header">
                        <h3>Топ устройств по событиям</h3>
                    </div>
                    <div class="card-body content-gap">
                        ${safeTopDevices.length
                            ? safeTopDevices.map((dev) => `
                                <div>
                                    <div class="device-row">
                                        <span class="device-name">${esc(dev.name ?? dev.deviceName ?? "")}</span>
                                        <span style="color:var(--text-muted);font-size:var(--font-xs);">${Number(dev.count ?? 0)}</span>
                                    </div>
                                    <div class="progress-track">
                                        <div class="progress-fill" style="width:${Math.max(2, Math.min(100, Number(dev.count ?? 0) * 10))}%"></div>
                                    </div>
                                </div>
                            `).join("")
                            : '<div class="empty-state"><div>Данные появятся после получения событий</div></div>'}
                    </div>
                </section>

                <section class="card">
                    <div class="card-header">
                        <h3>Пользователи</h3>
                        <span class="badge muted">${safeUsers.length}</span>
                    </div>
                    <div class="card-body table-wrap">
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
                                            canManage && String(currentUser.role || "").toLowerCase() === "admin"
                                            ? `<div style="display:flex;gap:6px;">
                                                <button class="btn mini ghost" data-edit="${u.id}">Edit</button>
                                                ${u.id === currentUser.id ? '<span class="badge muted">Self</span>' : `<button class="btn mini danger" data-del="${u.id}" data-name="${esc(u.username || "")}">Delete</button>`}
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
        </div>
    `;

    requestAnimationFrame(() => {
        drawActivityChart(root.querySelector("#activityChart"), safeActivity);
        const canvas = root.querySelector("#donutCanvas");
        if (canvas) drawDonut(canvas, stats);
    });

    root.querySelector("#dashboardRefreshBtn")?.addEventListener("click", () => onRefresh?.());

    root.querySelectorAll("[data-edit]").forEach((btn) => {
        btn.addEventListener("click", () => {
            const user = safeUsers.find((u) => String(u.id) === btn.dataset.edit);
            if (user) openEditUserModal(user, onRefresh, currentUser);
        });
    });

    root.querySelectorAll("[data-del]").forEach((btn) => {
        const id = btn.dataset.del;
        const name = btn.dataset.name;

        if (id === currentUser.id) {
            btn.style.display = "none";
            return;
        }

        btn.addEventListener("click", () => {
            openConfirmModal("Удалить пользователя?", `Удалить пользователя "${name}"? Это действие необратимо.`, async () => {
                try {
                    await deleteUser(id);
                    showToast("Пользователь удалён", "success");
                    onRefresh?.();
                } catch (e) {
                    showToast(e.message, "danger");
                }
            });
        });
    });

    return root;
}

function drawActivityChart(container, activity) {
    if (!container) return;

    const buckets = Array.isArray(activity)
        ? activity.map((item) => {
            if (item && typeof item === "object") {
                if (Object.prototype.hasOwnProperty.call(item, "hour")) {
                    return {
                        label: String(item.hour || ""),
                        count: Number(item.count || 0)
                    };
                }
                const raw = item.timestamp || item.time || item.triggeredAt || item.createdAt || item.date;
                return {
                    label: String(raw || ""),
                    count: 1
                };
            }
            return null;
        }).filter(Boolean)
        : [];

    if (!buckets.length) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-title">Нет данных активности</div>
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
                    <div class="db-bar-fill bar-anim" style="height:${h}%;"></div>
                </div>
                <div class="db-bar-label">${esc(b.label)}</div>
            </div>
        `;
    }).join("");
}

function drawDonut(canvas, stats) {
    const ctx = canvas.getContext("2d");
    const cx = 55, cy = 55, r = 46, ir = 28;
    const vals = [stats.criticalCount ?? 0, stats.highCount ?? 0, stats.mediumCount ?? 0, stats.lowCount ?? 0];
    const colors = ["#f85149", "#d29922", "#58a6ff", "#3fb950"];
    const total = vals.reduce((a, b) => a + b, 0);

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (total === 0) {
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.arc(cx, cy, ir, 0, Math.PI * 2, true);
        ctx.fillStyle = "rgba(255,255,255,0.06)";
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

    const modalHost = document.getElementById("modalHost");
    if (!modalHost) return;
    modalHost.appendChild(modal);

    let isActive = !!user.isActive;
    const toggle = modal.querySelector("#editActiveToggle");
    if (toggle) {
        toggle.addEventListener("click", () => {
            isActive = !isActive;
            toggle.classList.toggle("on", isActive);
            const label = modal.querySelector("#editActiveLabel");
            if (label) label.textContent = isActive ? "Активен" : "Отключён";
        });
    }

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
            onRefresh?.();
        } catch (e) {
            showModalError(errBox, e.message);
        }
    });

    modal.querySelector("#editCancelBtn")?.addEventListener("click", () => closeModal(modalId));
    openModal(modalId);
}

function showModalError(errBox, text) {
    errBox.textContent = text;
    errBox.classList.add("visible");
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
    return s.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;");
}