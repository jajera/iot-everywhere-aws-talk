import "./style.css";
import { lineChart } from "./charts.js";
import { esc, fmtRelative, fmtTs } from "./format.js";

const apiBase = (import.meta.env.VITE_API_URL || "").replace(/\/$/, "");
const pollMs = Number(import.meta.env.VITE_POLL_INTERVAL_MS || 15000);

const WINDOWS = [
  { s: 3600, label: "1h", pollMs: 30000 },
  { s: 21600, label: "6h", pollMs: 60000 },
  { s: 86400, label: "24h", pollMs: 120000 },
];
const PALETTE = ["#5c9ded", "#f5a623", "#7c5cfc", "#e85d75", "#3dcdc9", "#c4c4c4"];
const CACHE_SOFT_MS = { 3600: 25000, 21600: 55000, 86400: 110000 };
const CACHE_HARD_MS = { 3600: 120000, 21600: 300000, 86400: 600000 };

const state = {
  polling: true,
  windowS: WINDOWS[0].s,
  data: null,
  selected: null, // single host id
  sort: { key: "health", dir: 1 },
  timer: null,
  loading: false,
  fetchGen: 0,
  abort: null,
  cache: new Map(),
};

const windowPollMs = () => Math.max(pollMs, WINDOWS.find((w) => w.s === state.windowS).pollMs);

const app = document.querySelector("#app");
app.innerHTML = `
  <div class="shell">
    <header class="top">
      <div class="top-left">
        <span class="product">IoT Talk</span>
        <span class="sep">·</span>
        <span class="page">live</span>
      </div>
      <div class="top-right">
        <div class="seg" id="windowSeg" role="group" aria-label="Time window">
          ${WINDOWS.map((w) => `<button type="button" data-window="${w.s}">${w.label}</button>`).join("")}
        </div>
        <button id="refresh" type="button" class="btn">Refresh</button>
        <button id="togglePoll" type="button" class="btn ghost" aria-pressed="true"></button>
      </div>
    </header>

    <div id="banner" class="banner hidden"></div>

    <section class="kpi-row" id="kpiRow"></section>

    <div class="content">
      <section class="panel">
        <div class="panel-bar">
          <span id="tableMeta" class="mono">hosts</span>
          <span id="status" class="subtle mono"></span>
        </div>
        <div class="table-wrap">
          <table class="hosts" id="hostTable">
            <thead></thead>
            <tbody></tbody>
          </table>
        </div>
      </section>

      <section class="charts panel">
        <div class="panel-bar">
          <span class="mono">signal &amp; temp</span>
          <span id="legend" class="legend"></span>
        </div>
        <div class="chart-grid">
          <div class="chart-cell">
            <div class="chart-label">rssi (dBm)</div>
            <div id="rssiChart" class="lchart-wrap"></div>
          </div>
          <div class="chart-cell">
            <div class="chart-label">chip_temp_c</div>
            <div id="tempChart" class="lchart-wrap"></div>
          </div>
        </div>
      </section>

      <section class="bottom">
        <article class="panel grow">
          <div class="panel-bar"><span class="mono">events</span><span id="eventCount" class="chip">0</span></div>
          <div id="events" class="events mono subtle">No events.</div>
        </article>
        <article class="panel camera-panel hidden" id="cameraPanel">
          <div class="panel-bar"><span class="mono">camera</span><span id="cameraMeta" class="chip">—</span></div>
          <div id="cameraBody" class="camera-body subtle"></div>
        </article>
      </section>
    </div>

    <footer class="foot mono subtle">${apiBase || "Set VITE_API_URL"}</footer>
  </div>
`;

const $ = (sel) => document.querySelector(sel);
const els = {
  banner: $("#banner"),
  status: $("#status"),
  kpiRow: $("#kpiRow"),
  tableMeta: $("#tableMeta"),
  table: $("#hostTable"),
  legend: $("#legend"),
  rssiChart: $("#rssiChart"),
  tempChart: $("#tempChart"),
  events: $("#events"),
  eventCount: $("#eventCount"),
  cameraPanel: $("#cameraPanel"),
  cameraMeta: $("#cameraMeta"),
  cameraBody: $("#cameraBody"),
  togglePoll: $("#togglePoll"),
  refresh: $("#refresh"),
  windowSeg: $("#windowSeg"),
};

function setBanner(msg, tone = "bad") {
  if (!msg) {
    els.banner.classList.add("hidden");
    els.banner.textContent = "";
    return;
  }
  els.banner.classList.remove("hidden");
  els.banner.dataset.tone = tone;
  els.banner.textContent = msg;
}

async function getJson(path, { signal } = {}) {
  const res = await fetch(`${apiBase}${path}`, { signal });
  if (!res.ok) throw new Error(`${res.status} ${path}`);
  return res.json();
}

function cacheGet(windowS) {
  return state.cache.get(windowS) || null;
}

function cachePut(windowS, data) {
  state.cache.set(windowS, { at: Date.now(), data });
}

function cacheAge(entry) {
  return entry ? Date.now() - entry.at : Infinity;
}

function setWindowBusy(busy) {
  els.windowSeg.classList.toggle("busy", busy);
  els.refresh.disabled = busy;
}

function applyData(data, { paint = true } = {}) {
  state.data = data;
  if (paint) {
    const ids = new Set((data.devices || []).map((d) => d.device_id));
    if (!state.selected || !ids.has(state.selected)) {
      const prefer = (data.devices || []).find((d) => d.live) || (data.devices || [])[0];
      state.selected = prefer?.device_id || null;
    }
    renderAll();
  }
}

async function fetchAnalytics(windowS, { signal } = {}) {
  const data = await getJson(`/fleet/analytics?window=${windowS}`, { signal });
  cachePut(windowS, data);
  return data;
}

function prefetchOthers() {
  for (const w of WINDOWS) {
    if (w.s === state.windowS) continue;
    const hit = cacheGet(w.s);
    if (hit && cacheAge(hit) < (CACHE_SOFT_MS[w.s] || 30000)) continue;
    fetchAnalytics(w.s).catch(() => {});
  }
}

function devices() {
  return state.data?.devices || [];
}

function colorFor(id) {
  const ids = devices()
    .map((d) => d.device_id)
    .sort();
  return PALETTE[Math.max(0, ids.indexOf(id)) % PALETTE.length];
}

function renderKpis() {
  const f = state.data?.fleet || {};
  const cells = [
    [`${f.live_count ?? "—"}/${f.device_count ?? "—"}`, "hosts live"],
    [f.min_health ?? "—", "min health"],
    [f.avg_delivery_pct != null ? `${f.avg_delivery_pct}%` : "—", "avg delivery"],
  ];
  els.kpiRow.innerHTML = cells
    .map(
      ([value, label]) => `
    <div class="kpi">
      <div class="kpi-value mono">${esc(String(value))}</div>
      <div class="kpi-label">${esc(label)}</div>
    </div>`
    )
    .join("");
}

const COLUMNS = [
  { key: "live", label: "" },
  { key: "device_id", label: "host" },
  { key: "model", label: "model" },
  { key: "health", label: "health" },
  { key: "rssi", label: "rssi" },
  { key: "age_s", label: "last seen" },
];

function renderTable() {
  const rows = [...devices()].sort((a, b) => {
    const { key, dir } = state.sort;
    const av = sortVal(a, key);
    const bv = sortVal(b, key);
    if (av == null && bv == null) return 0;
    if (av == null) return 1;
    if (bv == null) return -1;
    if (av < bv) return -dir;
    if (av > bv) return dir;
    return a.device_id.localeCompare(b.device_id);
  });

  const live = rows.filter((d) => d.live).length;
  els.tableMeta.textContent = `hosts · ${live} live / ${rows.length}`;
  const thead = `<tr>${COLUMNS.map((c) => `<th data-sort="${c.key}">${c.label}</th>`).join("")}</tr>`;
  const body = rows
    .map((d) => {
      const sel = state.selected === d.device_id;
      const t = d.telemetry || {};
      return `<tr class="${sel ? "selected" : ""}" data-id="${esc(d.device_id)}">
        <td><span class="dot ${d.live ? "ok" : "bad"}" title="${d.live ? "live" : "stale"}"></span></td>
        <td class="mono">${esc(d.device_id)}</td>
        <td class="mono">${esc(d.model || "—")}</td>
        <td class="mono">${d.health ?? "—"}</td>
        <td class="mono">${t.rssi ?? "—"}</td>
        <td class="mono">${fmtRelative(d.last_seen_ts)}</td>
      </tr>`;
    })
    .join("");
  els.table.querySelector("thead").innerHTML = thead;
  els.table.querySelector("tbody").innerHTML =
    body || `<tr><td colspan="6" class="subtle">No hosts</td></tr>`;
}

function sortVal(d, key) {
  if (key === "live") return d.live ? 0 : 1;
  if (key === "rssi") return d.telemetry?.rssi;
  if (key === "model") return d.model || "";
  return d[key];
}

function selectedDevice() {
  if (!state.selected) return null;
  return devices().find((d) => d.device_id === state.selected) || null;
}

function selectedDevices() {
  const d = selectedDevice();
  return d ? [d] : [];
}

function tempEmptyMessage(shown) {
  if (!shown.length) return "Select a host.";
  const unsupported = shown.filter((d) => d.telemetry?.chip_temp_c == null && !d.stats?.temp);
  if (unsupported.length === shown.length) {
    return `n/a — no on-chip temp`;
  }
  return "No temperature in this window.";
}

function chartHeight(base) {
  if (window.matchMedia("(max-width: 640px)").matches) return Math.round(base * 0.75);
  return base;
}

function renderCharts() {
  const data = state.data;
  if (!data) return;
  const shown = selectedDevices();
  els.legend.innerHTML = shown
    .map((d) => `<span class="lg"><i style="background:${colorFor(d.device_id)}"></i>${esc(d.device_id)}</span>`)
    .join("");

  const mkSeries = (pick) =>
    shown.map((d) => ({
      id: d.device_id,
      label: d.device_id,
      color: colorFor(d.device_id),
      points: (d.series || []).map((b) => [b[0], pick(b, d)]),
    }));

  lineChart(els.rssiChart, {
    series: mkSeries((b) => b[1]),
    xStart: data.since,
    xEnd: data.now,
    // Series are bucketed (60s / 5m / 15m) — gap must be > bucket or every point is a move-to.
    gapS: (data.bucket_s || 60) * 2.5,
    height: chartHeight(160),
    fmt: (v) => `${Math.round(v)}`,
  });
  lineChart(els.tempChart, {
    series: mkSeries((b) => b[2]),
    xStart: data.since,
    xEnd: data.now,
    gapS: (data.bucket_s || 60) * 2.5,
    height: chartHeight(160),
    fmt: (v) => v.toFixed(1),
    empty: tempEmptyMessage(shown),
  });
}

function renderEvents() {
  const events = state.data?.events || [];
  const focus = state.selected
    ? events.filter((e) => e.device_id === state.selected)
    : events;
  els.eventCount.textContent = String(focus.length);
  els.events.innerHTML = focus.length
    ? focus
        .slice(0, 30)
        .map((e) => {
          const bits = [e.device_id, e.type, e.event || e.reset_reason || ""].filter(Boolean);
          return `<div><span class="ts">${fmtTs(e.ts)}</span> ${esc(bits.join(" · "))}</div>`;
        })
        .join("")
    : "No events.";
}

function renderCamera() {
  const d = selectedDevice();
  const cam = d?.camera?.url ? d.camera : null;
  if (!cam) {
    els.cameraPanel.classList.add("hidden");
    els.cameraMeta.textContent = "—";
    els.cameraBody.innerHTML = "";
    return;
  }

  els.cameraPanel.classList.remove("hidden");
  els.cameraMeta.textContent = d.device_id;
  const cap = `${d.device_id} · ${cam.ts ? fmtTs(cam.ts) : "—"}`;
  const existing = els.cameraBody.querySelector("img.camera-img");
  if (existing && existing.getAttribute("src") === cam.url) {
    const capEl = els.cameraBody.querySelector(".camera-cap");
    if (capEl) capEl.textContent = cap;
    return;
  }
  els.cameraBody.innerHTML = `
    <img class="camera-img" alt="latest frame" src="${esc(cam.url)}" loading="lazy" decoding="async" />
    <div class="camera-cap mono subtle">${esc(cap)}</div>`;
}

function renderAll() {
  if (!state.data) return;
  renderKpis();
  renderTable();
  renderEvents();
  renderCamera();
  els.status.textContent = new Date().toLocaleTimeString();
  requestAnimationFrame(() => renderCharts());
}

function renderSelection() {
  if (!state.data) return;
  els.table.querySelectorAll("tbody tr[data-id]").forEach((tr) => {
    tr.classList.toggle("selected", state.selected === tr.dataset.id);
  });
  renderEvents();
  renderCamera();
  requestAnimationFrame(() => renderCharts());
}

async function refresh({ force = false } = {}) {
  if (!apiBase) {
    setBanner("VITE_API_URL missing — rebuild with query API URL.");
    return;
  }

  const windowS = state.windowS;
  const soft = CACHE_SOFT_MS[windowS] || 30000;
  const hard = CACHE_HARD_MS[windowS] || 120000;
  const hit = cacheGet(windowS);
  const age = cacheAge(hit);

  if (!force && hit && age < soft) {
    applyData(hit.data);
    prefetchOthers();
    return;
  }
  if (!force && hit && age < hard) {
    applyData(hit.data);
  }

  const gen = ++state.fetchGen;
  if (state.abort) state.abort.abort();
  state.abort = new AbortController();
  state.loading = true;
  setWindowBusy(true);

  try {
    const data = await fetchAnalytics(windowS, { signal: state.abort.signal });
    if (gen !== state.fetchGen) return;
    applyData(data);
    setBanner("");
    prefetchOthers();
  } catch (err) {
    if (err.name === "AbortError") return;
    if (hit) {
      applyData(hit.data);
      setBanner(`Using cached data (${err.message})`, "warn");
    } else {
      setBanner(String(err.message || err));
    }
  } finally {
    if (gen === state.fetchGen) {
      state.loading = false;
      setWindowBusy(false);
    }
  }
}

function syncPollButton() {
  els.togglePoll.textContent = state.polling ? "Pause" : "Resume";
  els.togglePoll.setAttribute("aria-pressed", state.polling ? "true" : "false");
}

function schedule() {
  clearTimeout(state.timer);
  if (!state.polling) return;
  state.timer = setTimeout(async () => {
    await refresh({ force: false });
    schedule();
  }, windowPollMs());
}

els.windowSeg.addEventListener("click", (e) => {
  const btn = e.target.closest("[data-window]");
  if (!btn) return;
  const next = Number(btn.dataset.window);
  if (next === state.windowS) return;
  state.windowS = next;
  [...els.windowSeg.querySelectorAll("button")].forEach((b) =>
    b.classList.toggle("on", Number(b.dataset.window) === state.windowS)
  );
  refresh({ force: false }).then(schedule);
});

els.refresh.addEventListener("click", () => refresh({ force: true }).then(schedule));
els.togglePoll.addEventListener("click", () => {
  state.polling = !state.polling;
  syncPollButton();
  schedule();
});

document.addEventListener("click", (e) => {
  const th = e.target.closest("th[data-sort]");
  if (th) {
    const key = th.dataset.sort;
    if (state.sort.key === key) state.sort.dir *= -1;
    else state.sort = { key, dir: key === "health" || key === "rssi" ? 1 : -1 };
    renderTable();
    return;
  }
  const tr = e.target.closest("tr[data-id]");
  if (tr) {
    const id = tr.dataset.id;
    if (state.selected === id) return;
    state.selected = id;
    renderSelection();
  }
});

window.addEventListener(
  "resize",
  (() => {
    let t = 0;
    return () => {
      clearTimeout(t);
      t = setTimeout(() => {
        if (state.data) requestAnimationFrame(() => renderCharts());
      }, 120);
    };
  })()
);

[...els.windowSeg.querySelectorAll("button")].forEach((b) =>
  b.classList.toggle("on", Number(b.dataset.window) === state.windowS)
);
syncPollButton();
refresh().then(schedule);
