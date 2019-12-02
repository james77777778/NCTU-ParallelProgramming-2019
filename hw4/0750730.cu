/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
 #include <stdio.h>
 #include <stdlib.h>
 #include <math.h>
 #include <time.h>
 
 #define MAXPOINTS 1000000
 #define MAXSTEPS 1000000
 #define MINPOINTS 20
 #define PI 3.14159265
 #define BLOCKSIZE 128
 
 void check_param(void);
 void init_line(void);
 void update (void);
 void printfinal (void);
 
 int nsteps,                 	/* number of time steps */
     tpoints, 	     		/* total points along string */
     rcode;                  	/* generic return code */
 float  values[MAXPOINTS+2], 	/* values at time t */
        oldval[MAXPOINTS+2], 	/* values at time (t-dt) */
        newval[MAXPOINTS+2]; 	/* values at time (t+dt) */
 
 
 /**********************************************************************
  *	Checks input values from parameters
  *********************************************************************/
 void check_param(void)
 {
    char tchar[20];
 
    /* check number of points, number of iterations */
    while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
       printf("Enter number of points along vibrating string [%d-%d]: "
            ,MINPOINTS, MAXPOINTS);
       scanf("%s", tchar);
       tpoints = atoi(tchar);
       if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
          printf("Invalid. Please enter value between %d and %d\n", 
                  MINPOINTS, MAXPOINTS);
    }
    while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
       printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
       scanf("%s", tchar);
       nsteps = atoi(tchar);
       if ((nsteps < 1) || (nsteps > MAXSTEPS))
          printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
    }
 
    printf("Using points = %d, steps = %d\n", tpoints, nsteps);
 
 }
 
 /**********************************************************************
  *     Initialize points on line
  *********************************************************************/
//  void init_line(void)
//  {
//     int j;
 
//     /* Calculate initial values based on sine curve */
//     for (j = 1; j <= tpoints; j++) {
//        values[j] = sin (2.0 * PI * (float)(j-1)/(tpoints-1));
//        oldval[j] = values[j];
//     }
 
//  //    /* Initialize old values array */
//  //    for (i = 1; i <= tpoints; i++) 
//  //       oldval[i] = values[i];
//  }
 
 /**********************************************************************
  *      Calculate new values using wave equation
  *********************************************************************/
 // void do_math(int i)
 // {
 //    float dtime, c, dx, tau, sqtau;
 
 //    dtime = 0.3;
 //    c = 1.0;
 //    dx = 1.0;
 //    tau = (c * dtime / dx);
 //    sqtau = tau * tau;
 //    newval[i] = (2.0 * values[i]) - oldval[i] + (sqtau *  (-2.0)*values[i]);
 // }
 
 /**********************************************************************
  *     Update all values along line a specified number of times
  *********************************************************************/
  __global__ void update(float *values_cuda, int tpoints,  int nsteps)
 {
     int i;
     int j = blockIdx.x * blockDim.x + threadIdx.x + 1;
     if (j <= tpoints) {
         float values_tmp, newval_tmp, oldval_tmp;
         values_tmp = sin(2.0f * PI * (float)(j-1)/(tpoints-1));
         oldval_tmp = values_tmp;
         /* Update values for each time step */
         for (i = 1; i<= nsteps; i++) {
             /* global endpoints */
             if ((j == 1) || (j  == tpoints))
                 newval_tmp = 0.0f;
             else
                 newval_tmp = (2.0f * values_tmp) - oldval_tmp + (0.09f * (-2.0f) * values_tmp);
 
             /* Update old values with new values */
             oldval_tmp = values_tmp;
             values_tmp = newval_tmp;
         }
         values_cuda[j] = values_tmp;
     }
 }
 
 /**********************************************************************
  *     Print final results
  *********************************************************************/
 void printfinal()
 {
    int i;
 
    for (i = 1; i <= tpoints; i++) {
       printf("%6.4f ", values[i]);
       if (i%10 == 0)
          printf("\n");
    }
 }
 
 /**********************************************************************
  *	Main program
  *********************************************************************/
 int main(int argc, char *argv[])
 {
     sscanf(argv[1],"%d",&tpoints);
     sscanf(argv[2],"%d",&nsteps);
     check_param();
     printf("Initializing points on the line...\n");
     printf("Updating all points for all time steps...\n");
     float *values_cuda;
     int size = (tpoints)*sizeof(float);
     cudaMalloc((void**)&values_cuda, size);
     update<<<(tpoints)/BLOCKSIZE+1, BLOCKSIZE>>>(values_cuda, tpoints, nsteps);
     cudaMemcpy(values, values_cuda, size, cudaMemcpyDeviceToHost);
     printf("Printing final results...\n");
     printfinal();
     printf("\nDone.\n\n");
     
     cudaFree(values_cuda);
     return 0;
 }
 