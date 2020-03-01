#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <string.h>
char *nameInput, *nameOutput;
int numThreads;
double pi;
int N; //number of elements to do FT on them
typedef double complex cplx;
cplx *outVector, *buff;

   
void* threadFunction(void *var)
{
	int thread_id = *(int*)var;

	int start = ceil(N/numThreads) * thread_id;
	int end = fmin(ceil(N/numThreads) * (thread_id + 1), N);

	for (int i = start; i < end; i++){
		for (int j = 0; j < N; j ++){
			outVector[i] += buff[j] * cexp(-2 * I * pi / N * i * j);
		}
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



void getBuffer(FILE* in){
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
		i++;
	}
}

void writeValues(FILE* output){
	fprintf(output, "%d\n", N);
	for(int i = 0; i < N; i++){ 
		fprintf(output, "%lf %lf\n", creal(outVector[i]), cimag(outVector[i]));
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

	for(i = 0;i < numThreads; i++)
		thread_id[i] = i;

	for(i = 0; i < numThreads; i++) {
		pthread_create(&(tid[i]), NULL, threadFunction, &(thread_id[i]));
	}

	for(i = 0; i < numThreads; i++) {
		pthread_join(tid[i], NULL);
	}
	writeValues(output);
	fclose(output);
	
	free(buff);
	free(outVector);

	return 0;
}
