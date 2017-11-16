// Copyright (c) 2015-2017, RAPtor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause

#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/matrix.hpp"
#include "gallery/diffusion.hpp"
#include "gallery/stencil.hpp"
#include "gallery/matrix_IO.hpp"
using namespace raptor;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} // end of main() //

TEST(AnisoTest, TestsInGallery)
{
    int n_rows, n_cols; 
    int row, row_nnz, nnz;
    int start, end;
    double row_sum, sum;
    int grid[2] = {25, 25};
    double eps = 0.001;
    double theta = M_PI/8.0;
    double* stencil = diffusion_stencil_2d(eps, theta);
    CSRMatrix* A_sten = stencil_grid(stencil, grid, 2);
    CSRMatrix* A_io = readMatrix("../../../../test_data/aniso.pm");

    // Compare shapes
    ASSERT_EQ(A_io->n_rows, A_sten->n_rows);
    ASSERT_EQ(A_io->n_cols, A_sten->n_cols);

    A_sten->sort();
    //A_sten->remove_duplicates();

    A_io->sort();
    //A_io->remove_duplicates();

    ASSERT_EQ(A_sten->idx1[0], A_io->idx1[0]);
    for (int i = 0; i < n_rows; i++)
    {
        // Check correct row_ptrs
        ASSERT_EQ(A_sten->idx1[i+1], A_io->idx1[i+1]);
        start = A_sten->idx1[i];
        end = A_sten->idx1[i+1];

        // Check correct col indices / values
        sum = 0;
        for (int j = start; j < end; j++)
        {
            ASSERT_EQ(A_sten->idx2[j], A_io->idx2[j]);
            //ASSERT_NEAR(A_sten->vals[j], A_io->vals[j], 1e-12);
        }
    }

    delete A_io;
    delete[] stencil;
    delete A_sten;
} // end of TEST(AnisoTest, TestsInGallery) //

