#ifndef RAPTOR_KRYLOV_PAR_CG_HPP
#define RAPTOR_KRYLOV_PAR_CG_HPP

#include "core/types.hpp"
#include "core/par_matrix.hpp"
#include "core/par_vector.hpp"
#include "multilevel/par_multilevel.hpp"
#include <vector>

using namespace raptor;

void CG(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, 
        double tol = 1e-05, int max_iter = -1);
void PCG(ParCSRMatrix* A, ParMultilevel* ml, ParVector& x, ParVector& b, 
        aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);
void SRECG(ParCSRMatrix* A, ParVector& x, ParVector& b, int t, aligned_vector<double>& res,
        double tol = 1e-05, int max_iter = -1);
void PSRECG(ParCSRMatrix* A, ParMultilevel* ml_single, ParMultilevel* ml, ParVector& x, ParVector& b, int t, aligned_vector<double>& res,
        double tol = 1e-05, int max_iter = -1);

void CG(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& times, 
        aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);
void SRECG(ParCSRMatrix* A, ParVector& x, ParVector& b, int t, aligned_vector<double>& times,
        aligned_vector<double>& res, double tol = 1e-05, int max_iter = -1);
#endif
