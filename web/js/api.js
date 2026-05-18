const API_BASE = '';
let authToken = localStorage.getItem('authToken') || '';

function apiFetch(url, options = {}) {
    if (!options.headers) options.headers = {};
    if (authToken) options.headers['Authorization'] = 'Bearer ' + authToken;
    return fetch(API_BASE + url, options).then(async res => {
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'API error');
        return data;
    });
}

const api = {
    login(username, password) {
        return apiFetch('/api/auth/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
    },
    getDashboard() { return apiFetch('/api/dashboard'); },
    getEvents() { return apiFetch('/api/events'); },
    getAlerts() { return apiFetch('/api/alerts'); },
    updateAlertStatus(id, status) {
        return apiFetch(`/api/alerts/${id}/status`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ status })
        });
    },
    getUsers() { return apiFetch('/api/users'); },
    createUser(user) {
        return apiFetch('/api/users', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(user)
        });
    },
    updateUser(id, user) {
        return apiFetch(`/api/users/${id}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(user)
        });
    },
    deleteUser(id) {
        return apiFetch(`/api/users/${id}`, { method: 'DELETE' });
    },
    changePassword(currentPassword, newPassword) {
        return apiFetch('/api/users/change-password', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ currentPassword, newPassword })
        });
    },
    getRules() { return apiFetch('/api/rules'); },
    createRule(rule) {
        return apiFetch('/api/rules', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(rule)
        });
    },
    deleteRule(id) {
        return apiFetch(`/api/rules/${id}`, { method: 'DELETE' });
    },
    toggleRule(id, enabled) {
        return apiFetch(`/api/rules/${id}/toggle`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled })
        });
    },
    exportEventsCsv() { return apiFetch('/api/export/events/csv'); },
    exportAlertsCsv() { return apiFetch('/api/export/alerts/csv'); },
    exportReportJson() { return apiFetch('/api/export/report/json'); },
    clearEvents() { return apiFetch('/api/events/clear', { method: 'DELETE' }); },
    clearAlerts() { return apiFetch('/api/alerts/clear', { method: 'DELETE' }); },
};