#include <mpi.h>
#include <math.h>
#include "core/types.hpp"
#include "util/linalg/spmv.hpp"
#include "gallery/external/mfem_wrapper.hpp"
#include "gallery/external/hypre_wrapper.hpp"
#include "gallery/laplacian27pt.hpp"
#include "gallery/diffusion.hpp"
#include "gallery/stencil.hpp"
#include "hypre_async.h"
#include "core/puppers.hpp"
#include <unistd.h>

using namespace raptor;

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int num_levels = 25;
    int ids[num_levels][8];
    char names[num_levels][8][20];
    int init_id = 1;

    for (int i = 0; i < num_levels; i++)
    {
        snprintf(names[i][0], 20, "SpMV %d", i);
        //ids[i][0] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][0], init_id + (i*8) + 1);
        ids[i][0] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][0]);

        snprintf(names[i][1], 20, "DiagSpmv %d", i);
        //ids[i][1] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][1], init_id + (i*8) + 2);
        ids[i][1] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][1]);

        snprintf(names[i][2], 20, "OffdSpMV %d", i);
        //ids[i][2] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][2], init_id + (i*8) + 3);
        ids[i][2] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][2]);

        snprintf(names[i][3], 20, "Waitany (Recv) %d", i);
        //ids[i][3] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][3], init_id + (i*8) + 4);
        ids[i][3] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][3]);

        snprintf(names[i][4], 20, "Waitall (Recv) %d", i);
        //ids[i][4] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][4], init_id + (i*8) + 5);
        ids[i][4] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][4]);

        snprintf(names[i][5], 20, "Waitall (Send) %d", i);
        //ids[i][5] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][5], init_id + (i*8) + 6);
        ids[i][5] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][5]);

        snprintf(names[i][6], 20, "Irecv %d", i);
        //ids[i][6] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][6], init_id + (i*8) + 7);
        ids[i][6] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][6]);

        snprintf(names[i][7], 20, "Isend %d", i);
        //ids[i][7] = _TRACE_REGISTER_FUNCTION_ID((char*) names[i][7], init_id + (i*8) + 8);
        ids[i][7] = _TRACE_REGISTER_FUNCTION_NAME((char*) names[i][7]);
    }

MPI_Barrier(MPI_COMM_WORLD);
usleep(1000);
_TRACE_END();

    // Get Local Process Rank, Number of Processes
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Get Command Line Arguments (Must Have 5)
    // TODO -- Fix how we parse command line
    int num_tests = 10;
    int num_elements = 10;
    int async = 0;
    if (argc > 1)
    {
        num_tests = atoi(argv[1]);
        if (argc > 2)
        {
            num_elements = atoi(argv[2]);
            if (argc > 3)
            {
                async = atoi(argv[3]);
            }
        }
    }

    // Declare Variables
    ParMatrix* A;
    ParVector* x;
    ParVector* b;
    Hierarchy* ml;
    ParMatrix* A_l;
    ParVector* x_l;
    ParVector* b_l;

    long local_nnz;
    long global_nnz;
    index_t len_b, len_x;
    index_t local_rows;
    data_t b_norm;
    data_t t0, tfinal;
    data_t* b_data;
    data_t* x_data;

    // Get matrix and vectors from MFEM
    //mfem_laplace(&A, &x, &b, mesh, num_elements, order);
    int dim = 3;
    int grid[dim] = {num_elements, num_elements, num_elements};
    data_t* sten = laplace_stencil_27pt();
    A = stencil_grid(sten, grid, dim);
    delete[] sten;
    b = new ParVector(A->global_cols, A->local_cols, A->first_col_diag);
    x = new ParVector(A->global_rows, A->local_rows, A->first_row);
    x->set_const_value(1.0);

    // Calculate and Print Number of Nonzeros in Matrix
    local_nnz = 0;
    if (A->local_rows)
    {
        local_nnz = A->diag->nnz + A->offd->nnz;
    }
    global_nnz = 0;
    MPI_Reduce(&local_nnz, &global_nnz, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) printf("Num Nonzeros = %lu\n", global_nnz);

    t0, tfinal;

    // Create hypre (amg_data) and raptor (ml) hierarchies (they share data)
    ml = create_wrapped_hierarchy(A, x, b);
    num_levels = ml->num_levels;
    Level* l0 = ml->levels[0];
    l0->x = x;
    l0->b = b;
    l0->has_vec = true;



_TRACE_BEGIN();
MPI_Barrier(MPI_COMM_WORLD);
usleep(1000);

    for (int i = 0; i < num_levels; i++)
    {
        Level* l = ml->levels[i];

        A_l = l->A;
        x_l = l->x;
        b_l = l->b;

        int l_reg = MPI_Register((void*) &l, (MPI_PupFn) pup_par_level);

        MPI_Migrate();

        // Test CSC Synchronous SpMV
        _TRACE_BEGIN_FUNCTION_NAME(names[i][0]);
        for (int j = 0; j < num_tests; j++)
        {
            parallel_spmv(A_l, x_l, b_l, 1.0, 0.0, async, names[i]);
        }
        _TRACE_END_FUNCTION_NAME(names[i][0]);

    }

MPI_Barrier(MPI_COMM_WORLD);
usleep(1000);
_TRACE_END();


    delete ml;

    delete A;
    delete x;
    delete b;


    MPI_Finalize();

    return 0;
}



