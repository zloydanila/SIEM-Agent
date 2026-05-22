import * as api from "../api.js";
import { state } from "../state.js";
import { navigate } from "../router.js";
import { showToast } from "../components/toast.js";

export function renderLogin(root) {
  if (typeof root === "string") root = document.querySelector(root);
  if (!root) throw new Error("renderLogin: root must be a DOM element");

  root.innerHTML = `
    <section class="login-page">
      <div class="login-grid"></div>
      <form id="loginForm" class="login-form">
        <div class="login-logo">SA</div>
        <h1 class="login-title">SIEM Agent</h1>
        <p class="login-subtitle">Вход в систему</p>

        <div class="login-input-group">
          <label class="login-label" for="loginUsername">Логин</label>
          <div class="login-input-wrap">
            <input id="loginUsername" class="login-input" type="text" autocomplete="username" />
          </div>
        </div>

        <div class="login-input-group">
          <label class="login-label" for="loginPassword">Пароль</label>
          <div class="login-input-wrap">
            <input id="loginPassword" class="login-input" type="password" autocomplete="current-password" />
          </div>
        </div>

        <div id="loginError" class="login-error"></div>
        <button type="submit" class="login-btn">Войти</button>
      </form>
    </section>
  `;

  const form = root.querySelector("#loginForm");
  const username = root.querySelector("#loginUsername");
  const password = root.querySelector("#loginPassword");
  const errorBox = root.querySelector("#loginError");

  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    errorBox.classList.remove("visible");
    errorBox.textContent = "";
    try {
      const res = await api.login(username.value.trim(), password.value);
      const token = res?.token || res?.accessToken;
      if (token) localStorage.setItem("token", token);
      if (res?.mustChangePassword) localStorage.setItem("mustChangePassword", "true");
      if (res?.user) localStorage.setItem("user", JSON.stringify(res.user));
      state.mustChangePassword = !!res?.mustChangePassword;
      state.isAuthenticated = true;
      state.currentUser = res?.user || null;

      if (res?.mustChangePassword) {
        navigate("settings");
        showToast("Требуется смена пароля", "warning");
        return;
      }

      navigate("dashboard");
      showToast("Успешный вход", "success");
      window.dispatchEvent(new Event("routechange"));
    } catch (err) {
      errorBox.textContent = err.message || "Ошибка входа";
      errorBox.classList.add("visible");
      showToast(err.message || "Ошибка входа", "danger");
      form.classList.remove("shake");
      void form.offsetWidth;
      form.classList.add("shake");
    }
  });
}