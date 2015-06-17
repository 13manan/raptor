#ifndef RAPTOR_CORE_PARVECTOR_HPP
#define RAPTOR_CORE_PARVECTOR_HPP

#include <mpi.h>
#include <math.h>

#include "Types.hpp"
#include "Vector.hpp"

namespace raptor
{

class ParVector
{
public:
     ParVector(len_t N, len_t n);
     ParVector(ParVector&& x);
	~ParVector() {};

     template<int p> data_t norm() const
		{
			data_t result = local.lpNorm<p>();

			result = pow(result, p); // undoing root of p from local operation
			MPI_Allreduce(MPI_IN_PLACE, &result, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
			return pow(result, 1./p);
		}

     Vector & getLocalVector()
          { return local; }
     const Vector & getLocalVector() const
          { return local; }
     void axpy(const ParVector & x, data_t alpha)
          { local += x.getLocalVector() * alpha; }
     void scale(data_t alpha)
          { local *= alpha; }
     void setConstValue(data_t alpha)
          { local = Vector::Constant(localN, alpha); }

protected:
     Vector local;
     int globalN;
     int localN;
};

}
#endif
