// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "gtest/gtest.h"
#include "raptor.hpp"

using namespace raptor;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    int temp = RUN_ALL_TESTS();
    MPI_Finalize();
    return temp;
} // end of main() //

TEST(TAPAnisoSpMVTest, TestsInUtil)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    FILE* f;
    double b_val;
    int grid[2] = {25, 25};
    double eps = 0.001;
    double theta = M_PI/8.0;
    double* stencil = diffusion_stencil_2d(eps, theta);
    ParCSRMatrix* A = par_stencil_grid(stencil, grid, 2);
    setenv("PPN", "4", 1);
    A->tap_comm = new TAPComm(A->partition, A->off_proc_column_map, A->on_proc_column_map);

    ParVector x(A->global_num_cols, A->on_proc_num_cols);
    ParVector b(A->global_num_rows, A->local_num_rows);

    x.set_const_value(1.0);
    A->tap_mult(x, b);
    f = fopen("../../../../test_data/aniso_ones_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    }
    fclose(f);

    b.set_const_value(1.0);
    A->tap_mult_T(b, x);
    f = fopen("../../../../test_data/aniso_ones_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(x[i], b_val, 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        x[i] = A->partition->first_local_col + i;
    }
    A->tap_mult(x, b);
    f = fopen("../../../../test_data/aniso_inc_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->local_num_rows; i++)
    {
        b[i] = A->partition->first_local_row + i;
    }
    A->tap_mult_T(b, x);
    f = fopen("../../../../test_data/aniso_inc_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(x[i], b_val, 1e-06);
    }
    fclose(f);


    // TEST SAME WITH SIMPLE TAP COMM
    delete A->tap_comm;
    A->tap_comm = new TAPComm(A->partition, A->off_proc_column_map, A->on_proc_column_map, false);

    x.set_const_value(1.0);
    A->tap_mult(x, b);
    f = fopen("../../../../test_data/aniso_ones_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    }
    fclose(f);

    b.set_const_value(1.0);
    A->tap_mult_T(b, x);
    f = fopen("../../../../test_data/aniso_ones_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(x[i], b_val, 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        x[i] = A->partition->first_local_col + i;
    }
    A->tap_mult(x, b);
    f = fopen("../../../../test_data/aniso_inc_b.txt", "r");
    for (int i = 0; i < A->partition->first_local_row; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    }
    fclose(f);

    for (int i = 0; i < A->local_num_rows; i++)
    {
        b[i] = A->partition->first_local_row + i;
    }
    A->tap_mult_T(b, x);
    f = fopen("../../../../test_data/aniso_inc_b_T.txt", "r");
    for (int i = 0; i < A->partition->first_local_col; i++)
    {
        fscanf(f, "%lg\n", &b_val);
    }
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(x[i], b_val, 1e-06);
    }
    fclose(f);


    delete A;
    delete[] stencil;

    setenv("PPN", "16", 1);

} // end of TEST(ParAnisoSpMVTest, TestsInUtil) //
