const NS = "http://www.w3.org/2000/svg";
const observed = new WeakMap();

const resizeObserver =
  typeof ResizeObserver !== "undefined"
    ? new ResizeObserver((entries) => {
        for (const entry of entries) {
          const opts = observed.get(entry.target);
          if (opts) draw(entry.target, opts);
        }
      })
    : null;

function niceStep(span, ticks) {
  const raw = span / Math.max(ticks, 1);
  const mag = 10 ** Math.floor(Math.log10(Math.max(raw, 1e-9)));
  const norm = raw / mag;
  const step = norm >= 5 ? 10 : norm >= 2 ? 5 : norm >= 1 ? 2 : 1;
  return step * mag;
}

function fmtClock(ts, spanS) {
  const d = new Date(ts * 1000);
  const hm = d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  return spanS > 86400 ? `${d.toLocaleDateString([], { day: "numeric", month: "short" })} ${hm}` : hm;
}

function el(name, attrs = {}, text) {
  const node = document.createElementNS(NS, name);
  for (const [k, v] of Object.entries(attrs)) node.setAttribute(k, v);
  if (text != null) node.textContent = text;
  return node;
}

/**
 * Multi-series time chart.
 * series: [{ id, label, color, points: [[ts, value], ...] }]
 */
export function lineChart(container, opts) {
  observed.set(container, opts);
  if (resizeObserver) resizeObserver.observe(container);
  draw(container, opts);
}

function draw(container, opts) {
  const {
    series = [],
    xStart,
    xEnd,
    gapS = Infinity,
    fmt = (v) => String(v),
    band,
    height = 190,
    empty = "No data in this window.",
    focusId,
  } = opts;

  const visible = series
    .map((s) => ({
      ...s,
      points: (s.points || []).filter((p) => p && p[1] != null && Number.isFinite(p[1])),
    }))
    .filter((s) => s.points.length);

  // Keep tip element if present so ResizeObserver isn't fighting a new node each redraw.
  let tip = container.querySelector(":scope > .chart-tip");
  const prevSvg = container.querySelector(":scope > svg.lchart");
  if (prevSvg) prevSvg.remove();
  if (!tip) {
    tip = document.createElement("div");
    tip.className = "chart-tip hidden";
    tip.setAttribute("aria-hidden", "true");
  }

  if (!visible.length) {
    container.innerHTML = `<div class="chart-empty subtle">${empty}</div>`;
    return;
  }

  const width = Math.max(0, Math.floor(container.clientWidth || 0)) || 320;
  const pad = { l: 44, r: 10, t: 12, b: 26 };
  const iw = Math.max(1, width - pad.l - pad.r);
  const ih = Math.max(1, height - pad.t - pad.b);

  let lo = Infinity;
  let hi = -Infinity;
  for (const s of visible) {
    for (const [, v] of s.points) {
      lo = Math.min(lo, v);
      hi = Math.max(hi, v);
    }
  }
  if (band) {
    if (band.to != null && band.to > lo - (hi - lo)) lo = Math.min(lo, band.to);
  }
  if (!Number.isFinite(lo) || !Number.isFinite(hi)) {
    container.innerHTML = `<div class="chart-empty subtle">${empty}</div>`;
    return;
  }
  if (lo === hi) {
    lo -= 1;
    hi += 1;
  }
  const step = niceStep(hi - lo, 4);
  const yMin = Math.floor(lo / step) * step;
  const yMax = Math.ceil(hi / step) * step;
  const x0 = xStart;
  const x1 = Math.max(xEnd, xStart + 1);
  const spanS = x1 - x0;

  const sx = (ts) => pad.l + ((ts - x0) / spanS) * iw;
  const sy = (v) => pad.t + ih - ((v - yMin) / (yMax - yMin)) * ih;

  const svg = el("svg", {
    width,
    height,
    viewBox: `0 0 ${width} ${height}`,
    class: "lchart",
    role: "img",
  });

  if (band) {
    const top = sy(Math.min(yMax, band.to ?? yMax));
    const bottom = sy(Math.max(yMin, band.from ?? yMin));
    if (bottom > top) {
      svg.append(el("rect", { x: pad.l, y: top, width: iw, height: bottom - top, class: "band" }));
      if (band.label) svg.append(el("text", { x: pad.l + 6, y: bottom - 6, class: "band-label" }, band.label));
    }
  }

  for (let v = yMin; v <= yMax + step / 2; v += step) {
    const y = sy(v);
    svg.append(el("line", { x1: pad.l, x2: pad.l + iw, y1: y, y2: y, class: "grid" }));
    svg.append(el("text", { x: pad.l - 6, y: y + 4, class: "axis y" }, fmt(Number(v.toFixed(6)))));
  }

  const ticks = Math.max(2, Math.min(5, Math.floor(iw / 100)));
  for (let i = 0; i <= ticks; i += 1) {
    const ts = x0 + (spanS * i) / ticks;
    const x = sx(ts);
    svg.append(el("line", { x1: x, x2: x, y1: pad.t, y2: pad.t + ih, class: "grid faint" }));
    svg.append(
      el(
        "text",
        { x, y: height - 6, class: `axis x${i === 0 ? " start" : i === ticks ? " end" : ""}` },
        fmtClock(ts, spanS)
      )
    );
  }

  for (const s of visible) {
    let d = "";
    let prevTs = null;
    for (const [ts, v] of s.points) {
      const cmd = prevTs == null || ts - prevTs > gapS ? "M" : "L";
      d += `${cmd}${sx(ts).toFixed(1)},${sy(v).toFixed(1)} `;
      prevTs = ts;
    }
    const dim = focusId && focusId !== s.id;
    svg.append(
      el("path", {
        d: d.trim(),
        fill: "none",
        stroke: s.color,
        "stroke-width": focusId === s.id ? 2.4 : 2,
        "stroke-linejoin": "round",
        "stroke-linecap": "round",
        opacity: dim ? 0.28 : 1,
      })
    );
    // Mark samples when sparse so a short history still reads as a series.
    const markAll = s.points.length <= 48;
    for (const [ts, v] of s.points) {
      if (!markAll) continue;
      svg.append(
        el("circle", {
          cx: sx(ts),
          cy: sy(v),
          r: 2.5,
          fill: s.color,
          opacity: dim ? 0.28 : 1,
        })
      );
    }
    if (!markAll) {
      const last = s.points[s.points.length - 1];
      svg.append(el("circle", { cx: sx(last[0]), cy: sy(last[1]), r: 3, fill: s.color, opacity: dim ? 0.28 : 1 }));
    }
  }

  const cursor = el("line", { x1: 0, x2: 0, y1: pad.t, y2: pad.t + ih, class: "cursor hidden" });
  svg.append(cursor);
  const hit = el("rect", {
    x: pad.l,
    y: pad.t,
    width: iw,
    height: ih,
    fill: "transparent",
    style: "cursor: crosshair",
  });
  svg.append(hit);

  tip.className = "chart-tip hidden";
  tip.innerHTML = "";

  hit.addEventListener("mousemove", (ev) => {
    const rect = svg.getBoundingClientRect();
    const xPx = ev.clientX - rect.left;
    const tsCursor = x0 + ((xPx - pad.l) / iw) * spanS;

    // Snap to the nearest sample time across visible series (stable tip, no float jitter).
    let snapTs = null;
    let bestDist = Infinity;
    for (const s of visible) {
      for (const [ts] of s.points) {
        const dist = Math.abs(ts - tsCursor);
        if (dist < bestDist) {
          bestDist = dist;
          snapTs = ts;
        }
      }
    }
    if (snapTs == null || bestDist > gapS) {
      cursor.classList.add("hidden");
      tip.classList.add("hidden");
      return;
    }

    const rows = [];
    for (const s of visible) {
      const hitPt = s.points.find((p) => p[0] === snapTs) || nearest(s.points, snapTs, gapS);
      if (!hitPt) continue;
      rows.push(
        `<div class="tip-row"><i style="background:${s.color}"></i><span class="tip-id">${escapeHtml(
          s.label
        )}</span><b>${escapeHtml(fmt(hitPt[1]))}</b></div>`
      );
    }

    const cx = sx(snapTs);
    cursor.setAttribute("x1", cx);
    cursor.setAttribute("x2", cx);
    cursor.classList.remove("hidden");

    tip.innerHTML = `<div class="tip-time">${fmtClock(snapTs, spanS)}</div>${
      rows.join("") || `<div class="tip-row">No samples</div>`
    }`;
    tip.classList.remove("hidden");

    // Fixed to chart box; never change container size (avoids ResizeObserver redraw loop).
    const tipW = tip.offsetWidth || 160;
    const left = Math.min(Math.max(pad.l, cx + 10), width - tipW - 4);
    tip.style.left = `${left}px`;
    tip.style.top = `${pad.t + 4}px`;
  });

  hit.addEventListener("mouseleave", () => {
    cursor.classList.add("hidden");
    tip.classList.add("hidden");
  });

  container.append(svg);
  if (!tip.isConnected) container.append(tip);
}

function nearest(points, ts, maxDist) {
  let best = null;
  for (const p of points) {
    const d = Math.abs(p[0] - ts);
    if (d > maxDist) continue;
    if (!best || d < Math.abs(best[0] - ts)) best = p;
  }
  return best;
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

export function miniSpark(values, color, { min, max } = {}) {
  const nums = values.filter((v) => v != null);
  if (nums.length < 2) return `<span class="subtle">—</span>`;
  const w = 96;
  const h = 24;
  const lo = min ?? Math.min(...nums);
  const hi = max ?? Math.max(...nums);
  const span = hi - lo || 1;
  const pts = nums
    .map((v, i) => {
      const x = (i / (nums.length - 1)) * w;
      const y = h - 2 - ((v - lo) / span) * (h - 4);
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(" ");
  return `<svg class="mini" viewBox="0 0 ${w} ${h}" width="${w}" height="${h}"><polyline fill="none" stroke="${color}" stroke-width="1.5" points="${pts}"/></svg>`;
}
