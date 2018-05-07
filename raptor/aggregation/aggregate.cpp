// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "aggregation/aggregate.hpp"

int aggregate(CSRMatrix* A, CSRMatrix* S, aligned_vector<int>& states,
        aligned_vector<int>& aggregates)
{
    if (A->n_rows == 0)
    {
        return 0;
    }

    // Initialize Variables
    aggregates.resize(S->n_rows);
    for (int i = 0; i < S->n_rows; i++)
    {
        aggregates[i] = -1;
    }
    int n_aggs = 0;
    int start, end, col;
    int ctr, max_agg;
    double max_val, val;

    // Label aggregates as 0 - n_aggs
    for (int i = 0; i < S->n_rows; i++)
    {
        if (states[i] > 0)
            aggregates[i] = n_aggs++;
    }

    // Pass 1 : add each row to neighboring aggregate (if exists)
    for (int i = 0; i < S->n_rows; i++)
    {
        if (states[i] > 0) continue;

        start = S->idx1[i];
        end = S->idx1[i+1];
        for (int j = start; j < end; j++)
        {
            col = S->idx2[j];
            if (states[col] > 0)
            {
                aggregates[i] = aggregates[col];
                break;
            }
        }
    }

    // Pass 2 : add remaining aggregate to that of strongest neighbor
    // TODO - How should this be determined? Currently Sij with largest Aij
    //      - Could be largest number of Sij entries with same agg?
    //      - Or, largest neighboring aggregate?
    for (int i = 0; i < S->n_rows; i++)
    {
        if (aggregates[i] >= 0) continue;

        start = S->idx1[i];
        end = S->idx1[i+1];
        ctr = A->idx1[i];
        max_val = 0.0;
        max_agg = -1; 
        for (int j = start; j < end; j++)
        {
            col = S->idx2[j];
            while (A->idx2[ctr] != col)
                ctr++;

            val = fabs(A->vals[ctr]);
            if (val > max_val && aggregates[col] >= 0)
            {
                max_val = val;
                max_agg = aggregates[col];
            }
        }
        aggregates[i] = max_agg;
    } 

    return n_aggs;
}
