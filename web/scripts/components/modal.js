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

export function createModal({
  id,
  title = "",
  width = "460px",
  modalClass = "",
  onClose = null,
  isBlocking = false
} = {}) {
  const overlay = document.createElement("div");
  overlay.className = "modal-overlay" + (isBlocking ? " blocking" : "");
  overlay.id = id;
  overlay.style.pointerEvents = "auto";

  overlay.innerHTML = `
    <div
      class="modal ${modalClass}"
      style="width:${width};pointer-events:auto;"
      role="dialog"
      aria-modal="${isBlocking ? "true" : "false"}"
      aria-labelledby="${id}_title"
    >
      <div class="modal-header">
        <h3 id="${id}_title">${esc(title)}</h3>
        <button class="modal-close" type="button" data-modal-close ${isBlocking ? 'style="display:none;"' : ""}>×</button>
      </div>
      <div class="modal-body"></div>
      <div class="modal-footer"></div>
    </div>
  `;

  const closeHandler = () => {
    if (isBlocking) return;
    closeModal(id);
    onClose?.();
  };

  overlay.__closeHandler = closeHandler;

  overlay.addEventListener("click", e => {
    if (e.target === overlay && !isBlocking) {
      closeHandler();
    }
  });

  overlay.querySelector("[data-modal-close]")?.addEventListener("click", closeHandler);

  if (!isBlocking) {
    const keyHandler = e => {
      if (e.key === "Escape") {
        e.preventDefault();
        closeHandler();
      }
    };
    overlay.__keyHandler = keyHandler;
    document.addEventListener("keydown", keyHandler);
  }

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

  if (typeof el.__cleanupBlocking === "function") {
    el.__cleanupBlocking();
    el.__cleanupBlocking = null;
  }

  if (typeof el.__keyHandler === "function") {
    document.removeEventListener("keydown", el.__keyHandler);
    el.__keyHandler = null;
  }

  el.classList.remove("open");

  setTimeout(() => {
    if (el.isConnected) el.remove();
  }, 220);
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

export function openConfirmModal(title, message, onConfirm) {
  ensureModalHost();

  const modalId = `confirmModal_${Date.now()}`;
  const modal = createModal({
    id: modalId,
    title,
    width: "420px"
  });

  setModalBody(modal, `
    <div style="padding:12px;">
      <p style="margin:0;color:var(--text-secondary);line-height:1.5;">${esc(String(message))}</p>
    </div>
  `);

  setModalFooter(modal, `
    <button class="btn ghost" id="confirmCancel">Отмена</button>
    <button class="btn danger" id="confirmOk">Подтвердить</button>
  `);

  ensureModalHost().appendChild(modal);

  modal.querySelector("#confirmCancel")?.addEventListener("click", () => closeModal(modalId));
  modal.querySelector("#confirmOk")?.addEventListener("click", async () => {
    try {
      await onConfirm?.();
    } finally {
      closeModal(modalId);
    }
  });

  openModal(modalId);
}

function esc(v) {
  return String(v ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}