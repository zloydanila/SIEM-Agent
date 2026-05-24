import { state, subscribe, setState, setUser, setDashboard, clearSession } from "./state.js";
import { isAuthenticated, me, logout, dashboard, events, alerts, rules, users, status } from "./api.js";
import { createSidebar } from "./components/sidebar.js";
import { renderLogin } from "./pages/login.js";
import { renderDashboard } from "./pages/dashboard.js";
import { renderEvents } from "./pages/events.js";
import { renderAlerts } from "./pages/alerts.js";
import { renderSettings } from "./pages/settings.js";
import { renderRules } from "./pages/rules.js";

const pages = {
  dashboard: renderDashboard,
  events: renderEvents,
  alerts: renderAlerts,
  settings: renderSettings,
  rules: renderRules
};

let root = null;
let shellEl = null;
let sidebarEl = null;
let mainEl = null;
let pageContainerEl = null;
let renderQueued = false;
let lastRenderedPage = "";
let wsSocket = null;
let wsReconnectTimer = null;
let currentWsUrl = "";
let lastWsUpdateTime = 0;
const WS_UPDATE_THROTTLE_MS = 500;

function getUserRole(user = state.currentUser) {
  return String(user?.role || user?.user?.role || "").toLowerCase().trim();
}
function isAdmin(user = state.currentUser) { return getUserRole(user) === "admin"; }
function isOperator(user = state.currentUser) { return getUserRole(user) === "operator"; }
function isViewer(user = state.currentUser) { return getUserRole(user) === "viewer"; }
function canAccessRules(user = state.currentUser) { return isAdmin(user); }
function canClearData(user = state.currentUser) { return isAdmin(user); }
function canUpdateAlertStatus(user = state.currentUser) { return isAdmin(user) || isOperator(user); }

function normalizePage(page) {
  if (!pages[page]) return "dashboard";
  if (page === "rules" && !canAccessRules()) return "dashboard";
  return page;
}

function normalizeEventsPayload(data) {
  if (Array.isArray(data)) return data;
  if (Array.isArray(data?.events)) return data.events;
  return [];
}

function normalizeAlertsPayload(data) {
  if (Array.isArray(data)) return data;
  if (Array.isArray(data?.alerts)) return data.alerts;
  return [];
}

export function initRouter(selector = "#app") {
  root = document.querySelector(selector);
  if (!root) throw new Error(`Root element not found: ${selector}`);

  subscribe(scheduleRender);
  subscribe(handleWebSocketStateChange);

  if (isAuthenticated()) loadUserAndData();
  else renderLoginPage();
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
  shellEl = sidebarEl = mainEl = pageContainerEl = null;
  lastRenderedPage = "";
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
  pageContainerEl = document.createElement("div");
  pageContainerEl.className = "page";
  mainEl.appendChild(pageContainerEl);
  shellEl.appendChild(sidebarEl);
  shellEl.appendChild(mainEl);
  root.appendChild(shellEl);
}

async function loadUserAndData() {
  try {
    const meResponse = await me();
    const actualUser = meResponse?.user || meResponse;
    setUser(actualUser);
    setState({ mustChangePassword: !!actualUser?.mustChangePassword });
    await loadAllData();
    scheduleRender();
  } catch (_) {
    await logout().catch(() => {});
    clearSession();
    renderLoginPage();
  }
}

function getWebSocketUrl() {
  const protocol = state.wsSecure ? "wss:" : "ws:";
  const host = window.location.hostname || "127.0.0.1";
  const port = state.wsPort || 8081;
  return `${protocol}//${host}:${port}`;
}

function closeWebSocket() {
  if (wsSocket) {
    wsSocket.removeEventListener("open", handleWebSocketOpen);
    wsSocket.removeEventListener("message", handleWebSocketMessage);
    wsSocket.removeEventListener("close", handleWebSocketClose);
    wsSocket.removeEventListener("error", handleWebSocketError);
    wsSocket.close();
    wsSocket = null;
  }
  if (wsReconnectTimer) {
    clearTimeout(wsReconnectTimer);
    wsReconnectTimer = null;
  }
  currentWsUrl = "";
}

function scheduleWebSocketReconnect() {
  if (wsReconnectTimer) return;
  wsReconnectTimer = setTimeout(() => {
    wsReconnectTimer = null;
    if (state.isAuthenticated && state.wsConnected) createWebSocket();
  }, 2500);
}

function createWebSocket() {
  if (!state.isAuthenticated || !state.wsConnected) {
    closeWebSocket();
    return;
  }
  const url = getWebSocketUrl();
  if (
    wsSocket &&
    currentWsUrl === url &&
    (wsSocket.readyState === WebSocket.OPEN || wsSocket.readyState === WebSocket.CONNECTING)
  ) return;

  closeWebSocket();
  currentWsUrl = url;

  try {
    wsSocket = new WebSocket(url);
  } catch (_) {
    scheduleWebSocketReconnect();
    return;
  }

  wsSocket.addEventListener("open", handleWebSocketOpen);
  wsSocket.addEventListener("message", handleWebSocketMessage);
  wsSocket.addEventListener("close", handleWebSocketClose);
  wsSocket.addEventListener("error", handleWebSocketError);
}

function handleWebSocketOpen() { console.debug("WebSocket connected", currentWsUrl); }
function handleWebSocketError() { console.debug("WebSocket error", currentWsUrl); }
function handleWebSocketClose() {
  wsSocket = null;
  scheduleWebSocketReconnect();
}

function prependUniqueById(list, item, idKey = "id") {
  const id = item?.[idKey];
  const filtered = (list || []).filter((x) => !id || x?.[idKey] !== id);
  return [item, ...filtered];
}

function handleWebSocketMessage(event) {
  let data;
  try { data = JSON.parse(event.data); } catch (_) { return; }
  if (!data || typeof data !== "object") return;

  const type = String(data.type || "").toLowerCase();
  const payload = data.payload || data;
  const now = Date.now();
  if (now - lastWsUpdateTime < WS_UPDATE_THROTTLE_MS) return;
  lastWsUpdateTime = now;

  if (type === "event" || (payload?.eventType && payload?.deviceName)) {
    const eventItem = { ...payload };
    setState({ events: prependUniqueById(state.events, eventItem).slice(0, 500) });
    setDashboard({
      stats: updatedDashboardStatsForEvent(eventItem),
      recentEvents: prependUniqueById(state.dashboard?.recentEvents || [], eventItem, "id").slice(0, 10)
    });
    return;
  }

  if (type === "alert" || (payload?.title && payload?.status)) {
    const alertItem = { ...payload };
    const stats = { ...(state.dashboard?.stats || {}) };
    stats.totalAlerts = (Number(stats.totalAlerts) || 0) + 1;
    if (String(alertItem.status || "").toLowerCase() === "open") {
      stats.openAlerts = (Number(stats.openAlerts) || 0) + 1;
    }
    setState({ alerts: prependUniqueById(state.alerts, alertItem).slice(0, 500) });
    setDashboard({
      stats,
      alerts: prependUniqueById(state.dashboard?.alerts || [], alertItem, "id").slice(0, 20)
    });
  }
}

function updatedDashboardStatsForEvent(eventItem) {
  const stats = { ...(state.dashboard?.stats || {}) };
  stats.totalEvents = (Number(stats.totalEvents) || 0) + 1;
  const severity = String(eventItem.severity || "").toLowerCase();
  if (severity === "critical") stats.criticalCount = (Number(stats.criticalCount) || 0) + 1;
  else if (severity === "high") stats.highCount = (Number(stats.highCount) || 0) + 1;
  else if (severity === "medium") stats.mediumCount = (Number(stats.mediumCount) || 0) + 1;
  else if (severity === "low") stats.lowCount = (Number(stats.lowCount) || 0) + 1;
  return stats;
}

function handleWebSocketStateChange(currentState) {
  if (!currentState.isAuthenticated || !currentState.wsConnected) {
    closeWebSocket();
    return;
  }
  createWebSocket();
}

export async function loadAllData() {
  const admin = isAdmin();
  const tasks = [
    dashboard().then(data => ({ key: "dashboard", data })),
    events().then(data => ({ key: "events", data })),
    alerts().then(data => ({ key: "alerts", data })),
    status().then(data => ({ key: "status", data }))
  ];

  if (admin) {
    tasks.push(users().then(data => ({ key: "users", data })));
    tasks.push(rules().then(data => ({ key: "rules", data })));
  }

  const results = await Promise.allSettled(tasks);
  for (const result of results) {
    if (result.status !== "fulfilled") continue;
    const { key, data } = result.value;

    if (key === "dashboard") {
      const d = data || {};
      const stats = d.stats || {};
      setDashboard({
        stats,
        activity: Array.isArray(stats.activity) ? stats.activity : [],
        topDevices: Array.isArray(stats.topDevices) ? stats.topDevices : [],
        recentEvents: Array.isArray(d.recentEvents) ? d.recentEvents : [],
        alerts: Array.isArray(d.alerts) ? d.alerts : []
      });
    }
    if (key === "events") setState({ events: normalizeEventsPayload(data) });
    if (key === "alerts") setState({ alerts: normalizeAlertsPayload(data) });
    if (key === "users") setState({ users: Array.isArray(data) ? data : [] });
    if (key === "rules") setState({ rules: Array.isArray(data) ? data : [] });
    if (key === "status") {
      setState({
        wsConnected: !!data?.wsRunning,
        wsPort: data?.wsPort || 8081,
        wsSecure: !!data?.wsSecure,
        wsClients: data?.wsClients || 0
      });
    }
  }

  if (!admin) setState({ users: [], rules: [] });
}

export function navigate(page) {
  const nextPage = normalizePage(page);
  if (state.page !== nextPage) setState({ page: nextPage });
}

function renderCurrentPage() {
  if (!root) return;
  if (!state.isAuthenticated) {
    renderLoginPage();
    return;
  }

  ensureShell();
  const page = normalizePage(state.page);
  if (page !== state.page) {
    setState({ page });
    return;
  }

  const user = state.currentUser || {};
  if (sidebarEl) {
    const newSidebar = createSidebar({ active: page, currentUser: user, onNavigate: navigate });
    sidebarEl.replaceWith(newSidebar);
    sidebarEl = newSidebar;
  }

  const key = `${page}:${state.dataVersion}:${state.mustChangePassword ? "forced" : "normal"}`;
  if (key === lastRenderedPage) return;
  lastRenderedPage = key;
  patchCurrentPage();
}

function patchCurrentPage() {
  if (!pageContainerEl) return;

  const user = state.currentUser || {};
  const userRole = getUserRole(user);
  const pageFn = pages[normalizePage(state.page)] || renderDashboard;

  const props = buildPageProps({
    user,
    userRole,
    isAdmin: isAdmin(user),
    isOperator: isOperator(user),
    isViewer: isViewer(user),
    canAccessRules: canAccessRules(user),
    canClearData: canClearData(user),
    canUpdateAlertStatus: canUpdateAlertStatus(user)
  });

  Promise.resolve(pageFn(props)).then(pageNode => {
    pageContainerEl.innerHTML = "";
    if (pageNode instanceof HTMLElement) pageContainerEl.appendChild(pageNode);
    else pageContainerEl.innerHTML = pageNode || "";
  });
}

function buildPageProps(perms) {
  const base = {
    currentUser: perms.user,
    userRole: perms.userRole,
    currentUserRole: perms.userRole,
    isAdmin: perms.isAdmin,
    isOperator: perms.isOperator,
    isViewer: perms.isViewer,
    canManage: perms.isAdmin,
    canAccessRules: perms.canAccessRules,
    canClearData: perms.canClearData,
    canUpdateAlertStatus: perms.canUpdateAlertStatus,
    mustChangePassword: state.mustChangePassword,
    onRefresh: loadAllData,
    onNavigate: navigate
  };

  switch (normalizePage(state.page)) {
    case "dashboard":
      return {
        ...base,
        stats: state.dashboard?.stats || {},
        activity: state.dashboard?.activity || [],
        topDevices: state.dashboard?.topDevices || [],
        users: state.users || [],
        events: state.events || [],
        alerts: state.alerts || [],
        wsConnected: state.wsConnected || false,
        onLogout: async () => {
          await logout().catch(() => {});
          clearSession();
          renderLoginPage();
        },
        onCreateUser: null
      };
    case "events":
      return { ...base, events: state.events || [] };
    case "alerts":
      return { ...base, alerts: state.alerts || [], alertsFilter: state.alertsFilter || "" };
    case "rules":
      return { ...base, rules: state.rules || [] };
    case "settings":
      return {
        ...base,
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