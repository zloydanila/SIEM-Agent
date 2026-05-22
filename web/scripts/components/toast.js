export function showToast(message, type = "primary") {
  let host = document.getElementById("toastHost");
  if (!host) {
    host = document.createElement("div");
    host.id = "toastHost";
    host.style.cssText = "position:fixed;bottom:20px;right:20px;z-index:9999;display:flex;flex-direction:column;gap:8px;";
    document.body.appendChild(host);
  }

  const toast = document.createElement("div");
  toast.className = `toast ${type}`;
  toast.innerHTML = `
    <span>${esc(message)}</span>
    <button type="button" style="margin-left:8px;background:none;border:none;color:inherit;cursor:pointer;font-size:14px;opacity:0.6;">×</button>
  `;

  const remove = () => {
    toast.style.opacity = "0";
    toast.style.transform = "translateX(20px)";
    toast.style.transition = "all 300ms ease";
    setTimeout(() => toast.remove(), 300);
  };

  toast.querySelector("button").onclick = remove;
  host.appendChild(toast);
  setTimeout(remove, 3000);
}

function esc(v) {
  return String(v ?? "").replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;");
}