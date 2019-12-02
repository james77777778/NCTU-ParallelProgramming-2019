#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#ifndef W
#define W 20                                    // Width
#endif
#define LEFT_TAG 10
#define RIGHT_TAG 11
#define BALANCE_TAG 20
#define RECV_BALANCE_TAG 21
#define MIN_TEMP_TAG 30
int main(int argc, char **argv) {
  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
  int *temp = malloc(L*W*sizeof(int));          // Current temperature
  int *next = malloc(L*W*sizeof(int));          // Next time step

  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      temp[i*W+j] = random()>>3;
    }
  }
  // MPI init
  MPI_Init(&argc, &argv);
  int my_rank;
  int nproc;
  MPI_Status status; 
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  // split L into nproc pieces
  int L_size[nproc];
  for (int i = 0; i < nproc; i++) {
    L_size[i] = L / nproc;
  }
  for (int i = 0; i < L % nproc; i++) {
    L_size[i] += 1;
  }
  int left_ix = 0;
  for (int i = 0; i < my_rank; i++) {
    left_ix += L_size[i];
  }
  int right_ix = left_ix + L_size[my_rank];

  int count = 0, balance = 0, global_balance = 0;
  int *left_send_vector = malloc(W*sizeof(int));
  int *right_send_vector = malloc(W*sizeof(int));
  int *left_recv_vector = malloc(W*sizeof(int));
  int *right_recv_vector = malloc(W*sizeof(int));
  while (iteration--) {     // Compute with up, left, right, down points
    balance = 1;
    count++;
    // start heat conduction
    for (int i = left_ix; i < right_ix; i++) {
      for (int j = 0; j < W; j++) {
        next[i*W+j] = d*(temp[i*W+j]/d+temp[i*W+j]*-4 + temp[(i-1<0?0:i-1)*W+j] + temp[(i+1>=L?i:i+1)*W+j] + temp[i*W+(j-1<0?0:j-1)] + temp[i*W+(j+1>=W?j:j+1)]);
        if (next[i*W+j] != temp[i*W+j]) {
          balance = 0;
        }
      }
    }
    // exchange message
    // prepare MPI_Request
    MPI_Request left_request, right_request;
    // prepare send vector (left/right)
    if (my_rank != 0) {
      for (int j = 0; j < W; j++) {
        left_send_vector[j] = next[left_ix*W+j];
      }
      MPI_Isend(left_send_vector, W, MPI_INT, my_rank-1, LEFT_TAG, MPI_COMM_WORLD, &left_request);
      MPI_Irecv(left_recv_vector, W, MPI_INT, my_rank-1, RIGHT_TAG, MPI_COMM_WORLD, &left_request);
    }
    if (my_rank != nproc-1) {
      for (int j = 0; j < W; j++) {
        right_send_vector[j] = next[(right_ix-1)*W+j];
      }
      MPI_Isend(right_send_vector, W, MPI_INT, my_rank+1, RIGHT_TAG, MPI_COMM_WORLD, &right_request);
      MPI_Irecv(right_recv_vector, W, MPI_INT, my_rank+1, LEFT_TAG, MPI_COMM_WORLD, &right_request);
    }
    if (my_rank != 0) {
      MPI_Wait(&left_request, &status);
      for (int j = 0; j < W; j++) {
        next[(left_ix-1)*W+j] = left_recv_vector[j];
      }
    }
    if (my_rank != nproc-1) {
      MPI_Wait(&right_request, &status);
      for (int j = 0; j < W; j++) {
        next[right_ix*W+j] = right_recv_vector[j];
      }
    }
    int *tmp = temp;
    temp = next;
    next = tmp;
    MPI_Allreduce(&balance, &global_balance, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    if (global_balance){
      break;
    }
  }
  // check min temp
  int min = temp[left_ix*W];
  for (int i = left_ix; i < right_ix; i++) {
    for (int j = 0; j < W; j++) {
      if (temp[i*W+j] < min) {
        min = temp[i*W+j];
      }
    }
  }
  int global_min;
  MPI_Reduce(&min, &global_min, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
  if (my_rank == 0) {
    printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, global_min);
  }
  MPI_Finalize();
  return 0;
}