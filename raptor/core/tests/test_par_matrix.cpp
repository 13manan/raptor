#include <assert.h>

#include "core/types.hpp"
#include "core/matrix.hpp"
#include "core/par_matrix.hpp"
#include "gallery/stencil.hpp"
#include "gallery/par_stencil.hpp"
#include "gallery/diffusion.hpp"

using namespace raptor;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    double eps = 0.001;
    double theta = M_PI / 8.0;
    int grid[2] = {10, 10};
    double* stencil = diffusion_stencil_2d(eps, theta);
    CSRMatrix A;
    ParCSRMatrix A_par;

    stencil_grid(&A, stencil, grid, 2);
    par_stencil_grid(&A_par, stencil, grid, 2);

    ParCSCMatrix A_par_csc;
    A_par_csc.copy(&A_par);

    int lcl_nnz = A_par.local_nnz;
    int nnz;
    MPI_Allreduce(&lcl_nnz, &nnz, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    assert(A.nnz == nnz);

    double A_dense[10000] = {0};
    for (int i = 0; i < A.n_rows; i++)
    {
        for (int j = A.idx1[i]; j < A.idx1[i+1]; j++)
        {
            A_dense[i*100 + A.idx2[j]] = A.vals[j];
        }
    }

    // Compare A_par to A_dense
    for (int i = 0; i < A_par.local_num_rows; i++)
    {
        int row = i + A_par.first_local_row;
        for (int j = A_par.on_proc->idx1[i]; j < A_par.on_proc->idx1[i+1]; j++)
        {
            int col = A_par.on_proc->idx2[j] + A_par.first_local_col;
            assert(fabs(A_dense[row*100+col] - A_par.on_proc->vals[j]) < zero_tol);
        }

        for (int j = A_par.off_proc->idx1[i]; j < A_par.off_proc->idx1[i+1]; j++)
        {
            int col = A_par.off_proc_column_map[A_par.off_proc->idx2[j]];
            assert(fabs(A_dense[row*100+col] - A_par.off_proc->vals[j]) < zero_tol);
        }
    }

    // Compare A_par_csc to A_dense
    for (int i = 0; i < A_par_csc.local_num_cols; i++)
    {
        int col = i + A_par_csc.first_local_col;
        for (int j = A_par_csc.on_proc->idx1[i]; j < A_par_csc.on_proc->idx1[i+1]; j++)
        {
            int row = A_par_csc.on_proc->idx2[j] + A_par_csc.first_local_row;
            assert(fabs(A_dense[row*100+col] - A_par_csc.on_proc->vals[j]) < zero_tol);
        }
    }

    for (int i = 0; i < A_par_csc.off_proc_num_cols; i++)
    {
        int col = A_par_csc.off_proc_column_map[i];
        for (int j = A_par_csc.off_proc->idx1[i]; j < A_par_csc.off_proc->idx1[i+1]; j++)
        {
            int row = A_par_csc.off_proc->idx2[j] + A_par_csc.first_local_row;
            assert(fabs(A_dense[row*100+col] - A_par_csc.off_proc->vals[j]) < zero_tol);
        }
    }

    delete[] stencil;

    MPI_Finalize();
}


