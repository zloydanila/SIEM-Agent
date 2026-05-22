export function createHeader({
  currentUserName = "admin",
  currentUserRole = "viewer",
  wsConnected = false,
  canManage = false,
  onRefresh,
  onLogout
} = {}) {
  const header = document.createElement("div");
  header.className = "main-header";

  const roleUpper = String(currentUserRole).toUpperCase();

  header.innerHTML = `
    <div class="header-brand">
      <div class="header-brand-icon">S</div>
      <div class="header-brand-text">
        <div class="header-brand-title">SIEM Dashboard</div>
        <div class="header-brand-sub">${esc(currentUserName)}  |  ${esc(roleUpper)}</div>
      </div>
    </div>

    <div class="header-status">
      <div class="status-dot ${wsConnected ? "online" : "offline"}"></div>
      <div class="status-text ${wsConnected ? "online" : "offline"}">${wsConnected ? "Online" : "Offline"}</div>
    </div>

    <div class="header-actions">
      ${!canManage ? `<div class="role-badge">Только просмотр</div>` : ""}
      <button class="btn ghost" id="headerRefresh">Refresh</button>
      <button class="btn danger" id="headerLogout">Logout</button>
    </div>
  `;

  header.querySelector("#headerRefresh")?.addEventListener("click", () => onRefresh?.());
  header.querySelector("#headerLogout")?.addEventListener("click", () => onLogout?.());

  return header;
}

function esc(v) {
  return String(v || "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;");
}