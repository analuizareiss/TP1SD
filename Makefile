CC      = gcc
CFLAGS  = -O2 -Wall -Wextra

all: parte1 parte2

parte1:
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/producer_consumer_pipe \
		src/producer_consumer_pipe.c -lm

parte2:
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/producer_consumer_sem \
		src/producer_consumer_sem.c -lpthread -lm

test: all
	@echo "=== Parte 1: Pipe Producer-Consumer (20 numbers) ==="
	./bin/producer_consumer_pipe 20
	@echo ""
	@echo "=== Parte 2: Semaphore P-C (N=10, Np=2, Nc=4, M=200) ==="
	./bin/producer_consumer_sem 10 2 4 200

benchmark:
	cd bin && bash ../script/benchmark.sh && python3 ../plot_results.py

clean:
	rm -f bin/producer_consumer_pipe
	rm -f bin/producer_consumer_sem
	rm -f data/*.csv
	rm -f graphs/*.png

.PHONY: all parte1 parte2 test benchmark clean