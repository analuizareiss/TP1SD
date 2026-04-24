import os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.abspath(__file__))
DATA_DIR   = os.path.join(ROOT, "data")
GRAPHS_DIR = os.path.join(ROOT, "graphs")


def plot_execution_times(csv_path=None, out=None):
    if csv_path is None:
        csv_path = os.path.join(DATA_DIR, "results.csv")
    if out is None:
        out = os.path.join(GRAPHS_DIR, "exec_time.png")
    if not os.path.exists(csv_path):
        print(f"[WARN] {csv_path} not found – skipping execution-time plot.")
        return

    df = pd.read_csv(csv_path)
    df["config"] = df.apply(lambda r: f"({int(r.Np)},{int(r.Nc)})", axis=1)
    means = df.groupby(["N", "config"])["time"].mean().reset_index()

    config_order = ["(1,1)","(1,2)","(1,4)","(1,8)","(2,1)","(4,1)","(8,1)"]
    means["config"] = pd.Categorical(means["config"],
                                     categories=config_order, ordered=True)
    means = means.sort_values(["N", "config"])

    fig, ax = plt.subplots(figsize=(11, 6))
    colors = ["#e63946", "#457b9d", "#2a9d8f", "#e9c46a"]
    N_values = sorted(means["N"].unique())

    for idx, n_val in enumerate(N_values):
        sub = means[means["N"] == n_val]
        ax.plot(sub["config"], sub["time"],
                marker="o", linewidth=2.2, markersize=7,
                color=colors[idx % len(colors)],
                label=f"N={int(n_val)}")

    ax.set_xlabel("Configuração (Np, Nc)", fontsize=13)
    ax.set_ylabel("Tempo médio de execução (s)", fontsize=13)
    ax.set_title("Tempo médio de execução × Configuração de threads\n"
                 "Produtor-Consumidor com Semáforos (M=100 000)", fontsize=14)
    ax.legend(title="Tamanho do buffer", fontsize=11)
    ax.grid(axis="y", linestyle="--", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    plt.close(fig)
    print(f"[OK] Saved {out}")



def plot_occupancy(csv_path, out_path, title, N):
    if not os.path.exists(csv_path):
        print(f"[WARN] {csv_path} not found – skipping.")
        return

    df = pd.read_csv(csv_path)
    ops = df["operation"].values
    occ = df["occupancy"].values

    MAX_POINTS = 5000
    if len(ops) > MAX_POINTS:
        step = len(ops) // MAX_POINTS
        ops  = ops[::step]
        occ  = occ[::step]

    fig, ax = plt.subplots(figsize=(11, 5))
    ax.plot(ops, occ, linewidth=0.8, color="#e63946", alpha=0.85)
    ax.fill_between(ops, occ, alpha=0.15, color="#e63946")
    ax.axhline(N, color="#457b9d", linestyle="--", linewidth=1.2,
               label=f"Capacidade máxima (N={N})")
    ax.set_xlabel("Número da operação", fontsize=12)
    ax.set_ylabel("Ocupação do buffer", fontsize=12)
    ax.set_title(title, fontsize=13)
    ax.set_ylim(-0.5, N + 1)
    ax.legend(fontsize=10)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"[OK] Saved {out_path}")


if __name__ == "__main__":
    plot_execution_times()

    scenarios = [
        # (N, Np, Nc)
        (10,   1, 1),
        (10,   1, 4),
        (10,   4, 1),
        (100,  1, 1),
        (100,  1, 4),
        (100,  4, 1),
        (1000, 1, 1),
    ]

    for (n_val, np_val, nc_val) in scenarios:
        csv   = os.path.join(DATA_DIR,   f"occupancy_N{n_val}_Np{np_val}_Nc{nc_val}.csv")
        out   = os.path.join(GRAPHS_DIR, f"occupancy_N{n_val}_Np{np_val}_Nc{nc_val}.png")
        title = (f"Ocupação do buffer ao longo do tempo\n"
                 f"N={n_val}  Np={np_val}  Nc={nc_val}  M=100 000")
        plot_occupancy(csv, out, title, n_val)

    print("\nDone.")