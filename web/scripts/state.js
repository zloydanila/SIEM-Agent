export const state = {
  page: "dashboard",
  isAuthenticated: false,
  currentUser: null,
  mustChangePassword: localStorage.getItem("mustChangePassword") === "true",
  wsConnected: false,
  wsPort: 8081,
  wsSecure: false,
  wsClients: 0,
  dashboard: {
    stats: {},
    activity: [],
    topDevices: [],
    recentEvents: [],
    alerts: []
  },
  users: [],
  events: [],
  alerts: [],
  rules: [],
  alertsFilter: "",
  lastUpdated: null,
  dataVersion: 0
};

const listeners = new Set();

export function subscribe(fn) {
  listeners.add(fn);
  try {
    fn(state);
  } catch (e) {
    console.error(e);
  }
  return () => listeners.delete(fn);
}

export function notify() {
  listeners.forEach(fn => {
    try {
      fn(state);
    } catch (e) {
      console.error(e);
    }
  });
}

function equal(a, b) {
  return JSON.stringify(a) === JSON.stringify(b);
}

export function setState(patch) {
  let changed = false;

  for (const [key, value] of Object.entries(patch)) {
    if (!equal(state[key], value)) {
      state[key] = value;
      changed = true;
    }
  }

  if (!changed) return; // ← ВОТ ЭТО! Выходим если ничего не изменилось

  if ("currentUser" in patch) {
    const actualUser = patch.currentUser?.user || patch.currentUser || null;
    state.currentUser = actualUser;
    state.isAuthenticated = !!actualUser;
  }

  if ("mustChangePassword" in patch) {
    localStorage.setItem("mustChangePassword", String(!!patch.mustChangePassword));
  }

  state.lastUpdated = Date.now();
  state.dataVersion++; // ← теперь только если РЕАЛЬНО изменилось
  notify();
}

export function setDashboard(patch) {
  const next = { ...(state.dashboard || {}) };
  let changed = false;

  for (const [key, value] of Object.entries(patch)) {
    if (!equal(next[key], value)) {
      next[key] = value;
      changed = true;
    }
  }

  if (!changed) return; // ← ВОТ ЭТО! Выходим если ничего не изменилось

  state.dashboard = next;
  state.lastUpdated = Date.now();
  state.dataVersion++;
  notify();
}

export function setUser(user) {
  const actualUser = user?.user || user || null;
  state.currentUser = actualUser;
  state.isAuthenticated = !!actualUser;

  if (actualUser) localStorage.setItem("user", JSON.stringify(actualUser));
  else localStorage.removeItem("user");

  state.lastUpdated = Date.now();
  state.dataVersion++;
  notify();
}

export function setAlertsFilter(filter) {
  const next = String(filter || "");
  if (state.alertsFilter === next) return;

  state.alertsFilter = next;
  state.lastUpdated = Date.now();
  state.dataVersion++;
  notify();
}

export function clearSession() {
  state.page = "dashboard";
  state.isAuthenticated = false;
  state.currentUser = null;
  state.mustChangePassword = false;
  state.wsConnected = false;
  state.wsPort = 8081;
  state.wsSecure = false;
  state.wsClients = 0;
  state.dashboard = {
    stats: {},
    activity: [],
    topDevices: [],
    recentEvents: [],
    alerts: []
  };
  state.users = [];
  state.events = [];
  state.alerts = [];
  state.rules = [];
  state.alertsFilter = "";
  state.lastUpdated = Date.now();
  state.dataVersion++;

  localStorage.removeItem("token");
  localStorage.removeItem("mustChangePassword");
  localStorage.removeItem("user");

  notify();
}