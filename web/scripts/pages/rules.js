import { rules as loadRules, createRule, updateRule, deleteRule, toggleRule } from "../api.js";
import { showToast } from "../components/toast.js";
import {
  createModal,
  openModal,
  closeModal,
  setModalBody,
  setModalFooter,
  ensureModalHost,
  openConfirmModal
} from "../components/modal.js";

export async function renderRules({
  rules: initialRules = [],
  canAccessRules = false,
  canManage = false,
  onRefresh
} = {}) {
  const root = document.createElement("div");
  root.className = "page-inner";

  if (!canAccessRules) {
    root.innerHTML = `
      <section class="page-section">
        <div class="page-hero">
          <div>
            <h2>Правила корреляции</h2>
            <p>Раздел недоступен для вашей роли</p>
          </div>
        </div>
        <div class="page-note" style="border-color:var(--warning);color:var(--warning);">
          Просмотр и управление правилами корреляции доступны только администратору.
        </div>
      </section>
    `;
    return root;
  }

  let data = Array.isArray(initialRules) ? initialRules : [];
  let error = null;

  if (!data.length) {
    try {
      data = await loadRules();
    } catch (e) {
      error = e.message;
    }
  }

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div><h2>Правила корреляции</h2><p>${data.length} правил</p></div>
        <div class="page-actions">
          <button class="btn ghost" id="rulesRefreshBtn">Обновить</button>
          ${canManage ? `<button class="btn primary" id="addRuleBtn">+ Добавить правило</button>` : ""}
        </div>
      </div>

      ${error ? `<div class="page-note" style="border-color:var(--danger);color:var(--danger);">Ошибка: ${esc(error)}</div>` : ""}

      <div class="card slide-up">
        <div class="card-body table-wrap" style="padding:0;">
          <table class="table">
            <thead>
              <tr>
                <th style="width:24px;"></th>
                <th style="width:18%;">Название</th>
                <th style="width:12%;">Тип</th>
                <th style="width:14%;">Событие</th>
                <th style="width:8%;">Порог</th>
                <th style="width:8%;">Окно</th>
                <th style="width:12%;">Уровень</th>
                <th style="width:18%;">Заголовок</th>
                ${canManage ? `<th style="width:10%;">Действия</th>` : ""}
              </tr>
            </thead>
            <tbody>
              ${data.length ? data.map(r => `
                <tr>
                  <td>
                    <div
                      class="rule-toggle ${r.isEnabled ? "on" : ""} ${!canManage ? "disabled" : ""}"
                      ${canManage ? `data-toggle="${r.id}"` : ""}
                      title="${r.isEnabled ? "Включено" : "Отключено"}"
                      style="${!canManage ? "opacity:.6;pointer-events:none;" : ""}"
                    >
                      <div class="rule-toggle-knob"></div>
                    </div>
                  </td>
                  <td style="font-weight:600;color:${r.isEnabled ? "var(--text-primary)" : "var(--text-muted)"};">${esc(r.name || "")}</td>
                  <td><span class="badge ${r.ruleType === "threshold" ? "accent" : "warning"}">${esc(r.ruleType || "")}</span></td>
                  <td style="color:var(--text-muted);font-family:monospace;font-size:var(--font-xs);">${esc(r.matchEventType || "")}</td>
                  <td style="text-align:center;">${esc(String(r.threshold ?? ""))}</td>
                  <td style="text-align:center;">${esc(String(r.windowSeconds ?? ""))}</td>
                  <td><span class="badge ${sevBadgeClass(r.alertSeverity)}">${esc(r.alertSeverity || "")}</span></td>
                  <td style="color:var(--text-muted);">${esc(r.alertTitle || "")}</td>
                  ${canManage ? `<td><div style="display:flex;gap:6px;"><button class="btn mini ghost" data-edit="${r.id}">Edit</button><button class="btn mini danger" data-del="${r.id}" data-name="${esc(r.name || "")}">Del</button></div></td>` : ""}
                </tr>
              `).join("") : `<tr><td colspan="${canManage ? 9 : 8}" style="text-align:center;padding:20px;color:var(--text-muted);">Правил нет</td></tr>`}
            </tbody>
          </table>
        </div>
      </div>
    </section>
  `;

  if (canManage) {
    root.querySelectorAll("[data-toggle]").forEach(el => {
      el.addEventListener("click", async () => {
        const id = el.dataset.toggle;
        const enabled = !el.classList.contains("on");
        try {
          await toggleRule(id, enabled);
          showToast(enabled ? "Правило включено" : "Правило отключено", "success");
          await onRefresh?.();
        } catch (e) {
          showToast(e.message, "danger");
        }
      });
    });

    root.querySelectorAll("[data-edit]").forEach(btn => {
      btn.addEventListener("click", () => {
        const rule = data.find(r => String(r.id) === btn.dataset.edit);
        if (rule) openRuleModal(rule, onRefresh);
      });
    });

    root.querySelectorAll("[data-del]").forEach(btn => {
      btn.addEventListener("click", () => {
        const id = btn.dataset.del;
        const name = btn.dataset.name;

        openConfirmModal(
          "Удалить правило?",
          `Удалить правило «${name}»? Это действие необратимо.`,
          async () => {
            try {
              await deleteRule(id);
              showToast("Правило удалено", "success");
              await onRefresh?.();
            } catch (e) {
              showToast(e.message, "danger");
            }
          }
        );
      });
    });

    root.querySelector("#addRuleBtn")?.addEventListener("click", () => openRuleModal(null, onRefresh));
  }

  root.querySelector("#rulesRefreshBtn")?.addEventListener("click", async () => {
    await onRefresh?.();
  });

  return root;
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
      await onRefresh?.();
    } catch (e) {
      showErr(errBox, e.message);
    }
  });

  openModal(modalId);
}

function showErr(el, text) {
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