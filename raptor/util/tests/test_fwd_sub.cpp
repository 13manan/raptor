#include <assert.h>
#include <math.h>
#include "core/types.hpp"
#include "core/matrix.hpp"
#include "gallery/matrix_IO.hpp"

using namespace raptor;

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  
    // Read in lower triangular matrix
    char* fname = "LFAT5_low.mtx";
    CSRMatrix* A = readMatrix(fname, 0);

    Vector x(A->n_rows);
    Vector b(A->n_rows);
    b.set_const_value(1.0);

    A->fwd_sub_fanin(x, b);
 
    double x_norm = x.norm(2);
    printf("Seq A norm: %f\n", x_norm);

    delete A;
	MPI_Finalize();

}

