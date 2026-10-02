export function fmtTs(ts) {
  if (!ts) return "—";
  return new Date(ts * 1000).toLocaleString();
}

export function fmtRelative(ts) {
  if (!ts) return "—";
  const diff = Math.floor(Date.now() / 1000) - ts;
  if (diff < 60) return "just now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m ago`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h ago`;
  return `${Math.floor(diff / 86400)}d ago`;
}

export function fmtUptime(seconds) {
  if (seconds == null || Number.isNaN(seconds)) return "—";
  const s = Number(seconds);
  const days = Math.floor(s / 86400);
  const hours = Math.floor((s % 86400) / 3600);
  const mins = Math.floor((s % 3600) / 60);
  if (days > 0) return `${days}d ${hours}h ${mins}m`;
  if (hours > 0) return `${hours}h ${mins}m`;
  return `${mins}m ${s % 60}s`;
}

export function esc(value) {
  return String(value ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

export function fmtDuration(seconds) {
  if (seconds == null) return "—";
  const s = Number(seconds);
  if (s < 60) return `${s}s`;
  if (s < 3600) return `${Math.floor(s / 60)}m${s % 60 ? ` ${s % 60}s` : ""}`;
  return `${Math.floor(s / 3600)}h ${Math.floor((s % 3600) / 60)}m`;
}

// esp_reset_reason_t
const RESET_REASONS = [
  "unknown", "power-on", "external pin", "software", "panic", "interrupt WDT",
  "task WDT", "other WDT", "deep sleep", "brownout", "SDIO", "USB", "JTAG",
  "eFuse error", "power glitch", "CPU lockup",
];

export function resetReason(code) {
  if (code == null) return "—";
  return RESET_REASONS[code] ? `${RESET_REASONS[code]} (${code})` : String(code);
}

const WIFI_STATUS = { 0: "idle", 1: "no SSID", 3: "connected", 4: "connect failed", 5: "connection lost", 6: "disconnected" };

export function wifiStatus(code) {
  if (code == null) return "—";
  return WIFI_STATUS[code] ? `${WIFI_STATUS[code]} (${code})` : String(code);
}

export function fmtBytes(bytes) {
  if (bytes == null) return "—";
  const n = Number(bytes);
  if (n >= 1_000_000) return `${(n / 1_000_000).toFixed(1)} MB`;
  if (n >= 1000) return `${(n / 1000).toFixed(0)} KB`;
  return `${n} B`;
}
