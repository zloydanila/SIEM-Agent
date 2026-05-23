import { logout } from "../api.js";

const MENU_ITEMS = [
  { id: "dashboard", icon: "D", label: "Дашборд" },
  { id: "events", icon: "E", label: "События" },
  { id: "alerts", icon: "A", label: "Алерты" },
  { id: "rules", icon: "R", label: "Правила" },
  { id: "users", icon: "U", label: "Пользователи" },
  { id: "settings", icon: "S", label: "Настройки" }
];

export function createSidebar({ active = "dashboard", currentUser = null, onNavigate } = {}) {
  const sidebar = document.createElement("aside");
  sidebar.className = "sidebar";

  const userName = currentUser?.fullName || currentUser?.username || "Пользователь";
  const userRole = String(currentUser?.role || "").toLowerCase().trim();
  const firstLetter = (userName.charAt(0) || "U").toUpperCase();

  const allowedItems = MENU_ITEMS.filter(item => {
    if (userRole === "viewer") {
      return !["users", "rules"].includes(item.id);
    }
    return true;
  });

  sidebar.innerHTML = `
    <div class="sidebar-logo">
      <div class="sidebar-logo-icon">S</div>
      <div class="sidebar-logo-text">
        <div class="sidebar-logo-title">SIEM Agent</div>
        <div class="sidebar-logo-sub">Industrial Security</div>
      </div>
    </div>

    <nav class="sidebar-nav">
      ${allowedItems.map(item => `
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