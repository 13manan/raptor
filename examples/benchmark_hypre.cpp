#include <mpi.h>
#include <math.h>

#include "core/par_matrix.hpp"
#include "core/par_vector.hpp"
#include "core/hierarchy.hpp"
#include "gallery/stencil.hpp"
#include "gallery/diffusion.hpp"
#include "gallery/external/hypre_wrapper.hpp"
#include "util/linalg/relax.hpp"

//using namespace raptor;
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    index_t n = 50;

    index_t num_tests = 1;

    data_t eps = 0.1;
    data_t theta = 0.0;
    index_t* grid = new index_t[2];
    grid[0] = n;
    grid[1] = n;
    index_t dim = 2;
    data_t* stencil = diffusion_stencil_2d(eps, theta);
    ParMatrix* A = stencil_grid(stencil, grid, dim, CSR);
    delete[] stencil;
    delete[] grid;
    
    int global_num_rows = A->global_rows;
    int local_num_rows = A->local_rows;
    int global_num_cols = A->global_cols;
    int local_num_cols = A->local_cols;
    int first_row = A->first_row;
    int first_col_diag = A->first_col_diag;
    ParVector* b = new ParVector(global_num_cols, local_num_cols, first_col_diag);
    ParVector* x = new ParVector(global_num_rows, local_num_rows, first_row);
    ParVector* result = new ParVector(global_num_cols, local_num_cols, first_col_diag);
    b->set_const_value(0.0);
    x->set_const_value(1.0);
    result->set_const_value(0.0);

    Hierarchy* ml = create_wrapped_hierarchy(A, x, b);

    int num_levels = ml->num_levels;

    ml->solve(x, b);
    data_t r_norm;

    delete ml;
    delete A;
    delete x;
    delete b;

    MPI_Finalize();

    return 0;
}

