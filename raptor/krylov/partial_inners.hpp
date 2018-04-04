#ifndef RAPTOR_KRYLOV_PAR_INNER_HPP
#define RAPTOR_KRYLOV_PAR_INNER_HPP

#include "core/types.hpp"
#include "core/par_vector.hpp"
#include <vector>

using namespace raptor;

void half_inner_contig(ParVector &x, ParVector &y, int half, int part_global);
void half_inner_striped(ParVector &x, ParVector &y, int half, int part_global);

#endif
