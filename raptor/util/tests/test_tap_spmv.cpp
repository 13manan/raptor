#include <assert.h>
#include <math.h>
#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/stencil.hpp"
#include "gallery/par_stencil.hpp"
#include "gallery/diffusion.hpp"

using namespace raptor;

void compare(ParVector& b_par, ParVector& b_tap)
{
    double b_par_norm = b_par.norm(2);
    double b_tap_norm = b_tap.norm(2);

    assert(fabs(b_tap_norm - b_par_norm) < 1e-06);

    Vector& b_par_lcl = b_par.local;
    Vector& b_tap_lcl = b_tap.local;
    for (int i = 0; i < b_par.local_n; i++)
    {
        assert(fabs(b_par_lcl[i] - b_tap_lcl[i]) < 1e-06);
    }
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    double eps = 0.001;
    double theta = M_PI/8.0;
    int grid[2] = {10, 10};
    int dense_n = 100;
    double* stencil = diffusion_stencil_2d(eps, theta);

    // Create Parallel Matrix A_par (and vectors x_par
    // and b_par) and mult b_par <- A_par*x_par
    ParCSRMatrix* A = par_stencil_grid(stencil, grid, 2);
    ParVector x(A->global_num_cols, A->on_proc_num_cols, A->partition->first_local_col);
    ParVector b(A->global_num_rows, A->local_num_rows, A->partition->first_local_row);
    ParVector b_tap(A->global_num_rows, A->local_num_rows, A->partition->first_local_row);
    x.set_const_value(1.0);

    A->mult(x, b);
    A->tap_mult(x, b_tap);
    compare(b, b_tap);

    // Set x and x_par to same random values
    for (int i = 0; i < x.local_n; i++)
    {
        srand(i+x.first_local);
        x.local[i] = ((double)rand()) / RAND_MAX;
    }
    A->mult(x, b);
    A->tap_mult(x, b_tap);
    compare(b, b_tap);

    delete[] stencil;
    delete A;

    MPI_Finalize();
}

