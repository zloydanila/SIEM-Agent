import { state, subscribe, setState, setUser, setDashboard, clearSession } from "./state.js";
import { isAuthenticated, me, logout, dashboard, users, events, alerts, rules, status } from "./api.js";
import { createSidebar } from "./components/sidebar.js";
import { createHeader } from "./components/header.js";
import { renderLogin } from "./pages/login.js";
import { renderDashboard } from "./pages/dashboard.js";
import { renderEvents } from "./pages/events.js";
import { renderAlerts } from "./pages/alerts.js";
import { renderSettings } from "./pages/settings.js";
import { renderUsers } from "./pages/users.js";
import { renderRules } from "./pages/rules.js";

const pages = { dashboard: renderDashboard, events: renderEvents, alerts: renderAlerts, settings: renderSettings, users: renderUsers, rules: renderRules };

let root = null;
let wsCheckInterval = null;
let dataRefreshInterval = null;
let visibilityHandler = null;
let shellEl = null;
let sidebarEl = null;
let mainEl = null;
let headerEl = null;
let pageContainerEl = null;
let renderQueued = false;
let lastRenderedPage = "";
let lastRenderedVersion = -1;

export function initRouter(selector = "#app") {
  root = document.querySelector(selector);
  if (!root) throw new Error(`Root element not found: ${selector}`);

  subscribe(scheduleRender);

  if (isAuthenticated()) loadUserAndData();
  else renderLoginPage();

  if (!wsCheckInterval) wsCheckInterval = setInterval(pollStatus, 5000);
  if (!dataRefreshInterval) dataRefreshInterval = setInterval(refreshDataSilently, 3000);

  if (!visibilityHandler) {
    visibilityHandler = () => {
      if (!document.hidden && state.isAuthenticated) loadAllData().then(scheduleRender);
    };
    document.addEventListener("visibilitychange", visibilityHandler);
  }
}

function scheduleRender() {
  if (renderQueued) return;
  renderQueued = true;
  requestAnimationFrame(() => {
    renderQueued = false;
    renderCurrentPage();
  });
}

function renderLoginPage() {
  if (!root) return;
  root.innerHTML = "";
  shellEl = sidebarEl = mainEl = headerEl = pageContainerEl = null;
  lastRenderedPage = "";
  lastRenderedVersion = -1;
  renderLogin(root);
}

function ensureShell() {
  if (!root) return;
  if (shellEl && root.contains(shellEl)) return;

  root.innerHTML = "";
  shellEl = document.createElement("div");
  shellEl.className = "shell";

  sidebarEl = document.createElement("div");
  mainEl = document.createElement("div");
  mainEl.className = "main";
  headerEl = document.createElement("div");
  pageContainerEl = document.createElement("div");
  pageContainerEl.className = "page";

  mainEl.appendChild(headerEl);
  mainEl.appendChild(pageContainerEl);
  shellEl.appendChild(sidebarEl);
  shellEl.appendChild(mainEl);
  root.appendChild(shellEl);
}

async function loadUserAndData() {
  try {
    const user = await me();
    setUser(user);
    await loadAllData();
    scheduleRender();
  } catch (_) {
    await logout();
    clearSession();
    renderLoginPage();
  }
}

async function pollStatus() {
  if (!state.isAuthenticated) return;
  try {
    const s = await status();
    setState({
      wsConnected: !!s.wsRunning,
      wsPort: s.wsPort || 8080,
      wsClients: s.wsClients || 0
    });
  } catch (_) {}
}

async function refreshDataSilently() {
  if (!state.isAuthenticated || document.hidden) return;
  try {
    const [eventsData, alertsData, dashData] = await Promise.allSettled([events(), alerts(), dashboard()]);
    if (eventsData.status === "fulfilled") setState({ events: eventsData.value || [] });
    if (alertsData.status === "fulfilled") setState({ alerts: alertsData.value || [] });
    if (dashData.status === "fulfilled") {
      const d = dashData.value || {};
      setDashboard({
        stats: d.stats || {},
        activity: Array.isArray(d.stats?.activity) ? d.stats.activity : [],
        topDevices: Array.isArray(d.stats?.topDevices) ? d.stats.topDevices : [],
        recentEvents: Array.isArray(d.recentEvents) ? d.recentEvents : [],
        alerts: Array.isArray(d.alerts) ? d.alerts : []
      });
    }
  } catch (_) {}
}

export async function loadAllData() {
  try {
    const [dashData, usersData, eventsData, alertsData, rulesData, statusData] = await Promise.allSettled([
      dashboard(), users(), events(), alerts(), rules(), status()
    ]);

    if (dashData.status === "fulfilled") {
      const d = dashData.value || {};
      setDashboard({
        stats: d.stats || {},
        activity: Array.isArray(d.stats?.activity) ? d.stats.activity : [],
        topDevices: Array.isArray(d.stats?.topDevices) ? d.stats.topDevices : [],
        recentEvents: Array.isArray(d.recentEvents) ? d.recentEvents : [],
        alerts: Array.isArray(d.alerts) ? d.alerts : []
      });
    }

    if (usersData.status === "fulfilled") setState({ users: usersData.value || [] });
    if (eventsData.status === "fulfilled") setState({ events: eventsData.value || [] });
    if (alertsData.status === "fulfilled") setState({ alerts: alertsData.value || [] });
    if (rulesData.status === "fulfilled") setState({ rules: rulesData.value || [] });
    if (statusData.status === "fulfilled") {
      setState({
        wsConnected: !!statusData.value?.wsRunning,
        wsPort: statusData.value?.wsPort || 8080,
        wsClients: statusData.value?.wsClients || 0
      });
    }
  } catch (e) {
    console.error("Data load error:", e);
  }
}

export function navigate(page) {
  if (!pages[page]) return;
  if (state.page !== page) {
    state.page = page;
    scheduleRender();
  }
}

function renderCurrentPage() {
  if (!root) return;
  if (!state.isAuthenticated) {
    renderLoginPage();
    return;
  }

  ensureShell();

  const user = state.currentUser || {};
  const rawRole = user.role || user.user?.role || "";
  const userRole = String(rawRole).toLowerCase().trim();
  const canManage = userRole === "admin" || userRole === "operator";

  if (sidebarEl) {
    sidebarEl.replaceWith(createSidebar({ active: state.page, currentUser: user, onNavigate: navigate }));
    sidebarEl = shellEl.firstChild;
  }

  if (headerEl) {
    headerEl.replaceWith(createHeader({
      currentUserName: user.fullName || user.username || "admin",
      currentUserRole: userRole,
      wsConnected: state.wsConnected,
      canManage,
      onRefresh: loadAllData,
      onLogout: async () => {
        await logout();
        clearSession();
        renderLoginPage();
      }
    }));
    headerEl = mainEl.firstChild;
  }

  const key = `${state.page}:${state.dataVersion}`;
  if (key === lastRenderedPage) return;
  lastRenderedPage = key;
  lastRenderedVersion = state.dataVersion;
  patchCurrentPage();
}

function patchCurrentPage() {
  if (!pageContainerEl) return;

  const user = state.currentUser || {};
  const rawRole = user.role || user.user?.role || "";
  const userRole = String(rawRole).toLowerCase().trim();
  const canManage = userRole === "admin" || userRole === "operator";
  const isAdmin = userRole === "admin";

  const pageFn = pages[state.page] || renderDashboard;
  const props = buildPageProps(state.page, { user, canManage, isAdmin, userRole });

  Promise.resolve(pageFn(props)).then(pageNode => {
    pageContainerEl.innerHTML = "";
    if (pageNode instanceof HTMLElement) pageContainerEl.appendChild(pageNode);
    else pageContainerEl.innerHTML = pageNode || "";
  });
}

function buildPageProps(page, { user, canManage, isAdmin, userRole }) {
  const base = {
    currentUser: user,
    canManage,
    isAdmin,
    userRole,
    onRefresh: loadAllData,
    onNavigate: navigate
  };

  switch (page) {
    case "dashboard":
      return {
        ...base,
        stats: state.dashboard?.stats || {},
        activity: state.dashboard?.activity || [],
        topDevices: state.dashboard?.topDevices || [],
        users: state.users || [],
        events: state.events || [],
        alerts: state.alerts || []
      };
    case "events":
      return { ...base, events: state.events || [] };
    case "alerts":
      return { ...base, alerts: state.alerts || [], alertsFilter: state.alertsFilter || "" };
    case "users":
      return { ...base, users: state.users || [] };
    case "rules":
      return { ...base, rules: state.rules || [] };
    case "settings":
      return {
        ...base,
        mustChangePassword: state.mustChangePassword,
        wsConnected: state.wsConnected,
        wsPort: state.wsPort,
        wsClients: state.wsClients,
        rules: state.rules || []
      };
    default:
      return base;
  }
}

document.addEventListener("DOMContentLoaded", () => initRouter("#app"));