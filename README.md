# Trabalho Prático 1 — Sistemas Distribuídos
**CEFET-MG | Professora: Michelle Hanne | 2026/1**

Implementação dos mecanismos de IPC (Interprocess Communication) com pipes anônimos e semáforos, utilizando linguagem C.

---

## Requisitos

- Sistema operacional **Linux**
- Compilador **GCC**
- **Python 3** com as bibliotecas `pandas` e `matplotlib`

```bash
sudo apt install build-essential
pip install pandas matplotlib --break-system-packages
```

---

## Estrutura do Projeto

```
trabalhoSD/
├── src/
│   ├── producer_consumer_pipe.c   # Parte 1: Produtor-Consumidor com Pipes
│   └── producer_consumer_sem.c    # Parte 2: Produtor-Consumidor com Semáforos
├── bin/                           # Binários compilados (gerado pelo make)
├── data/                          # CSVs de resultados e ocupação
├── graphs/                        # Gráficos gerados pelo plot_results.py
├── script/
│   ├── benchmark.sh               # Automação do estudo de caso (tempos)
│   └── occupancy_script.sh        # Geração dos CSVs de ocupação
├── plot_results.py                # Geração dos gráficos
├── Makefile
└── README.md
```

---

## Compilação

```bash
make
```

Isso cria as pastas `bin/`, `data/` e `graphs/` e compila os dois programas.

---

## Parte 1 — Produtor-Consumidor com Pipes

Cria dois processos via `fork()` que se comunicam por um pipe anônimo. O processo pai (produtor) gera números inteiros crescentes com incremento aleatório Δ ∈ [1, 100] e os envia pelo pipe em mensagens de tamanho fixo de 20 bytes. O processo filho (consumidor) recebe os números e verifica se são primos, imprimindo o resultado. O produtor envia o valor `0` ao final para sinalizar o término.

### Uso

```bash
./bin/producer_consumer_pipe <quantidade_de_numeros>
```

**Exemplos:**
```bash
./bin/producer_consumer_pipe 5
./bin/producer_consumer_pipe 100
./bin/producer_consumer_pipe 1000
```

**Saída esperada:**
```
[Producer PID=1234] Starting. Will generate 5 numbers.
[Consumer PID=1235] Starting. Waiting for numbers...
[Producer] Sent: 1
[Consumer] #1  num=1                not prime
[Producer] Sent: 47
[Consumer] #2  num=47               PRIME
[Producer PID=1234] Sent sentinel 0. Exiting.
[Consumer] Received sentinel 0. Terminating.
[Consumer PID=1235] Processed 2 numbers. Exiting.
[Main] Both processes finished.
```

> A ordem das mensagens pode variar entre execuções — o escalonador do SO decide qual processo roda primeiro. Os dados chegam sempre na ordem correta.

---

## Parte 2 — Produtor-Consumidor com Semáforos

Cria múltiplas threads produtoras e consumidoras que compartilham um buffer circular de tamanho N. Três semáforos coordenam o acesso: um mutex para exclusão mútua, um semáforo contador de slots livres e um de slots ocupados. Produtores bloqueiam quando o buffer está cheio, consumidores bloqueiam quando está vazio. O programa encerra após consumir M números e salva a ocupação do buffer em CSV.

### Uso

```bash
./bin/producer_consumer_sem <N> <Np> <Nc> [M] [output_dir] [--no-log]
```

| Parâmetro | Descrição | Padrão |
|---|---|---|
| `N` | Tamanho do buffer compartilhado | — |
| `Np` | Número de threads produtoras | — |
| `Nc` | Número de threads consumidoras | — |
| `M` | Total de números a consumir | 100000 |
| `output_dir` | Diretório para salvar o CSV de ocupação | `.` |
| `--no-log` | Desabilita o log de ocupação (mais rápido) | — |

**Exemplos:**
```bash
./bin/producer_consumer_sem 10 1 1
./bin/producer_consumer_sem 100 4 1 100000 data
./bin/producer_consumer_sem 10 1 1 100000 data --no-log
```

---

## Estudo de Caso — Fluxo Completo

### 1. Benchmark de tempo (280 execuções)
```bash
bash script/benchmark.sh
```
Gera `data/results.csv` com os tempos de execução para todas as combinações de N ∈ {1, 10, 100, 1000} e (Np, Nc) ∈ {(1,1), (1,2), (1,4), (1,8), (2,1), (4,1), (8,1)}, com 10 execuções cada.

### 2. CSVs de ocupação do buffer (28 cenários)
```bash
bash script/occupancy_script.sh
```
Gera os 28 arquivos `data/occupancy_N*_Np*_Nc*.csv` com a ocupação do buffer ao longo do tempo para cada cenário.

### 3. Gráficos
```bash
python3 plot_results.py
```
Gera em `graphs/`:
- `exec_time.png` — tempo médio de execução por configuração de threads, uma curva por valor de N
- `occupancy_N*_Np*_Nc*.png` — ocupação do buffer ao longo do tempo para cada cenário