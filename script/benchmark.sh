SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$ROOT/bin/producer_consumer_sem"
M=100000
RUNS=10

N_VALUES=(1 10 100 1000)
NP_NC_PAIRS=("1 1" "1 2" "1 4" "1 8" "2 1" "4 1" "8 1")

CSV="$ROOT/data/results.csv"
echo "N,Np,Nc,run,time" > "$CSV"

for N in "${N_VALUES[@]}"; do
    for pair in "${NP_NC_PAIRS[@]}"; do
        Np=$(echo $pair | awk '{print $1}')
        Nc=$(echo $pair | awk '{print $2}')
        echo "--- N=$N Np=$Np Nc=$Nc ---"
        for ((run=1; run<=RUNS; run++)); do
            output=$(LC_ALL=C $BINARY $N $Np $Nc $M "$ROOT/data" 2>/dev/null)
            t=$(echo "$output" | grep "^RESULT" | sed 's/.*time=//;s/ .*//')
            echo "  run $run: ${t}s"
            echo "$N,$Np,$Nc,$run,$t" >> "$CSV"
        done
    done
done

echo ""
echo "Results saved to $CSV"