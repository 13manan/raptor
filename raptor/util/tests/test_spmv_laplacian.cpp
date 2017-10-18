// EXPECT_EQ and ASSERT_EQ are macros
// EXPECT_EQ test execution and continues even if there is a failure
// ASSERT_EQ test execution and aborts if there is a failure
// The ASSERT_* variants abort the program execution if an assertion fails 
// while EXPECT_* variants continue with the run.


#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/matrix.hpp"
#include "gallery/laplacian27pt.hpp"
#include "gallery/stencil.hpp"
#include "gallery/matrix_IO.hpp"

using namespace raptor;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

} // end of main() //

TEST(LaplacianSpMVTest, TestsInUtil)
{
    double b_val;
    int grid[3] = {10, 10, 10};
    double* stencil = laplace_stencil_27pt();
    CSRMatrix* A_sten = stencil_grid(stencil, grid, 3);

    Vector x(A_sten->n_rows);
    Vector b(A_sten->n_rows);
    
    // Test b <- A*ones
    x.set_const_value(1.0);
    A_sten->mult(x, b);
    FILE* f = fopen("../../tests/laplacian27_ones_b.txt", "r");
    for (int i = 0; i < A_sten->n_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    } 
    fclose(f);

    // Test b <- A_T*ones
    A_sten->mult_T(x, b);
    f = fopen("../../tests/laplacian27_ones_b_T.txt", "r");
    for (int i = 0; i < A_sten->n_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    } 
    fclose(f);

    // Tests b <- A*incr
    for (int i = 0; i < A_sten->n_rows; i++)
    {
        x[i] = i;
    }
    A_sten->mult(x, b);
    f = fopen("../../tests/laplacian27_inc_b.txt", "r");
    for (int i = 0; i < A_sten->n_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    } 
    fclose(f);

    // Tests b <- A_T*incr
    A_sten->mult_T(x, b);
    f = fopen("../../tests/laplacian27_inc_b_T.txt", "r");
    for (int i = 0; i < A_sten->n_rows; i++)
    {
        fscanf(f, "%lg\n", &b_val);
        ASSERT_NEAR(b[i], b_val, 1e-06);
    } 
    fclose(f);

} // end of TEST(LaplacianSpMVTest, TestsInUtil) //

