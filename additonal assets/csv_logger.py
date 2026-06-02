import csv
from collections import defaultdict, deque

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.lines import Line2D
import serial
from serial import SerialException

PORT = "COM10"          # uprav podla svojho COM portu
BAUDRATE = 115200
MAX_POINTS = 300
SAVE_FILE = "log_metrics_blackout.csv"

INCLUDE_PATHS = {0, 1, 2}   # 0=ETH_DIRECT, 1=ETH_PLC, 2=BT ; RS485 vynechane

path_names = {
    0: "ETH_1",
    1: "ETH_2",
    2: "BT",

}

path_colors = {
    0: "tab:blue",
    1: "tab:orange",
    2: "tab:green",

}

buffers = defaultdict(lambda: {
    "tick": deque(maxlen=MAX_POINTS),
    "rtt": deque(maxlen=MAX_POINTS),
    "jitter": deque(maxlen=MAX_POINTS),
    "throughput": deque(maxlen=MAX_POINTS),
    "loss": deque(maxlen=MAX_POINTS),
    "reliability": deque(maxlen=MAX_POINTS),
    "texp": deque(maxlen=MAX_POINTS),
    "terr": deque(maxlen=MAX_POINTS),
    "missed": deque(maxlen=MAX_POINTS),
    "seq": deque(maxlen=MAX_POINTS),
})

lines = {}

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
except SerialException as e:
    raise SystemExit(f"Nepodarilo sa otvorit {PORT}: {e}")

csv_file = open(SAVE_FILE, "w", newline="", encoding="utf-8")
writer = csv.writer(csv_file)
writer.writerow([
    "tick_ms",
    "link_type",
    "path_id",
    "rtt_ms",
    "jitter_ms",
    "throughput_bps",
    "loss_permille",
    "reliability_score",
    "t_expected_ms",
    "timing_error_ms",
    "missed_intervals",
    "seq"
])
csv_file.flush()

fig = plt.figure(figsize=(15, 11))

ax1 = plt.subplot(3, 1, 1)
ax2 = plt.subplot(3, 1, 2, sharex=ax1)
ax3 = plt.subplot(3, 1, 3, sharex=ax1)

ax2b = ax2.twinx()
ax3b = ax3.twinx()

def ensure_lines(path_id):
    if path_id in lines:
        return

    color = path_colors.get(path_id, None)
    label = path_names.get(path_id, f"PATH_{path_id}")

    lines[path_id] = {
        "rtt": ax1.plot([], [], color=color, linestyle="-", linewidth=2, label=f"{label} RTT")[0],
        "jitter": ax1.plot([], [], color=color, linestyle="--", linewidth=1.8, label=f"{label} Jitter")[0],

        "throughput": ax2.plot([], [], color=color, linestyle="-", linewidth=2, label=f"{label} Throughput")[0],
        "loss": ax2b.plot([], [], color=color, linestyle=":", linewidth=2, label=f"{label} Loss")[0],

        "reliability": ax3.plot([], [], color=color, linestyle="-", linewidth=2, label=f"{label} Reliability")[0],
        "terr": ax3b.plot([], [], color=color, linestyle="--", linewidth=1.8, label=f"{label} Timing error")[0],
    }

def read_serial():
    rows = []

    while ser.in_waiting:
        raw = ser.readline().decode(errors="ignore").strip()

        if raw:
            print(raw)

        if not raw.startswith("CSV,"):
            continue

        parts = raw.split(",")

        if len(parts) != 13:
            continue

        try:
            tick = int(parts[1])
            link_type = int(parts[2])
            path_id = int(parts[3])
            rtt = int(parts[4])
            jitter = int(parts[5])
            tp = int(parts[6])
            loss = int(parts[7])
            reliability = int(parts[8])
            texp = int(parts[9])
            terr = int(parts[10])
            missed = int(parts[11])
            seq = int(parts[12])
        except ValueError:
            continue

        rows.append((
            tick, link_type, path_id, rtt, jitter, tp,
            loss, reliability, texp, terr, missed, seq
        ))

    return rows

def append_rows(rows):
    if not rows:
        return

    for tick, link_type, path_id, rtt, jitter, tp, loss, reliability, texp, terr, missed, seq in rows:
        writer.writerow([
            tick, link_type, path_id, rtt, jitter, tp,
            loss, reliability, texp, terr, missed, seq
        ])

        if path_id not in INCLUDE_PATHS:
            continue

        ensure_lines(path_id)

        b = buffers[path_id]
        b["tick"].append(tick / 1000.0)
        b["rtt"].append(rtt)
        b["jitter"].append(jitter)
        b["throughput"].append(tp)
        b["loss"].append(loss)
        b["reliability"].append(reliability)
        b["texp"].append(texp)
        b["terr"].append(terr)
        b["missed"].append(missed)
        b["seq"].append(seq)

    csv_file.flush()

def build_legends():
    path_handles = [
        Line2D([0], [0], color=path_colors[p], lw=3, label=path_names[p])
        for p in INCLUDE_PATHS
    ]

    metric_handles_1 = [
        Line2D([0], [0], color="black", lw=2, linestyle="-", label="RTT"),
        Line2D([0], [0], color="black", lw=2, linestyle="--", label="Jitter"),
    ]

    metric_handles_2 = [
        Line2D([0], [0], color="black", lw=2, linestyle="-", label="Throughput"),
        Line2D([0], [0], color="black", lw=2, linestyle=":", label="Loss"),
    ]

    metric_handles_3 = [
        Line2D([0], [0], color="black", lw=2, linestyle="-", label="Reliability"),
        Line2D([0], [0], color="black", lw=2, linestyle="--", label="Timing error"),
    ]

    ax1.legend(handles=path_handles + metric_handles_1, loc="upper right", ncol=2)
    ax2.legend(handles=path_handles + metric_handles_2, loc="upper right", ncol=2)
    ax3.legend(handles=path_handles + metric_handles_3, loc="upper right", ncol=2)

build_legends()

def update(_frame):
    rows = read_serial()
    append_rows(rows)

    for path_id in INCLUDE_PATHS:
        if path_id not in lines:
            continue

        b = buffers[path_id]
        x = list(b["tick"])

        lines[path_id]["rtt"].set_data(x, list(b["rtt"]))
        lines[path_id]["jitter"].set_data(x, list(b["jitter"]))

        lines[path_id]["throughput"].set_data(x, list(b["throughput"]))
        lines[path_id]["loss"].set_data(x, list(b["loss"]))

        lines[path_id]["reliability"].set_data(x, list(b["reliability"]))
        lines[path_id]["terr"].set_data(x, list(b["terr"]))

    for ax in [ax1, ax2, ax2b, ax3, ax3b]:
        ax.relim()
        ax.autoscale_view()

    ax1.set_title("RTT and Jitter")
    ax1.set_ylabel("Time [ms]")
    ax1.grid(True, alpha=0.3)

    ax2.set_title("Throughput and Packet Loss")
    ax2.set_ylabel("Throughput [bps]")
    ax2b.set_ylabel("Loss [permille]")
    ax2.grid(True, alpha=0.3)

    ax3.set_title("Reliability and Timing Error")
    ax3.set_ylabel("Reliability score")
    ax3b.set_ylabel("Timing error [ms]")
    ax3.set_xlabel("Time [s]")
    ax3.grid(True, alpha=0.3)

    fig.suptitle(f"STM32 live metrics from {PORT} (RS485 hidden)")

def on_close(_event):
    try:
        if ser.is_open:
            ser.close()
    finally:
        csv_file.close()

fig.canvas.mpl_connect("close_event", on_close)

ani = FuncAnimation(fig, update, interval=200, cache_frame_data=False)
plt.tight_layout()
plt.show()
