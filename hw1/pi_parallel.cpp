#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <iostream>
using namespace std;

// fastrand
inline int fastrand(register unsigned int& g_seed){ 
  return ((g_seed = (214013*g_seed+2531011))>>16)&0x7FFF; 
} 

struct tosses_param
{
    unsigned long long number_of_cpu, number_of_tosses;
    unsigned int g_seed;
};

pthread_mutex_t mutex;
unsigned long long total_in_circle;

// pthread job
inline void* pthread_tosses(void* param)
{
    struct tosses_param* p = (struct tosses_param*) param;
    register unsigned long long number_in_circle = 0;
    register unsigned long long number_of_tosses = p->number_of_tosses / p->number_of_cpu;
    register unsigned int seed = p->g_seed;
    register int x, y;
    // printf("ntoss: %llu, ncpu: %llu, nptoss: %llu\n", number_of_tosses, p->number_of_cpu, p->number_of_tosses);
    for (register unsigned long long toss = 0; toss < number_of_tosses; toss++) {
        // x = random double between -1 and 1;
        x = fastrand(seed);
        // y = random double between -1 and 1;
        y = fastrand(seed);
        if (x*x + y*y <= 1073676289)
            number_in_circle++;
    }
    pthread_mutex_lock(&mutex);
    total_in_circle += number_in_circle;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char **argv)
{
    double pi_estimate;
    unsigned long long  number_of_cpu, number_of_tosses;
    if ( argc < 2) {
        exit(-1);
    }
    number_of_cpu = atoi(argv[1]);
    number_of_tosses = atoll(argv[2]);
    if (( number_of_cpu < 1) || ( number_of_tosses < 0)) {
        exit(-1);
    }

    long thread;
    pthread_t* thread_handles;
    thread_handles = (pthread_t*) malloc (number_of_cpu*sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);

    struct tosses_param p;
    p.g_seed = time(NULL);
    p.number_of_cpu = number_of_cpu;
    p.number_of_tosses = number_of_tosses;
    for (thread = 0; thread < number_of_cpu; thread++)
        pthread_create(&thread_handles[thread], NULL, pthread_tosses, &p);
        
    for (thread = 0; thread < number_of_cpu; thread++)
        pthread_join(thread_handles[thread], NULL);
    
    pi_estimate = 4*total_in_circle/((double) number_of_tosses);

    printf("%f\n",pi_estimate);
    return 0;
}
