#include <mpi.h>
#include <Eigen/Sparse>
#include <Eigen/Dense>
//using Eigen::VectorXd;

#include "vector.hpp"
#include "par_matrix.hpp"
#include "par_vector.hpp"
using namespace raptor;

void sequential_spmv(Matrix* A, Vector* x, Vector* y, data_t alpha, data_t beta);
void parallel_spmv(ParMatrix* A, ParVector* x, ParVector* y, data_t alpha, data_t beta);

void parallel_spmv(ParMatrix* A, ParVector* x, ParVector* y, data_t alpha, data_t beta)
{
    // Get MPI Information
    index_t rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	// TODO must enforce that y and x are not aliased, or this will NOT work
	//    or we could use blocking sends as long as we post the iRecvs first

    // Declare communication variables
	MPI_Request*                            send_requests;
    MPI_Request*                            recv_requests;
    data_t*                                 send_buffer;
    data_t*                                 recv_buffer;
    data_t*                                 local_data;
    Vector                                  offd_tmp;
    index_t                                 num_recv_procs;
    index_t                                 num_send_procs;
    index_t                                 tmp_size;
    index_t                                 begin;
    index_t                                 ctr;
    index_t                                 request_ctr;
    ParComm*                                comm;
    std::vector<index_t>                    send_procs;
    std::vector<index_t>                    recv_procs;
    std::map<index_t, std::vector<index_t>> send_indices;
    std::map<index_t, std::vector<index_t>> recv_indices;

    // Initialize communication variables
    comm          = A->comm;
    send_procs    = comm->send_procs;
    recv_procs    = comm->recv_procs;
    send_indices  = comm->send_indices;
    recv_indices  = comm->recv_indices;
    tmp_size      = comm->size_recvs;
    local_data    = x->local->data();
    if (A->offd_num_cols)
    {
	    // TODO we do not want to malloc these every time
	    send_requests = new MPI_Request [send_procs.size()];
	    recv_requests = new MPI_Request [recv_procs.size()];
        send_buffer = new data_t [comm->size_sends];
        recv_buffer = new data_t [comm->size_recvs];
    }

    // Send and receive vector data
    if (A->offd_num_cols)
    {
	    // Begin sending and gathering off-diagonal entries
        begin = 0;
        ctr = 0;
        request_ctr = 0;
        for (auto proc : recv_procs)
        {
            index_t num_recv = recv_indices[proc].size();
            MPI_Irecv(&recv_buffer[begin], num_recv, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &(recv_requests[request_ctr++]));
            begin += num_recv;
        }

        begin = 0;
        request_ctr = 0;
	    for (auto proc : send_procs)
        {
            ctr = 0;
            for (auto send_idx : send_indices[proc])
            {
                send_buffer[begin + ctr] = local_data[send_idx];
                ctr++;
            }
            MPI_Isend(&send_buffer[begin], ctr, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &(send_requests[request_ctr++]));
            begin += ctr;
        }
    }

	// Add contribution of beta
	y->scale(beta);

	// Compute partial SpMV with local information
    sequential_spmv(A->diag, x->local, y->local, alpha, 0.0); 
	
    // Once data is available, add contribution of off-diagonals
	// TODO Deal with new entries as they become available
	// TODO Add an error check on the status
    // TODO Using MPI_STATUS_IGNORE (delete[] recvStatus was causing a segfault)
    if (A->offd_num_cols)
    {
        // Wait for all receives to finish
	    MPI_Waitall(recv_procs.size(), recv_requests, MPI_STATUS_IGNORE);

        // Add received data to Vector
        offd_tmp = Vector::Map(recv_buffer, tmp_size);

        // Compute partial SpMV with offd information
        sequential_spmv(A->offd, &offd_tmp, y->local, alpha, 1.0); 

	    // Wait for all sends to finish
	    // TODO Add an error check on the status
	    MPI_Waitall(send_procs.size(), send_requests, MPI_STATUS_IGNORE);

        // Delete MPI_Requests
	    delete[] send_requests; 
	    delete[] recv_requests; 
        
    }
}

//Sequential SpMV (alpha*A*x + beta*y)
void sequential_spmv(Matrix* A, Vector* x, Vector* y, data_t alpha, data_t beta)
{
    *y = alpha*((*(A->m))*(*x)) + beta * (*y);
}

