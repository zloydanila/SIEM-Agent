import { users as loadUsers, createUser, updateUser, deleteUser } from "../api.js";
import { showToast } from "../components/toast.js";
import { createModal, ensureModalHost, setModalBody, setModalFooter, openModal, closeModal } from "../components/modal.js";

export async function renderUsers({ users: initialUsers = [], currentUser = {}, onRefresh } = {}) {
  const root = document.createElement("div");
  root.className = "page-inner fade-in";

  let data = Array.isArray(initialUsers) ? initialUsers : [];
  let error = null;
  const isAdmin = String(currentUser.role || "").toLowerCase() === "admin";

  if (!data.length) {
    try { data = await loadUsers(); } catch (e) { error = e.message; }
  }

  root.innerHTML = `
    <section class="page-section">
      <div class="page-hero">
        <div>
          <h2>Пользователи</h2>
          <p>${data.length} пользователей</p>
        </div>
        <div class="page-actions">
          <button class="btn ghost" id="usersRefreshBtn">Refresh</button>
          ${isAdmin ? `<button class="btn success" id="addUserBtn">+ Add User</button>` : ""}
        </div>
      </div>

      ${error ? `<div class="page-note" style="border-color:var(--danger);color:var(--danger);">Ошибка: ${esc(error)}</div>` : ""}

      <div class="card">
        <div class="card-body table-wrap" style="padding:0;">
          <table class="table users-table">
            <thead>
              <tr>
                <th>USERNAME</th>
                <th>FULL NAME</th>
                <th>EMAIL</th>
                <th>ROLE</th>
                <th>STATUS</th>
                <th>ACTIONS</th>
              </tr>
            </thead>
            <tbody>
              ${data.length ? data.map(u => `
                <tr>
                  <td>${esc(u.username || "")}</td>
                  <td>${esc(u.fullName || "—")}</td>
                  <td>${esc(u.email || "—")}</td>
                  <td>${esc(u.role || "")}</td>
                  <td>${u.isActive ? "Active" : "Inactive"}</td>
                  <td>
                    ${isAdmin ? `
                      <button class="btn mini ghost" data-edit-user="${u.id}">Edit</button>
                      <button class="btn mini danger" data-del-user="${u.id}">Delete</button>
                    ` : "—"}
                  </td>
                </tr>
              `).join("") : `<tr><td colspan="6" style="text-align:center;color:var(--text-muted);">Нет пользователей</td></tr>`}
            </tbody>
          </table>
        </div>
      </div>
    </section>
  `;

  root.querySelector("#usersRefreshBtn")?.addEventListener("click", () => onRefresh?.());
  root.querySelector("#addUserBtn")?.addEventListener("click", () => openUserModal(null, onRefresh));

  root.querySelectorAll("[data-edit-user]").forEach(btn => {
    btn.addEventListener("click", () => {
      const user = data.find(u => String(u.id) === btn.dataset.editUser);
      if (user) openUserModal(user, onRefresh);
    });
  });

  root.querySelectorAll("[data-del-user]").forEach(btn => {
    btn.addEventListener("click", async () => {
      const id = btn.dataset.delUser;
      const user = data.find(u => String(u.id) === id);
      if (!confirm(`Удалить пользователя "${user?.username || id}"?`)) return;
      try {
        await deleteUser(id);
        showToast("Пользователь удалён", "success");
        onRefresh?.();
      } catch (e) {
        showToast(e.message, "danger");
      }
    });
  });

  return root;
}

function openUserModal(user, onRefresh) {
  ensureModalHost();
  const isEdit = !!user;
  const modal = createModal({
    id: isEdit ? "editUserModal" : "addUserModal",
    title: isEdit ? "Редактировать пользователя" : "Создать пользователя",
    width: "460px"
  });

  setModalBody(modal, `
    <div class="form-grid">
      <div class="form-group">
        <label class="form-label">USERNAME</label>
        <input class="input" id="uUsername" value="${esc(isEdit ? user.username || "" : "")}" />
      </div>
      <div class="form-group">
        <label class="form-label">FULL NAME</label>
        <input class="input" id="uFullName" value="${esc(isEdit ? user.fullName || "" : "")}" />
      </div>
      <div class="form-group">
        <label class="form-label">EMAIL</label>
        <input class="input" id="uEmail" value="${esc(isEdit ? user.email || "" : "")}" />
      </div>
      <div class="form-group">
        <label class="form-label">ROLE</label>
        <select class="input" id="uRole">
          <option value="viewer" ${isEdit && user.role === "viewer" ? "selected" : ""}>viewer</option>
          <option value="operator" ${isEdit && user.role === "operator" ? "selected" : ""}>operator</option>
          <option value="admin" ${isEdit && user.role === "admin" ? "selected" : ""}>admin</option>
        </select>
      </div>
      ${!isEdit ? `
        <div class="form-group">
          <label class="form-label">PASSWORD</label>
          <input class="input" type="password" id="uPassword" />
        </div>
        <div class="form-group">
          <label class="form-label">CONFIRM</label>
          <input class="input" type="password" id="uConfirm" />
        </div>
      ` : ""}
      <div class="form-group">
        <label class="form-label" style="display:flex;align-items:center;gap:8px;">
          <input type="checkbox" id="uActive" ${!isEdit || user.isActive ? "checked" : ""} />
          <span>Active</span>
        </label>
      </div>
      <div class="error-box" id="uError"></div>
    </div>
  `);

  setModalFooter(modal, `
    <button class="btn ghost" id="uCancelBtn">Отмена</button>
    <button class="btn primary" id="uSaveBtn">${isEdit ? "Сохранить" : "Создать"}</button>
  `);

  ensureModalHost().appendChild(modal);

  modal.querySelector("#uCancelBtn")?.addEventListener("click", () => closeModal(isEdit ? "editUserModal" : "addUserModal"));

  modal.querySelector("#uSaveBtn")?.addEventListener("click", async () => {
    const errBox = modal.querySelector("#uError");
    errBox.textContent = "";

    const payload = {
      username: modal.querySelector("#uUsername").value.trim(),
      fullName: modal.querySelector("#uFullName").value.trim(),
      email: modal.querySelector("#uEmail").value.trim(),
      role: modal.querySelector("#uRole").value,
      isActive: modal.querySelector("#uActive").checked
    };

    if (!payload.username) return showErr(errBox, "Введите username");
    if (!payload.fullName) return showErr(errBox, "Введите full name");

    if (!isEdit) {
      const pwd = modal.querySelector("#uPassword").value;
      const cnf = modal.querySelector("#uConfirm").value;
      if (!pwd) return showErr(errBox, "Введите пароль");
      if (pwd !== cnf) return showErr(errBox, "Пароли не совпадают");
      payload.password = pwd;
    }

    try {
      if (isEdit) await updateUser(user.id, payload);
      else await createUser(payload);
      showToast(isEdit ? "Пользователь обновлён" : "Пользователь создан", "success");
      closeModal(isEdit ? "editUserModal" : "addUserModal");
      onRefresh?.();
    } catch (e) {
      showErr(errBox, e.message);
    }
  });

  openModal(isEdit ? "editUserModal" : "addUserModal");
}

function showErr(el, text) {
  el.textContent = text;
}

function esc(v) {
  return String(v ?? "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;");
}