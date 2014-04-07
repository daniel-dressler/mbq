all: mbq_benchmark.c mbq.c
	cc -O3 mbq.c mbq_benchmark.c -o benchmark
	
