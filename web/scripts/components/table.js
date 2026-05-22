export function table(headers, rows) {
  const wrap = document.createElement('div');
  wrap.className = 'table-wrap';
  
  const tbl = document.createElement('table');
  tbl.className = 'table';
  
  const thead = document.createElement('thead');
  thead.innerHTML = `<tr>${headers.map(h => `<th>${h}</th>`).join('')}</tr>`;
  
  const tbody = document.createElement('tbody');
  tbody.innerHTML = rows.map(row => 
    `<tr>${row.map(cell => `<td>${cell ?? ''}</td>`).join('')}</tr>`
  ).join('');
  
  tbl.appendChild(thead);
  tbl.appendChild(tbody);
  wrap.appendChild(tbl);
  
  return wrap;
}