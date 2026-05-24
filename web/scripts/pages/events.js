const EVENTS_PER_PAGE = 15;

// Чистая рендер-функция — данные только из пропсов, никаких fetch внутри
export function renderEvents({ events: initialEvents = [], onRefresh } = {}) {
  const root = document.createElement("div");
  root.className = "page-inner";
  const data = Array.isArray(initialEvents) ? initialEvents : [];
  let currentPage = 1;
  const totalPages = Math.ceil(data.length / EVENTS_PER_PAGE);

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div>
          <h2>События</h2>
          <p>Всего: ${data.length}</p>
        </div>
        <div class="page-actions">
          <button class="btn primary" id="eventsRefreshBtn">Обновить</button>
        </div>
      </div>
      <div class="card slide-up">
        <div class="card-body" style="padding:0;">
          <div id="eventsContent"></div>
        </div>
      </div>
      ${totalPages > 1 ? `
        <div id="eventsPagination" style="display:flex;justify-content:center;align-items:center;gap:12px;margin-top:16px;">
          <button class="btn ghost" id="eventsPrevBtn">← Назад</button>
          <span id="eventsPageInfo" style="color:var(--text-muted);font-size:14px;"></span>
          <button class="btn ghost" id="eventsNextBtn">Далее →</button>
        </div>
      ` : ""}
    </section>
  `;

  function renderPaginatedEvents(page) {
    currentPage = Math.max(1, Math.min(page, totalPages || 1));
    const start = (currentPage - 1) * EVENTS_PER_PAGE;
    const pageData = data.slice(start, start + EVENTS_PER_PAGE);
    const contentDiv = root.querySelector("#eventsContent");
    if (!contentDiv) return;

    contentDiv.innerHTML = pageData.length
      ? pageData.map(ev => `
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
        `).join("")
      : `<div class="empty-state">
           <div class="empty-state-title">Событий пока нет</div>
           <div>Запустите simulator.py для генерации событий</div>
         </div>`;

    updatePaginationControls();
  }

  function updatePaginationControls() {
    const paginationDiv = root.querySelector("#eventsPagination");
    if (!paginationDiv || totalPages <= 1) {
      if (paginationDiv) paginationDiv.style.display = "none";
      return;
    }
    paginationDiv.style.display = "flex";
    const prevBtn = paginationDiv.querySelector("#eventsPrevBtn");
    const nextBtn = paginationDiv.querySelector("#eventsNextBtn");
    const pageInfo = paginationDiv.querySelector("#eventsPageInfo");
    prevBtn.disabled = currentPage === 1;
    nextBtn.disabled = currentPage === totalPages;
    pageInfo.textContent = `Страница ${currentPage} из ${totalPages}`;
    prevBtn.onclick = () => renderPaginatedEvents(currentPage - 1);
    nextBtn.onclick = () => renderPaginatedEvents(currentPage + 1);
  }

  renderPaginatedEvents(1);

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
  return String(v ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}