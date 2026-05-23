import { events as loadEvents } from "../api.js";
import { showToast } from "../components/toast.js";

export async function renderEvents({ events: initialEvents = [], onRefresh } = {}) {
  const root = document.createElement("div");
  root.className = "page-inner";

  let data = initialEvents;
  let error = null;

  if (!data.length) {
    try {
      data = await loadEvents();
    } catch (e) {
      error = e.message;
    }
  }

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div>
          <h2>События</h2>
          <p>Загружено: ${data.length}</p>
        </div>
        <div class="page-actions">
          <button class="btn primary" id="eventsRefreshBtn">Обновить</button>
        </div>
      </div>

      ${error ? `<div class="page-note" style="border-color:var(--danger);color:var(--danger);">Ошибка: ${esc(error)}</div>` : ""}

      <div class="card slide-up">
        <div class="card-body" style="padding:0;">
          ${data.length ? data.map(ev => `
            <div class="alert-card">
              <div class="alert-severity-bar" style="background:${sevColor(ev.severity)};"></div>
              <div class="alert-card-content">
                <div class="alert-card-header">
                  <span style="font-weight:700;color:var(--text-primary);">${esc(ev.deviceName || ev.device || "")}</span>
                  <span style="color:var(--text-muted);">—</span>
                  <span style="color:var(--text-primary);flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;">${esc(ev.action || ev.message || "")}</span>
                  <span class="badge ${sevBadgeClass(ev.severity)}">${sevLabel(ev.severity)}</span>
                </div>
                <div class="alert-card-meta">
                  <span>Место: ${esc(ev.location || "—")}</span>
                  <span>Время: ${esc(formatTime(ev.timestamp || ev.time || ""))}</span>
                </div>
                <div class="alert-card-desc" style="font-style:italic;">${esc(ev.rawLog || "")}</div>
              </div>
            </div>
          `).join("") : `
            <div class="empty-state">
              <div class="empty-state-title">Событий пока нет</div>
              <div>Запустите simulator.py для генерации событий</div>
            </div>
          `}
        </div>
      </div>
    </section>
  `;

  root.querySelector("#eventsRefreshBtn")?.addEventListener("click", () => onRefresh?.());
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

function formatTime(ts) {
  if (!ts) return "";
  return String(ts).replace("T", " ").substring(0, 19);
}

function esc(v) {
  return String(v ?? "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;");
}