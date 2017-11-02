// EXPECT_EQ and ASSERT_EQ are macros
// EXPECT_EQ test execution and continues even if there is a failure
// ASSERT_EQ test execution and aborts if there is a failure
// The ASSERT_* variants abort the program execution if an assertion fails 
// while EXPECT_* variants continue with the run.


#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/par_matrix_IO.hpp"
#include "tests/par_compare.hpp"

using namespace raptor;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    MPI_Finalize();

} // end of main() //


TEST(ParStrengthTest, TestsInTests)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    ParCSRMatrix* A;
    ParCSRMatrix* S;
    ParCSRBoolMatrix* S_rap;

    A = readParMatrix("../../../test_data/rss_A0.mtx", MPI_COMM_WORLD, 1, 1);
    S = readParMatrix("../../../test_data/rss_S0.mtx", MPI_COMM_WORLD, 1, 1);
    S_rap = A->strength(0.25);
    compare(S, S_rap);
    delete A;
    delete S;
    delete S_rap;

    A = readParMatrix("../../../test_data/rss_A1.mtx", MPI_COMM_WORLD, 1, 0);
    S = readParMatrix("../../../test_data/rss_S1.mtx", MPI_COMM_WORLD, 1, 0);
    S_rap = A->strength(0.25);
    compare(S, S_rap);
    delete A;
    delete S;
    delete S_rap;

} // end of  TEST(ParStrengthTest, TestsInTests) //
