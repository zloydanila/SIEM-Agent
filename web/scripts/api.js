const API_BASE = "";

function token() {
  return localStorage.getItem("token") || "";
}

function authHeaders(extra = {}) {
  const headers = { ...extra };
  const t = token();
  if (t) headers.Authorization = `Bearer ${t}`;
  return headers;
}

async function request(path, options = {}) {
  const res = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      ...(options.headers || {}),
      ...authHeaders(options.headers || {})
    },
    credentials: "include"
  });

  if (res.status === 204) return null;

  const contentType = res.headers.get("content-type") || "";
  const data = contentType.includes("application/json")
    ? await res.json().catch(() => null)
    : await res.text().catch(() => "");

  if (!res.ok) {
    const msg =
      data?.message ||
      data?.error ||
      (typeof data === "string" && data) ||
      `HTTP ${res.status}`;

    const err = new Error(msg);
    err.status = res.status;
    err.data = data;
    throw err;
  }

  return data;
}

function downloadBlob(path, filename) {
  return fetch(`${API_BASE}${path}`, {
    headers: authHeaders(),
    credentials: "include"
  }).then(async (res) => {
    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(text || `HTTP ${res.status}`);
    }
    const blob = await res.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
    return blob;
  });
}

export const isAuthenticated = () => !!token();

export async function login(username, password) {
  const result = await request("/api/auth/login", {
    method: "POST",
    body: JSON.stringify({ username, password })
  });

  if (result?.token) localStorage.setItem("token", result.token);
  if (result?.user) localStorage.setItem("user", JSON.stringify(result.user));
  localStorage.setItem("mustChangePassword", String(!!result?.mustChangePassword));

  return result;
}

export function logout() {
  const cleanup = () => {
    localStorage.removeItem("token");
    localStorage.removeItem("mustChangePassword");
    localStorage.removeItem("user");
  };

  if (!token()) {
    cleanup();
    return Promise.resolve();
  }

  return request("/api/auth/logout", { method: "POST" }).finally(cleanup);
}

export async function me() {
  const result = await request("/api/me");
  const actualUser = result?.user || result || null;
  if (actualUser) {
    localStorage.setItem("user", JSON.stringify(actualUser));
    localStorage.setItem("mustChangePassword", String(!!actualUser.mustChangePassword));
  }
  return actualUser;
}

export const dashboard = () => request("/api/dashboard");
export const users = () => request("/api/users");
export const events = () => request("/api/events");
export const alerts = () => request("/api/alerts");
export const rules = () => request("/api/rules");
export const status = () => request("/api/status");

export const createUser = (payload) =>
  request("/api/users", { method: "POST", body: JSON.stringify(payload) });

export const updateUser = (id, payload) =>
  request(`/api/users/${id}`, { method: "PUT", body: JSON.stringify(payload) });

export const deleteUser = (id) =>
  request(`/api/users/${id}`, { method: "DELETE" });

export async function changePassword(currentPassword, newPassword) {
  const result = await request("/api/auth/change-password", {
    method: "POST",
    body: JSON.stringify({ currentPassword, newPassword })
  });
  localStorage.setItem("mustChangePassword", "false");
  return result;
}

export const clearEvents = () => request("/api/events", { method: "DELETE" });
export const clearAlerts = () => request("/api/alerts", { method: "DELETE" });

export const createRule = (payload) =>
  request("/api/rules", { method: "POST", body: JSON.stringify(payload) });

export const updateRule = (id, payload) =>
  request(`/api/rules/${id}`, { method: "PUT", body: JSON.stringify(payload) });

export const deleteRule = (id) =>
  request(`/api/rules/${id}`, { method: "DELETE" });

export const toggleRule = (id, enabled) =>
  request(`/api/rules/${id}`, {
    method: "PATCH",
    body: JSON.stringify({ isEnabled: enabled })
  });

export const updateAlertStatus = (id, statusValue) =>
  request(`/api/alerts/${id}`, {
    method: "PATCH",
    body: JSON.stringify({ status: statusValue })
  });

export const exportEventsCsv = () => downloadBlob("/api/reports/events/csv", "events.csv");
export const exportAlertsCsv = () => downloadBlob("/api/reports/alerts/csv", "alerts.csv");
export const exportReportJson = () => downloadBlob("/api/reports/report/json", "report.json");