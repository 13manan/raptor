#include <math.h>
#include "Matrix.hpp"
#include "ParMatrix.hpp"
#include "ParVector.hpp"
#include "Diffusion.hpp"
#include "Stencil.hpp"
#include "spmv.hpp"
int main ( int argc, char *argv[] )
{  
    int ilower, iupper;
    int local_size, extra;
    double strong_threshold;

    double eps = 1.0;
    double theta = 0.0;

    int* grid = (int*) calloc(2, sizeof(int));
    grid[0] = 3;
    grid[1] = 3;

    int dim = 2;
   
    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
 
    double* stencil = diffusion_stencil_2d(eps, theta);
    ParMatrix* A = stencil_grid(stencil, grid, dim);

    int globalNumRows = A->globalRows;
    int localNumRows = A->localRows;

    // Create the rhs and solution
    ParVector* b = new ParVector(globalNumRows, localNumRows);
    ParVector* x = new ParVector(globalNumRows, localNumRows);
   
    x->setConstValue(1.);
    parallelSPMV(A, x, b, 1., 0.);
   
   // Finalize MPI
   MPI_Finalize();
   
   return(0);
}


