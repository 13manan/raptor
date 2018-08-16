// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause

#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/matrix.hpp"
#include "core/vector.hpp"
using namespace raptor;


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

} // end of main() //

TEST(BSRMatrixTest, TestsInCore)
{
/*    aligned_vector<int> row_ptr = {0,2,3,5};
    aligned_vector<int> cols = {0,1,1,1,2};
    aligned_vector<double> vals = {1.0, 0.0, 2.0, 1.0, 6.0, 7.0, 8.0, 2.0, 1.0, 4.0, 5.0, 1.0,
                                4.0, 3.0, 0.0, 0.0, 7.0, 2.0, 0.0, 0.0};

    aligned_vector<double[4]> blocks = {{1.0, 0.0, 2.0, 1.0}, {6.0, 7.0, 8.0, 2.0},
	    		{1.0, 4.0, 5.0, 1.0}, {4.0, 3.0, 0.0, 0.0}, {7.0, 2.0, 0.0, 0.0}};

    aligned_vector<aligned_vector<int>> indx = {{0,0}, {0,1}, {1,1}, {2,1}, {2,2}};

    int rows_in_block = 2;
    int cols_in_block = 2;
    int n = 6;

    // Create BSR Matrices (6x6)
    const BSRMatrix A_BSR1(n, n, rows_in_block, cols_in_block, row_ptr, cols, vals);
    BSRMatrix A_BSR2(n, n, rows_in_block, cols_in_block);

    // Add blocks
    for(int i=0; i<blocks.size(); i++){
        A_BSR2.add_value(indx[i][0], indx[i][1], blocks[i]);
    }

    // Check dimensions of A_BSR2
    ASSERT_EQ(A_BSR2.nnz, A_BSR1.nnz);
    //ASSERT_EQ(A_BSR2.n_blocks, A_BSR2.n_blocks);

    // Check row_ptr
    for (int i=0; i<A_BSR1.idx1.size(); i++)
    {
        ASSERT_EQ(A_BSR2.idx1[i], A_BSR1.idx1[i]);
    }

    // Check column indices
    for (int i=0; i<A_BSR1.nnz; i++)
    {
        ASSERT_EQ(A_BSR2.idx2[i], A_BSR1.idx2[i]);
    }

    // Check data
    //for (int i=0; i<A_BSR1.nnz; i++){
    //    ASSERT_EQ(A_BSR2.vals[i], A_BSR1.vals[i]);
    //}
*/
} // end of TEST(MatrixTest, TestsInCore) //

