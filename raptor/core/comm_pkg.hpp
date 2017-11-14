// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef RAPTOR_CORE_PARCOMM_HPP
#define RAPTOR_CORE_PARCOMM_HPP

#include <mpi.h>
#include "comm_data.hpp"
#include "matrix.hpp"
#include "partition.hpp"
#include "par_vector.hpp"
//#include "par_matrix.hpp"

#define STANDARD_PPN 4
#define STANDARD_PROC_LAYOUT 1
#define eager_cutoff 1000
#define short_cutoff 62
#define ideal_n_comm 4

/**************************************************************
 *****   CommPkg Class:
 **************************************************************
 ***** This class constructs a parallel communicator, containing
 ***** which messages must be sent/recieved for matrix operations
 *****
 ***** Methods
 ***** -------
 ***** communicate(data_t* values)
 *****    Communicates values to processes, based on underlying
 *****    communication package
 ***** form_col_to_proc(...)
 *****    Maps each column in off_proc_column_map to process 
 *****    on which corresponding values are stored
 **************************************************************/
namespace raptor
{
    class ParCSRMatrix;

    class CommPkg
    {
      public:
        CommPkg(Partition* partition)
        {
            topology = partition->topology;
            topology->num_shared++;
        }
        
        CommPkg(Topology* _topology)
        {
            topology = _topology;
            topology->num_shared++;
        }

        virtual ~CommPkg()
        {
            if (topology)
            {
                if (topology->num_shared)
                {
                    topology->num_shared--;
                }
                else
                {
                    delete topology;
                }
            }
        }


        std::vector<double>& communicate(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD);


        virtual std::vector<double>& communicate(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual std::vector<double>& communicate(const data_t* values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual std::vector<int>& communicate(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual std::vector<int>& communicate(const int* values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;

        virtual void communicate_T(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void communicate_T(const data_t* values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void communicate_T(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void communicate_T(const int* values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;

        virtual void init_comm(const std::vector<data_t>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm(const std::vector<int>& values,
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm(const int* values, MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual std::vector<double>& complete_comm() = 0;
        virtual std::vector<int>& complete_int_comm() = 0;

        virtual void init_comm_T(const std::vector<data_t>& values, 
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm_T(const std::vector<int>& values,
                MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm_T(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void init_comm_T(const int* values, MPI_Comm comm = MPI_COMM_WORLD) = 0;
        virtual void complete_comm_T() = 0;
        virtual void complete_int_comm_T() = 0;

        virtual CSRMatrix* communicate(std::vector<int>& rowptr, 
                std::vector<int>& col_indices,
                std::vector<double>& values, MPI_Comm comm = MPI_COMM_WORLD) = 0;
        CSRMatrix* communicate(ParCSRMatrix* A, MPI_Comm comm = MPI_COMM_WORLD);
/*
        CSRMatrix* communicate_T(std::vector<int>& rowptr, 
                std::vector<int>& col_indices,
                std::vector<double>& values, MPI_Comm comm = MPI_COMM_WORLD) {} 
*/
        virtual std::vector<double>& get_recv_buffer() = 0;
        virtual std::vector<int>& get_int_recv_buffer() = 0;

        Topology* topology;
    };


    /**************************************************************
    *****   ParComm Class
    **************************************************************
    ***** This class constructs a standard parallel communicator: 
    ***** which messages must be sent/recieved for matrix operations
    *****
    ***** Attributes
    ***** -------------
    ***** num_sends : index_t
    *****    Number of messages this process must send during 
    *****    matrix operations
    ***** num_recvs : index_t
    *****    Number of messages this process will recv during
    *****    matrix operations
    ***** size_sends : index_t 
    *****    Total number of elements this process sends in all
    *****    messages
    ***** size_recvs : index_t 
    *****    Total number of elements this process recvs from
    *****    all messages
    ***** send_procs : std::vector<index_t>
    *****    Distant processes messages are to be sent to
    ***** send_row_starts : std::vector<index_t>
    *****    Pointer to first position in send_row_indices
    *****    that a given process will send.
    ***** send_row_indices : std::vector<index_t> 
    *****    The indices of values that must be sent to each
    *****    process in send_procs
    ***** recv_procs : std::vector<index_t>
    *****    Distant processes messages are to be recvd from
    ***** recv_col_starts : std::vector<index_t>
    *****    Pointer to first column recvd from each process
    *****    in recv_procs
    ***** col_to_proc : std::vector<index_t>
    *****    Maps each local column in the off-diagonal block
    *****    to the process that holds corresponding data
    **************************************************************/
    class ParComm : public CommPkg
    {
      public:
        /**************************************************************
        *****   ParComm Class Constructor
        **************************************************************
        ***** Initializes an empty ParComm, setting send and recv
        ***** sizes to 0
        *****
        ***** Parameters
        ***** -------------
        ***** _key : int (optional)
        *****    Tag to be used in MPI Communication (default 0)
        **************************************************************/
        ParComm(Partition* partition, int _key = 0) : CommPkg(partition)
        {
            key = _key;
            send_data = new CommData();
            recv_data = new CommData();
        }
        ParComm(Topology* topology, int _key = 0) : CommPkg(topology)
        {
            key = _key;
            send_data = new CommData();
            recv_data = new CommData();
        }

        /**************************************************************
        *****   ParComm Class Constructor
        **************************************************************
        ***** Initializes a ParComm object based on the off_proc Matrix
        *****
        ***** Parameters
        ***** -------------
        ***** off_proc_column_map : std::vector<index_t>&
        *****    Maps local off_proc columns indices to global
        ***** _key : int (optional)
        *****    Tag to be used in MPI Communication (default 9999)
        **************************************************************/
        ParComm(Partition* partition,
                const std::vector<int>& off_proc_column_map,
                int _key = 9999,
                MPI_Comm comm = MPI_COMM_WORLD) : CommPkg(partition)
        {
            init_par_comm(partition, off_proc_column_map, _key, comm);
        }

        ParComm(Partition* partition,
                const std::vector<int>& off_proc_column_map,
                const std::vector<int>& on_proc_column_map,
                int _key = 9999, 
                MPI_Comm comm = MPI_COMM_WORLD) : CommPkg(partition)
        {
            int idx;
            int ctr = 0;
            std::vector<int> part_col_to_new;

            init_par_comm(partition, off_proc_column_map, _key, comm);
            
            if (partition->local_num_cols)
            {
                part_col_to_new.resize(partition->local_num_cols, -1);
            }
            for (std::vector<int>::const_iterator it = on_proc_column_map.begin();
                    it != on_proc_column_map.end(); ++it)
            {
                part_col_to_new[*it - partition->first_local_col] = ctr++;
            }

            for (int i = 0; i < send_data->size_msgs; i++)
            {
                idx = send_data->indices[i];
                send_data->indices[i] = part_col_to_new[idx];
                assert(part_col_to_new[idx] >= 0);
            }
        }

        void init_par_comm(Partition* partition,
                const std::vector<int>& off_proc_column_map,
                int _key, MPI_Comm comm)
        {
            // Get MPI Information
            int rank, num_procs;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &num_procs);

            // Initialize class variables
            key = _key;
            send_data = new CommData();
            recv_data = new CommData();

            // Declare communication variables
            int send_start, send_end;
            int proc, prev_proc;
            int count;
            int tag = 12345;  // TODO -- switch this to key?
            int off_proc_num_cols = off_proc_column_map.size();
            MPI_Status recv_status;

            std::vector<int> off_proc_col_to_proc(off_proc_num_cols);
            std::vector<int> tmp_send_buffer;

            partition->form_col_to_proc(off_proc_column_map, off_proc_col_to_proc);

            // Determine processes columns are received from,
            // and adds corresponding messages to recv data.
            // Assumes columns are partitioned across processes
            // in contiguous blocks, and are sorted
            if (off_proc_num_cols)
            {
                prev_proc = off_proc_col_to_proc[0];
                int prev_idx = 0;
                for (int i = 1; i < off_proc_num_cols; i++)
                {
                    proc = off_proc_col_to_proc[i];
                    if (proc != prev_proc)
                    {
                        recv_data->add_msg(prev_proc, i - prev_idx);
                        prev_proc = proc;
                        prev_idx = i;
                    }
                }
                recv_data->add_msg(prev_proc, off_proc_num_cols - prev_idx);
                recv_data->finalize();
            }

            // For each process I recv from, send the global column indices
            // for which I must recv corresponding rows 
            if (recv_data->size_msgs)
            {
                tmp_send_buffer.resize(recv_data->size_msgs);
            }
            for (int i = 0; i < recv_data->num_msgs; i++)
            {
                proc = recv_data->procs[i];
                send_start = recv_data->indptr[i];
                send_end = recv_data->indptr[i+1];
                for (int j = send_start; j < send_end; j++)
                {
                    tmp_send_buffer[j] = off_proc_column_map[j];
                }
                MPI_Issend(&(tmp_send_buffer[send_start]), send_end - send_start, MPI_INT, 
                        proc, tag, comm, &(recv_data->requests[i]));
            }

            // Determine which processes to which I send messages,
            // and what vector indices to send to each.
            // Receive any messages, regardless of source (which is unknown)
            int finished, msg_avail;
            MPI_Request barrier_request;
	        if (recv_data->num_msgs)
	        {
            	MPI_Testall(recv_data->num_msgs, recv_data->requests.data(), &finished,
                        MPI_STATUSES_IGNORE);
                while (!finished)
                {
                    MPI_Iprobe(MPI_ANY_SOURCE, tag, comm, &msg_avail, &recv_status);
                    if (msg_avail)
                    {
                        MPI_Get_count(&recv_status, MPI_INT, &count);
                        proc = recv_status.MPI_SOURCE;
                        int recvbuf[count];
                        MPI_Recv(recvbuf, count, MPI_INT, proc, tag, comm, &recv_status);
                        for (int i = 0; i < count; i++)
                        {
                            recvbuf[i] -= partition->first_local_col;
                        }
                        send_data->add_msg(proc, count, recvbuf);
                    }
                    MPI_Testall(recv_data->num_msgs, recv_data->requests.data(), &finished,
                            MPI_STATUSES_IGNORE);
                }
	        }
            MPI_Ibarrier(comm, &barrier_request);
            MPI_Test(&barrier_request, &finished, MPI_STATUS_IGNORE);
            while (!finished)
            {
                MPI_Iprobe(MPI_ANY_SOURCE, tag, comm, &msg_avail, &recv_status);
                if (msg_avail)
                {
                    MPI_Get_count(&recv_status, MPI_INT, &count);
                    proc = recv_status.MPI_SOURCE;
                    int recvbuf[count];
                    MPI_Recv(recvbuf, count, MPI_INT, proc, tag, comm, &recv_status);
                    for (int i = 0; i < count; i++)
                    {
                        recvbuf[i] -= partition->first_local_col;
                    }
                    send_data->add_msg(proc, count, recvbuf);
                }
                MPI_Test(&barrier_request, &finished, MPI_STATUS_IGNORE);
            }
            if (send_data->num_msgs)
            {
                send_data->finalize();
            }
        }

        ParComm(ParComm* comm) : CommPkg(comm->topology)
        {
            send_data = new CommData(comm->send_data);
            recv_data = new CommData(comm->recv_data);
            key = comm->key;
        }

        ParComm(ParComm* comm, const std::vector<int>& off_proc_col_to_new)
            : CommPkg(comm->topology)
        {
            bool comm_proc;
            int proc, start, end;
            int idx, new_idx;

            send_data = new CommData();
            recv_data = new CommData();

            if (comm == NULL)
            {
                key = 0;
                return;
            }
            key = comm->key;

            comm->communicate_T(off_proc_col_to_new);

            for (int i = 0; i < comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                proc = comm->recv_data->procs[i];
                start = comm->recv_data->indptr[i];
                end = comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = comm->recv_data->indices[j];
                    new_idx = off_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        recv_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    recv_data->procs.push_back(proc);
                    recv_data->indptr.push_back(recv_data->indices.size());
                }
            }
            recv_data->num_msgs = recv_data->procs.size();
            recv_data->size_msgs = recv_data->indices.size();
            recv_data->finalize();

            for (int i = 0; i < comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                proc = comm->send_data->procs[i];
                start = comm->send_data->indptr[i];
                end = comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    if (comm->send_data->int_buffer[j] != -1)
                    {
                        comm_proc = true;
                        send_data->indices.push_back(comm->send_data->indices[j]);
                    }
                }
                if (comm_proc)
                {
                    send_data->procs.push_back(proc);
                    send_data->indptr.push_back(send_data->indices.size());
                }
            }
            send_data->num_msgs = send_data->procs.size();
            send_data->size_msgs = send_data->indices.size();
            send_data->finalize();
        }

        ParComm(ParComm* comm, const std::vector<int>& on_proc_col_to_new,
                const std::vector<int>& off_proc_col_to_new) 
            : CommPkg(comm->topology)
        {
            bool comm_proc;
            int proc, start, end;
            int idx, new_idx;

            send_data = new CommData();
            recv_data = new CommData();

            if (comm == NULL)
            {
                key = 0;
                return;
            }
            key = comm->key;

            comm->communicate_T(off_proc_col_to_new);

            for (int i = 0; i < comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                proc = comm->recv_data->procs[i];
                start = comm->recv_data->indptr[i];
                end = comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = comm->recv_data->indices[j];
                    new_idx = off_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        recv_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    recv_data->procs.push_back(proc);
                    recv_data->indptr.push_back(recv_data->indices.size());
                }
            }
            recv_data->num_msgs = recv_data->procs.size();
            recv_data->size_msgs = recv_data->indices.size();
            recv_data->finalize();

            for (int i = 0; i < comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                proc = comm->send_data->procs[i];
                start = comm->send_data->indptr[i];
                end = comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = comm->send_data->indices[j];
                    new_idx = on_proc_col_to_new[idx];
                    if (new_idx != -1 && comm->send_data->int_buffer[j] != -1)
                    {
                        comm_proc = true;
                        send_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    send_data->procs.push_back(proc);
                    send_data->indptr.push_back(send_data->indices.size());
                }
            }
            send_data->num_msgs = send_data->procs.size();
            send_data->size_msgs = send_data->indices.size();
            send_data->finalize();

        }

        /**************************************************************
        *****   ParComm Class Destructor
        **************************************************************
        ***** 
        **************************************************************/
        ~ParComm()
        {
            delete send_data;
            delete recv_data;
        }

        std::vector<double>& communicate(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<double>& communicate(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<int>& communicate(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<int>& communicate(const int* values, MPI_Comm comm = MPI_COMM_WORLD);

        void communicate_T(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const int* values, MPI_Comm comm = MPI_COMM_WORLD);

        void init_comm(const std::vector<data_t>& values,
                MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const std::vector<int>& values,
                MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const int* values, MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<double>& complete_comm();
        std::vector<int>& complete_int_comm();

        void init_comm_T(const std::vector<data_t>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const std::vector<int>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const int* values, MPI_Comm comm = MPI_COMM_WORLD);
        void complete_comm_T();
        void complete_int_comm_T();

        CSRMatrix* communicate(std::vector<int>& rowptr, std::vector<int>& col_indices,
                std::vector<double>& values, MPI_Comm comm = MPI_COMM_WORLD);
        CSRMatrix* communicate_T(std::vector<int>& rowptr, std::vector<int>& col_indices,
                std::vector<double>& values, MPI_Comm comm = MPI_COMM_WORLD);
        CSRMatrix* communication_helper(std::vector<int>& rowptr, 
                std::vector<int>& col_indices, std::vector<double>& values,
                MPI_Comm comm, CommData* send_comm, CommData* recv_comm);

        std::vector<double>& communicate(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD)
        {
            return CommPkg::communicate(v, comm);
        }
        CSRMatrix* communicate(ParCSRMatrix* A, MPI_Comm comm = MPI_COMM_WORLD)
        {
            return CommPkg::communicate(A, comm);
        }
        void init_comm(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD)
        {
            CommPkg::init_comm(v, comm);
        }

        std::vector<double>& get_recv_buffer()
        {
            return recv_data->buffer;
        }
        std::vector<int>& get_int_recv_buffer()
        {
            return recv_data->int_buffer;
        }

        int key;
        CommData* send_data;
        CommData* recv_data;
    };



    /**************************************************************
    *****   TAPComm Class
    **************************************************************
    ***** This class constructs a topology-aware parallel communicator: 
    ***** which messages must be sent/recieved for matrix operations,
    ***** using topology-aware methods to limit the number and size
    ***** of inter-node messages
    *****
    ***** Attributes
    ***** -------------
    ***** local_S_par_comm : ParComm*
    *****    Parallel communication package for sending data that originates
    *****    on rank to other processes local to node, before inter-node
    *****    communication occurs.
    ***** local_R_par_comm : ParComm*
    *****    Parallel communication package for redistributing previously
    *****    received values (from inter-node communication step) to 
    *****    processes local to rank which need said values
    ***** local_L_par_comm : ParComm* 
    *****    Parallel communication package for communicating values
    *****    that both originate and have a final destination on node
    *****    (fully intra-node communication)
    ***** global_par_comm : ParComm*
    *****    Parallel communication package for sole inter-node step.
    ***** recv_buffer : Vector
    *****    Combination of local_L_par_comm and local_R_par_comm
    *****    recv buffers, ordered to match off_proc_column_map
    ***** L_to_orig : std::vector<int>
    *****    Maps the columns recvd by local_L_par_comm to original
    *****    position (in off_proc_column_map)
    ***** R_to_orig : std::vector<int>
    *****    Maps the columns recvd by local_R_par_comm to original
    *****    position (in off_proc_column_map)
    ***** Partition* partition
    *****    Partition, holding information about topology
    **************************************************************/
    class TAPComm : public CommPkg
    {
        public:

        TAPComm(Partition* partition) : CommPkg(partition)
        {
            local_S_par_comm = new ParComm(partition, 2345);
            local_R_par_comm = new ParComm(partition, 3456);
            local_L_par_comm = new ParComm(partition, 4567);
            global_par_comm = new ParComm(partition, 5678);
        }


        /**************************************************************
        *****   TAPComm Class Constructor
        **************************************************************
        ***** Initializes a TAPComm for a matrix without contiguous
        ***** row-wise partitions across processes.  Instead, each
        ***** process holds a random assortment of rows. 
        *****
        ***** Parameters
        ***** -------------
        ***** off_proc_column_map : std::vector<int>&
        *****    Maps local off_proc columns indices to global
        ***** global_num_cols : int
        *****    Number of global columns in matrix
        ***** local_num_cols : int
        *****    Number of columns local to rank
        **************************************************************/
        TAPComm(Partition* partition, 
                const std::vector<int>& off_proc_column_map,
                MPI_Comm comm = MPI_COMM_WORLD) : CommPkg(partition)
        {
            init_tap_comm(partition, off_proc_column_map, comm);
        }

        TAPComm(Partition* partition,
                const std::vector<int>& off_proc_column_map,
                const std::vector<int>& on_proc_column_map,
                MPI_Comm comm = MPI_COMM_WORLD) : CommPkg(partition)
        {
            init_tap_comm(partition, off_proc_column_map, comm);

            std::vector<int> on_proc_to_new;
            int on_proc_num_cols = on_proc_column_map.size();
            if (partition->local_num_cols)
            {
                on_proc_to_new.resize(partition->local_num_cols);
                for (int i = 0; i < on_proc_num_cols; i++)
                {
                    on_proc_to_new[on_proc_column_map[i] - partition->first_local_col] = i;
                }
            }
            
            for (std::vector<int>::iterator it = local_S_par_comm->send_data->indices.begin();
                    it != local_S_par_comm->send_data->indices.end(); ++it)
            {
                *it = on_proc_to_new[*it];
            }

            for (std::vector<int>::iterator it = local_L_par_comm->send_data->indices.begin();
                    it != local_L_par_comm->send_data->indices.end(); ++it)
            {
                *it = on_proc_to_new[*it];
            }
        }

        /**************************************************************
        *****   TAPComm Class Constructor
        **************************************************************
        ***** Create topology-aware communication class from 
        ***** original communication package (which processes rank
        ***** communication which, and what is sent to / recv from
        ***** each process.
        *****
        ***** Parameters
        ***** -------------
        ***** orig_comm : ParComm*
        *****    Existing standard communication package from which
        *****    to form topology-aware communicator
        **************************************************************/
        TAPComm(ParComm* orig_comm) : CommPkg(orig_comm->topology)
        {
            //TODO -- Write this constructor
        }

        TAPComm(TAPComm* tap_comm) : CommPkg(tap_comm->topology)
        {
            global_par_comm = new ParComm(tap_comm->global_par_comm);
            local_S_par_comm = new ParComm(tap_comm->local_S_par_comm);
            local_R_par_comm = new ParComm(tap_comm->local_R_par_comm);
            local_L_par_comm = new ParComm(tap_comm->local_L_par_comm);

            int recv_size = tap_comm->recv_buffer.size();
            if (recv_size)
            {
                recv_buffer.resize(recv_size);
                int_recv_buffer.resize(recv_size);
                orig_to_L.resize(recv_size);
                orig_to_R.resize(recv_size);
                if (tap_comm->L_to_orig.size())
                {
                    L_to_orig.resize(tap_comm->L_to_orig.size());
                }
                if (tap_comm->R_to_orig.size())
                {
                    R_to_orig.resize(tap_comm->R_to_orig.size());
                }
                for (int i = 0; i < (int) L_to_orig.size(); i++)
                {
                    L_to_orig[i] = tap_comm->L_to_orig[i];
                }
                for (int i = 0; i < (int) R_to_orig.size(); i++)
                {
                    R_to_orig[i] = tap_comm->R_to_orig[i];
                }
                for (int i = 0; i < recv_size; i++)
                {
                    orig_to_L[i] = tap_comm->orig_to_L[i];
                    orig_to_R[i] = tap_comm->orig_to_R[i];
                }
            }
        }

        TAPComm(TAPComm* tap_comm, const std::vector<int>& on_proc_col_to_new,
                const std::vector<int>& off_proc_col_to_new, bool communicate = false) 
            : CommPkg(tap_comm->topology)
        {
            bool comm_proc;
            int proc, start, end;
            int idx, new_idx;
            int ctr;
            int new_idx_L;
            int new_idx_R;

            local_L_par_comm = new ParComm(tap_comm->topology, 
                    tap_comm->local_L_par_comm->key);
            local_S_par_comm = new ParComm(tap_comm->topology,
                    tap_comm->local_S_par_comm->key);
            local_R_par_comm = new ParComm(tap_comm->topology,
                    tap_comm->local_R_par_comm->key);
            global_par_comm = new ParComm(tap_comm->topology,
                    tap_comm->global_par_comm->key);
            
            // Communicate the col_to_new lists to other local procs
            tap_comm->local_S_par_comm->communicate(on_proc_col_to_new, topology->local_comm);
            tap_comm->local_R_par_comm->communicate_T(off_proc_col_to_new,
                    topology->local_comm);

            // Form col_to_new for S_recv, R_send, and global_recv
            std::vector<int>& S_recv_col_to_new 
                = tap_comm->local_S_par_comm->recv_data->int_buffer;
            std::vector<int>& R_send_col_to_new = 
                tap_comm->local_R_par_comm->send_data->int_buffer;
            std::vector<int> global_recv_col_to_new;
            if (tap_comm->global_par_comm->recv_data->size_msgs)
            {
                global_recv_col_to_new.resize(tap_comm->global_par_comm->recv_data->size_msgs);
            }
            for (int i = 0; i < tap_comm->local_R_par_comm->send_data->size_msgs; i++)
            {
                idx = tap_comm->local_R_par_comm->send_data->indices[i];
                global_recv_col_to_new[idx] = R_send_col_to_new[i];
            }

            // Update local_L_par_comm
            for (int i = 0; i < tap_comm->local_L_par_comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_L_par_comm->send_data->indptr[i];
                end = tap_comm->local_L_par_comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->local_L_par_comm->send_data->indices[j];
                    new_idx = on_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        local_L_par_comm->send_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_L_par_comm->send_data->procs[i];
                    local_L_par_comm->send_data->procs.push_back(proc);
                    local_L_par_comm->send_data->indptr.push_back(
                            local_L_par_comm->send_data->indices.size());
                }
            }
            local_L_par_comm->send_data->num_msgs = local_L_par_comm->send_data->procs.size();
            local_L_par_comm->send_data->size_msgs = local_L_par_comm->send_data->indices.size();
            local_L_par_comm->send_data->finalize();

            for (int i = 0; i < tap_comm->local_L_par_comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_L_par_comm->recv_data->indptr[i];
                end = tap_comm->local_L_par_comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->L_to_orig[j];
                    new_idx = off_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        new_idx_L = L_to_orig.size();
                        L_to_orig.push_back(new_idx);
                        local_L_par_comm->recv_data->indices.push_back(new_idx_L);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_L_par_comm->recv_data->procs[i];
                    local_L_par_comm->recv_data->procs.push_back(proc);
                    local_L_par_comm->recv_data->indptr.push_back(
                            local_L_par_comm->recv_data->indices.size());
                }
            }
            local_L_par_comm->recv_data->num_msgs = local_L_par_comm->recv_data->procs.size();
            local_L_par_comm->recv_data->size_msgs = local_L_par_comm->recv_data->indices.size();
            local_L_par_comm->recv_data->finalize();



            // Update local_S_par_comm
            for (int i = 0; i < tap_comm->local_S_par_comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_S_par_comm->send_data->indptr[i];
                end = tap_comm->local_S_par_comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->local_S_par_comm->send_data->indices[j];
                    new_idx = on_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        local_S_par_comm->send_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_S_par_comm->send_data->procs[i];
                    local_S_par_comm->send_data->procs.push_back(proc);
                    local_S_par_comm->send_data->indptr.push_back(
                            local_S_par_comm->send_data->indices.size());
                }
            }
            local_S_par_comm->send_data->num_msgs = local_S_par_comm->send_data->procs.size();
            local_S_par_comm->send_data->size_msgs = local_S_par_comm->send_data->indices.size();
            local_S_par_comm->send_data->finalize();

            for (int i = 0; i < tap_comm->local_S_par_comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_S_par_comm->recv_data->indptr[i];
                end = tap_comm->local_S_par_comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    new_idx = S_recv_col_to_new[j];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        local_S_par_comm->recv_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_S_par_comm->recv_data->procs[i];
                    local_S_par_comm->recv_data->procs.push_back(proc);
                    local_S_par_comm->recv_data->indptr.push_back(
                            local_S_par_comm->recv_data->indices.size());
                }
            }
            local_S_par_comm->recv_data->num_msgs = local_S_par_comm->recv_data->procs.size();
            local_S_par_comm->recv_data->size_msgs = local_S_par_comm->recv_data->indices.size();
            local_S_par_comm->recv_data->finalize();


            // Update local_R_par_comm
            for (int i = 0; i < tap_comm->local_R_par_comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_R_par_comm->recv_data->indptr[i];
                end = tap_comm->local_R_par_comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->R_to_orig[j];
                    new_idx = off_proc_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        new_idx_R = R_to_orig.size();
                        R_to_orig.push_back(new_idx);
                        local_R_par_comm->recv_data->indices.push_back(new_idx_R);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_R_par_comm->recv_data->procs[i];
                    local_R_par_comm->recv_data->procs.push_back(proc);
                    local_R_par_comm->recv_data->indptr.push_back(
                            local_R_par_comm->recv_data->indices.size());
                }
            }
            local_R_par_comm->recv_data->num_msgs = local_R_par_comm->recv_data->procs.size();
            local_R_par_comm->recv_data->size_msgs = local_R_par_comm->recv_data->indices.size();
            local_R_par_comm->recv_data->finalize();

            for (int i = 0; i < tap_comm->local_R_par_comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->local_R_par_comm->send_data->indptr[i];
                end = tap_comm->local_R_par_comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->local_R_par_comm->send_data->indices[j];
                    new_idx = R_send_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        local_R_par_comm->send_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->local_R_par_comm->send_data->procs[i];
                    local_R_par_comm->send_data->procs.push_back(proc);
                    local_R_par_comm->send_data->indptr.push_back(
                            local_R_par_comm->send_data->indices.size());
                }
            }
            local_R_par_comm->send_data->num_msgs = local_R_par_comm->send_data->procs.size();
            local_R_par_comm->send_data->size_msgs = local_R_par_comm->send_data->indices.size();
            local_R_par_comm->send_data->finalize();



            // Update global par comm
            for (int i = 0; i < tap_comm->global_par_comm->send_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->global_par_comm->send_data->indptr[i];
                end = tap_comm->global_par_comm->send_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    idx = tap_comm->global_par_comm->send_data->indices[j];
                    new_idx = S_recv_col_to_new[idx];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        global_par_comm->send_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->global_par_comm->send_data->procs[i];
                    global_par_comm->send_data->procs.push_back(proc);
                    global_par_comm->send_data->indptr.push_back(
                            global_par_comm->send_data->indices.size());
                }
            }
            global_par_comm->send_data->num_msgs = global_par_comm->send_data->procs.size();
            global_par_comm->send_data->size_msgs = global_par_comm->send_data->indices.size();
            global_par_comm->send_data->finalize();


            for (int i = 0; i < tap_comm->global_par_comm->recv_data->num_msgs; i++)
            {
                comm_proc = false;
                start = tap_comm->global_par_comm->recv_data->indptr[i];
                end = tap_comm->global_par_comm->recv_data->indptr[i+1];
                for (int j = start; j < end; j++)
                {
                    new_idx = global_recv_col_to_new[j];
                    if (new_idx != -1)
                    {
                        comm_proc = true;
                        global_par_comm->recv_data->indices.push_back(new_idx);
                    }
                }
                if (comm_proc)
                {
                    proc = tap_comm->global_par_comm->recv_data->procs[i];
                    global_par_comm->recv_data->procs.push_back(proc);
                    global_par_comm->recv_data->indptr.push_back(
                            global_par_comm->recv_data->indices.size());
                }
            }
            global_par_comm->recv_data->num_msgs = global_par_comm->recv_data->procs.size();
            global_par_comm->recv_data->size_msgs = global_par_comm->recv_data->indices.size();
            global_par_comm->recv_data->finalize();

            
            int recv_size = local_R_par_comm->recv_data->size_msgs +
                local_L_par_comm->recv_data->size_msgs;
            if (recv_size)
            {
                recv_buffer.resize(recv_size);
                int_recv_buffer.resize(recv_size);
                orig_to_R.resize(recv_size, -1);
                orig_to_L.resize(recv_size, -1);

                ctr = 0;
                for (std::vector<int>::iterator it = R_to_orig.begin();
                        it != R_to_orig.end(); ++it)
                {
                    orig_to_R[*it] = ctr++;
                }

                ctr = 0;
                for (std::vector<int>::iterator it = L_to_orig.begin();
                        it != L_to_orig.end(); ++it)
                {
                    orig_to_L[*it] = ctr++;
                }
            }
        }

        /**************************************************************
        *****   ParComm Class Destructor
        **************************************************************
        ***** 
        **************************************************************/
        ~TAPComm()
        {
            delete global_par_comm;
            delete local_S_par_comm;
            delete local_R_par_comm;
            delete local_L_par_comm;
        }

        void init_tap_comm(Partition* partition,
                const std::vector<int>& off_proc_column_map,
                MPI_Comm comm)
        {
            // Get MPI Information
            int rank, num_procs;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &num_procs);

            // Initialize class variables
            local_S_par_comm = new ParComm(partition, 2345);
            local_R_par_comm = new ParComm(partition, 3456);
            local_L_par_comm = new ParComm(partition, 4567);
            global_par_comm = new ParComm(partition, 5678);

            // Initialize Variables
            int idx;
            int recv_size;
            std::vector<int> off_proc_col_to_proc;
            std::vector<int> on_node_column_map;
            std::vector<int> on_node_col_to_proc;
            std::vector<int> off_node_column_map;
            std::vector<int> off_node_col_to_node;
            std::vector<int> on_node_to_off_proc;
            std::vector<int> off_node_to_off_proc;
            std::vector<int> recv_nodes;
            std::vector<int> orig_procs;
            std::vector<int> node_to_local_proc;

            // Find process on which vector value associated with each column is
            // stored
            partition->form_col_to_proc(off_proc_column_map, off_proc_col_to_proc);

            // Partition off_proc cols into on_node and off_node
            split_off_proc_cols(off_proc_column_map, off_proc_col_to_proc,
                   on_node_column_map, on_node_col_to_proc, on_node_to_off_proc,
                   off_node_column_map, off_node_col_to_node, off_node_to_off_proc);

            // Gather all nodes with which any local process must communication
            form_local_R_par_comm(off_node_column_map, off_node_col_to_node, 
                    orig_procs);

            // Find global processes with which rank communications
            form_global_par_comm(orig_procs);

            // Form local_S_par_comm: initial distribution of values among local
            // processes, before inter-node communication
            form_local_S_par_comm(orig_procs);

            // Adjust send indices (currently global vector indices) to be index 
            // of global vector value from previous recv
            adjust_send_indices(partition->first_local_col);

            // Form local_L_par_comm: fully local communication (origin and
            // destination processes both local to node)
            form_local_L_par_comm(on_node_column_map, on_node_col_to_proc,
                    partition->first_local_col);

            // Determine size of final recvs (should be equal to 
            // number of off_proc cols)
            recv_size = local_R_par_comm->recv_data->size_msgs +
                local_L_par_comm->recv_data->size_msgs;
            if (recv_size)
            {
                // Want a single recv buffer local_R and local_L par_comms
                recv_buffer.resize(recv_size);
                int_recv_buffer.resize(recv_size);
                orig_to_R.resize(recv_size, -1);
                orig_to_L.resize(recv_size, -1);

                // Map local_R recvs to original off_proc_column_map
                if (local_R_par_comm->recv_data->size_msgs)
                {
                    R_to_orig.resize(local_R_par_comm->recv_data->size_msgs);
                    for (int i = 0; i < local_R_par_comm->recv_data->size_msgs; i++)
                    {
                        idx = local_R_par_comm->recv_data->indices[i];
                        int orig_i = off_node_to_off_proc[idx];
                        R_to_orig[i] = orig_i;
                        orig_to_R[orig_i] = i;
                    }
                }


                // Map local_L recvs to original off_proc_column_map
                if (local_L_par_comm->recv_data->size_msgs)
                {
                    L_to_orig.resize(local_L_par_comm->recv_data->size_msgs);
                    for (int i = 0; i < local_L_par_comm->recv_data->size_msgs; i++)
                    {
                        idx = local_L_par_comm->recv_data->indices[i];
                        int orig_i = on_node_to_off_proc[idx];
                        L_to_orig[i] = orig_i;
                        orig_to_L[orig_i] = i;
                    }
                }
            }
        }

        // Helper methods for forming TAPComm:
        void split_off_proc_cols(const std::vector<int>& off_proc_column_map,
                const std::vector<int>& off_proc_col_to_proc,
                std::vector<int>& on_node_column_map,
                std::vector<int>& on_node_col_to_proc,
                std::vector<int>& on_node_to_off_proc,
                std::vector<int>& off_node_column_map,
                std::vector<int>& off_node_col_to_node,
                std::vector<int>& off_node_to_off_proc);
        void form_local_R_par_comm(const std::vector<int>& off_node_column_map,
                const std::vector<int>& off_node_col_to_node,
                std::vector<int>& orig_procs);
        void form_global_par_comm(std::vector<int>& orig_procs);
        void form_local_S_par_comm(std::vector<int>& orig_procs);
        void adjust_send_indices(const int first_local_col);
        void form_local_L_par_comm(const std::vector<int>& on_node_column_map,
                const std::vector<int>& on_node_col_to_proc,
                const int first_local_col);

        // Class Methods
        std::vector<double>& communicate(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<double>& communicate(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<int>& communicate(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<int>& communicate(const int* values, MPI_Comm comm = MPI_COMM_WORLD);

        void communicate_T(const std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const std::vector<int>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);
        void communicate_T(const int* values, MPI_Comm comm = MPI_COMM_WORLD);

        void init_comm(const std::vector<data_t>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const std::vector<int>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm(const int* values, MPI_Comm comm = MPI_COMM_WORLD);
        std::vector<double>& complete_comm();
        std::vector<int>& complete_int_comm();

        void init_comm_T(const std::vector<data_t>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const std::vector<int>& values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const data_t* values, MPI_Comm comm = MPI_COMM_WORLD);
        void init_comm_T(const int* values, MPI_Comm comm = MPI_COMM_WORLD);
        void complete_comm_T();
        void complete_int_comm_T();

        CSRMatrix* communicate(std::vector<int>& rowptr, std::vector<int>& col_indices,
                std::vector<double>& values, MPI_Comm comm = MPI_COMM_WORLD);
        std::pair<CSRMatrix*, CSRMatrix*> communicate_T(std::vector<int>& rowptr, 
                std::vector<int>& col_indices, std::vector<double>& values, 
                MPI_Comm comm = MPI_COMM_WORLD);

        std::vector<double>& get_recv_buffer()
        {
            return recv_buffer;
        }
        std::vector<int>& get_int_recv_buffer()
        {
            return int_recv_buffer;
        }

        std::vector<double>& communicate(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD)
        {
            return CommPkg::communicate(v, comm);
        }
        CSRMatrix* communicate(ParCSRMatrix* A, MPI_Comm comm = MPI_COMM_WORLD)
        {
            return CommPkg::communicate(A, comm);
        }
        void init_comm(ParVector& v, MPI_Comm comm = MPI_COMM_WORLD)
        {
            CommPkg::init_comm(v, comm);
        }



        // Class Attributes
        ParComm* local_S_par_comm;
        ParComm* local_R_par_comm;
        ParComm* local_L_par_comm;
        ParComm* global_par_comm;
        std::vector<double> recv_buffer;
        std::vector<int> int_recv_buffer;
        std::vector<int> L_to_orig;
        std::vector<int> R_to_orig;
        std::vector<int> orig_to_L;
        std::vector<int> orig_to_R;
        std::vector<int> custom_rank_order;
    };
}
#endif
