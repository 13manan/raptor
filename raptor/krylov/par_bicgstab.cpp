// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "krylov/par_bicgstab.hpp"

using namespace raptor;

void BiCGStab(ParCSRMatrix* A, ParVector& x, ParVector& b, aligned_vector<double>& res, double tol, int max_iter)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    ParVector r;
    ParVector r_star;
    ParVector s;
    ParVector p;
    ParVector Ap;
    ParVector As;

    int iter;
    data_t alpha, beta, omega;
    data_t rr_inner, next_inner, Apr_inner, As_inner, AsAs_inner;
    double norm_r;

    // Same max iterations definition as pyAMG
    if (max_iter <= 0)
    {
        max_iter = ((int)(1.3*b.global_n)) + 2;
    }

    // Fixed Constructors
    r.resize(b.global_n, b.local_n, b.first_local);
    r_star.resize(b.global_n, b.local_n, b.first_local);
    p.resize(b.global_n, b.local_n, b.first_local);
    Ap.resize(b.global_n, b.local_n, b.first_local);
    As.resize(b.global_n, b.local_n, b.first_local);

    // r0 = b - A * x0
    A->residual(x, b, r);

    // r* = r0
    r_star.copy(r);

    // p0 = r0
    p.copy(r);

    rr_inner = r.inner_product(r_star);
    norm_r = r.norm(2);
    res.push_back(norm_r);

    if (norm_r != 0.0)
    {
        tol = tol * norm_r;
    }

    // Main BiCGStab Loop
    while (norm_r > tol && iter < max_iter)
    {
        // alpha_i = (r_i, r*) / (Ap_i, r*)
        A->mult(p, Ap);
        Apr_inner = Ap.inner_product(r_star);
        alpha = rr_inner / Apr_inner;

	// s_i = r_i - alpha_i * Ap_i
	s.copy(r);
	s.axpy(Ap, -1.0*alpha);

        // omega_i = (As_i, s_i) / (As_i, As_i)
	A->mult(s, As);
	As_inner = As.inner_product(s);
	AsAs_inner = As.inner_product(As);
	omega = As_inner / AsAs_inner;

	// x_{i+1} = x_i + alpha_i * p_i + omega_i * s_i
	x.axpy(p, alpha);
	x.axpy(s, omega);

	// r_{i+1} = s_i - omega_i * As_i
	r.copy(s);
	r.axpy(As, -1.0*omega);

	// beta_i = (r_{i+1}, r_star) / (r_i, r_star) * alpha_i / omega_i
	next_inner = r.inner_product(r_star);
	beta = (next_inner / rr_inner) * (alpha / omega);

	// p_{i+1} = r_{i+1} + beta_i * (p_i - omega_i * Ap_i)
	p.scale(beta);
	p.axpy(r, 1.0);
	p.axpy(Ap, -1.0*beta*omega);

        // Update next inner product
        rr_inner = next_inner;
        norm_r = r.norm(2);
	res.push_back(norm_r);

        iter++;
    }

    if (rank == 0)
    {
        if (iter == max_iter)
        {
            printf("Max Iterations Reached.\n");
            printf("2 Norm of Residual: %lg\n\n", norm_r);
        }
        else
        {
            printf("%d Iteration required to converge\n", iter);
            printf("2 Norm of Residual: %lg\n\n", norm_r);
        }
    }

    return;
}



int PBiCGStab(ParCSRMatrix* A, ParMultilevel* ml, ParVector& x, 
            ParVector& b, aligned_vector<double>& res, double tol, int max_iter, int precond_iter)
{
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    ParVector r;
    ParVector r_star;
    ParVector s;
    ParVector p;
    ParVector Mp;
    ParVector Ms;
    ParVector Ap;
    ParVector As;
  
    int iter;
    data_t alpha, beta, omega;
    data_t rr_inner, next_inner, Apr_inner, As_inner, AsAs_inner;
    double norm_r;

    // Same max iterations definition as pyAMG
    if (max_iter <= 0)
    {
        max_iter = ((int)(1.3*b.global_n)) + 2;
    }

    // Fixed Constructors
    r.resize(b.global_n, b.local_n, b.first_local);
    r_star.resize(b.global_n, b.local_n, b.first_local);
    p.resize(b.global_n, b.local_n, b.first_local);
    Ap.resize(b.global_n, b.local_n, b.first_local);
    As.resize(b.global_n, b.local_n, b.first_local);
    Mp.resize(b.global_n, b.local_n, b.first_local);
    Ms.resize(b.global_n, b.local_n, b.first_local);

    // r0 = b - A * x0
    A->residual(x, b, r);

    // r* = r0
    r_star.copy(r);

    // p0 = r0
    p.copy(r);

    rr_inner = r.inner_product(r_star);
    norm_r = r.norm(2);
    res.push_back(norm_r);

    if (norm_r != 0.0)
    {
        tol = tol * norm_r;
    }

    // Main BiCGStab Loop
    while (norm_r > tol && iter < max_iter)
    {
        // alpha_i = (r_i, r*) / (Ap_i, r*)
        Mp.set_const_value(0.0);
        for (int i = 0; i < precond_iter; i++)
            ml->cycle(Mp, p);
        A->mult(Mp, Ap);
        Apr_inner = Ap.inner_product(r_star);
        alpha = rr_inner / Apr_inner;

	// s_i = r_i - alpha_i * Ap_i
	s.copy(r);
	s.axpy(Ap, -1.0*alpha);

        // omega_i = (As_i, s_i) / (As_i, As_i)
        Ms.set_const_value(0.0);
        for (int i = 0; i < precond_iter; i++)
            ml->cycle(Ms, s);
	A->mult(Ms, As);
	As_inner = As.inner_product(s);
	AsAs_inner = As.inner_product(As);
	omega = As_inner / AsAs_inner;

	// x_{i+1} = x_i + alpha_i * p_i + omega_i * s_i
	x.axpy(Mp, alpha);
	x.axpy(Ms, omega);

	// r_{i+1} = s_i - omega_i * As_i
	r.copy(s);
	r.axpy(As, -1.0*omega);

	// beta_i = (r_{i+1}, r_star) / (r_i, r_star) * alpha_i / omega_i
	next_inner = r.inner_product(r_star);
	beta = (next_inner / rr_inner) * (alpha / omega);

	// p_{i+1} = r_{i+1} + beta_i * (p_i - omega_i * Ap_i)
	p.scale(beta);
	p.axpy(r, 1.0);
	p.axpy(Ap, -1.0*beta*omega);

        // Update next inner product
        rr_inner = next_inner;
        norm_r = r.norm(2);
	res.push_back(norm_r);

        iter++;
    }

    return iter;
}


