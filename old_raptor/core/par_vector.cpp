// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "par_vector.hpp"

using namespace raptor;


/**************************************************************
*****   ParVector Set Constant Value
**************************************************************
***** Sets each element of the local vector to a constant value
*****
***** Parameters
***** -------------
***** alpha : data_t
*****    Value to set each element of local vector to
**************************************************************/
void ParVector::set_const_value(data_t alpha)
{
    if (local_n)
    {
        local->set_const_value(alpha);
    }
}

/**************************************************************
*****   ParVector Set Random Values
**************************************************************
***** Sets each element of the local vector to a random value
**************************************************************/
void ParVector::set_rand_values()
{
    if (local_n)
    {
        local->set_rand_values();
    }
}

/**************************************************************
*****   ParVector Append ParVector
**************************************************************
***** Appends a ParVector to the ParVector
**************************************************************/
void ParBVector::append(ParBVector& P)
{
    local->append(*(P.local));
}


/**************************************************************
*****   ParVector Add_Val
**************************************************************
***** Adds a Value to the ParVector 
**************************************************************/
void ParVector::add_val(data_t val, index_t vec, index_t global_n)
{
    if ((global_n >= first_local) && (global_n < first_local + local_n))
    {
        local->values[vec*local_n + (global_n-first_local)] = val;
    }
}

/**************************************************************
*****   ParBVector Add_Val ParVector
**************************************************************
***** Adds a Value to the ParVector to the ParVector
**************************************************************/
void ParBVector::add_val(data_t val, index_t vec, index_t global_n)
{
    if ((global_n >= first_local) && (global_n < first_local + local_n))
    {
        local->values[vec*local_n + (global_n-first_local)] = val;
    }
}

/**************************************************************
*****   ParVector Split ParVector
**************************************************************
***** Splits a ParVector into t bvecs making a ParBVector
***** Storing the local values in W at the appropriate bvec
***** index
**************************************************************/
void ParVector::split(ParVector& W, int t)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (t == num_procs) local->split(*(W.local), t, rank);
    else local->split_range(*(W.local), t, rank % t);
    //else if (t < num_procs) local->split(*(W.local), t, rank % t);
}

/**************************************************************
*****   ParVector Split Contiguous ParVector
**************************************************************
***** Splits a ParVector into t bvecs making a ParBVector
***** Storing the local values in equal sized contiguous blocks
***** for each rank
**************************************************************/
void ParVector::split_contig(ParVector& W, int t)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (num_procs == t) local->split(*(W.local), t, rank);
    else
    {
        int group_size = global_n / t;
        int n;
        for (int i = 0; i < local_n; i++)
        {
            n = first_local + i;
            W.add_val(local->values[i], n/group_size, n); 
        }
    }    
    //else local->split_contig(*(W.local), t, first_local, global_n);
    //else if (global_n % t != 0) this->split(W, t);
}
