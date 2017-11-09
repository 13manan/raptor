#include <assert.h>

#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/exxon_reader.hpp"
#include "gallery/matrix_IO.hpp"

using namespace raptor;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Create A from diffusion stencil
    char* folder = (char *)"/Users/abienz/Documents/Parallel/exxonmobil/verification-twomatrices/DA_V5_blk_coord_binary-3x3-blk-np16";
    char* fname = (char *)"DA_V5_blk_coord_binary-3x3-blk-np16_TS6_TSA0_NI0_R";
    char* iname = (char *)"index_R";
    char* suffix = (char *)".bcoord_bin";
    char* suffix_x = (char *)".sol_bin";
    char* suffix_b = (char *)".rhs_bin";

    std::vector<int> on_proc_column_map;

    ParCSRMatrix* A = exxon_reader(folder, iname, fname, suffix, on_proc_column_map);
    ParVector x;
    ParVector b;
    exxon_vector_reader(folder, fname, suffix_x, A->partition->first_local_col, x);
    exxon_vector_reader(folder, fname, suffix_b, A->partition->first_local_row, b);

    ParVector b_rap = ParVector(A->global_num_rows, A->local_num_rows, 
            A->partition->first_local_row);
    A->mult(x, b_rap);


    for (int i = 0; i < A->local_num_rows; i++)
    {
        assert(fabs(b.local[i] - b_rap.local[i]) < 1e-06);
    }


    ParVector b_tap = ParVector(A->global_num_rows, A->local_num_rows, 
            A->partition->first_local_row);
    A->tap_comm = new TAPComm(A->partition, A->off_proc_column_map);
    A->tap_mult(x, b_tap);
    for (int i = 0; i < A->local_num_rows; i++)
    {
        assert(fabs(b.local[i] - b_tap.local[i]) < 1e-08);
    }

    delete A;

    MPI_Finalize();
}   

