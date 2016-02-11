// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef RAPTOR_CORE_PARCOMM_HPP
#define RAPTOR_CORE_PARCOMM_HPP

#include <mpi.h>
#include <math.h>

#include "matrix.hpp"
#include "array.hpp"
#include <map>

namespace raptor
{
class ParComm
{
public:
    void init_col_to_proc(const MPI_Comm comm_mat, const index_t num_cols, std::map<index_t, index_t>& global_to_local, const index_t* global_col_starts);
    void init_comm_recvs(const MPI_Comm comm_mat, const index_t num_cols, std::map<index_t, index_t>& global_to_local);
    void init_comm_sends_unsym(const MPI_Comm comm_mat, Array<index_t>& map_to_global, const index_t* global_col_starts);

    // TODO
    ParComm()
    {
        num_sends = 0;
        size_sends = 0;
        num_recvs = 0;
        size_recvs = 0;
    }

    ParComm(const Matrix* offd, Array<index_t>& map_to_global, std::map<index_t, index_t>& global_to_local, const index_t* global_col_starts, const MPI_Comm comm_mat, const int symmetric = 1)
    {
        num_sends = 0;
        size_sends = 0;
        num_recvs = 0;
        size_recvs = 0;

        // Get MPI Information
        int rank, num_procs;
        MPI_Comm_rank(comm_mat, &rank);
        MPI_Comm_size(comm_mat, &num_procs);

        // Declare communication variables
        index_t offd_num_cols = map_to_global.size();

        // For each offd col, find proc it lies on.  Add proc and list
        // of columns it holds to map recvIndices
        if (offd_num_cols)
        {
            // Create map from columns to processors they lie on
            init_col_to_proc(comm_mat, offd_num_cols, global_to_local, global_col_starts);
            
            // Init recvs
            init_comm_recvs(comm_mat, offd_num_cols, global_to_local);
        }

        // Add processors needing to send to, and what to send to each
        init_comm_sends_unsym(comm_mat, map_to_global, global_col_starts);
    }

    ~ParComm()
    {
    };

    index_t num_sends;
    index_t num_recvs;
    index_t size_sends;
    index_t size_recvs;
    Array<index_t> send_procs;
    Array<index_t> send_row_starts;
    Array<index_t> send_row_indices;
    Array<index_t> recv_procs;
    Array<index_t> recv_col_starts;
    Array<index_t> col_to_proc; 
};
}
#endif
