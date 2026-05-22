import { logout } from "../api.js";

const MENU_ITEMS = [
  { id: "dashboard", icon: "", label: "Дашборд" },
  { id: "events", icon: "", label: "События" },
  { id: "alerts", icon: "", label: "Алерты" },
  { id: "rules", icon: "", label: "Правила" },
  { id: "settings", icon: "", label: "Настройки" }
];

export function createSidebar({ active = "dashboard", currentUser = null, onNavigate } = {}) {
  const sidebar = document.createElement("aside");
  sidebar.className = "sidebar";

  const userName = currentUser?.fullName || currentUser?.username || "Пользователь";
  const userRole = String(currentUser?.role || "");
  const firstLetter = (userName.charAt(0) || "U").toUpperCase();

  sidebar.innerHTML = `
    <div class="sidebar-logo">
      <div class="sidebar-logo-icon">S</div>
      <div class="sidebar-logo-text">
        <div class="sidebar-logo-title">SIEM Agent</div>
        <div class="sidebar-logo-sub">Industrial Security</div>
      </div>
    </div>

    <nav class="sidebar-nav">
      ${MENU_ITEMS.map(item => `
        <button class="sidebar-item ${active === item.id ? "active" : ""}" data-nav="${item.id}">
          <span class="sidebar-item-icon">${item.icon}</span>
          <span>${item.label}</span>
        </button>
      `).join("")}
    </nav>

    <div class="sidebar-footer">
      <div class="sidebar-avatar">${firstLetter}</div>
      <div class="sidebar-user-info">
        <div class="sidebar-user-name">${esc(userName)}</div>
        <div class="sidebar-user-role">${esc(userRole)}</div>
      </div>
      <button class="sidebar-logout" id="sidebarLogout" title="Выход">→</button>
    </div>
  `;

  sidebar.querySelectorAll("[data-nav]").forEach(btn => {
    btn.addEventListener("click", () => onNavigate?.(btn.dataset.nav));
  });

  sidebar.querySelector("#sidebarLogout")?.addEventListener("click", async () => {
    await logout();
    window.location.reload();
  });

  return sidebar;
}

function esc(v) {
  return String(v || "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;");
}