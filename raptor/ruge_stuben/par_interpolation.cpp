// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "assert.h"
#include "core/types.hpp"
#include "core/par_matrix.hpp"

using namespace raptor;

// TODO -- if in S, col is positive, otherwise col is -(col+1)
CSRMatrix* communicate(ParCSRMatrix* A, ParCSRMatrix* S, const aligned_vector<int>& states,
        const aligned_vector<int>& off_proc_states, CommPkg* comm)
{
    
    int start, end, col;
    int ctr_S, end_S, global_col;
    int tmp_col;

    aligned_vector<int> rowptr(A->local_num_rows + 1);
    aligned_vector<int> col_indices;
    aligned_vector<double> values;
    if (A->local_nnz)
    {
        col_indices.reserve(A->local_nnz);
        values.reserve(A->local_nnz);
    }
    rowptr[0] = 0;
    for (int i = 0; i < A->local_num_rows; i++)
    {
        start = A->on_proc->idx1[i]+1;
        end = A->on_proc->idx1[i+1];
        ctr_S = S->on_proc->idx1[i]+1;
        end_S = S->on_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = A->on_proc->idx2[j];
            if (states[col] == 1)
            {
                global_col = A->on_proc_column_map[col];
                if (ctr_S < end_S && S->on_proc->idx2[ctr_S] == col)
                {
                    col_indices.emplace_back(global_col);
                    ctr_S++;
                }
                else
                {
                    col_indices.emplace_back(-(global_col+1));
                }
                values.emplace_back(A->on_proc->vals[j]);
            }
            else if (ctr_S < end_S && S->on_proc->idx2[ctr_S] == col)
            {
                ctr_S++;
            }
        }

        start = A->off_proc->idx1[i];
        end = A->off_proc->idx1[i+1];
        ctr_S = S->off_proc->idx1[i];
        end_S = S->off_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = A->off_proc->idx2[j];
            if (off_proc_states[col] == -3) continue;
            global_col = A->off_proc_column_map[col];
            if (ctr_S < end_S && S->off_proc_column_map[S->off_proc->idx2[ctr_S]] == global_col)
            {
                if (off_proc_states[col] == 0)
                {
                    global_col += A->partition->global_num_cols;
                }
                col_indices.emplace_back(global_col);
                ctr_S++;
            }
            else
            {
                if (off_proc_states[col] == 0)
                {
                    global_col += A->partition->global_num_cols;
                }
                col_indices.emplace_back(-(global_col+1));
            }
            values.emplace_back(A->off_proc->vals[j]);
        }
        rowptr[i+1] = col_indices.size();
    }

    return comm->communicate(rowptr, col_indices, values);
   
}


CSRMatrix*  communicate(ParCSRMatrix* A, const aligned_vector<int>& states,
        const aligned_vector<int>& off_proc_states, CommPkg* comm)
{
    int start, end, col;

    aligned_vector<int> rowptr(A->local_num_rows + 1);
    aligned_vector<int> col_indices;
    aligned_vector<double> values;
    if (A->local_nnz)
    {
        col_indices.reserve(A->local_nnz);
        values.reserve(A->local_nnz);
    }

    rowptr[0] = 0;
    for (int i = 0; i < A->local_num_rows; i++)
    {
        start = A->on_proc->idx1[i];
        end = A->on_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = A->on_proc->idx2[j];
            if (states[col] == 1)
            {
                col_indices.emplace_back(A->on_proc_column_map[col]);
                values.emplace_back(A->on_proc->vals[j]);
            }
        }
        start = A->off_proc->idx1[i];
        end = A->off_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = A->off_proc->idx2[j];
            if (off_proc_states[col] == 1)
            {
                col_indices.emplace_back(A->off_proc_column_map[col]);
                values.emplace_back(A->off_proc->vals[j]);
            }
        }
        rowptr[i+1] = col_indices.size();
    }

    return comm->communicate(rowptr, col_indices, values);
}

ParCSRMatrix* extended_interpolation(ParCSRMatrix* A,
        ParCSRMatrix* S, const aligned_vector<int>& states,
        const aligned_vector<int>& off_proc_states, 
        bool tap_interp, int num_variables, int* variables, 
        data_t* comm_t, data_t* comm_mat_t)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

double t0, tfinal;
MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();

    int start, end, idx, idx_k;
    int start_S, end_S;
    int start_k, end_k;
    int col, global_col;
    int ctr, col_k, col_P;
    int global_num_cols;
    int on_proc_cols, off_proc_cols;
    int sign;
    int row_start_on, row_end_on;
    int row_start_off, row_end_off;
    double val, weak_sum;

    CommPkg* comm = A->comm;
    CommPkg* mat_comm = A->comm;
    if (tap_interp)
    {
        comm = A->tap_comm;
        mat_comm = A->tap_mat_comm;
    }

    std::set<int> global_set;
    std::map<int, int> global_to_local;
    aligned_vector<int> off_proc_column_map;
    aligned_vector<int> off_variables;

    CSRMatrix* recv_mat; // On Proc Block of Recvd A
    CSRMatrix* A_recv_on;
    CSRMatrix* S_recv_on;
    CSRMatrix* D_recv_on;
    CSRMatrix* A_recv_off;
    CSRMatrix* S_recv_off;

    A->sort();
    S->sort();
    A->on_proc->move_diag();
    S->on_proc->move_diag();


    aligned_vector<double> weak_sums;
    aligned_vector<int> signs;
    if (A->on_proc_num_cols)
    {
        weak_sums.resize(A->on_proc_num_cols);
        signs.resize(A->on_proc_num_cols);
    }
    for (int i = 0; i < A->on_proc->n_rows; i++)
    {
        weak_sums[i] = A->on_proc->vals[A->on_proc->idx1[i]];
        if (weak_sums[i] < 0) 
            signs[i] = -1;
        else 
            signs[i] = 1;
    }
tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Init: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();
    if (A->off_proc_num_cols) off_variables.resize(A->off_proc_num_cols);
    if (num_variables > 1)
    {
        if (comm_t) *comm_t -= MPI_Wtime();
        comm->communicate(variables);
        if (comm_t) *comm_t += MPI_Wtime();
        
        for (int i = 0; i < A->off_proc_num_cols; i++)
        {
            off_variables[i] = comm->get_int_buffer()[i];
        }
    }

    // Communicate parallel matrix A (Costly!)
    if (comm_mat_t) *comm_mat_t -= MPI_Wtime();
    recv_mat = communicate(A, S, states, off_proc_states, mat_comm);
    if (comm_mat_t) *comm_mat_t += MPI_Wtime();

tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Communication: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();
    int tmp_col = col;
    int* on_proc_partition_to_col = A->map_partition_to_local();
    
    aligned_vector<int> A_recv_on_ptr(recv_mat->n_rows + 1);
    aligned_vector<int> S_recv_on_ptr(recv_mat->n_rows + 1);
    aligned_vector<int> A_recv_off_ptr(recv_mat->n_rows + 1);
    aligned_vector<int> S_recv_off_ptr(recv_mat->n_rows + 1);
    aligned_vector<int> A_recv_on_idx(recv_mat->nnz);
    aligned_vector<int> S_recv_on_idx(recv_mat->nnz);
    aligned_vector<int> A_recv_off_idx(recv_mat->nnz);
    aligned_vector<int> S_recv_off_idx(recv_mat->nnz);

    A_recv_on_ptr[0] = 0;
    S_recv_on_ptr[0] = 0;
    A_recv_off_ptr[0] = 0;
    S_recv_off_ptr[0] = 0;

    aligned_vector<int> D_recv_ptr(recv_mat->n_rows + 1);
    aligned_vector<int> D_recv_idx(recv_mat->nnz);
    aligned_vector<int> col_sizes(A->on_proc_num_cols, 0);
    D_recv_ptr[0] = 0;

    int A_recv_on_ctr = 0;
    int S_recv_on_ctr = 0;
    int D_recv_on_ctr = 0;
    int A_recv_off_ctr = 0;
    int S_recv_off_ctr = 0;
    for (int i = 0; i < recv_mat->n_rows; i++)
    {
        start = recv_mat->idx1[i];
        end = recv_mat->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = recv_mat->idx2[j];
            val = recv_mat->vals[j];

            tmp_col = col;
            if (col < 0) 
            {
                col = (-tmp_col) - 1;
            }
            if (col >= A->partition->global_num_cols)
            {
                col -= A->partition->global_num_cols;
                if (col >= A->partition->first_local_col &&
                        col <= A->partition->last_local_col)
                {
                    col = on_proc_partition_to_col[col - A->partition->first_local_row];
                    recv_mat->idx2[j] = col;
                    if (val * signs[col] < 0) 
                    {
                        D_recv_idx[D_recv_on_ctr++] = j;
                        col_sizes[col]++;
                    }
                }
            }
            else if (col < A->partition->first_local_col || 
                    col > A->partition->last_local_col)
            {
                recv_mat->idx2[j] = col;
                if (tmp_col > 0)
                {
                    S_recv_off_idx[S_recv_off_ctr++] = j;
                }
                else
                {
                    A_recv_off_idx[A_recv_off_ctr++] = j;
                }
            }
            else
            {
                col = on_proc_partition_to_col[col - A->partition->first_local_row];
                recv_mat->idx2[j] = col;
                if (tmp_col > 0)
                {
                    S_recv_on_idx[S_recv_on_ctr++] = j;
                }
                else
                {
                    A_recv_on_idx[A_recv_on_ctr++] = j;
                }
            }
        }
        A_recv_on_ptr[i+1] = A_recv_on_ctr;
        S_recv_on_ptr[i+1] = S_recv_on_ctr;
        D_recv_ptr[i+1] = D_recv_on_ctr;
        A_recv_off_ptr[i+1] = A_recv_off_ctr;
        S_recv_off_ptr[i+1] = S_recv_off_ctr;
    }
    A_recv_on_idx.resize(A_recv_on_ctr);
    A_recv_on_idx.shrink_to_fit();
    S_recv_on_idx.resize(S_recv_on_ctr);
    S_recv_on_idx.shrink_to_fit();
    D_recv_idx.resize(D_recv_on_ctr);
    D_recv_idx.shrink_to_fit();
    A_recv_off_idx.resize(A_recv_off_ctr);
    A_recv_off_idx.shrink_to_fit();
    S_recv_off_idx.resize(S_recv_off_ctr);
    S_recv_off_idx.shrink_to_fit();

    delete[] on_proc_partition_to_col;

    aligned_vector<int> D_recv_on_csc_ptr(A->on_proc_num_cols+1);
    aligned_vector<int> D_recv_on_csc_idx(D_recv_on_ctr);
    D_recv_on_csc_ptr[0] = 0;
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        D_recv_on_csc_ptr[i+1] = D_recv_on_csc_ptr[i] + col_sizes[i];
        col_sizes[i] = 0;
    }
    for (int i = 0; i < recv_mat->n_rows; i++)
    {
        start = D_recv_ptr[i];
        end = D_recv_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = D_recv_idx[j];
            col = recv_mat->idx2[idx];
            val = recv_mat->vals[idx];
            idx_k = D_recv_on_csc_ptr[col] + col_sizes[col]++;
            D_recv_on_csc_idx[idx_k] = idx;
            recv_mat->idx2[idx] = i;
        }
    }
    D_recv_ptr.clear();
    D_recv_ptr.shrink_to_fit();
    D_recv_idx.clear();
    D_recv_idx.shrink_to_fit();

tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Restructure RecvMat: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();
    // Change off_proc_cols to local (remove cols not on rank)
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (off_proc_states[i] == Selected)
        {
            global_set.insert(S->off_proc_column_map[i]);
        }
    }
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (off_proc_states[i] == Unselected)
        {
            start = A_recv_off_ptr[i];
            end = A_recv_off_ptr[i+1];
            for (int j = start; j < end; j++)
            {
                global_set.insert(recv_mat->idx2[A_recv_off_idx[j]]);
            }

            start = S_recv_off_ptr[i];
            end = S_recv_off_ptr[i+1];
            for (int j = start; j < end; j++)
            {
                global_set.insert(recv_mat->idx2[S_recv_off_idx[j]]);
            }
        }
    }
    for (std::set<int>::iterator it = global_set.begin(); it != global_set.end(); ++it)
    {
        global_to_local[*it] = off_proc_column_map.size();
        off_proc_column_map.emplace_back(*it);
    }
    off_proc_cols = off_proc_column_map.size();

    for (aligned_vector<int>::iterator it = A_recv_off_idx.begin(); 
            it != A_recv_off_idx.end(); ++it)
    {
        recv_mat->idx2[*it] = global_to_local[recv_mat->idx2[*it]];
    }
    for (aligned_vector<int>::iterator it = S_recv_off_idx.begin();
            it != S_recv_off_idx.end(); ++it)
    {
        recv_mat->idx2[*it] = global_to_local[recv_mat->idx2[*it]];
    }

tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Reindex Columns of Recv: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();
    // Initialize P
    aligned_vector<int> on_proc_col_to_new;
    aligned_vector<bool> col_exists;
    if (S->on_proc_num_cols)
    {
        on_proc_col_to_new.resize(S->on_proc_num_cols, -1);
    }
    if (off_proc_cols)
    {
        col_exists.resize(off_proc_cols, false);
    }
    on_proc_cols = 0;
    for (int i = 0; i < S->on_proc_num_cols; i++)
    {
        if (states[i] == Selected)
        {
            on_proc_cols++;
        }
    }
    // Initialize AllReduce to determine global num cols
    MPI_Request reduce_request;
    int reduce_buf = on_proc_cols;
    MPI_Iallreduce(&(reduce_buf), &global_num_cols, 1, MPI_INT, MPI_SUM, 
            MPI_COMM_WORLD, &reduce_request);
   
    ParCSRMatrix* P = new ParCSRMatrix(A->partition, A->global_num_rows, -1, 
            A->local_num_rows, on_proc_cols, off_proc_cols);

    for (int i = 0; i < S->on_proc_num_cols; i++)
    {
        if (states[i] == Selected)
        {
            on_proc_col_to_new[i] = P->on_proc_column_map.size();
            P->on_proc_column_map.emplace_back(S->on_proc_column_map[i]);
        }
    }
    P->local_row_map = S->get_local_row_map();

    aligned_vector<int> off_proc_A_to_P;
    if (A->off_proc_num_cols) 
    {
	    off_proc_A_to_P.resize(A->off_proc_num_cols, -1);
    }
    ctr = 0;
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (off_proc_states[i] != Selected)
        {
            continue; // Only for coarse points
        }

        while (off_proc_column_map[ctr] < A->off_proc_column_map[i])
        {
            ctr++;
        }
        off_proc_A_to_P[i] = ctr;
    }

    // For each row, will calculate coarse sums and store 
    // strong connections in vector
    aligned_vector<int> pos;
    aligned_vector<int> off_proc_pos;
    aligned_vector<double> row_strong;
    aligned_vector<double> off_proc_row_strong;
    aligned_vector<double> coarse_sum;
    aligned_vector<double> off_proc_coarse_sum;
    if (A->on_proc_num_cols)
    {
        pos.resize(A->on_proc_num_cols, -1);
        row_strong.resize(A->on_proc_num_cols, 0);
        coarse_sum.resize(A->on_proc_num_cols);
    }
    if (A->off_proc_num_cols)
    {
        off_proc_row_strong.resize(A->off_proc_num_cols, 0);
        off_proc_coarse_sum.resize(A->off_proc_num_cols);
    }
    if (P->off_proc_num_cols)
    {
        off_proc_pos.resize(P->off_proc_num_cols, -1);
    }

tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Initialize P: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();
    // Create SS_on, SU_on, NS_on, SS_off, SU_off, NS_off
    aligned_vector<int> SS_on_ptr(A->local_num_rows+1);
    aligned_vector<int> SU_on_ptr(A->local_num_rows+1);
    aligned_vector<int> NS_on_ptr(A->local_num_rows+1);
    aligned_vector<int> U_on_ptr(A->local_num_rows+1);
    aligned_vector<int> SS_off_ptr(A->local_num_rows+1);
    aligned_vector<int> SU_off_ptr(A->local_num_rows+1);
    aligned_vector<int> NS_off_ptr(A->local_num_rows+1);

    int SS_on_ctr = 0;
    int SU_on_ctr = 0;
    int NS_on_ctr = 0;
    int SS_off_ctr = 0;
    int SU_off_ctr = 0;
    int NS_off_ctr = 0;
    int U_on_ctr = 0;
    std::fill(col_sizes.begin(), col_sizes.end(), 0);
    for (int i = 0; i < A->on_proc->n_rows; i++)
    {
        start = A->on_proc->idx1[i] + 1;
        end = A->on_proc->idx1[i+1];
        ctr = S->on_proc->idx1[i]+1;
        end_S = S->on_proc->idx1[i+1];

        for (int j = start; j < end; j++)
        {
            col = A->on_proc->idx2[j];
            val = A->on_proc->vals[j];
            if (ctr < end_S && S->on_proc->idx2[ctr] == col)
            {
                if (states[col] == Selected)
                {
                    SS_on_ctr++;
                }
                else
                {
                    SU_on_ctr++;
                    if (val * signs[col] < 0)
                    {
                        U_on_ctr++;
                        col_sizes[col]++;
                    }
                }
                ctr++;
            }
            else
            {
                if (states[col] == Selected)
                {
                    NS_on_ctr++;
                }
                else if (val * signs[col] < 0)
                {
                    U_on_ctr++;
                    col_sizes[col]++;
                }
            }
        }
        SS_on_ptr[i+1] = SS_on_ctr;
        SU_on_ptr[i+1] = SU_on_ctr;
        NS_on_ptr[i+1] = NS_on_ctr;
        U_on_ptr[i+1] = U_on_ctr;        
        
        start = A->off_proc->idx1[i];
        end = A->off_proc->idx1[i+1];
        ctr = S->off_proc->idx1[i];
        end_S = S->off_proc->idx1[i+1];

        for (int j = start; j < end; j++)
        {
            col = A->off_proc->idx2[j];
            // If strong connection
            if (ctr < end_S && S->off_proc->idx2[ctr] == col)
            {
                if (off_proc_states[col] == Selected)
                {
                    SS_off_ctr++;
                }
                else 
                {
                    SU_off_ctr++;
                }
                ctr++;
            }
            else
            {
                NS_off_ctr++;
            }
        }
        SS_off_ptr[i+1] = SS_off_ctr;
        SU_off_ptr[i+1] = SU_off_ctr;
        NS_off_ptr[i+1] = NS_off_ctr;
    }

    CSCMatrix* U_on_csc = new CSCMatrix(A->on_proc_num_cols, A->local_num_rows);
    U_on_csc->idx2.resize(U_on_ctr);
    U_on_csc->vals.resize(U_on_ctr);
    U_on_csc->idx1[0] = 0;
    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        U_on_csc->idx1[i+1] = U_on_csc->idx1[i] + col_sizes[i];
        col_sizes[i] = 0;
    }

    aligned_vector<int> SS_on_idx(SS_on_ctr);
    aligned_vector<int> SU_on_idx(SU_on_ctr);
    aligned_vector<int> NS_on_idx(NS_on_ctr);
    aligned_vector<int> SS_off_idx(SS_off_ctr);
    aligned_vector<int> SU_off_idx(SU_off_ctr);
    aligned_vector<int> NS_off_idx(NS_off_ctr);

    SS_on_ctr = 0;
    SU_on_ctr = 0;
    NS_on_ctr = 0;
    SS_off_ctr = 0;
    SU_off_ctr = 0;
    NS_off_ctr = 0;
    U_on_ctr = 0;
    for (int i = 0; i < A->on_proc->n_rows; i++)
    {
        start = A->on_proc->idx1[i] + 1;
        end = A->on_proc->idx1[i+1];
        ctr = S->on_proc->idx1[i]+1;
        end_S = S->on_proc->idx1[i+1];

        for (int j = start; j < end; j++)
        {
            col = A->on_proc->idx2[j];
            val = A->on_proc->vals[j];
            if (ctr < end_S && S->on_proc->idx2[ctr] == col)
            {
                if (states[col] == Selected)
                {
                    SS_on_idx[SS_on_ctr++] = j;
                }
                else
                {
                    SU_on_idx[SU_on_ctr++] = j;
                    if (val * signs[col] < 0)
                    {
                        idx = U_on_csc->idx1[col] + col_sizes[col]++;
                        U_on_csc->idx2[idx] = i;
                        U_on_csc->vals[idx] = val;
                    }
                }
                ctr++;
            }
            else
            {
                if (states[col] == Selected)
                {
                    NS_on_idx[NS_on_ctr++] = j;
                }
                else if (val * signs[col] < 0)
                {
                    idx = U_on_csc->idx1[col] + col_sizes[col]++;
                    U_on_csc->idx2[idx] = i;
                    U_on_csc->vals[idx] = val;
                }
                if (num_variables == 1 || variables[i] == variables[col])// weak connection
                {
                    if (states[col] != NoNeighbors)
                    {
                        weak_sums[i] += val;
                    }
                }
            }
        }      
        
        start = A->off_proc->idx1[i];
        end = A->off_proc->idx1[i+1];
        ctr = S->off_proc->idx1[i];
        end_S = S->off_proc->idx1[i+1];

        for (int j = start; j < end; j++)
        {
            col = A->off_proc->idx2[j];
            val = A->off_proc->vals[j];
            
            // If strong connection
            if (ctr < end_S && S->off_proc->idx2[ctr] == col)
            {
                if (off_proc_states[col] == Selected)
                {
                    SS_off_idx[SS_off_ctr++] = j;
                }
                else 
                {
                    SU_off_idx[SU_off_ctr++] = j;
                }
                ctr++;
            }
            else
            {
                NS_off_idx[NS_off_ctr++] = j;
                if (num_variables == 1 || variables[i] == off_variables[col])
                {
                    if (off_proc_states[col] != NoNeighbors)
                    {
                        weak_sums[i] += val;
                    }
                }
            }
        }
    }

    // Find upperbound size of P->on_proc and P->off_proc
    int nnz_on = 0;
    int nnz_off = 0;
    for (int i = 0; i < A->local_num_rows; i++)
    {
        if (states[i] == Unselected)
        {
            nnz_on += SS_on_ptr[i+1] - SS_on_ptr[i];
            nnz_off += SS_off_ptr[i+1] - SS_off_ptr[i];

            start = SU_on_ptr[i];
            end = SU_on_ptr[i+1];
            for (int j = start; j < end; j++)
            {
                idx = SU_on_idx[j];
                col = A->on_proc->idx2[idx];
                nnz_on += (SS_on_ptr[col+1] - SS_on_ptr[col]);
                nnz_off += (SS_off_ptr[col+1] - SS_off_ptr[col]);
            }
            start = SU_off_ptr[i];
            end = SU_off_ptr[i+1];
            for (int j = start; j < end; j++)
            {
                idx = SU_off_idx[j];
                col = A->off_proc->idx2[idx];
                nnz_on += (S_recv_on_ptr[col+1] - S_recv_on_ptr[col]);
                nnz_off += (S_recv_off_ptr[col+1] - S_recv_off_ptr[col]);
            }
        }
        else
        {
            nnz_on++;
        }
    }
    P->on_proc->idx2.resize(nnz_on);
    P->on_proc->vals.resize(nnz_on);
    P->off_proc->idx2.resize(nnz_off);
    P->off_proc->vals.resize(nnz_off);
tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Split On Proc: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();

    nnz_on = 0;
    nnz_off = 0;
    for (int i = 0; i < A->local_num_rows; i++)
    {
        // If coarse row, add to P
        if (states[i] != Unselected)
        {
            if (states[i] == Selected)
            {
                P->on_proc->idx2[nnz_on] = on_proc_col_to_new[i];
                P->on_proc->vals[nnz_on++] = 1;
            }
            P->on_proc->idx1[i+1] = nnz_on;
            P->off_proc->idx1[i+1] = nnz_off;
            continue;
        }

        // Store diagonal value, create initial weak sum
        sign = signs[i];
        weak_sum = weak_sums[i];

        // Add strong fine points to row_strong
        start = SU_on_ptr[i];
        end = SU_on_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_on_idx[j];
            col = A->on_proc->idx2[idx];
            val = A->on_proc->vals[idx];
            row_strong[col] = val;
        }
        start = SU_off_ptr[i];
        end = SU_off_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_off_idx[j];
            col = A->off_proc->idx2[idx];
            val = A->off_proc->vals[idx];
            off_proc_row_strong[col] = val;
        }

        // Go through strong coarse points, 
        // add to row coarse and create sparsity of P (dist1)
        row_start_on = nnz_on;
        row_start_off = nnz_off;

        pos[i] = A->global_num_rows;
        start = SS_on_ptr[i];
        end = SS_on_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SS_on_idx[j];
            col = A->on_proc->idx2[idx];
            val = A->on_proc->vals[idx];
            pos[col] = nnz_on;
            P->on_proc->idx2[nnz_on] = col;
            P->on_proc->vals[nnz_on++] = val;
        }
        start = SS_off_ptr[i];
        end = SS_off_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SS_off_idx[j];
            col = A->off_proc->idx2[idx];
            val = A->off_proc->vals[idx];
            col_P = off_proc_A_to_P[col];
            off_proc_pos[col_P] = nnz_off;
            col_exists[col_P] = true;
            P->off_proc->idx2[nnz_off] = col_P;
            P->off_proc->vals[nnz_off++] = val;
        }

        // Add distance-2 processes to row_coarse
        // and add to sparsity pattern of P
        start = SU_on_ptr[i];
        end = SU_on_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_on_idx[j];
            col = A->on_proc->idx2[idx];

            start_k = SS_on_ptr[col];
            end_k = SS_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_on_idx[k];
                col_k = A->on_proc->idx2[idx];
                if (pos[col_k] == -1)
                {
                    pos[col_k] = nnz_on;
                    P->on_proc->idx2[nnz_on] = col_k;
                    P->on_proc->vals[nnz_on++] = 0.0;
                }
            }
            start_k = SS_off_ptr[col];
            end_k = SS_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_off_idx[k];
                col_k = A->off_proc->idx2[idx];
                col_P = off_proc_A_to_P[col_k];
                if (off_proc_pos[col_P] == -1)
                {
                    off_proc_pos[col_P] = nnz_off;
                    P->off_proc->idx2[nnz_off] = col_P;
                    P->off_proc->vals[nnz_off++] = 0.0;
                    col_exists[col_P] = true;
                }
            }
        }
        start = SU_off_ptr[i];
        end = SU_off_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_off_idx[j];
            col = A->off_proc->idx2[idx];
            start_k = S_recv_on_ptr[col];
            end_k = S_recv_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_on_idx[k];
                col_k = recv_mat->idx2[idx];
                if (pos[col_k] == -1)
                {
                    pos[col_k] = nnz_on;
                    P->on_proc->idx2[nnz_on] = col_k;
                    P->on_proc->vals[nnz_on++] = 0.0;
                }
            }
            start_k = S_recv_off_ptr[col];
            end_k = S_recv_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_off_idx[k];
                col_k = recv_mat->idx2[idx];
                if (off_proc_pos[col_k] == -1)
                {
                    off_proc_pos[col_k] = nnz_off;
                    P->off_proc->idx2[nnz_off] = col_k;
                    P->off_proc->vals[nnz_off++] = 0.0;
                    col_exists[col_k] = true;
                }
            }
        }

        row_end_on = nnz_on;
        row_end_off = nnz_off;


        start = SU_on_ptr[i];
        end = SU_on_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_on_idx[j];
            col = A->on_proc->idx2[idx]; // k
            // Find sum of all coarse points in row k (with sign NOT equal to diag)
            double col_sum = 0;

            start_k = NS_on_ptr[col];
            end_k = NS_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = NS_on_idx[k];
                col_k = A->on_proc->idx2[idx];  // m
                val = A->on_proc->vals[idx];
                if (val * sign < 0 && pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }
            start_k = SS_on_ptr[col];
            end_k = SS_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_on_idx[k];
                col_k = A->on_proc->idx2[idx];  // m
                val = A->on_proc->vals[idx];
                if (val * sign < 0 && pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }

            start_k = NS_off_ptr[col];
            end_k = NS_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = NS_off_idx[k];
                col_k = A->off_proc->idx2[idx]; 
                val = A->off_proc->vals[idx];
                col_P = off_proc_A_to_P[col_k];
                if (col_P >= 0 && val * sign < 0 && off_proc_pos[col_P] > -1)
                {
                    col_sum += val;
                }
            }
            start_k = SS_off_ptr[col];
            end_k = SS_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_off_idx[k];
                col_k = A->off_proc->idx2[idx]; 
                val = A->off_proc->vals[idx];
                col_P = off_proc_A_to_P[col_k];
                if (col_P >= 0 && val * sign < 0 && off_proc_pos[col_P] > -1)
                {
                    col_sum += val;
                }
            }
            
            coarse_sum[col] = col_sum;
        }

        start_k = U_on_csc->idx1[i];
        end_k = U_on_csc->idx1[i+1];
        for (int k = start_k; k < end_k; k++)
        {
            int row = U_on_csc->idx2[k];
            val = U_on_csc->vals[k];
            coarse_sum[row] += val;
        }

        for (int j = start; j < end; j++)
        {
            idx = SU_on_idx[j];
            col = A->on_proc->idx2[idx]; // k
            val = coarse_sum[col];

            if (fabs(val) < zero_tol)
            {
                weak_sum += A->on_proc->vals[idx];
                row_strong[col] = 0;
            }
            else
            {
                row_strong[col] /= val;  // holds val for a_ik/sum_C(a_km)
            }
        }


        start = SU_off_ptr[i];
        end = SU_off_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_off_idx[j];
            col = A->off_proc->idx2[idx];
            double col_sum = 0;
            // Strong connection... create 

            // Add recvd values not in S
            start_k = A_recv_on_ptr[col];
            end_k = A_recv_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = A_recv_on_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                if (val * sign < 0 && pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }

            // Add recvd values in S
            start_k = S_recv_on_ptr[col];
            end_k = S_recv_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_on_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                if (val * sign < 0 && pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }

            start_k = A_recv_off_ptr[col];
            end_k = A_recv_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = A_recv_off_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                if (val * sign < 0 && off_proc_pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }
            start_k = S_recv_off_ptr[col];
            end_k = S_recv_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_off_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                if (val * sign < 0 && off_proc_pos[col_k] > -1)
                {
                    col_sum += val;
                }
            }

            off_proc_coarse_sum[col] = col_sum;
        }

        start_k = D_recv_on_csc_ptr[i];
        end_k = D_recv_on_csc_ptr[i+1];
        for (int k = start_k; k < end_k; k++)
        {
            idx = D_recv_on_csc_idx[k];
            int row = recv_mat->idx2[idx];
            val = recv_mat->vals[idx];
            off_proc_coarse_sum[row] += val;
        }

        for (int j = start; j < end; j++)
        {
            idx = SU_off_idx[j];
            col = A->off_proc->idx2[idx];
            val = off_proc_coarse_sum[col];

            if (fabs(val) < zero_tol)
            {
                weak_sum += A->off_proc->vals[idx];
                off_proc_row_strong[col] = 0;
            }
            else
            {
                off_proc_row_strong[col] /= val; // holds val for a_ik/sum_C(a_km)
            }
        }

        start_k = U_on_csc->idx1[i];
        end_k = U_on_csc->idx1[i+1];
        for (int k = start_k; k < end_k; k++)
        {
            int row = U_on_csc->idx2[k];
            val = U_on_csc->vals[k];
            if (val * sign < 0)
                weak_sum += (row_strong[row] * val);
        }        
        start_k = D_recv_on_csc_ptr[i];
        end_k = D_recv_on_csc_ptr[i+1];
        for (int k = start_k; k < end_k; k++)
        {
            idx = D_recv_on_csc_idx[k];
            int row = recv_mat->idx2[idx];
            val = recv_mat->vals[idx];
            if (val * sign < 0)
                weak_sum += (off_proc_row_strong[row] * val);
        }


        int idx;
        // Find weight for each Sij
        start = SU_on_ptr[i];
        end = SU_on_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_on_idx[j];
            col = A->on_proc->idx2[idx]; // k
            double row_val = row_strong[col];

            start_k = SS_on_ptr[col];
            end_k = SS_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_on_idx[k];
                col_k = A->on_proc->idx2[idx];
                val = A->on_proc->vals[idx];
                idx = pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->on_proc->vals[idx] += (row_val * val);
                }
            }
            start_k = NS_on_ptr[col];
            end_k = NS_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = NS_on_idx[k];
                col_k = A->on_proc->idx2[idx];
                val = A->on_proc->vals[idx];
                idx = pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->on_proc->vals[idx] += (row_val * val);
                }
            }

            start_k = NS_off_ptr[col];
            end_k = NS_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = NS_off_idx[k];
                col_k = A->off_proc->idx2[idx];
                col_P = off_proc_A_to_P[col_k];
                if (col_P >= 0)
                {
                    val = A->off_proc->vals[idx];
                    idx = off_proc_pos[col_P];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->off_proc->vals[idx] += (row_val * val);
                    }
                }
            }
            start_k = SS_off_ptr[col];
            end_k = SS_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = SS_off_idx[k];
                col_k = A->off_proc->idx2[idx];
                col_P = off_proc_A_to_P[col_k];
                if (col_P >= 0)
                {
                    val = A->off_proc->vals[idx];
                    idx = off_proc_pos[col_P];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->off_proc->vals[idx] += (row_val * val);
                    }
                }
            }

            row_strong[col] = 0;
        }

        start = SU_off_ptr[i];
        end = SU_off_ptr[i+1];
        for (int j = start; j < end; j++)
        {
            idx = SU_off_idx[j];
            col = A->off_proc->idx2[idx];
            double col_val = off_proc_row_strong[col];

            start_k = A_recv_on_ptr[col];
            end_k = A_recv_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = A_recv_on_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                idx = pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->on_proc->vals[idx] += (col_val * val);
                }
            }
            start_k = S_recv_on_ptr[col];
            end_k = S_recv_on_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_on_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                idx = pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->on_proc->vals[idx] += (col_val * val);
                }
            }

            start_k = A_recv_off_ptr[col];
            end_k = A_recv_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = A_recv_off_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                idx = off_proc_pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->off_proc->vals[idx] += (col_val * val);
                }
            }
            start_k = S_recv_off_ptr[col];
            end_k = S_recv_off_ptr[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                idx = S_recv_off_idx[k];
                col_k = recv_mat->idx2[idx];
                val = recv_mat->vals[idx];
                idx = off_proc_pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->off_proc->vals[idx] += (col_val * val);
                }
            }

            off_proc_row_strong[col] = 0;
        }

        // Divide by weak sum and clear row values
        for (int j = row_start_on; j < row_end_on; j++)
        {
            col = P->on_proc->idx2[j];
            P->on_proc->idx2[j] = on_proc_col_to_new[col];
            P->on_proc->vals[j] /= -weak_sum;
            pos[col] = -1;
        }
        for (int j = row_start_off; j < row_end_off; j++)
        {
            col = P->off_proc->idx2[j];
            P->off_proc->vals[j] /= -weak_sum;
            off_proc_pos[col] = -1;
        }
        pos[i] = -1;

        P->on_proc->idx1[i+1] = row_end_on;
        P->off_proc->idx1[i+1] = row_end_off;
    }
    P->on_proc->nnz = nnz_on;
    P->off_proc->nnz = nnz_off;
tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Main Loop: %e\n", t0);

MPI_Barrier(MPI_COMM_WORLD);
t0 = MPI_Wtime();

    P->on_proc->idx2.resize(nnz_on);
    P->on_proc->idx2.shrink_to_fit();
    P->on_proc->vals.resize(nnz_on);
    P->on_proc->vals.shrink_to_fit();

    P->off_proc->idx2.resize(nnz_off);
    P->off_proc->idx2.shrink_to_fit();
    P->off_proc->vals.resize(nnz_off);
    P->off_proc->vals.shrink_to_fit();

    P->local_nnz = P->on_proc->nnz + P->off_proc->nnz;

    // Update off_proc columns in P (remove col j if col_exists[j] is false)
    if (P->off_proc_num_cols)
    {
    	aligned_vector<int> P_to_new(P->off_proc_num_cols);
    	for (int i = 0; i < P->off_proc_num_cols; i++)
    	{
        	if (col_exists[i])
        	{
            	P_to_new[i] = P->off_proc_column_map.size();
            	P->off_proc_column_map.emplace_back(off_proc_column_map[i]);
        	}
    	}
        for (aligned_vector<int>::iterator it = P->off_proc->idx2.begin(); 
                it != P->off_proc->idx2.end(); ++it)
        {
            *it = P_to_new[*it];
        }
    }

    P->off_proc_num_cols = P->off_proc_column_map.size();
    P->on_proc_num_cols = P->on_proc_column_map.size();
    P->off_proc->n_cols = P->off_proc_num_cols;
    P->on_proc->n_cols = P->on_proc_num_cols;

    if (tap_interp)
    {
        P->init_tap_communicators(MPI_COMM_WORLD, comm_t);
    }
    else
    {
        P->comm = new ParComm(P->partition, P->off_proc_column_map,
                P->on_proc_column_map, 9243, MPI_COMM_WORLD, comm_t);
    }
tfinal = MPI_Wtime() - t0;
MPI_Reduce(&tfinal, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
if (rank == 0) printf("Finalize: %e\n", t0);
    

    delete recv_mat;
    delete U_on_csc;  // Used for +i portion of extended+i

    // Finish Allreduce and set global number of columns
    MPI_Wait(&reduce_request, MPI_STATUS_IGNORE);
    P->global_num_cols = global_num_cols;

    return P;
}

ParCSRMatrix* mod_classical_interpolation(ParCSRMatrix* A,
        ParCSRMatrix* S, const aligned_vector<int>& states,
        const aligned_vector<int>& off_proc_states, 
        bool tap_interp, int num_variables, int* variables, 
        data_t* comm_t, data_t* comm_mat_t)
{
    int start, end;
    int start_k, end_k;
    int end_S;
    int col, col_k;
    int ctr;
    int global_col;
    int global_num_cols;
    double diag, val;
    double weak_sum, coarse_sum;
    double sign;
    aligned_vector<int> off_variables;
    if (A->off_proc_num_cols) off_variables.resize(A->off_proc_num_cols);

    CommPkg* comm = A->comm;
    CommPkg* mat_comm = A->comm;
    if (tap_interp)
    {
        comm = A->tap_comm;
        mat_comm = A->tap_mat_comm;
    }

    CSRMatrix* recv_mat; // On Proc Block of Recvd A

    A->sort();
    S->sort();
    A->on_proc->move_diag();
    S->on_proc->move_diag();

    if (num_variables > 1)
    {
        if (comm_t) *comm_t -= MPI_Wtime();
        comm->communicate(variables);
        if (comm_t) *comm_t += MPI_Wtime();

        for (int i = 0; i < A->off_proc_num_cols; i++)
        {
            off_variables[i] = comm->get_int_buffer()[i];
        }
    }

    // Initialize P
    aligned_vector<int> on_proc_col_to_new;
    aligned_vector<int> off_proc_col_to_new;
    aligned_vector<bool> col_exists;
    if (S->on_proc_num_cols)
    {
        on_proc_col_to_new.resize(S->on_proc_num_cols, -1);
    }
    if (S->off_proc_num_cols)
    {
        off_proc_col_to_new.resize(S->off_proc_num_cols, -1);
        col_exists.resize(S->off_proc_num_cols, false);
    }

    int off_proc_cols = 0;
    int on_proc_cols = 0;
    for (int i = 0; i < S->on_proc_num_cols; i++)
    {
        if (states[i] == Selected)
        {
            on_proc_cols++;
        }
    }
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (off_proc_states[i] == Selected)
        {
            off_proc_cols++;
        }
    }
    MPI_Allreduce(&(on_proc_cols), &global_num_cols, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   
    ParCSRMatrix* P = new ParCSRMatrix(A->partition, A->global_num_rows, global_num_cols, 
            A->local_num_rows, on_proc_cols, off_proc_cols);

    for (int i = 0; i < A->on_proc_num_cols; i++)
    {
        if (states[i] == Selected)
        {
            on_proc_col_to_new[i] = P->on_proc_column_map.size();
            P->on_proc_column_map.emplace_back(S->on_proc_column_map[i]);
        }
    }
    P->local_row_map = S->get_local_row_map();

    // Communicate parallel matrix A (Costly!)
    if (comm_mat_t) *comm_mat_t -= MPI_Wtime();
    recv_mat = communicate(A, states, off_proc_states, mat_comm);
    if (comm_mat_t) *comm_mat_t += MPI_Wtime();

    CSRMatrix* recv_on = new CSRMatrix(recv_mat->n_rows, -1, recv_mat->nnz);
    CSRMatrix* recv_off = new CSRMatrix(recv_mat->n_rows, -1, recv_mat->nnz);
    for (int i = 0; i < recv_mat->n_rows; i++)
    {
        start = recv_mat->idx1[i];
        end = recv_mat->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = recv_mat->idx2[j];
            if (col < A->partition->first_local_col || col > A->partition->last_local_col)
            {
                recv_off->idx2.emplace_back(col);
                recv_off->vals.emplace_back(recv_mat->vals[j]);
            }
            else
            {
                recv_on->idx2.emplace_back(col);
                recv_on->vals.emplace_back(recv_mat->vals[j]);
            }
        }
        recv_on->idx1[i+1] = recv_on->idx2.size();
        recv_off->idx1[i+1] = recv_off->idx2.size();
    }
    recv_on->nnz = recv_on->idx2.size();
    recv_off->nnz = recv_off->idx2.size();

    delete recv_mat;






    // Change on_proc_cols to local
    recv_on->n_cols = A->on_proc_num_cols;
    int* on_proc_partition_to_col = A->map_partition_to_local();
    for (aligned_vector<int>::iterator it = recv_on->idx2.begin();
            it != recv_on->idx2.end(); ++it)
    {
        *it = on_proc_partition_to_col[*it - A->partition->first_local_row];
    }
    delete[] on_proc_partition_to_col;

    // Change off_proc_cols to local (remove cols not on rank)
    ctr = 0;
    std::map<int, int> global_to_local;
    for (aligned_vector<int>::iterator it = A->off_proc_column_map.begin();
            it != A->off_proc_column_map.end(); ++it)
    {
        global_to_local[*it] = ctr++;
    }
    recv_off->n_cols = A->off_proc_num_cols;
    ctr = 0;
    start = recv_off->idx1[0];
    for (int i = 0; i < recv_off->n_rows; i++)
    {
        end = recv_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            global_col = recv_off->idx2[j];
            std::map<int, int>::iterator it = global_to_local.find(global_col);
            if (it != global_to_local.end())
            {
                recv_off->idx2[ctr] = it->second;
                recv_off->vals[ctr++] = recv_off->vals[j];
            }
        }
        recv_off->idx1[i+1] = ctr;
        start = end;
    }
    recv_off->nnz = ctr;
    recv_off->idx2.resize(ctr);
    recv_off->vals.resize(ctr);

    // For each row, will calculate coarse sums and store 
    // strong connections in vector
    aligned_vector<int> pos;
    aligned_vector<int> off_proc_pos;
    aligned_vector<int> row_coarse;
    aligned_vector<int> off_proc_row_coarse;
    aligned_vector<double> row_strong;
    aligned_vector<double> off_proc_row_strong;
    if (A->on_proc_num_cols)
    {
        pos.resize(A->on_proc_num_cols, -1);
        row_coarse.resize(A->on_proc_num_cols, 0);
        row_strong.resize(A->on_proc_num_cols, 0);
    }
    if (A->off_proc_num_cols)
    {
        off_proc_pos.resize(A->off_proc_num_cols, -1);
        off_proc_row_coarse.resize(A->off_proc_num_cols, 0);
        off_proc_row_strong.resize(A->off_proc_num_cols, 0);
    }


    // Create SS_on, SU_on, NS_on, SS_off, SU_off, NS_off
    CSRMatrix* SS_on = new CSRMatrix(A->on_proc->n_rows, A->on_proc->n_cols);
    CSRMatrix* SU_on = new CSRMatrix(A->on_proc->n_rows, A->on_proc->n_cols);
    CSRMatrix* NS_on = new CSRMatrix(A->on_proc->n_rows, A->on_proc->n_cols);

    CSRMatrix* SS_off = new CSRMatrix(A->off_proc->n_rows, A->off_proc->n_cols);
    CSRMatrix* SU_off = new CSRMatrix(A->off_proc->n_rows, A->off_proc->n_cols);
    CSRMatrix* NS_off = new CSRMatrix(A->off_proc->n_rows, A->off_proc->n_cols);
    for (int i = 0; i < A->on_proc->n_rows; i++)
    {
        start = A->on_proc->idx1[i];
        end = A->on_proc->idx1[i+1];
        ctr = S->on_proc->idx1[i]+1;
        end_S = S->on_proc->idx1[i+1];

        NS_on->idx2.emplace_back(A->on_proc->idx2[start]);
        NS_on->vals.emplace_back(A->on_proc->vals[start++]);

        for (int j = start; j < end; j++)
        {
            col = A->on_proc->idx2[j];
            val = A->on_proc->vals[j];
            if (ctr < end_S && S->on_proc->idx2[ctr] == col)
            {
                if (states[col] == Selected)
                {
                    SS_on->idx2.emplace_back(col);
                    SS_on->vals.emplace_back(val);
                }
                else
                {
                    SU_on->idx2.emplace_back(col);
                    SU_on->vals.emplace_back(val);
                }
                ctr++;
            }
            else
            {
                NS_on->idx2.emplace_back(col);
                NS_on->vals.emplace_back(val);
            }
        }
        SS_on->idx1[i+1] = SS_on->idx2.size();
        SU_on->idx1[i+1] = SU_on->idx2.size();
        NS_on->idx1[i+1] = NS_on->idx2.size();


        start = A->off_proc->idx1[i];
        end = A->off_proc->idx1[i+1];
        ctr = S->off_proc->idx1[i];
        end_S = S->off_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = A->off_proc->idx2[j];
            val = A->off_proc->vals[j];
            
            // If strong connection
            if (ctr < end_S && S->off_proc->idx2[ctr] == col)
            {
                if (off_proc_states[col] == Selected)
                {
                    SS_off->idx2.emplace_back(col);
                    SS_off->vals.emplace_back(val);
                }
                else 
                {
                    SU_off->idx2.emplace_back(col);
                    SU_off->vals.emplace_back(val);
                }
                ctr++;
            }
            else
            {
                NS_off->idx2.emplace_back(col);
                NS_off->vals.emplace_back(val);
            }
        }
        SS_off->idx1[i+1] = SS_off->idx2.size();
        SU_off->idx1[i+1] = SU_off->idx2.size();
        NS_off->idx1[i+1] = NS_off->idx2.size();
    }

    for (int i = 0; i < A->local_num_rows; i++)
    {
        // If coarse row, add to P
        if (states[i] == Selected)
        {
            P->on_proc->idx2.emplace_back(on_proc_col_to_new[i]);
            P->on_proc->vals.emplace_back(1);
            P->on_proc->idx1[i+1] = P->on_proc->idx2.size();
            P->off_proc->idx1[i+1] = P->off_proc->idx2.size();
            continue;
        }

        // Form weak sum
        start = NS_on->idx1[i];
        end = NS_on->idx1[i+1];
        weak_sum = NS_on->vals[start++];
        if (weak_sum < 0)
            sign = -1.0;
        else sign = 1.0;
        for (int j = start; j < end; j++)
        {
            col = NS_on->idx2[j];
            val = NS_on->vals[j];
            if (num_variables == 1 || variables[i] == variables[col]) // weak connection
            {
                weak_sum += val;
            }
        }
        start = NS_off->idx1[i];
        end = NS_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = NS_off->idx2[j];
            val = NS_off->vals[j];
            if (num_variables == 1 || variables[i] == variables[col]) // weak connection
            {
                weak_sum += val;
            }
        }

        // Add selected states to P on
        start = SS_on->idx1[i];
        end = SS_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SS_on->idx2[j];
            val = SS_on->vals[j];
            pos[col] = P->on_proc->idx2.size();
            P->on_proc->idx2.emplace_back(on_proc_col_to_new[col]);
            P->on_proc->vals.emplace_back(val);
            row_coarse[col] = 1;
        }
        start = SS_off->idx1[i];
        end = SS_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SS_off->idx2[j];
            val = SS_off->vals[j];
            off_proc_pos[col] = P->off_proc->idx2.size();
            col_exists[col] = true;
            P->off_proc->idx2.emplace_back(col);
            P->off_proc->vals.emplace_back(val);
            off_proc_row_coarse[col] = off_proc_states[col];
        }

        // Add unselected states to row_strong
        start = SU_on->idx1[i];
        end = SU_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_on->idx2[j];
            val = SU_on->vals[j];
            row_strong[col] = val;
        }
        start = SU_off->idx1[i];
        end = SU_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_off->idx2[j];
            val = SU_off->vals[j];
            off_proc_row_strong[col] = val;
        }


        // Find coarse sum for each unselected column
        start = SU_on->idx1[i];
        end = SU_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_on->idx2[j];

            // Find sum of all coarse points in row k (with sign NOT equal to diag)
            coarse_sum = 0;
            start_k = NS_on->idx1[col] + 1;
            end_k = NS_on->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = NS_on->idx2[k];  // m
                val = NS_on->vals[k] * row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }
            start_k = SS_on->idx1[col];
            end_k = SS_on->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = SS_on->idx2[k];  // m
                val = SS_on->vals[k] * row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }
            start_k = NS_off->idx1[col];
            end_k = NS_off->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = NS_off->idx2[k];
                val = NS_off->vals[k] * off_proc_row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }
            start_k = SS_off->idx1[col];
            end_k = SS_off->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = SS_off->idx2[k];
                val = SS_off->vals[k] * off_proc_row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }

            if (fabs(coarse_sum) < zero_tol)
            {
                weak_sum += A->on_proc->vals[j];
                row_strong[col] = 0;
            }
            else
            {
                row_strong[col] /= coarse_sum;  // holds val for a_ik/sum_C(a_km)
            }
        }

        start = SU_off->idx1[i];
        end = SU_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_off->idx2[j];

            // Strong connection... create 
            coarse_sum = 0;
            start_k = recv_on->idx1[col];
            end_k = recv_on->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = recv_on->idx2[k];
                val = recv_on->vals[k] * row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }
            start_k = recv_off->idx1[col];
            end_k = recv_off->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                col_k = recv_off->idx2[k];
                val = recv_off->vals[k] * off_proc_row_coarse[col_k];
                if (val * sign < 0)
                {
                    coarse_sum += val;
                }
            }
            if (fabs(coarse_sum) < zero_tol)
            {
                weak_sum += A->off_proc->vals[j];
                off_proc_row_strong[col] = 0;
            }
            else
            {
                off_proc_row_strong[col] /= coarse_sum; // holds val for a_ik/sum_C(a_km)
            }
        }


        // Find weight for each Sij
        int idx;
        start = SU_on->idx1[i];
        end = SU_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_on->idx2[j];
            double row_val = row_strong[col]; 
            if (row_strong[col]) // k in D_i^S
            {
                start_k = SS_on->idx1[col];
                end_k = SS_on->idx1[col+1];
                for (int k = start_k; k < end_k; k++)
                {
                    col_k = SS_on->idx2[k];
                    val = SS_on->vals[k];
                    idx = pos[col_k];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->on_proc->vals[idx] += (row_strong[col] * val);
                    }
                }
                start_k = NS_on->idx1[col]+1;
                end_k = NS_on->idx1[col+1];
                for (int k = start_k; k < end_k; k++)
                {
                    col_k = NS_on->idx2[k];
                    val = NS_on->vals[k];
                    idx = pos[col_k];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->on_proc->vals[idx] += (row_strong[col] * val);
                    }
                }

                start_k = SS_off->idx1[col];
                end_k = SS_off->idx1[col+1];
                for (int k = start_k; k < end_k; k++)
                {
                    col_k = SS_off->idx2[k];
                    val = SS_off->vals[k];
                    idx = off_proc_pos[col_k];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->off_proc->vals[idx] += (row_strong[col] * val);
                    }
                }
                start_k = NS_off->idx1[col];
                end_k = NS_off->idx1[col+1];
                for (int k = start_k; k < end_k; k++)
                {
                    col_k = NS_off->idx2[k];
                    val = NS_off->vals[k];
                    idx = off_proc_pos[col_k];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->off_proc->vals[idx] += (row_strong[col] * val);
                    }
                }
            }
        }

        start = SU_off->idx1[i];
        end = SU_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_off->idx2[j];
            start_k = recv_on->idx1[col];
            end_k = recv_on->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                val = recv_on->vals[k];
                col_k = recv_on->idx2[k];
                idx = pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->on_proc->vals[idx] += (off_proc_row_strong[col] * val);
                }
            }

            start_k = recv_off->idx1[col];
            end_k = recv_off->idx1[col+1];
            for (int k = start_k; k < end_k; k++)
            {
                val = recv_off->vals[k];
                col_k = recv_off->idx2[k];
                idx = off_proc_pos[col_k];
                if (val * sign < 0 && idx >= 0)
                {
                    P->off_proc->vals[idx] += (off_proc_row_strong[col] * val);
                }
            }
        }

        start = SS_on->idx1[i];
        end = SS_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SS_on->idx2[j];
            idx = pos[col];
            P->on_proc->vals[idx] /= -weak_sum;
        }

        start = S->off_proc->idx1[i];
        end = S->off_proc->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = S->off_proc->idx2[j];
            idx = off_proc_pos[col];
            if (off_proc_states[col] == Selected)
            {
                P->off_proc->vals[idx] /= -weak_sum;
            }
        }

        // Clear row values
        start = SS_on->idx1[i];
        end = SS_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SS_on->idx2[j];
            pos[col] = -1;
            row_coarse[col] = 0;
        }
        start = SU_on->idx1[i];
        end = SU_on->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_on->idx2[j];
            row_strong[col] = 0;
        }

        start = SS_off->idx1[i];
        end = SS_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SS_off->idx2[j];
            off_proc_pos[col] = -1;
            off_proc_row_coarse[col] = 0;
        }

        start = SU_off->idx1[i];
        end = SU_off->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = SU_off->idx2[j];
            off_proc_row_strong[col] = 0;
        }

        P->on_proc->idx1[i+1] = P->on_proc->idx2.size();
        P->off_proc->idx1[i+1] = P->off_proc->idx2.size();
    }
    P->on_proc->nnz = P->on_proc->idx2.size();
    P->off_proc->nnz = P->off_proc->idx2.size();
    P->local_nnz = P->on_proc->nnz + P->off_proc->nnz;

    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (col_exists[i])
        {
            off_proc_col_to_new[i] = P->off_proc_column_map.size();
            P->off_proc_column_map.emplace_back(S->off_proc_column_map[i]);
        }
    }
    for (aligned_vector<int>::iterator it = P->off_proc->idx2.begin(); 
            it != P->off_proc->idx2.end(); ++it)
    {
        *it = off_proc_col_to_new[*it];
    }

    P->off_proc_num_cols = P->off_proc_column_map.size();
    P->on_proc_num_cols = P->on_proc_column_map.size();
    P->off_proc->n_cols = P->off_proc_num_cols;
    P->on_proc->n_cols = P->on_proc_num_cols;

    if (tap_interp)
    {
        P->update_tap_comm(S, on_proc_col_to_new, off_proc_col_to_new, comm_t);
    }
    else
    {
        P->comm = new ParComm(S->comm, on_proc_col_to_new, off_proc_col_to_new,
                comm_t);
    }

    delete SS_on;
    delete SU_on;
    delete NS_on;
    delete SS_off;
    delete SU_off;
    delete NS_off;

    delete recv_on;
    delete recv_off;

    return P;
}


ParCSRMatrix* direct_interpolation(ParCSRMatrix* A,
        ParCSRMatrix* S, const aligned_vector<int>& states,
        const aligned_vector<int>& off_proc_states, 
        bool tap_interp, data_t* comm_t)
{
    int start, end, col;
    int global_num_cols;
    int ctr;
    double sum_strong_pos, sum_strong_neg;
    double sum_all_pos, sum_all_neg;
    double val, alpha, beta, diag;
    double neg_coeff, pos_coeff;

    A->sort();
    S->sort();
    A->on_proc->move_diag();
    S->on_proc->move_diag();

    // Copy entries of A into sparsity pattern of S
    aligned_vector<double> sa_on;
    aligned_vector<double> sa_off;
    if (S->on_proc->nnz)
    {
        sa_on.resize(S->on_proc->nnz);
    }
    if (S->off_proc->nnz)
    {
        sa_off.resize(S->off_proc->nnz);
    }
    for (int i = 0; i < A->local_num_rows; i++)
    {
        start = S->on_proc->idx1[i];
        end = S->on_proc->idx1[i+1];
        ctr = A->on_proc->idx1[i];
        for (int j = start; j < end; j++)
        {
            col = S->on_proc_column_map[S->on_proc->idx2[j]];
            while (A->on_proc_column_map[A->on_proc->idx2[ctr]] != col)
            {
                ctr++;
            }
            sa_on[j] = A->on_proc->vals[ctr];
        }

        start = S->off_proc->idx1[i];
        end = S->off_proc->idx1[i+1];
        ctr = A->off_proc->idx1[i];
        for (int j = start; j < end; j++)
        {
            col = S->off_proc_column_map[S->off_proc->idx2[j]];
            while (A->off_proc_column_map[A->off_proc->idx2[ctr]] != col)
            {
                ctr++;
            }
            sa_off[j] = A->off_proc->vals[ctr];
        }
    }

    aligned_vector<int> on_proc_col_to_new;
    aligned_vector<int> off_proc_col_to_new;
    if (S->on_proc_num_cols)
    {
        on_proc_col_to_new.resize(S->on_proc_num_cols, -1);
    }
    if (S->off_proc_num_cols)
    {
        off_proc_col_to_new.resize(S->off_proc_num_cols, -1);
    }

    int off_proc_cols = 0;
    int on_proc_cols = 0;
    for (int i = 0; i < S->on_proc_num_cols; i++)
    {
        if (states[i])
        {
            on_proc_cols++;
        }
    }
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (off_proc_states[i])
        {
            off_proc_cols++;
        }
    }
    MPI_Allreduce(&(on_proc_cols), &global_num_cols, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   
    ParCSRMatrix* P = new ParCSRMatrix(S->partition, S->global_num_rows, global_num_cols, 
            S->local_num_rows, on_proc_cols, off_proc_cols);

    for (int i = 0; i < S->on_proc_num_cols; i++)
    {
        if (states[i])
        {
            on_proc_col_to_new[i] = P->on_proc_column_map.size();
            P->on_proc_column_map.emplace_back(S->on_proc_column_map[i]);
        }
    }
    aligned_vector<bool> col_exists;
    if (S->off_proc_num_cols)
    {
        col_exists.resize(S->off_proc_num_cols, false);
    }
    P->local_row_map = S->get_local_row_map();

    for (int i = 0; i < A->local_num_rows; i++)
    {
        if (states[i] == 1)
        {
            P->on_proc->idx2.emplace_back(on_proc_col_to_new[i]);
            P->on_proc->vals.emplace_back(1);
        }
        else
        {
            sum_strong_pos = 0;
            sum_strong_neg = 0;
            sum_all_pos = 0;
            sum_all_neg = 0;

            start = S->on_proc->idx1[i];
            end = S->on_proc->idx1[i+1];
            if (S->on_proc->idx2[start] == i)
            {
                start++;
            }
            for (int j = start; j < end; j++)
            {
                col = S->on_proc->idx2[j]; 
                if (states[col] == 1)
                {
                    val = sa_on[j];
                    if (val < 0)
                    {
                        sum_strong_neg += val;
                    }
                    else
                    {
                        sum_strong_pos += val;
                    }
                }
            }
            start = S->off_proc->idx1[i];
            end = S->off_proc->idx1[i+1];
            for (int j = start; j < end; j++)
            {
                col = S->off_proc->idx2[j];

                if (off_proc_states[col] == 1)
                {
                    val = sa_off[j];
                    if (val < 0)
                    {
                        sum_strong_neg += val;
                    }
                    else
                    {
                        sum_strong_pos += val;
                    }
                }
            }

            start = A->on_proc->idx1[i];
            end = A->on_proc->idx1[i+1];
            diag = A->on_proc->vals[start]; // Diag stored first
            start++;
            for (int j = start; j < end; j++)
            {
                val = A->on_proc->vals[j];
                if (val < 0)
                {
                    sum_all_neg += val;
                }
                else
                {
                    sum_all_pos += val;
                }
            }
            start = A->off_proc->idx1[i];
            end = A->off_proc->idx1[i+1];
            for (int j = start; j < end; j++)
            {
                val = A->off_proc->vals[j];
                if (val < 0)
                {
                    sum_all_neg += val;
                }
                else
                {
                    sum_all_pos += val;
                }
            }

            alpha = sum_all_neg / sum_strong_neg;
           

            //if (sum_strong_neg == 0)
            //{
            //    alpha = 0;
            //}
            //else
            //{
            //    alpha = sum_all_neg / sum_strong_neg;
            //}

            if (sum_strong_pos == 0)
            {
                diag += sum_all_pos;
                beta = 0;
            }
            else
            {
                beta = sum_all_pos / sum_strong_pos;
            }

            neg_coeff = -alpha / diag;
            pos_coeff = -beta / diag;

            start = S->on_proc->idx1[i];
            end = S->on_proc->idx1[i+1];
            if (S->on_proc->idx2[start] == i)
            {
                start++;
            }
            for (int j = start; j < end; j++)
            {
                col = S->on_proc->idx2[j];
                if (states[col] == 1)
                {
                    val = sa_on[j];
                    P->on_proc->idx2.emplace_back(on_proc_col_to_new[col]);
                    
                    if (val < 0)
                    {
                        P->on_proc->vals.emplace_back(neg_coeff * val);
                    }
                    else
                    {
                        P->on_proc->vals.emplace_back(pos_coeff * val);
                    }
                }
            }
            start = S->off_proc->idx1[i];
            end = S->off_proc->idx1[i+1];
            for (int j = start; j < end; j++)
            {
                col = S->off_proc->idx2[j];
                if (off_proc_states[col] == 1)
                {
                    val = sa_off[j];
                    col_exists[col] = true;
                    P->off_proc->idx2.emplace_back(col);

                    if (val < 0)
                    {
                        P->off_proc->vals.emplace_back(neg_coeff * val);
                    }
                    else
                    {
                        P->off_proc->vals.emplace_back(pos_coeff * val);
                    }
                }
            }
        }
        P->on_proc->idx1[i+1] = P->on_proc->idx2.size();
        P->off_proc->idx1[i+1] = P->off_proc->idx2.size();
    }
    P->on_proc->nnz = P->on_proc->idx2.size();
    P->off_proc->nnz = P->off_proc->idx2.size();
    P->local_nnz = P->on_proc->nnz + P->off_proc->nnz;
    
    for (int i = 0; i < S->off_proc_num_cols; i++)
    {
        if (col_exists[i])
        {
            off_proc_col_to_new[i] = P->off_proc_column_map.size();
            P->off_proc_column_map.emplace_back(S->off_proc_column_map[i]);
        }
    }
    for (aligned_vector<int>::iterator it = P->off_proc->idx2.begin(); 
            it != P->off_proc->idx2.end(); ++it)
    {
        *it = off_proc_col_to_new[*it];
    }


    P->off_proc_num_cols = P->off_proc_column_map.size();
    P->on_proc_num_cols = P->on_proc_column_map.size();
    P->off_proc->n_cols = P->off_proc_num_cols;
    P->on_proc->n_cols = P->on_proc_num_cols;

    if (tap_interp)
    {
        P->update_tap_comm(S, on_proc_col_to_new, off_proc_col_to_new, comm_t);
    }
    else
    {
        P->comm = new ParComm(S->comm, on_proc_col_to_new, off_proc_col_to_new,
                comm_t);
    }


    return P;
}
