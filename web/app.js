// ==================== 通用请求函数 ====================
const api = async (url, data) => {
  const res = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data)
  });
  return res.json();
};

// ==================== 登录/登出 ====================
function login(role) {
  const pwd = document.getElementById("adminPwd")?.value || "";
  api("/api/login", { role, password: pwd }).then(r => {
    if (!r.success) { alert(r.message || "登录失败"); return; }
    document.getElementById("loginPanel").classList.add("d-none");
    if (role === "user") {
      document.getElementById("userPanel").classList.remove("d-none");
    } else {
      document.getElementById("adminPanel").classList.remove("d-none");
      loadLineList();
    }
  });
}

function showAdminLogin() {
  document.getElementById("adminPwdBox").classList.remove("d-none");
}

function logout() {
  location.reload();
}

// ==================== 线路查询（快速） ====================
async function queryRoute() {
  const from = document.getElementById("from").value;
  const to = document.getElementById("to").value;
  const strategy = parseInt(document.getElementById("strategy").value);
  const r = await api("/api/route_query", { from, to, strategy });
  const box = document.getElementById("routeResult");
  if (!r.success) {
    box.innerHTML = `<div class="alert alert-warning">未找到路线</div>`;
    return;
  }

  let html = `<div class="route-card">
    <h5>查询结果</h5>
    <div>总站数：<b>${r.totalstops}</b>　换乘：<b>${r.totaltransfer}</b> 次　时间：<b>${r.totaltime}</b> 分钟</div>`;
  let cur = "";
  for (const s of r.steps) {
    if (s.line !== cur) {
      html += `<div class="line-label ${cur === "" ? "" : "transfer"}">${cur === "" ? "乘坐" : "换乘"} ${s.line}</div>`;
      cur = s.line;
    }
    html += `<div class="station">${s.station}</div>`;
  }
  html += `</div>`;
  box.innerHTML = html;
}

// ==================== 所有可达路线（详细） ====================
async function queryAllRoutes() {
  const from = document.getElementById("from").value;
  const to = document.getElementById("to").value;
  const r = await api("/api/all_routes", { from, to });
  const box = document.getElementById("routeResult");
  if (!r.success) {
    box.innerHTML = `<div class="alert alert-warning">${r.message}</div>`;
    return;
  }

  let html = `<div class="route-card"><h5>共 ${r.routes.length} 条可达路线</h5>`;
  r.routes.forEach((route, idx) => {
    html += `<div style="margin-bottom:18px; padding-bottom:14px; border-bottom:1px dashed #e0e6ee;">`;
    html += `<b>[${idx + 1}]</b> `;
    route.lines.forEach((l, i) => {
      html += l + (i + 1 < route.lines.length ? " → " : "");
    });
    html += ` <span style="color:#e58e26; font-weight:600;">【`;
    if (route.isbesttime) html += "最快 ";
    if (route.isbeststops) html += "最少站点 ";
    if (route.isbesttrans) html += "最少换乘 ";
    html += `】</span><br>`;
    html += `总站数 <b>${route.totalstops}</b>，换乘 <b>${route.totaltransfer}</b> 次，约 <b>${route.totaltime}</b> 分钟<br>`;
    html += `<span style="color:#666; font-size:14px;">`;
    route.steps.forEach((s, i) => {
      html += s.station + (i + 1 < route.steps.length ? " → " : "");
    });
    html += `</span></div>`;
  });
  html += `</div>`;
  box.innerHTML = html;
}

// ==================== 车次查询 ====================
async function queryLine() {
  const lineid = document.getElementById("lineid").value;
  const r = await api("/api/line_query", { lineid });
  const box = document.getElementById("lineResult");
  if (!r.success) {
    box.innerHTML = `<div class="alert alert-warning">${r.message}</div>`;
    return;
  }

  let html = `<div class="route-card">
    <h5>${r.lineid}</h5>
    <div>首班：<b>${r.firstbus}</b>　末班：<b>${r.lastbus}</b></div><hr>`;
  r.stations.forEach((s, i) => {
    html += `<div class="station">${i + 1}. ${s.name}${s.interval ? `（到下一站约 ${s.interval} 分钟）` : ""}</div>`;
  });
  html += `</div>`;
  box.innerHTML = html;
}

// ==================== 管理员：站点行操作 ====================
function addStationRow(containerId, name = "", interval = 0) {
  const box = document.getElementById(containerId);
  const row = document.createElement("div");
  row.className = "input-group mb-2";
  row.innerHTML = `
    <input class="form-control station-name" placeholder="站点名" value="${name}">
    <input class="form-control station-interval" placeholder="到下一站分钟" value="${interval}" style="max-width:150px">
    <button class="btn btn-outline-danger" onclick="this.parentElement.remove()">删除</button>`;
  box.appendChild(row);
}

function collectStations(containerId) {
  const rows = document.querySelectorAll(`#${containerId} .input-group`);
  const arr = [];
  rows.forEach(r => {
    const name = r.querySelector(".station-name").value.trim();
    const interval = parseInt(r.querySelector(".station-interval").value) || 0;
    if (name) arr.push({ name, interval });
  });
  return arr;
}

// ==================== 管理员：新增线路 ====================
async function addLine() {
  const lineid = document.getElementById("addLineid").value.trim();
  const firstbus = document.getElementById("addFirst").value.trim();
  const lastbus = document.getElementById("addLast").value.trim();
  const stations = collectStations("addStations");
  if (!lineid) { alert("请填写线路名称"); return; }
  const r = await api("/api/admin/add_line", { lineid, firstbus, lastbus, stations });
  alert(r.message);
  if (r.success) {
    loadLineList();          // 刷新下拉框
    document.getElementById("addLineid").value = "";
    document.getElementById("addFirst").value = "";
    document.getElementById("addLast").value = "";
    document.getElementById("addStations").innerHTML = "";
  }
}

// ==================== 管理员：修改线路 ====================
async function loadLineList() {
  const res = await fetch("/api/lines");
  const lines = await res.json();
  const sel = document.getElementById("modSelect");
  sel.innerHTML = "";
  lines.forEach(l => {
    const opt = document.createElement("option");
    opt.value = l; opt.textContent = l;
    sel.appendChild(opt);
  });
  if (lines.length) loadLineForEdit();
}

async function loadLineForEdit() {
  const lineid = document.getElementById("modSelect").value;
  if (!lineid) return;
  const r = await api("/api/line_query", { lineid });
  if (!r.success) return;
  document.getElementById("modFirst").value = r.firstbus;
  document.getElementById("modLast").value = r.lastbus;
  const box = document.getElementById("modStations");
  box.innerHTML = "";
  r.stations.forEach(s => addStationRow("modStations", s.name, s.interval));
}

async function modifyLine() {
  const lineid = document.getElementById("modSelect").value;
  const firstbus = document.getElementById("modFirst").value.trim();
  const lastbus = document.getElementById("modLast").value.trim();
  const stations = collectStations("modStations");
  if (!lineid) { alert("请选择线路"); return; }
  const r = await api("/api/admin/modify_line", { lineid, firstbus, lastbus, stations });
  alert(r.message);
  if (r.success) loadLineList();
}
