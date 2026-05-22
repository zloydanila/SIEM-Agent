export function ensureModalHost() {
  let host = document.getElementById("modalHost");
  if (!host) {
    host = document.createElement("div");
    host.id = "modalHost";
    host.style.cssText = "position:fixed;inset:0;z-index:1000;pointer-events:none;";
    document.body.appendChild(host);
  }
  return host;
}

export function createModal({ id, title = "", width = "460px", modalClass = "", onClose = null } = {}) {
  const overlay = document.createElement("div");
  overlay.className = "modal-overlay";
  overlay.id = id;
  overlay.innerHTML = `
    <div class="modal ${modalClass}" style="width:${width};pointer-events:auto;">
      <div class="modal-header">
        <h3>${title}</h3>
        <button class="modal-close" type="button" data-modal-close>×</button>
      </div>
      <div class="modal-body"></div>
      <div class="modal-footer"></div>
    </div>
  `;

  overlay.addEventListener("click", e => {
    if (e.target === overlay) {
      closeModal(id);
      onClose?.();
    }
  });

  overlay.querySelector("[data-modal-close]")?.addEventListener("click", () => {
    closeModal(id);
    onClose?.();
  });

  return overlay;
}

export function openModal(id) {
  const el = document.getElementById(id);
  if (!el) return;
  const host = ensureModalHost();
  if (!host.contains(el)) host.appendChild(el);
  requestAnimationFrame(() => el.classList.add("open"));
}

export function closeModal(id) {
  const el = document.getElementById(id);
  if (!el) return;
  el.classList.remove("open");
  setTimeout(() => el.remove(), 220);
}

export function setModalBody(modal, html) {
  if (!(modal instanceof Element)) {
    console.error("setModalBody: modal is not a DOM element", modal);
    return;
  }
  const body = modal.querySelector(".modal-body");
  if (body) body.innerHTML = html;
}

export function setModalFooter(modal, html) {
  if (!(modal instanceof Element)) {
    console.error("setModalFooter: modal is not a DOM element", modal);
    return;
  }
  const footer = modal.querySelector(".modal-footer");
  if (footer) footer.innerHTML = html;
}