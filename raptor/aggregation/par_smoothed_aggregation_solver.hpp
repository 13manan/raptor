// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef RAPTOR_PAR_SMOOTHED_AGGREGATION_SOLVER_HPP
#define RAPTOR_PAR_SMOOTHED_AGGREGATION_SOLVER_HPP

#include "multilevel/par_multilevel.hpp"
#include "aggregation/par_mis.hpp"
#include "aggregation/par_aggregate.hpp"
#include "aggregation/par_candidates.hpp"
#include "aggregation/par_prolongation.hpp"

namespace raptor
{
    class ParSmoothedAggregationSolver : public ParMultilevel
    {
      public:
        ParSmoothedAggregationSolver(double _strong_threshold = 0.0, 
                agg_t _agg_type = MIS, 
                prolong_t _prolong_type = JacobiProlongation,
                strength_t _strength_type = Symmetric,
                relax_t _relax_type = SOR,
                int _prolong_smooth_steps = 1, 
                double _prolong_weight = 4.0/3) 
            : ParMultilevel(_strong_threshold, _strength_type, _relax_type)
        {
            agg_type = _agg_type;
            prolong_type = _prolong_type;
            num_candidates = 1;
            interp_tol = 1e-10;
            prolong_smooth_steps = _prolong_smooth_steps;
            prolong_weight = _prolong_weight;
        }

        ~ParSmoothedAggregationSolver()
        {
        }

        void setup(ParCSRMatrix* Af) 
        {
            // TODO -- add option for B to be passed as variable
            num_candidates = 1;
            B.resize(Af->local_num_rows);
            for (int i = 0; i < Af->local_num_rows; i++)
            {
                B[i] = 1.0;
            }

            setup_helper(Af);
        }

        void extend_hierarchy()
        {
            int level_ctr = levels.size() - 1;

            ParCSRMatrix* A = levels[level_ctr]->A;
            ParCSRMatrix* S;
            ParCSRMatrix* T;
            ParCSRMatrix* P;
            ParCSRMatrix* AP;
            bool tap_level = A->comm_type != Standard && tap_amg <= level_ctr;

            aligned_vector<int> states;
            aligned_vector<int> off_proc_states;
            aligned_vector<int> aggregates;
            aligned_vector<double> R;
            int n_aggs;

            // Form strength of connection
            S = A->strength(strength_type, strong_threshold, tap_level, 
                    1, NULL);

            // Aggregate Nodes
            switch (agg_type)
            {
                case MIS:
                    mis2(S, states, off_proc_states, tap_level, weights);
                    n_aggs = aggregate(A, S, states, off_proc_states, 
                            aggregates, tap_level);
                    break;
            }

            // Form tentative interpolation
            T = fit_candidates(A, n_aggs, aggregates, B, R, 
                    num_candidates, false, interp_tol);
            
            if (prolong_smooth_steps)    
            {
                switch (prolong_type)
                {
                    case JacobiProlongation:
                        P = jacobi_prolongation(A, T, tap_level, 
                                prolong_weight, prolong_smooth_steps);
                        break;
                }
                delete T;
            }
            else
            {
                P = T;
            }
            levels[level_ctr]->P = P;

            // Form coarse grid operator
            levels.emplace_back(new ParLevel());

            AP = A->mult(levels[level_ctr]->P, tap_level);

            A = AP->mult_T(P, tap_level);

            level_ctr++;
            levels[level_ctr]->A = A;
            levels[level_ctr]->x.resize(A->global_num_rows, A->local_num_rows);
            levels[level_ctr]->b.resize(A->global_num_rows, A->local_num_rows);
            levels[level_ctr]->tmp.resize(A->global_num_rows, A->local_num_rows);
            levels[level_ctr]->P = NULL;

            if (tap_amg <= level_ctr)
            {
                A->init_communicators(levels[level_ctr-1]->A->comm->key,
                        levels[level_ctr-1]->A->comm->mpi_comm);
            }
            else
            {
                A->comm = new ParComm(A->partition, A->off_proc_column_map, 
                        A->on_proc_column_map, levels[level_ctr-1]->A->comm->key,
                        levels[level_ctr-1]->A->comm->mpi_comm);
            }

            std::copy(R.begin(), R.end(), B.begin());

            delete AP;
            delete S;
        }    


        agg_t agg_type;
        prolong_t prolong_type;
        aligned_vector<double> B;

        double interp_tol;
        double prolong_weight;
        int prolong_smooth_steps;
        int num_candidates;

    };
}
   

#endif




