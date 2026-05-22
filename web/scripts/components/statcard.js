export function statCard(label, value, color = 'accent', hint = '') {
  const div = document.createElement('div');
  div.className = `card kpi-card ${color}`;
  div.innerHTML = `
    <div class="kpi-label">${label}</div>
    <div class="kpi-value" style="color:var(--${color});">${value}</div>
    ${hint ? `<div class="kpi-hint">${hint}</div>` : ''}
  `;
  return div;
}