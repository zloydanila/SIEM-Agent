import { showToast } from "../components/toast.js";
import {
  changePassword,
  clearEvents,
  clearAlerts,
  exportEventsCsv,
  exportAlertsCsv,
  exportReportJson,
  status as loadStatus,
  createRule,
  updateRule,
  deleteRule,
  toggleRule
} from "../api.js";
import {
  createModal,
  openModal,
  closeModal,
  setModalBody,
  setModalFooter,
  ensureModalHost,
  openConfirmModal
} from "../components/modal.js";
import { setState } from "../state.js";

export async function renderSettings({
  mustChangePassword = false,
  currentUser = {},
  canManage = false,
  canAccessRules = false,
  canClearData = false,
  rules: initialRules = [],
  onRefresh
} = {}) {
  const root = document.createElement("div");
  root.className = "page-inner settings-page";

  let rulesData = Array.isArray(initialRules) ? initialRules : [];
  let wsStatus = { wsRunning: false, wsPort: 8080, wsClients: 0 };

  try {
    wsStatus = await loadStatus();
  } catch (_) {}

  const forcedPasswordNotice = mustChangePassword
    ? `
      <div class="page-note settings-warning-note">
        Требуется обязательная смена пароля. Пока пароль не будет изменён, работа с системой заблокирована.
      </div>
    `
    : "";

  root.innerHTML = `
    <section class="page-section settings-section">
      <div class="settings-page-top">
        <div>
          <h2 class="settings-page-title">Настройки</h2>
          <p class="settings-page-subtitle">System information and data management</p>
        </div>
        ${!canManage ? `<div class="role-badge">Ограниченный доступ</div>` : ""}
      </div>

      ${forcedPasswordNotice}

      <div class="settings-grid">
        <section class="card settings-card">
          <div class="card-header"><h3>WebSocket сервер</h3></div>
          <div class="card-body">
            <div class="settings-rows">
              <div class="settings-row">
                <span>Статус</span>
                <span class="badge ${wsStatus.wsRunning ? "success" : "danger"}">
                  ${wsStatus.wsRunning ? "Работает" : "Остановлен"}
                </span>
              </div>
              <div class="settings-row">
                <span>Порт</span>
                <span>${Number(wsStatus.wsPort || 8080)}</span>
              </div>
              <div class="settings-row">
                <span>Адрес</span>
                <span style="color:var(--accent);font-family:monospace;">ws://127.0.0.1:${Number(wsStatus.wsPort || 8080)}</span>
              </div>
              <div class="settings-row">
                <span>Клиентов подключено</span>
                <span class="badge muted">${Number(wsStatus.wsClients || 0)}</span>
              </div>
            </div>
          </div>
        </section>

        <section class="card settings-card">
          <div class="card-header"><h3>Смена пароля</h3></div>
          <div class="card-body">
            <div class="settings-rows">
              <div class="settings-row">
                <span>Хеширование</span>
                <span class="badge success">PBKDF2 · 100K</span>
              </div>
              <div class="settings-row">
                <span>Соль</span>
                <span>256-bit CSPRNG</span>
              </div>
              <div class="settings-row">
                <span>Смена пароля при входе</span>
                <span class="badge success">Включено</span>
              </div>
            </div>
            <div style="margin-top:14px;">
              <button class="btn primary" id="changePwBtn" style="width:100%;">Сменить пароль</button>
            </div>
          </div>
        </section>

        <section class="card settings-card">
          <div class="card-header"><h3>База данных</h3></div>
          <div class="card-body">
            <div class="settings-rows">
              <div class="settings-row">
                <span>Тип</span>
                <span>SQLite</span>
              </div>
              <div class="settings-row">
                <span>Файл</span>
                <span style="font-family:monospace;color:var(--text-muted);">siem_agent.db</span>
              </div>
              <div class="settings-row">
                <span>Расположение</span>
                <span style="font-family:monospace;color:var(--text-muted);">~/.local/share/SIEMAgent</span>
              </div>
            </div>

            ${canClearData ? `
              <div class="settings-danger-row">
                <button class="btn danger" id="clearEventsBtn">Очистить события</button>
                <button class="btn danger" id="clearAlertsBtn">Очистить алерты</button>
              </div>
            ` : ""}
          </div>
        </section>

        <section class="card settings-card">
          <div class="card-header"><h3>О системе</h3></div>
          <div class="card-body">
            <div class="settings-rows">
              <div class="settings-row">
                <span>Название</span>
                <span>SIEM Agent</span>
              </div>
              <div class="settings-row">
                <span>Версия</span>
                <span>1.0.0</span>
              </div>
              <div class="settings-row">
                <span>Платформа</span>
                <span>Web console</span>
              </div>
              <div class="settings-row">
                <span>Назначение</span>
                <span>Мониторинг безопасности</span>
              </div>
            </div>
          </div>
        </section>
      </div>

      ${canAccessRules ? `
        <section class="card settings-rules-card">
          <div class="card-header settings-rules-header">
            <h3>Правила корреляции</h3>
            <div class="settings-rules-actions">
              <span class="badge muted">${rulesData.length} правил</span>
              ${canManage ? `<button class="btn primary" id="addRuleBtn">+ Добавить правило</button>` : ""}
            </div>
          </div>
          <div class="card-body settings-rules-body">
            <div id="rulesList" class="settings-rules-list">
              ${renderRulesList(rulesData, canManage)}
            </div>
          </div>
        </section>
      ` : ""}

      <section class="card settings-export-card">
        <div class="card-header"><h3>Экспорт отчетов</h3></div>
        <div class="card-body">
          <div class="settings-export-grid">
            <button class="btn primary" id="expEventsBtn">События (CSV)</button>
            <button class="btn primary" id="expAlertsBtn">Алерты (CSV)</button>
            <button class="btn primary" id="expReportBtn">Отчет (JSON)</button>
          </div>
        </div>
      </section>
    </section>
  `;

  root.querySelector("#changePwBtn")?.addEventListener("click", () => {
    openChangePasswordModal({
      mustChangePasswordNow: mustChangePassword,
      onSuccess: async () => {
        setState({ mustChangePassword: false });
        await onRefresh?.();
      }
    });
  });

  if (mustChangePassword) {
    requestAnimationFrame(() => {
      openChangePasswordModal({
        mustChangePasswordNow: true,
        onSuccess: async () => {
          setState({ mustChangePassword: false });
          await onRefresh?.();
        }
      });
    });
  }

  root.querySelector("#clearEventsBtn")?.addEventListener("click", () => {
    openConfirmModal(
      "Очистить события?",
      "Это действие необратимо — все события будут удалены из базы данных.",
      async () => {
        try {
          await clearEvents();
          showToast("События очищены", "success");
          await onRefresh?.();
        } catch (e) {
          showToast(e.message || "Не удалось очистить события", "danger");
        }
      }
    );
  });

  root.querySelector("#clearAlertsBtn")?.addEventListener("click", () => {
    openConfirmModal(
      "Очистить алерты?",
      "Это действие необратимо — все алерты будут удалены из базы данных.",
      async () => {
        try {
          await clearAlerts();
          showToast("Алерты очищены", "success");
          await onRefresh?.();
        } catch (e) {
          showToast(e.message || "Не удалось очистить алерты", "danger");
        }
      }
    );
  });

  root.querySelector("#expEventsBtn")?.addEventListener("click", async () => {
    try {
      await exportEventsCsv();
      showToast("События экспортированы", "success");
    } catch (e) {
      showToast(e.message || "Не удалось экспортировать события", "danger");
    }
  });

  root.querySelector("#expAlertsBtn")?.addEventListener("click", async () => {
    try {
      await exportAlertsCsv();
      showToast("Алерты экспортированы", "success");
    } catch (e) {
      showToast(e.message || "Не удалось экспортировать алерты", "danger");
    }
  });

  root.querySelector("#expReportBtn")?.addEventListener("click", async () => {
    try {
      await exportReportJson();
      showToast("Отчет экспортирован", "success");
    } catch (e) {
      showToast(e.message || "Не удалось экспортировать отчет", "danger");
    }
  });

  if (canAccessRules) {
    root.querySelectorAll("[data-toggle-rule]").forEach((el) => {
      el.addEventListener("click", async () => {
        if (!canManage) return;

        const id = el.dataset.toggleRule;
        const enabled = el.dataset.enabled === "true";

        try {
          await toggleRule(id, !enabled);
          showToast(!enabled ? "Правило включено" : "Правило отключено", "success");
          await onRefresh?.();
        } catch (e) {
          showToast(e.message || "Не удалось переключить правило", "danger");
        }
      });
    });

    root.querySelectorAll("[data-edit-rule]").forEach((btn) => {
      btn.addEventListener("click", () => {
        if (!canManage) return;
        const rule = rulesData.find((r) => String(r.id) === String(btn.dataset.editRule));
        if (rule) openRuleModal(rule, onRefresh);
      });
    });

    root.querySelectorAll("[data-del-rule]").forEach((btn) => {
      btn.addEventListener("click", () => {
        if (!canManage) return;

        const id = btn.dataset.delRule;
        const name = btn.dataset.name || "";

        openConfirmModal(
          "Удалить правило?",
          `Правило «${esc(name)}» будет удалено без возможности восстановления.`,
          async () => {
            try {
              await deleteRule(id);
              showToast("Правило удалено", "success");
              await onRefresh?.();
            } catch (e) {
              showToast(e.message || "Не удалось удалить правило", "danger");
            }
          }
        );
      });
    });

    root.querySelector("#addRuleBtn")?.addEventListener("click", () => {
      if (!canManage) return;
      openRuleModal(null, onRefresh);
    });
  }

  return root;
}

function renderRulesList(rules, canManage) {
  if (!Array.isArray(rules) || !rules.length) {
    return `
      <div class="empty-state" style="padding:32px 16px;">
        <div>Правил нет</div>
      </div>
    `;
  }

  return `
    <table class="table rules-table">
      <thead>
        <tr>
          <th style="text-align:left;">Название</th>
          <th style="text-align:center;">Тип</th>
          <th style="text-align:left;">Событие</th>
          <th style="text-align:center;">Порог</th>
          <th style="text-align:center;">Окно (сек)</th>
          <th style="text-align:center;">Угроза</th>
          <th style="text-align:center;">Статус</th>
          ${canManage ? `<th style="text-align:center;">Действия</th>` : ""}
        </tr>
      </thead>
      <tbody>
        ${rules.map((r) => `
          <tr>
            <td>
              <span style="font-weight:600;color:var(--text-primary);">${esc(r.name || "")}</span>
            </td>
            <td style="text-align:center;">
              <span class="badge ${r.ruleType === "threshold" ? "accent" : "warning"}">
                ${esc(r.ruleType || "")}
              </span>
            </td>
            <td style="font-family:monospace;color:var(--text-muted);">
              ${esc(r.matchEventType || "")}
            </td>
            <td style="text-align:center;color:var(--text-secondary);">
              ${Number(r.threshold || 1)}
            </td>
            <td style="text-align:center;color:var(--text-secondary);">
              ${Number(r.windowSeconds || 60)}
            </td>
            <td style="text-align:center;">
              <span class="badge ${sevBadgeClass(r.alertSeverity)}">
                ${esc(r.alertSeverity || "")}
              </span>
            </td>
            <td style="text-align:center;">
              <div
                class="rule-toggle ${r.isEnabled ? "on" : ""}"
                data-toggle-rule="${esc(r.id)}"
                data-enabled="${r.isEnabled ? "true" : "false"}"
                style="${!canManage ? "opacity:.6;pointer-events:none;" : ""}"
              >
                <div class="rule-toggle-knob"></div>
              </div>
            </td>
            ${canManage ? `
              <td style="text-align:center;">
                <div style="display:flex;justify-content:center;gap:6px;">
                  <button class="btn mini ghost" data-edit-rule="${esc(r.id)}">Edit</button>
                  <button class="btn mini danger" data-del-rule="${esc(r.id)}" data-name="${esc(r.name || "")}">Del</button>
                </div>
              </td>
            ` : ""}
          </tr>
        `).join("")}
      </tbody>
    </table>
  `;
}

function openChangePasswordModal({ mustChangePasswordNow = false, onSuccess } = {}) {
  ensureModalHost();

  const modalId = "changePwModal";
  const existing = document.getElementById(modalId);
  if (existing) existing.remove();

  const modal = createModal({
    id: modalId,
    title: "Смена пароля",
    width: "460px",
    isBlocking: mustChangePasswordNow
  });

  setModalBody(modal, `
    <div class="form-grid">
      ${mustChangePasswordNow ? `
        <div class="page-note" style="margin:0 0 12px 0;border-color:var(--warning);color:var(--warning);">
          Вы обязаны сменить пароль перед продолжением работы.
        </div>
      ` : ""}

      <div class="form-group">
        <label class="form-label">ТЕКУЩИЙ ПАРОЛЬ</label>
        <input class="input" type="password" id="pwCurrent" autocomplete="current-password" />
      </div>

      <div class="form-group">
        <label class="form-label">НОВЫЙ ПАРОЛЬ</label>
        <input class="input" type="password" id="pwNew" autocomplete="new-password" />
      </div>

      <div class="form-group">
        <label class="form-label">ПОДТВЕРЖДЕНИЕ</label>
        <input class="input" type="password" id="pwConfirm" autocomplete="new-password" />
      </div>

      <div class="error-box" id="pwError"></div>
    </div>
  `);

  setModalFooter(modal, `
    ${!mustChangePasswordNow ? '<button class="btn ghost" id="pwCancelBtn">Отмена</button>' : ""}
    <button class="btn primary" id="pwSaveBtn" style="${mustChangePasswordNow ? "width:100%;" : ""}">Сменить пароль</button>
  `);

  ensureModalHost().appendChild(modal);

  const errBox = modal.querySelector("#pwError");
  const currentInput = modal.querySelector("#pwCurrent");
  const newInput = modal.querySelector("#pwNew");
  const confirmInput = modal.querySelector("#pwConfirm");
  const saveBtn = modal.querySelector("#pwSaveBtn");
  const cancelBtn = modal.querySelector("#pwCancelBtn");

  if (!mustChangePasswordNow && cancelBtn) {
    cancelBtn.addEventListener("click", () => closeModal(modalId));
  }

  if (mustChangePasswordNow) {
    modal.querySelectorAll("[data-modal-close], .modal-close, .modal-backdrop").forEach((el) => {
      el.remove();
    });

    const stopEscape = (e) => {
      if (e.key === "Escape") {
        e.preventDefault();
        e.stopPropagation();
      }
    };

    document.addEventListener("keydown", stopEscape, true);

    const observer = new MutationObserver(() => {
      if (!document.getElementById(modalId)) {
        document.addEventListener("keydown", stopEscape, true);
        requestAnimationFrame(() => openModal(modalId));
      }
    });

    observer.observe(document.body, { childList: true, subtree: true });

    modal.__cleanupBlocking = () => {
      document.removeEventListener("keydown", stopEscape, true);
      observer.disconnect();
    };
  }

  saveBtn?.addEventListener("click", async () => {
    errBox.classList.remove("visible");
    errBox.textContent = "";

    const currentPassword = currentInput?.value?.trim() || "";
    const newPassword = newInput?.value?.trim() || "";
    const confirmPassword = confirmInput?.value?.trim() || "";

    if (!currentPassword) return setPwErr(errBox, "Введите текущий пароль");
    if (newPassword.length < 6) return setPwErr(errBox, "Новый пароль должен быть не менее 6 символов");
    if (newPassword !== confirmPassword) return setPwErr(errBox, "Пароли должны совпадать");
    if (currentPassword === newPassword) return setPwErr(errBox, "Новый пароль должен отличаться от текущего");

    saveBtn.disabled = true;

    try {
      await changePassword(currentPassword, newPassword);
      showToast("Пароль изменён", "success");
      if (typeof modal.__cleanupBlocking === "function") modal.__cleanupBlocking();
      closeModal(modalId);
      await onSuccess?.();
    } catch (e) {
      setPwErr(errBox, e.message || "Не удалось сменить пароль");
    } finally {
      saveBtn.disabled = false;
    }
  });

  openModal(modalId);
}

function openRuleModal(rule, onRefresh) {
  ensureModalHost();

  const isEdit = !!rule;
  const modalId = isEdit ? "editRuleModal" : "addRuleModal";
  const existing = document.getElementById(modalId);
  if (existing) existing.remove();

  const modal = createModal({
    id: modalId,
    title: isEdit ? "Редактировать правило" : "Новое правило корреляции",
    width: "560px"
  });

  const ruleType = isEdit ? String(rule.ruleType || "threshold") : "threshold";
  const showSecondary = ruleType === "correlation";

  setModalBody(modal, `
    <div class="form-grid">
      <div class="form-group">
        <label class="form-label">НАЗВАНИЕ ПРАВИЛА</label>
        <input class="input" id="rName" value="${esc(isEdit ? rule.name || "" : "")}" />
      </div>

      <div class="form-group">
        <label class="form-label">ТИП ПРАВИЛА</label>
        <select class="input" id="rType">
          <option value="threshold" ${ruleType === "threshold" ? "selected" : ""}>Порог (threshold)</option>
          <option value="correlation" ${ruleType === "correlation" ? "selected" : ""}>Корреляция (correlation)</option>
        </select>
      </div>

      <div class="form-group">
        <label class="form-label">ТИП СОБЫТИЯ</label>
        <input class="input" id="rMatchEvent" value="${esc(isEdit ? rule.matchEventType || "" : "")}" />
      </div>

      <div class="form-group" id="rSecondaryGroup" style="${showSecondary ? "" : "display:none;"}">
        <label class="form-label">ВТОРИЧНОЕ СОБЫТИЕ</label>
        <input class="input" id="rSecondaryEvent" value="${esc(isEdit ? rule.secondaryEventType || "" : "")}" />
      </div>

      <div class="form-row">
        <div class="form-group">
          <label class="form-label">ПОРОГ</label>
          <input class="input" type="number" id="rThreshold" value="${isEdit ? Number(rule.threshold || 1) : 1}" min="1" />
        </div>
        <div class="form-group">
          <label class="form-label">ОКНО (сек)</label>
          <input class="input" type="number" id="rWindow" value="${isEdit ? Number(rule.windowSeconds || 60) : 60}" min="1" />
        </div>
        <div class="form-group">
          <label class="form-label">COOLDOWN (сек)</label>
          <input class="input" type="number" id="rCooldown" value="${isEdit ? Number(rule.cooldownSeconds || 60) : 60}" min="1" />
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
      secondaryEventType: typeSelect.value === "correlation"
        ? modal.querySelector("#rSecondaryEvent").value.trim()
        : "",
      threshold: parseInt(modal.querySelector("#rThreshold").value, 10) || 1,
      windowSeconds: parseInt(modal.querySelector("#rWindow").value, 10) || 60,
      cooldownSeconds: parseInt(modal.querySelector("#rCooldown").value, 10) || 60,
      alertSeverity: modal.querySelector("#rSeverity").value,
      alertTitle: modal.querySelector("#rTitle").value.trim(),
      alertDescription: modal.querySelector("#rDesc").value.trim(),
      isEnabled: isEdit ? !!rule.isEnabled : true
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
      await onRefresh?.();
    } catch (e) {
      showErr(errBox, e.message || "Ошибка сохранения");
    }
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
  return String(v ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}