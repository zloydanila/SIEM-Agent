import { showToast } from "../components/toast.js";
import {
  changePassword,
  clearEvents,
  clearAlerts,
  exportEventsCsv,
  exportAlertsCsv,
  exportReportJson,
  rules as loadRules,
  status as loadStatus,
  createRule,
  updateRule,
  deleteRule,
  toggleRule
} from "../api.js";
import { createModal, openModal, closeModal, setModalBody, setModalFooter, ensureModalHost, openConfirmModal } from "../components/modal.js";

export async function renderSettings({
  mustChangePassword = false,
  currentUser = {},
  canManage = false,
  rules: initialRules = [],
  onRefresh
} = {}) {
  const root = document.createElement("div");
  root.className = "page-inner";

  let rulesData = Array.isArray(initialRules) ? initialRules : [];
  let data = { totalEvents: 0, totalAlerts: 0 };
  let wsStatus = { wsRunning: false, wsPort: 8080, wsClients: 0 };

  try { wsStatus = await loadStatus(); } catch (_) {}
  try {
    const [ev, al] = await Promise.all([fetchCount("/api/events"), fetchCount("/api/alerts")]);
    data.totalEvents = ev;
    data.totalAlerts = al;
  } catch (_) {}
  if (!rulesData.length) {
    try { rulesData = await loadRules(); } catch (_) {}
  }

  const forcedPasswordNotice = mustChangePassword ? `<div class="page-note" style="border-color:var(--warning);color:var(--warning);margin-top:16px;">Требуется смена пароля. Сначала обновите пароль, чтобы продолжить работу.</div>` : "";

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div>
          <h2>Настройки</h2>
          <p>System information and data management</p>
        </div>
        ${!canManage ? `<div class="role-badge">Только просмотр</div>` : ""}
      </div>
      ${forcedPasswordNotice}

      <div class="two-col">
        <div class="content-gap">
          <div class="card slide-up">
            <div class="card-header"><h3>WebSocket сервер</h3></div>
            <div class="card-body">
              <div class="settings-rows">
                <div class="settings-row"><span>Статус</span><span class="badge ${wsStatus.wsRunning ? "success" : "danger"}">${wsStatus.wsRunning ? "Работает" : "Остановлен"}</span></div>
                <div class="settings-row"><span>Порт</span><span>${wsStatus.wsPort || 8080}</span></div>
                <div class="settings-row"><span>Клиентов</span><span class="badge muted">${wsStatus.wsClients || 0}</span></div>
              </div>
            </div>
          </div>

          <div class="card slide-up">
            <div class="card-header"><h3>Смена пароля</h3></div>
            <div class="card-body">
              <div class="settings-rows">
                <div class="settings-row"><span>Хеширование</span><span class="badge success">PBKDF2 · 100K</span></div>
                <div class="settings-row"><span>Соль</span><span class="badge success">256-bit CSPRNG</span></div>
              </div>
              <div style="margin-top:12px;"><button class="btn primary" id="changePwBtn" style="width:100%;">Сменить пароль</button></div>
            </div>
          </div>

          <div class="card slide-up">
            <div class="card-header"><h3>База данных</h3></div>
            <div class="card-body">
              <div class="settings-rows">
                <div class="settings-row"><span>Тип</span><span>SQLite</span></div>
                <div class="settings-row"><span>Файл</span><span style="font-family:monospace;color:var(--text-muted);">siem_agent.db</span></div>
                <div class="settings-row"><span>Событий</span><span id="dbEvents">${data.totalEvents}</span></div>
                <div class="settings-row"><span>Алертов</span><span id="dbAlerts">${data.totalAlerts}</span></div>
              </div>
              ${canManage ? `<div style="display:flex;gap:8px;margin-top:12px;"><button class="btn danger" style="flex:1;" id="clearEventsBtn">Очистить события</button><button class="btn danger" style="flex:1;" id="clearAlertsBtn">Очистить алерты</button></div>` : ""}
            </div>
          </div>
        </div>

        <div class="content-gap">
          <div class="card slide-up">
            <div class="card-header"><h3>О системе</h3></div>
            <div class="card-body">
              <div class="settings-rows">
                <div class="settings-row"><span>Название</span><span>SIEM Agent</span></div>
                <div class="settings-row"><span>Версия</span><span>1.0.0</span></div>
                <div class="settings-row"><span>Платформа</span><span>Web console</span></div>
                <div class="settings-row"><span>Назначение</span><span>Мониторинг безопасности</span></div>
              </div>
            </div>
          </div>

          <div class="card slide-up">
            <div class="card-header"><h3>Экспорт отчетов</h3></div>
            <div class="card-body content-gap">
              <button class="btn primary" id="expEventsBtn" style="width:100%;">События (CSV)</button>
              <button class="btn primary" id="expAlertsBtn" style="width:100%;">Алерты (CSV)</button>
              <button class="btn primary" id="expReportBtn" style="width:100%;">Отчет (JSON)</button>
            </div>
          </div>

          <div class="card slide-up">
            <div class="card-header"><h3>Правила корреляции</h3><span class="badge muted" id="rulesCount">${rulesData.length} правил</span></div>
            <div class="card-body">
              <div id="rulesList" class="content-gap" style="max-height:300px;overflow-y:auto;">${renderRulesList(rulesData, canManage, currentUser)}</div>
              ${canManage ? `<button class="btn primary" style="width:100%;margin-top:12px;" id="addRuleBtn">+ Добавить правило</button>` : ""}
            </div>
          </div>
        </div>
      </div>
    </section>
  `;

  root.querySelector("#changePwBtn")?.addEventListener("click", () => openChangePasswordModal(mustChangePassword, onRefresh));

  if (mustChangePassword && !document.getElementById("changePwModal")) {
    requestAnimationFrame(() => openChangePasswordModal(true, onRefresh));
  }

  root.querySelector("#clearEventsBtn")?.addEventListener("click", () => {
    openConfirmModal("Очистить события?", "Это действие необратимо - все события будут удалены из базы данных.", async () => {
      try {
        await clearEvents();
        showToast("События очищены", "success");
        root.querySelector("#dbEvents").textContent = "0";
      } catch (e) { showToast(e.message, "danger"); }
    });
  });

  root.querySelector("#clearAlertsBtn")?.addEventListener("click", () => {
    openConfirmModal("Очистить алерты?", "Это действие необратимо - все алерты будут удалены из базы данных.", async () => {
      try {
        await clearAlerts();
        showToast("Алерты очищены", "success");
        root.querySelector("#dbAlerts").textContent = "0";
      } catch (e) { showToast(e.message, "danger"); }
    });
  });

  root.querySelector("#expEventsBtn")?.addEventListener("click", async () => {
    try { downloadBlob(await exportEventsCsv(), "events.csv"); showToast("События экспортированы", "success"); }
    catch (e) { showToast(e.message, "danger"); }
  });

  root.querySelector("#expAlertsBtn")?.addEventListener("click", async () => {
    try { downloadBlob(await exportAlertsCsv(), "alerts.csv"); showToast("Алерты экспортированы", "success"); }
    catch (e) { showToast(e.message, "danger"); }
  });

  root.querySelector("#expReportBtn")?.addEventListener("click", async () => {
    try { downloadBlob(await exportReportJson(), "report.json"); showToast("Отчет экспортирован", "success"); }
    catch (e) { showToast(e.message, "danger"); }
  });

  root.querySelectorAll("[data-toggle-rule]").forEach(el => {
    el.addEventListener("click", async () => {
      const id = el.dataset.toggleRule;
      const enabled = el.dataset.enabled === "true";
      try {
        await toggleRule(id, !enabled);
        showToast(!enabled ? "Правило включено" : "Правило отключено", "success");
        onRefresh?.();
      } catch (e) { showToast(e.message, "danger"); }
    });
  });

  root.querySelectorAll("[data-edit-rule]").forEach(btn => {
    btn.addEventListener("click", () => {
      const rule = rulesData.find(r => String(r.id) === btn.dataset.editRule);
      if (rule) openRuleModal(rule, onRefresh);
    });
  });

  root.querySelectorAll("[data-del-rule]").forEach(btn => {
    btn.addEventListener("click", async () => {
      const id = btn.dataset.delRule;
      const name = btn.dataset.name;
      if (!confirm(`Удалить правило «${name}»? Это действие необратимо.`)) return;
      try {
        await deleteRule(id);
        showToast("Правило удалено", "success");
        onRefresh?.();
      } catch (e) { showToast(e.message, "danger"); }
    });
  });

  root.querySelector("#addRuleBtn")?.addEventListener("click", () => openRuleModal(null, onRefresh));
  return root;
}

function fetchCount(path) {
  return fetch(path, {
    headers: {
      Authorization: localStorage.getItem("token") ? `Bearer ${localStorage.getItem("token")}` : ""
    },
    credentials: "include"
  })
    .then(r => r.json())
    .then(d => Array.isArray(d) ? d.length : 0)
    .catch(() => 0);
}

function downloadBlob(blob, filename) {
  if (!(blob instanceof Blob)) throw new Error("Invalid data for download");
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  requestAnimationFrame(() => {
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  });
}

function openChangePasswordModal(mustChangePasswordNow = false, onSuccess) {
  ensureModalHost();
  const modalId = "changePwModal";
  const modal = createModal({ 
    id: modalId, 
    title: "Смена пароля", 
    width: "440px",
    isBlocking: mustChangePasswordNow 
  });

  setModalBody(modal, `
    <div class="form-grid">
      <div class="form-group">
        <label class="form-label">${mustChangePasswordNow ? 'ВХОД ЗАБЛОКИРОВАН' : 'ТЕКУЩИЙ ПАРОЛЬ'}</label>
        ${mustChangePasswordNow ? '<div style="padding:12px;background:var(--bg-secondary);border-radius:var(--radius-small);color:var(--text-secondary);font-size:var(--font-sm);margin-bottom:12px;">Вы должны сменить пароль перед использованием системы. Это требуемая операция.</div>' : '<input class="input" type="password" id="pwCurrent" />'}
      </div>
      <div class="form-group">
        <label class="form-label">НОВЫЙ ПАРОЛЬ</label>
        <input class="input" type="password" id="pwNew" />
      </div>
      <div class="form-group">
        <label class="form-label">ПОДТВЕРЖДЕНИЕ</label>
        <input class="input" type="password" id="pwConfirm" />
      </div>
      <div class="error-box" id="pwError"></div>
    </div>
  `);

  setModalFooter(modal, `
    ${!mustChangePasswordNow ? '<button class="btn ghost" id="pwCancelBtn">Отмена</button>' : ''}
    <button class="btn primary" id="pwSaveBtn" style="${mustChangePasswordNow ? 'width:100%;' : ''}">Сменить пароль</button>
  `);

  ensureModalHost().appendChild(modal);

  const saveBtn = modal.querySelector("#pwSaveBtn");
  const cancelBtn = modal.querySelector("#pwCancelBtn");
  if (!saveBtn) return;

  if (cancelBtn) {
    cancelBtn.addEventListener("click", () => closeModal(modalId));
  }

  saveBtn.addEventListener("click", async () => {
    const currentPassword = mustChangePasswordNow ? "" : (modal.querySelector("#pwCurrent").value.trim());
    const newPassword = modal.querySelector("#pwNew").value.trim();
    const confirmPassword = modal.querySelector("#pwConfirm").value.trim();
    const errBox = modal.querySelector("#pwError");

    errBox.classList.remove("visible");
    errBox.textContent = "";

    if (!mustChangePasswordNow && !currentPassword) return setPwErr(errBox, "Введите текущий пароль");
    if (newPassword.length < 6) return setPwErr(errBox, "Новый пароль должен быть не менее 6 символов");
    if (newPassword !== confirmPassword) return setPwErr(errBox, "Пароли должны совпадать");

    try {
      await changePassword(currentPassword, newPassword);
      showToast("Пароль изменён", "success");
      if (mustChangePasswordNow) localStorage.removeItem("mustChangePassword");
      closeModal(modalId);
      onSuccess?.();
    } catch (e) {
      setPwErr(errBox, e.message);
    }
  });

  openModal(modalId);

  // Блокировка Escape при обязательной смене
  if (mustChangePasswordNow) {
    const handleEscape = (e) => {
      if (e.key === "Escape") e.preventDefault();
    };
    document.addEventListener("keydown", handleEscape);
    const cleanup = () => document.removeEventListener("keydown", handleEscape);
    modal.addEventListener("remove", cleanup);
  }
}

function openRuleModal(rule, onRefresh) {
  ensureModalHost();
  const isEdit = !!rule;
  const modalId = isEdit ? "editRuleModal" : "addRuleModal";
  const modal = createModal({ id: modalId, title: isEdit ? "Редактировать правило" : "Новое правило корреляции", width: "560px" });

  setModalBody(modal, `
    <div class="form-grid">
      <div class="form-group">
        <label class="form-label">НАЗВАНИЕ ПРАВИЛА</label>
        <input class="input" id="rName" value="${esc(isEdit ? rule.name || "" : "")}" />
      </div>
      <div class="form-group">
        <label class="form-label">ТИП ПРАВИЛА</label>
        <select class="input" id="rType">
          <option value="threshold" ${isEdit && rule.ruleType === "threshold" ? "selected" : ""}>Порог (threshold)</option>
          <option value="correlation" ${isEdit && rule.ruleType === "correlation" ? "selected" : ""}>Корреляция (correlation)</option>
        </select>
      </div>
      <div class="form-group">
        <label class="form-label">ТИП СОБЫТИЯ</label>
        <input class="input" id="rMatchEvent" value="${esc(isEdit ? rule.matchEventType || "" : "")}" />
      </div>
      <div class="form-group" id="rSecondaryGroup" style="${isEdit && rule.ruleType === "correlation" ? "" : "display:none;"}">
        <label class="form-label">ВТОРИЧНОЕ СОБЫТИЕ</label>
        <input class="input" id="rSecondaryEvent" value="${esc(isEdit ? rule.secondaryEventType || "" : "")}" />
      </div>
      <div class="form-row">
        <div class="form-group">
          <label class="form-label">ПОРОГ</label>
          <input class="input" type="number" id="rThreshold" value="${isEdit ? (rule.threshold || 1) : 1}" min="1" />
        </div>
        <div class="form-group">
          <label class="form-label">ОКНО (сек)</label>
          <input class="input" type="number" id="rWindow" value="${isEdit ? (rule.windowSeconds || 60) : 60}" min="1" />
        </div>
        <div class="form-group">
          <label class="form-label">COOLDOWN (сек)</label>
          <input class="input" type="number" id="rCooldown" value="${isEdit ? (rule.cooldownSeconds || 60) : 60}" min="1" />
        </div>
      </div>
      <div class="form-row">
        <div class="form-group">
          <label class="form-label">УРОВЕНЬ УГРОЗЫ</label>
          <select class="input" id="rSeverity">
            <option value="low" ${isEdit && rule.alertSeverity === "low" ? "selected" : ""}>low</option>
            <option value="medium" ${isEdit && rule.alertSeverity === "medium" ? "selected" : ""}>medium</option>
            <option value="high" ${(!isEdit || rule.alertSeverity === "high") ? "selected" : ""}>high</option>
            <option value="critical" ${isEdit && rule.alertSeverity === "critical" ? "selected" : ""}>critical</option>
          </select>
        </div>
        <div class="form-group" style="grid-column:span 2;">
          <label class="form-label">ЗАГОЛОВОК АЛЕРТА</label>
          <input class="input" id="rTitle" value="${esc(isEdit ? rule.alertTitle || "" : "")}" />
        </div>
      </div>
      <div class="form-group">
        <label class="form-label">ОПИСАНИЕ АЛЕРТА</label>
        <input class="input" id="rDesc" value="${esc(isEdit ? rule.alertDescription || "" : "")}" />
      </div>
      <div class="error-box" id="rError"></div>
    </div>
  `);

  setModalFooter(modal, `
    <button class="btn ghost" id="rCancelBtn">Отмена</button>
    <button class="btn primary" id="rSaveBtn">${isEdit ? "Сохранить" : "Добавить правило"}</button>
  `);

  ensureModalHost().appendChild(modal);

  const typeSelect = modal.querySelector("#rType");
  const secondaryGroup = modal.querySelector("#rSecondaryGroup");
  typeSelect?.addEventListener("change", () => {
    secondaryGroup.style.display = typeSelect.value === "correlation" ? "" : "none";
  });

  modal.querySelector("#rCancelBtn")?.addEventListener("click", () => closeModal(modalId));

  modal.querySelector("#rSaveBtn")?.addEventListener("click", async () => {
    const payload = {
      name: modal.querySelector("#rName").value.trim(),
      ruleType: typeSelect.value,
      matchEventType: modal.querySelector("#rMatchEvent").value.trim(),
      secondaryEventType: typeSelect.value === "correlation" ? modal.querySelector("#rSecondaryEvent").value.trim() : "",
      threshold: parseInt(modal.querySelector("#rThreshold").value, 10) || 1,
      windowSeconds: parseInt(modal.querySelector("#rWindow").value, 10) || 60,
      cooldownSeconds: parseInt(modal.querySelector("#rCooldown").value, 10) || 60,
      alertSeverity: modal.querySelector("#rSeverity").value,
      alertTitle: modal.querySelector("#rTitle").value.trim(),
      alertDescription: modal.querySelector("#rDesc").value.trim(),
      isEnabled: true
    };

    const errBox = modal.querySelector("#rError");
    errBox.classList.remove("visible");
    errBox.textContent = "";

    if (!payload.name) return showErr(errBox, "Введите название");
    if (!payload.matchEventType) return showErr(errBox, "Введите тип события");
    if (payload.ruleType === "correlation" && !payload.secondaryEventType) return showErr(errBox, "Введите вторичное событие");
    if (!payload.alertTitle) return showErr(errBox, "Введите заголовок алерта");

    try {
      if (isEdit) {
        await updateRule(rule.id, payload);
        showToast("Правило обновлено", "success");
      } else {
        await createRule(payload);
        showToast("Правило создано", "success");
      }
      closeModal(modalId);
      onRefresh?.();
    } catch (e) {
      showErr(errBox, e.message);
    }
  });

  openModal(modalId);
}

function renderRulesList(rules, canManage, currentUser) {
  if (!rules.length) return '<div style="text-align:center;padding:20px;color:var(--text-muted);">Правил нет. Нажмите «+ Добавить правило»</div>';
  return rules.map(r => `
    <div style="display:flex;align-items:center;gap:8px;padding:8px;border-radius:var(--radius-small);border:1px solid ${r.isEnabled ? "var(--border)" : "transparent"};">
      <div class="rule-toggle ${r.isEnabled ? "on" : ""}" data-toggle-rule="${r.id}" data-enabled="${r.isEnabled}" style="flex-shrink:0;">
        <div class="rule-toggle-knob"></div>
      </div>
      <div style="flex:1;min-width:0;">
        <div style="font-weight:600;font-size:var(--font-sm);color:${r.isEnabled ? "var(--text-primary)" : "var(--text-muted)"};">${esc(r.name || "")}</div>
        <div style="font-size:var(--font-xs);color:var(--text-muted);">
          <span class="badge ${r.ruleType === "threshold" ? "accent" : "warning"}" style="height:18px;padding:0 6px;font-size:10px;">${esc(r.ruleType || "")}</span>
          <span style="font-family:monospace;">${esc(r.matchEventType || "")}</span>
          <span class="badge ${sevBadgeClass(r.alertSeverity)}" style="height:18px;padding:0 6px;font-size:10px;">${esc(r.alertSeverity || "")}</span>
        </div>
      </div>
      ${canManage ? `
        <div style="display:flex;gap:4px;flex-shrink:0;">
          <button class="btn mini ghost" data-edit-rule="${r.id}">Edit</button>
          ${r.id === currentUser?.id ? '<span class="badge muted">Self</span>' : `<button class="btn mini danger" data-del-rule="${r.id}" data-name="${esc(r.name || "")}">Del</button>`}
        </div>` : ""}
    </div>
  `).join("");
}

function openConfirmModal(title, message, onConfirm) {
  ensureModalHost();
  const modalId = "confirmModal_" + Date.now();
  const modal = createModal({ id: modalId, title: title, width: "420px" });

  setModalBody(modal, `
    <div style="padding:12px 0;">
      <p style="color:var(--text-secondary);margin:0;line-height:1.5;">${message}</p>
    </div>
  `);

  setModalFooter(modal, `
    <button class="btn ghost" id="confirmCancel">Отмена</button>
    <button class="btn danger" id="confirmOk">Подтвердить</button>
  `);

  ensureModalHost().appendChild(modal);

  modal.querySelector("#confirmCancel")?.addEventListener("click", () => closeModal(modalId));
  modal.querySelector("#confirmOk")?.addEventListener("click", async () => {
    closeModal(modalId);
    await onConfirm?.();
  });

  openModal(modalId);
}

function showErr(el, text) {
  el.textContent = text;
  el.classList.add("visible");
}

function setPwErr(el, text) {
  el.textContent = text;
  el.classList.add("visible");
}

function sevBadgeClass(s) {
  const v = String(s || "").toLowerCase();
  if (v === "critical") return "danger";
  if (v === "high") return "warning";
  if (v === "medium") return "accent";
  return "success";
}

function esc(v) {
  return String(v ?? "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;");
}