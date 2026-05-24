import { logout } from "../api.js";

const MENU_ITEMS = [
  { id: "dashboard", label: "Дашборд" },
  { id: "events", label: "События" },
  { id: "alerts", label: "Алерты" },
  { id: "rules", label: "Правила" },
  { id: "settings", label: "Настройки" }
];

function getUserRole(user) {
  return String(user?.role || user?.user?.role || "").toLowerCase().trim();
}

function roleLabel(role) {
  if (role === "admin") return "Администратор";
  if (role === "operator") return "Оператор";
  if (role === "viewer") return "Наблюдатель";
  return "Пользователь";
}

function canAccessItem(role, itemId) {
  if (role === "admin") return true;
  if (role === "operator" || role === "viewer") {
    return !["users", "rules"].includes(itemId);
  }
  return !["users", "rules"].includes(itemId);
}

export function createSidebar({ active = "dashboard", currentUser = null, onNavigate } = {}) {
  const sidebar = document.createElement("aside");
  sidebar.className = "sidebar";

  const userName = currentUser?.fullName || currentUser?.username || "Пользователь";
  const userRole = getUserRole(currentUser);
  const firstLetter = (userName.charAt(0) || "U").toUpperCase();

  const allowedItems = MENU_ITEMS.filter(item => canAccessItem(userRole, item.id));

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
          <span class="sidebar-item-label">${item.label}</span>
        </button>
      `).join("")}
    </nav>

    <div class="sidebar-footer">
      <div class="sidebar-avatar">${firstLetter}</div>
      <div class="sidebar-user-info">
        <div class="sidebar-user-name">${esc(userName)}</div>
        <div class="sidebar-user-role">${esc(roleLabel(userRole))}</div>
      </div>
      <button class="sidebar-logout" id="sidebarLogout" title="Выход">→</button>
    </div>
  `;

  sidebar.querySelectorAll("[data-nav]").forEach(btn => {
    btn.addEventListener("click", () => onNavigate?.(btn.dataset.nav));
  });

  sidebar.querySelector("#sidebarLogout")?.addEventListener("click", async () => {
    await logout().catch(() => {});
    window.location.reload();
  });

  return sidebar;
}

function esc(v) {
  return String(v || "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}