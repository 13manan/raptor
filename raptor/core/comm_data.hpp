// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef RAPTOR_CORE_COMMDATA_HPP
#define RAPTOR_CORE_COMMDATA_HPP

#define WITH_MPI 1

#include <mpi.h>
#include "types.hpp"
#include "vector.hpp"

/**************************************************************
 *****   CommData Class
 **************************************************************
 **************************************************************/
namespace raptor
{
    // Forward Declaration
    class Vector;

class CommData
{
public:
    /**************************************************************
    *****   CommData Class Constructor
    **************************************************************
    ***** Initializes an empty CommData, setting number and size of 
    ***** messages to zero.

    **************************************************************/
    CommData()
    {
        num_msgs = 0;
        size_msgs = 0;
        indptr.push_back(0);
    }

    CommData(CommData* data)
    {
        num_msgs = data->num_msgs;
        size_msgs = data->size_msgs;
        int n_procs = data->procs.size();
        int n_indptr = data->indptr.size();
        int n_indices = data->indices.size();
        if (n_procs)
        {
            procs.resize(n_procs);
            for (int i = 0; i < n_procs; i++)
            {
                procs[i] = data->procs[i];
            }
        }

        if (n_indptr)
        {
            indptr.resize(n_indptr);
            for (int i = 0; i < n_indptr; i++)
            {
                indptr[i] = data->indptr[i];
            }
        }

        if (n_indices)
        {
            indices.resize(n_indices);
            for (int i = 0; i < n_indices; i++)
            {
                indices[i] = data->indices[i];
            }
        }

        if (num_msgs)
        {
            requests = new MPI_Request[num_msgs];
        }

        if (size_msgs)
        {
            buffer.set_size(size_msgs);
        }
    }

    /**************************************************************
    *****   ParComm Class Destructor
    **************************************************************
    ***** 
    **************************************************************/
    ~CommData()
    {
        if (num_msgs)
        {
            delete[] requests;
        }
    };

    void add_msg(int proc,
            int msg_size,
            int* msg_indices)
    {
        int last_ptr = indptr[num_msgs];
        procs.push_back(proc);
        indptr.push_back(last_ptr + msg_size);
        for (int i = 0; i < msg_size; i++)
        {
            indices.push_back(msg_indices[i]);
        }

        num_msgs++;
        size_msgs += msg_size;
    }

    void add_msg(int proc,
            int msg_size)
    {
        int last_ptr = indptr[num_msgs];
        int idx_start = 0;
        if (size_msgs)
        {
            idx_start = indices[size_msgs-1];
        }
        procs.push_back(proc);
        indptr.push_back(last_ptr + msg_size);

        for (int i = 0; i < msg_size; i++)
        {
            indices.push_back(idx_start + i + 1);
        }

        num_msgs++;
        size_msgs += msg_size;
    }

    void finalize()
    {
        if (num_msgs)
        {
            requests = new MPI_Request[num_msgs];
        }
        buffer.set_size(size_msgs);
    }

    int num_msgs;
    int size_msgs;
    std::vector<int> procs;
    std::vector<int> indptr;
    std::vector<int> indices;
    MPI_Request* requests;
    Vector buffer;
};
}
#endif

