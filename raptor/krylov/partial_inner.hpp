#ifndef RAPTOR_KRYLOV_PAR_INNER_HPP
#define RAPTOR_KRYLOV_PAR_INNER_HPP

#include "core/types.hpp"
#include "core/par_vector.hpp"
#include <vector>

using namespace raptor;

data_t half_inner_contig(ParVector &x, ParVector &y, int half, int part_global);
//data_t half_inner_striped(ParVector &x, ParVector &y, int half, int part_global);
void create_partial_inner_comm(MPI_Comm &inner_comm, ParVector &x, int &my_color, int &first_root, int &second_root,
                               int &part_global, int contig);
data_t half_inner(MPI_Comm &inner_comm, ParVector &x, ParVector &y, int &my_color, int send_color, int &inner_root, 
                  int &recv_root, int part_global);

data_t sequential_inner(ParVector &x, ParVector &y);
data_t sequential_norm(ParVector &x, index_t p);

#endif
