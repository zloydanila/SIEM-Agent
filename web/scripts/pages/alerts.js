import { alerts as loadAlerts, updateAlertStatus } from "../api.js";
import { showToast } from "../components/toast.js";
import { state, setAlertsFilter } from "../state.js";

export async function renderAlerts({
  alerts: initialAlerts = [],
  currentUser = {},
  canUpdateAlertStatus = false,
  onRefresh,
  alertsFilter = ""
} = {}) {
  const root = document.createElement("div");
  root.className = "page-inner";

  let data = Array.isArray(initialAlerts) ? initialAlerts : [];
  let error = null;
  let activeFilter = alertsFilter || state.alertsFilter || "";

  if (!data.length) {
    try {
      data = await loadAlerts();
    } catch (e) {
      error = e.message;
    }
  }

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div>
          <h2>Алерты</h2>
          <p id="alertsCount">Всего: ${data.length}</p>
        </div>

        <div class="page-actions">
          ${!canUpdateAlertStatus ? `<div class="role-badge">Только просмотр</div>` : ""}
          <div class="filter-pills">
            <button class="filter-pill ${activeFilter === "" ? "active" : ""}" data-filter="">Все</button>
            <button class="filter-pill ${activeFilter === "open" ? "active" : ""}" data-filter="open">Открытые</button>
            <button class="filter-pill ${activeFilter === "investigating" ? "active" : ""}" data-filter="investigating">Изучаются</button>
            <button class="filter-pill ${activeFilter === "closed" ? "active" : ""}" data-filter="closed">Закрытые</button>
          </div>
        </div>
      </div>

      ${error ? `<div class="page-note" style="border-color:var(--danger);color:var(--danger);">Ошибка: ${esc(error)}</div>` : ""}

      <div id="alertsContainer" class="content-gap"></div>
    </section>
  `;

  const container = root.querySelector("#alertsContainer");

  function filtered() {
    return activeFilter
      ? data.filter(a => String(a.status || "").toLowerCase() === activeFilter)
      : data;
  }

  function renderContent() {
    const list = filtered();

    container.innerHTML = list.length
      ? list.map(al => {
          const status = String(al.status || "").toLowerCase();

          const actions = canUpdateAlertStatus
            ? status === "open"
              ? `
                <div class="alert-card-actions">
                  <button class="alert-action-btn warning" data-id="${al.id}" data-action="investigate">Взять в работу</button>
                  <button class="alert-action-btn success" data-id="${al.id}" data-action="close">Закрыть</button>
                </div>
              `
              : status === "investigating"
                ? `
                  <div class="alert-card-actions">
                    <button class="alert-action-btn success" data-id="${al.id}" data-action="close">Закрыть</button>
                  </div>
                `
                : ""
            : "";

          return `
            <div class="alert-card">
              <div class="alert-severity-bar" style="background:${sevColor(al.severity)};"></div>
              <div class="alert-card-content">
                <div class="alert-card-header">
                  <span class="alert-card-title">${esc(al.title || al.message || "")}</span>
                  <span class="badge ${sevBadgeClass(al.severity)}">${sevLabel(al.severity)}</span>
                  <span class="badge ${statusBadgeClass(al.status)}">${statusLabel(al.status)}</span>
                </div>

                <div class="alert-card-desc">${esc(al.description || "")}</div>

                <div class="alert-card-meta">
                  <span>Устройство: ${esc(al.deviceName || al.device || "—")}</span>
                  <span>Время: ${esc(formatTime(al.triggeredAt || al.time || ""))}</span>
                  ${actions}
                </div>
              </div>
            </div>
          `;
        }).join("")
      : `<div class="empty-state"><div class="empty-state-title">${activeFilter ? "Нет алертов с таким статусом" : "Алертов нет"}</div></div>`;

    root.querySelector("#alertsCount").textContent =
      `Всего: ${data.length}${activeFilter ? " · показано: " + list.length : ""}`;
  }

  root.querySelectorAll("[data-filter]").forEach(btn => {
    btn.addEventListener("click", () => {
      activeFilter = btn.dataset.filter;
      setAlertsFilter(activeFilter);

      root.querySelectorAll("[data-filter]").forEach(b => {
        b.classList.toggle("active", b.dataset.filter === activeFilter);
      });

      renderContent();
    });
  });

  container.addEventListener("click", e => {
    const btn = e.target.closest("[data-action]");
    if (!btn || !canUpdateAlertStatus) return;

    const id = btn.dataset.id;
    const action = btn.dataset.action;
    const newStatus = action === "investigate" ? "investigating" : "closed";

    (async () => {
      try {
        await updateAlertStatus(id, newStatus);
        showToast(action === "close" ? "Алерт закрыт" : "Алерт взят в работу", "success");
        await onRefresh?.();
      } catch (err) {
        showToast(err.message, "danger");
      }
    })();
  });

  renderContent();
  return root;
}

function sevColor(s) {
  const v = String(s || "").toLowerCase();
  if (v === "critical") return "#f85149";
  if (v === "high") return "#d29922";
  if (v === "medium") return "#58a6ff";
  return "#3fb950";
}

function sevBadgeClass(s) {
  const v = String(s || "").toLowerCase();
  if (v === "critical") return "danger";
  if (v === "high") return "warning";
  if (v === "medium") return "accent";
  return "success";
}

function sevLabel(s) {
  const v = String(s || "").toLowerCase();
  if (v === "critical") return "КРИТИЧНО";
  if (v === "high") return "ВЫСОКИЙ";
  if (v === "medium") return "СРЕДНИЙ";
  return "НИЗКИЙ";
}

function statusBadgeClass(s) {
  const v = String(s || "").toLowerCase();
  if (v === "open") return "warning";
  if (v === "investigating") return "accent";
  return "success";
}

function statusLabel(s) {
  const v = String(s || "").toLowerCase();
  if (v === "open") return "Открыт";
  if (v === "investigating") return "Изучается";
  if (v === "closed") return "Закрыт";
  return s || "";
}

function formatTime(ts) {
  return ts ? String(ts).replace("T", " ").substring(0, 19) : "";
}

function esc(v) {
  return String(v ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}