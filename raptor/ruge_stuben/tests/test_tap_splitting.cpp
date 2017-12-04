// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause


#include "gtest/gtest.h"
#include "mpi.h"
#include "gallery/stencil.hpp"
#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "gallery/par_matrix_IO.hpp"
#include "ruge_stuben/par_cf_splitting.hpp"
#include <iostream>
#include <fstream>

using namespace raptor;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    int temp = RUN_ALL_TESTS();
    MPI_Finalize();
    return temp;
} // end of main() //

TEST(TestTAPSplitting, TestsInRuge_Stuben)
{ 
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    setenv("PPN", "4", 1);

    FILE* f;
    std::vector<int> states;
    std::vector<int> off_proc_states;
    int cf;

    ParCSRMatrix* S;

    const char* S0_fn = "../../../../test_data/rss_S0.pm";
    const char* S1_fn = "../../../../test_data/rss_S1.pm";
    const char* cf0_fn = "../../../../test_data/rss_cf0.txt";
    const char* cf1_fn = "../../../../test_data/rss_cf1.txt";
    const char* weights_fn = "../../../../test_data/weights.txt";

    // TEST LEVEL 0
    S = readParMatrix(S0_fn);
    S->tap_comm = new TAPComm(S->partition, S->off_proc_column_map);

    f = fopen(weights_fn, "r");
    std::vector<double> weights(S->local_num_rows);
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%lf\n", &weights[0]);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%lf\n", &weights[i]);
    }
    fclose(f);
    tap_split_cljp(S, states, off_proc_states, weights.data());
    
    f = fopen(cf0_fn, "r");
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%d\n", &cf);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%d\n", &cf);
        ASSERT_EQ(cf, states[i]);
    }
    fclose(f);

    delete S;

    // TEST LEVEL 1
    S = readParMatrix(S1_fn);
    S->tap_comm = new TAPComm(S->partition, S->off_proc_column_map);

    f = fopen(weights_fn, "r");
    weights.resize(S->local_num_rows);
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%lf\n", &weights[0]);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%lf\n", &weights[i]);
    }
    fclose(f);
    tap_split_cljp(S, states, off_proc_states, weights.data());
    
    f = fopen(cf1_fn, "r");
    for (int i = 0; i < S->partition->first_local_row; i++)
    {
        fscanf(f, "%d\n", &cf);
    }
    for (int i = 0; i < S->local_num_rows; i++)
    {
        fscanf(f, "%d\n", &cf);
        ASSERT_EQ(cf, states[i]);
    }
    fclose(f);

    delete S;

    setenv("PPN", "16", 1);    
   

} // end of TEST(TestParSplitting, TestsInRuge_Stuben) //


