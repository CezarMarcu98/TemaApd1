#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <string.h>
typedef double complex cplx;
char *nameInput, *nameOutput;
int numThreads;
double pi;
int N; //number of elements to do FT on them
cplx *outVector, *buff;
pthread_barrier_t barrier;
pthread_barrier_t barrierlast;


void _fft(cplx buf[], cplx out[], int n, int step)
{
	if (step < n) {
			_fft(out, buf, n, step * 2);
			_fft(out + step, buf + step, n, step * 2);
 
		for (int i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * pi * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}

void* threadFunction(void *var)
{
	int thread_id = *(int*)var;

	int start = ceil(N/numThreads) * thread_id;
	int end = fmin(ceil(N/numThreads) * (thread_id + 1), N);
	int step = 1;
	
	switch(numThreads){
		case 1:
			_fft(buff, outVector, N, 1);
			
			break;
		case 2:
			if (thread_id == 0){
				_fft(outVector, buff, N, step * 2);
			}
			else {
				_fft(outVector + step, buff + step, N, step * 2);
			}
			pthread_barrier_wait(&barrier);

			for (int i = start; i < end; i += 2 * step) {
				cplx t = cexp(-I * pi * i / N) * outVector[i + step];
				buff[i / 2]     = outVector[i] + t;
				buff[(i + N)/2] = outVector[i] - t;
			}
			pthread_barrier_wait(&barrier);
			break;

		case 4:
			if (thread_id == 0){
				_fft(buff, outVector, N, step * 2 * 2);

				pthread_barrier_wait(&barrier);
				for (int i = 0; i < N/2; i += 2 * 2) {
					cplx t = cexp(-I * pi * i / N) * buff[i + 2];
					outVector[i / 2]     = buff[i] + t;
					outVector[(i + N)/2] = buff[i] - t;
				}

			}
			else if (thread_id == 1){
				_fft(buff + step * 2, outVector + step * 2, N, step * 2 * 2);

				pthread_barrier_wait(&barrier);
				for (int i = N/2; i < N; i += 2 * 2) {
					cplx t = cexp(-I * pi * i / N) * buff[i + 2];
					outVector[i / 2]     = buff[i] + t;
					outVector[(i + N)/2] = buff[i] - t;
				}
			}
			else if (thread_id == 2){
				_fft(buff + 1, outVector + 1, N, step * 2 * 2);

				pthread_barrier_wait(&barrier);
				for (int i = 0; i < N/2; i += 2 * 2) {
					cplx t = cexp(-I * pi * i / N) * (buff + 1)[i + 2];
					(outVector + 1)[i / 2]     = (buff + 1)[i] + t;
					(outVector + 1)[(i + N)/2] = (buff + 1)[i] - t;
				}
			}
			else if (thread_id == 3){
				_fft(buff + step * 2+1, outVector + step * 2+1, N, step * 2 * 2);
			
				pthread_barrier_wait(&barrier);
				for (int i = N/2 ; i < N; i += 2 * 2) {
					cplx t = cexp(-I * pi * i / N) * (buff + 1)[i + 2];
					(outVector + 1)[i / 2]     = (buff + 1)[i] + t;
					(outVector + 1)[(i + N)/2] = (buff + 1)[i] - t;
				}

			}
			pthread_barrier_wait(&barrierlast);

			for (int i = start; i < end; i += 2 * step) {
				cplx t = cexp(-I * pi * i / N) * outVector[i + step];
				buff[i / 2]     = outVector[i] + t;
				buff[(i + N)/2] = outVector[i] - t;
			}

			break;

	}
}


void getArgs(int argc, char **argv)
{
	if(argc < 4) {
		printf("Not enough paramters\n");
		exit(1);
	}
	
	nameInput = argv[1];
	nameOutput = argv[2];
	numThreads = atoi(argv[3]);
}


void getBuffer(FILE* in)
{
	double num;
	int i = 0;

	if (fscanf(in, "%d", &N) == EOF){
		printf("Fscanf error\n");
		exit(1);
	}
 

	buff =(cplx*) malloc(sizeof(cplx)* N);
	outVector =(cplx*) malloc(sizeof(cplx)* N);
 
	if (buff == NULL){
		printf("malloc failed\n");
		exit(1); 
	}
	if (outVector == NULL){
		printf("malloc failed\n");
		exit(1);
	}
	while (fscanf(in, "%lf", &num) != EOF){
		buff[i] = num;
		outVector[i] = num; // scap de functia fft din rosetta
		i++;

	}
}

void writeValues(FILE* output)
{
	fprintf(output, "%d\n", N);
	for(int i = 0; i < N; i++){ 
		fprintf(output, "%lf %lf\n", creal(buff[i]), cimag(buff[i]));
	}

}

int main(int argc, char *argv[])
{
	int i;
	pi = atan2(1, 1) * 4;
	getArgs(argc, argv);
	pthread_t tid[numThreads];
	int thread_id[numThreads];
	
	// opening files
	FILE* input = fopen(nameInput, "r");
	FILE* output = fopen(nameOutput, "w");

	getBuffer(input);
	fclose(input); // no further need


	pthread_barrier_init(&barrier, NULL, numThreads);
	if (numThreads == 4)
		pthread_barrier_init(&barrierlast, NULL, numThreads);


	for(i = 0;i < numThreads; i++)
		thread_id[i] = i;

	for(i = 0; i < numThreads; i++) {
		pthread_create(&(tid[i]), NULL, threadFunction, &(thread_id[i]));
	}


	for(i = 0; i < numThreads; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_barrier_destroy(&barrier);

	if (numThreads ==4)
		pthread_barrier_destroy(&barrierlast);

	writeValues(output);
	fclose(output);

	free(outVector);
	free(buff);


	return 0;
}
