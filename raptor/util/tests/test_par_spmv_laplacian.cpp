#include <assert.h>

#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/laplacian27pt.hpp"
#include "gallery/par_stencil.hpp"

using namespace raptor;

int main(int argc, char* argv[])
{    
    MPI_Init(&argc, &argv);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    FILE* f;
    double b_val;
    int grid[3] = {10, 10, 10};
    double* stencil = laplace_stencil_27pt();
    ParCSRMatrix* A = par_stencil_grid(stencil, grid, 3);

    ParVector x(A->global_num_cols, A->on_proc_num_cols, A->partition->first_local_col);
    ParVector b(A->global_num_rows, A->local_num_rows, A->partition->first_local_row);

    x.set_const_value(1.0);
    A->mult(x, b);
    f = fopen("../../tests/laplacian27_ones_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        assert(fabs(b[i] - b_val) < 1e-06);
    }
    fclose(f);

    b.set_const_value(1.0);
    A->mult_T(b, x);
    f = fopen("../../tests/laplacian27_ones_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        assert(fabs(x[i] - b_val) < 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        x[i] = A->partition->first_local_col + i;
    }
    A->mult(x, b);
    f = fopen("../../tests/laplacian27_inc_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        assert(fabs(b[i] - b_val) < 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->local_num_rows; i++)
    {
        b[i] = A->partition->first_local_row + i;
    }
    A->mult_T(b, x);
    f = fopen("../../tests/laplacian27_inc_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        assert(fabs(x[i] - b_val) < 1e-06);
    }
    fclose(f);


    delete A;
    delete[] stencil;

    MPI_Finalize();

}