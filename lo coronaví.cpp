#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <random>
#include <chrono>
#include <math.h>
#include <string.h>
#include <string>
#include <atomic>
#include <tuple>
#include <sys/mman.h>

using namespace std;


/*
#define THRESHOLD 0.3
#define THRESHOLD_SUPER 0.1
#define MAX_DAYS 3
#define M 4
*/

void* share_mem(int size);

void contagio(int dia, atomic<int> * sick, int MAX_DAYS, int M, float THRESHOLD, float THRESHOLD_SUPER);

void simulate(atomic<int> * sick, int MAX_DAYS, int M, float THRESHOLD, float THRESHOLD_SUPER)
{
	int infectados = 0;
	int pid = 0;

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
	uniform_real_distribution<double> distribution(0.0,1.0);

	bool super = (distribution(generator) <= THRESHOLD_SUPER);

	for(int p = 0; p < M; p++){
		if(distribution(generator) <= THRESHOLD || super)
		{
			// Si se infecta uno o hay un Super_Threshold
			infectados++;
			pid = fork();
			if(pid == 0){
				contagio(1, sick, MAX_DAYS, M, THRESHOLD, THRESHOLD_SUPER);
				exit(0);
			}
		}
	}
	sick[0] += infectados;

	for(int x = 0; x < infectados; x++)
		wait(NULL);

	printf("[%d][Dia %d] Infecté a %d personas.\n", getpid(), 0, infectados);
	
	return;
}

void contagio(int dia, atomic<int> * sick, int MAX_DAYS, int M, float THRESHOLD, float THRESHOLD_SUPER)
{
	
	if (dia == MAX_DAYS + 1)
		return;

	int pid = 0;
	int infectados = 0;

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator (seed);
	uniform_real_distribution<double> distribution(0.0,1.0);

	bool super = (distribution(generator) <= THRESHOLD_SUPER);

	for(int p = 0; p < M; p++)
	{
		// Lo que pasa con cada persona
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine generator (seed);

		if(distribution(generator) <= THRESHOLD || super)
		{
			// Si se infecta uno o hay un Super_Threshold
			infectados++;
			pid = fork();
			if(pid == 0)
				contagio(dia + 1, sick, MAX_DAYS, M, THRESHOLD, THRESHOLD_SUPER);
		}
	}
	sick[dia] += infectados;
	
	for(int x = 0; x < infectados; x++)
		wait(NULL);

	printf("[%d][Dia %d] Infecté a %d personas. Fui infectado por %d\n", getpid(), dia, infectados, getppid());

	exit(0);
}


int main(int argc, char *argv[])
{
	int MAX_DAYS = atoi(argv[1]);
	int M = atoi(argv[2]);
	float THRESHOLD = atof(argv[3]);
	float THRESHOLD_SUPER = atof(argv[4]);

 	int size = MAX_DAYS;

	atomic<int> * sick = (atomic<int>*)share_mem(size);

	for(int d = 0; d <= MAX_DAYS; d++)
		sick[d] = 0;

	simulate(sick, MAX_DAYS, M, THRESHOLD, THRESHOLD_SUPER);

	float media = 0;
	int dia_mayor = 0;

	for(int d = 0; d < MAX_DAYS; d++)
	{
		printf("Infectados en el día %d: %u\n", d, unsigned(sick[d]));
		media += sick[d];

		if(sick[d] > sick[dia_mayor] || d == 0)
			dia_mayor = d;
	}

	media = media/MAX_DAYS;

	printf("La media por día fue: %f. y el dia con mayor contagio fue el día %d\n", media, dia_mayor);

	munmap(sick, sizeof(atomic<int>)*size);

	return 0;
}

void* share_mem(int size)
{
    void * mem;
    if( MAP_FAILED == (mem = (atomic<int>*)mmap(NULL, sizeof(atomic<int>)*size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0) ) )
    {
        perror( "mmap failed" );
        exit( EXIT_FAILURE );
    }
    return mem;	
}
