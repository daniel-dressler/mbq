all: mbq_benchmark.c mbq.c
	cc -std=c99 -O3 mbq.c mbq_benchmark.c -o benchmark
	
