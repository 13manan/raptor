// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "par_matrix.hpp"

using namespace raptor;

/**************************************************************
*****   ParMatrix Add Value
**************************************************************
***** Adds a value to the local portion of the parallel matrix,
***** determining whether it should be added to diagonal or 
***** off-diagonal block. 
*****
***** Parameters
***** -------------
***** row : index_t
*****    Local row of value
***** global_col : index_t 
*****    Global column of value
***** value : data_t
*****    Value to be added to parallel matrix
**************************************************************/    
void ParMatrix::add_value(
        int row, 
        index_t global_col, 
        data_t value)
{
    if (global_col >= partition->first_local_col 
            && global_col <= partition->last_local_col)
    {
        on_proc->add_value(row, global_col - partition->first_local_col, value);
    }
    else 
    {
        off_proc->add_value(row, global_col, value);
    }
}

/**************************************************************
*****   ParMatrix Add Global Value
**************************************************************
***** Adds a value to the local portion of the parallel matrix,
***** determining whether it should be added to diagonal or 
***** off-diagonal block. 
*****
***** Parameters
***** -------------
***** global_row : index_t
*****    Global row of value
***** global_col : index_t 
*****    Global column of value
***** value : data_t
*****    Value to be added to parallel matrix
**************************************************************/ 
void ParMatrix::add_global_value(
        index_t global_row, 
        index_t global_col, 
        data_t value)
{
    add_value(global_row - partition->first_local_row, global_col, value);
}

/**************************************************************
*****   ParMatrix Finalize
**************************************************************
***** Finalizes the diagonal and off-diagonal matrices.  Sorts
***** the local_to_global indices, and creates the parallel
***** communicator
*****
***** Parameters
***** -------------
***** create_comm : bool (optional)
*****    Boolean for whether parallel communicator should be 
*****    created (default is true)
**************************************************************/
void ParMatrix::finalize(bool create_comm)
{
    // Condense columns in off_proc, storing global
    // columns as 0-num_cols, and store mapping
    if (off_proc->nnz)
    {
        off_proc->condense_cols();
        off_proc->sort();
        off_proc_column_map = off_proc->get_col_list();
        off_proc_num_cols = off_proc_column_map.size();   
        off_proc->resize(local_num_rows, off_proc_num_cols);
    }
    else
    {
        off_proc_num_cols = 0;
    }

    if (on_proc->nnz)
    {
        on_proc->sort();

        // Assume nonzeros in each on_proc column
        on_proc->col_list.resize(on_proc->n_cols);
        for (int i = 0; i < on_proc->n_cols; i++)
        {
            on_proc->col_list[i] = i + partition->first_local_col;
        }
        on_proc_column_map = on_proc->get_col_list();
        on_proc_num_cols = on_proc_column_map.size();

        local_row_map = get_on_proc_column_map();
    }

    map_partition_to_local();
        
    local_nnz = on_proc->nnz + off_proc->nnz;

    if (create_comm)
        comm = new ParComm(partition, off_proc_column_map);
    else
        comm = new ParComm(partition);
}

void ParMatrix::map_partition_to_local()
{
    if (on_proc->nnz)
    {
        if (partition->local_num_cols)
        {
            on_proc_partition_to_col.resize(partition->local_num_cols);
            for (int i = 0; i < on_proc_num_cols; i++)
            {
                on_proc_partition_to_col[on_proc_column_map[i] - partition->first_local_col] = i;
            }
        }
    }
}

void ParMatrix::copy(ParCOOMatrix* A)
{
    partition = A->partition;
    partition->num_shared++;

    local_nnz = A->local_nnz;
    local_num_rows = A->local_num_rows;
    global_num_rows = A->global_num_rows;
    global_num_cols = A->global_num_cols;

    off_proc_column_map = off_proc->get_col_list();
    on_proc_column_map = on_proc->get_col_list();

    if (local_num_rows)
    {
        local_row_map.reserve(local_num_rows);
        for (std::vector<int>::iterator it = A->local_row_map.begin();
                it != A->local_row_map.end(); ++it)
        {
            local_row_map.push_back(*it);
        }
    }
    
    off_proc_num_cols = off_proc_column_map.size();
    on_proc_num_cols = on_proc_column_map.size();

    map_partition_to_local();

    if (A->comm)
    {
        comm = new ParComm((ParComm*) A->comm);
    }
    else
    {   
        comm = NULL;
    }
    
    if (A->tap_comm)
    {
        tap_comm = new TAPComm((TAPComm*) A->tap_comm);
    }
    else
    {
        tap_comm = NULL;
    }
}

void ParMatrix::copy(ParCSRMatrix* A)
{
    partition = A->partition;
    partition->num_shared++;

    local_nnz = A->local_nnz;
    local_num_rows = A->local_num_rows;
    global_num_rows = A->global_num_rows;
    global_num_cols = A->global_num_cols;

    off_proc_column_map = off_proc->get_col_list();
    on_proc_column_map = on_proc->get_col_list();
    
    off_proc_num_cols = off_proc_column_map.size();
    on_proc_num_cols = on_proc_column_map.size();

    if (local_num_rows)
    {
        local_row_map.reserve(local_num_rows);
        for (std::vector<int>::iterator it = A->local_row_map.begin();
                it != A->local_row_map.end(); ++it)
        {
            local_row_map.push_back(*it);
        }
    }

    map_partition_to_local();

    if (A->comm)
    {
        comm = new ParComm((ParComm*) A->comm);
    }
    else
    {   
        comm = NULL;
    }
    
    if (A->tap_comm)
    {
        tap_comm = new TAPComm((TAPComm*) A->tap_comm);
    }
    else
    {
        tap_comm = NULL;
    }
}

void ParMatrix::copy(ParCSCMatrix* A)
{
    partition = A->partition;
    partition->num_shared++;

    local_nnz = A->local_nnz;
    local_num_rows = A->local_num_rows;
    global_num_rows = A->global_num_rows;
    global_num_cols = A->global_num_cols;

    off_proc_column_map = off_proc->get_col_list();
    on_proc_column_map = on_proc->get_col_list();
    
    off_proc_num_cols = off_proc_column_map.size();
    on_proc_num_cols = on_proc_column_map.size();

    if (local_num_rows)
    {
        local_row_map.reserve(local_num_rows);
        for (std::vector<int>::iterator it = A->local_row_map.begin();
                it != A->local_row_map.end(); ++it)
        {
            local_row_map.push_back(*it);
        }
    }

    map_partition_to_local();

    if (A->comm)
    {
        comm = new ParComm((ParComm*) A->comm);
    }
    else
    {   
        comm = NULL;
    }
    
    if (A->tap_comm)
    {
        tap_comm = new TAPComm((TAPComm*) A->tap_comm);
    }
    else
    {
        tap_comm = NULL;
    }
}

void ParCOOMatrix::copy(ParCSRMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new COOMatrix((CSRMatrix*) A->on_proc);
    off_proc = new COOMatrix((CSRMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCOOMatrix::copy(ParCSCMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new COOMatrix((CSCMatrix*) A->on_proc);
    off_proc = new COOMatrix((CSCMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCOOMatrix::copy(ParCOOMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new COOMatrix((COOMatrix*) A->on_proc);
    off_proc = new COOMatrix((COOMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSRMatrix::copy(ParCSRMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }
    on_proc = new CSRMatrix((CSRMatrix*) A->on_proc);
    off_proc = new CSRMatrix((CSRMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSRMatrix::copy(ParCSCMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }
    on_proc = new CSRMatrix((CSCMatrix*) A->on_proc);
    off_proc = new CSRMatrix((CSCMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSRMatrix::copy(ParCOOMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new CSRMatrix((COOMatrix*) A->on_proc);
    off_proc = new CSRMatrix((COOMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSCMatrix::copy(ParCSRMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new CSCMatrix((CSRMatrix*) A->on_proc);
    off_proc = new CSCMatrix((CSRMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSCMatrix::copy(ParCSCMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new CSCMatrix((CSCMatrix*) A->on_proc);
    off_proc = new CSCMatrix((CSCMatrix*) A->off_proc);

    ParMatrix::copy(A);
}

void ParCSCMatrix::copy(ParCOOMatrix* A)
{
    if (on_proc)
    {   
        delete on_proc;
    }
    if (off_proc)
    {
        delete off_proc;
    }

    on_proc = new CSCMatrix((COOMatrix*) A->on_proc);
    off_proc = new CSCMatrix((COOMatrix*) A->off_proc);

    ParMatrix::copy(A);
}


