// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "assert.h"
#include "core/types.hpp"
#include "interpolation.hpp"

using namespace raptor;

CSRMatrix* mod_classical_interpolation(CSRMatrix* A,
        CSRMatrix* S, const std::vector<int>& states)
{
    int startA, endA;
    int startS, endS;
    int start_k, end_k, col_k;
    int col, ctr, row;

    double weight;
    double weak_sum;
    double strong_sum;
    double diag;
    double val, val_k;
    double sign;
    double coarse_sum;
    std::vector<int> pos;
    std::vector<int> row_coarse;
    std::vector<double> row_strong;
    if (A->n_rows)
    {
        pos.resize(A->n_rows, -1);
        row_coarse.resize(A->n_rows, 0);
        row_strong.resize(A->n_rows, 0);
    }

    // Copy values of A into S

    A->sort();
    A->move_diag();
    S->sort();
    S->move_diag();
    
    std::vector<int> col_to_new;
    if (A->n_cols)
    {
        col_to_new.resize(A->n_cols, -1);
    }

    ctr = 0;
    for (int i = 0; i < A->n_cols; i++)
    {
        if (states[i])
        {
            col_to_new[i] = ctr++;
        }
    }

    // Form P
    CSRMatrix* P = new CSRMatrix(A->n_rows, ctr, A->nnz);

    // Main loop.. add entries to P
    P->idx1[0] = 0;
    for (int i = 0; i < A->n_rows; i++)
    {
        if (states[i] == 1)
        {
            P->idx2.push_back(col_to_new[i]);
            P->vals.push_back(1.0);
            P->idx1[i+1] = P->idx2.size();
            continue;
        }

        startA = A->idx1[i]+1;
        endA = A->idx1[i+1];
        startS = S->idx1[i]+1;
        endS = S->idx1[i+1];

        // Skip over diagonal values
        diag = A->vals[startA-1];
        weak_sum = diag;

        if (diag < 0)
        {
            sign = -1.0;
        }
        else
        {
            sign = 1.0;
        }

        // Find weak sum, and save coarse cols / strong col values
        ctr = startS;
        for (int j = startA; j < endA; j++)
        {
            col = A->idx2[j];
            val = A->vals[j];
            if (ctr < endS && S->idx2[ctr] == col) // Strong
            {
                if (states[col] == 1)
                {
                    pos[col] = P->idx2.size();
                    P->idx2.push_back(col_to_new[col]);
                    P->vals.push_back(val);
                }
                
                if (states[col] != -3)
                {
                    row_coarse[col] = states[col];
                    row_strong[col] = (1 - states[col]) * val;
                }
                ctr++;
            }
            else // Weak
            {
                weak_sum += val;
            }
        }

        // Find row coarse sums 
        ctr = startS;
        for (int j = startA; j < endA; j++)
        {
            col = A->idx2[j];
            if (ctr < endS && S->idx2[ctr] == col)
            {
                if (states[col] == 0)
                {
                    coarse_sum = 0;
                    start_k = A->idx1[col] + 1;
                    end_k = A->idx1[col+1];
                    for (int k = start_k; k < end_k; k++)
                    {
                        col_k = A->idx2[k];
                        val = A->vals[k] * row_coarse[col_k];
                        if (val * sign < 0)
                        {
                            coarse_sum += val;
                        }
                    }
                    if (fabs(coarse_sum) < zero_tol)
                    {
                        weak_sum += A->vals[j];
                        row_strong[col] = 0;
                    }
                    else
                    {
                        row_strong[col] /= coarse_sum;
                    }
                }
                ctr++;
            }
        }

        int idx;
        for (int j = startS; j < endS; j++)
        {
            col = S->idx2[j];
            if (states[col] == 0)
            {
                start_k = A->idx1[col]+1;
                end_k = A->idx1[col+1];
                for (int k = start_k; k < end_k; k++)
                {
                    col_k = A->idx2[k];
                    val = A->vals[k];
                    idx = pos[col_k];
                    if (val * sign < 0 && idx >= 0)
                    {
                        P->vals[idx] += (row_strong[col] * val);
                    }
                }
            }
        }

        for (int j = startS; j < endS; j++)
        {
            col = S->idx2[j];
            idx = pos[col];
            if (states[col] == 1)
            {
                P->vals[idx] /= -weak_sum;
            }
        }

        for (int j = startS; j < endS; j++)
        {
            col = S->idx2[j];
            pos[col] = -1;
            row_coarse[col] = 0;
            row_strong[col] = 0;
        }

        P->idx1[i+1] = P->idx2.size();
    }
    P->nnz = P->idx2.size();

    return P;
}

CSRMatrix* direct_interpolation(CSRMatrix* A,
        CSRMatrix* S, const std::vector<int>& states)
{
    int start, end, col;
    int idx, new_idx, ctr;
    double sum_strong_pos, sum_strong_neg;
    double sum_all_pos, sum_all_neg;
    double val, alpha, beta, diag;
    double neg_coeff, pos_coeff;

    A->sort();
    S->sort();
    A->move_diag();
    S->move_diag();

    // Copy entries of A into sparsity pattern of S
    std::vector<double> sa;
    if (S->nnz)
    {
        sa.resize(S->nnz);
    }
    for (int i = 0; i < A->n_rows; i++)
    {
        start = S->idx1[i];
        end = S->idx1[i+1];
        ctr = A->idx1[i];
        for (int j = start; j < end; j++)
        {
            col = S->idx2[j];
            while (A->idx2[ctr] != col)
            {
                ctr++;
            }
            sa[j] = A->vals[ctr];
        }
    }

    std::vector<int> col_to_new;
    if (A->n_cols)
    {
        col_to_new.resize(A->n_cols, -1);
    }

    ctr = 0;
    for (int i = 0; i < A->n_cols; i++)
    {
        if (states[i])
        {
            col_to_new[i] = ctr++;
        }
    }

    CSRMatrix* P = new CSRMatrix(A->n_rows, ctr, A->nnz);

    P->idx1[0] = 0;
    for (int i = 0; i < A->n_rows; i++)
    {
        if (states[i] == 1)
        {
            P->idx2.push_back(col_to_new[i]);
            P->vals.push_back(1);
        }
        else
        {
            sum_strong_pos = 0;
            sum_strong_neg = 0;
            sum_all_pos = 0;
            sum_all_neg = 0;

            start = S->idx1[i];
            end = S->idx1[i+1];
            if (S->idx2[start] == i)
            {
                start++;
            }
            for (int j = start; j < end; j++)
            {
                col = S->idx2[j];
                if (states[col] == 1)
                {
                    val = sa[j];
                    if (val < 0)
                    {
                        sum_strong_neg += val;
                    }
                    else
                    {
                        sum_strong_pos += val;
                    }
                }
            }

            start = A->idx1[i];
            end = A->idx1[i+1];
            diag = A->vals[start];
            for (int j = start+1; j < end; j++)
            {
                val = A->vals[j];
                if (val < 0)
                {
                    sum_all_neg += val;
                }
                else
                {
                    sum_all_pos += val;
                }
            }

            alpha = sum_all_neg / sum_strong_neg;

            if (sum_strong_pos == 0)
            {
                diag += sum_all_pos;
                beta = 0;
            }
            else
            {
                beta = sum_all_pos / sum_strong_pos;
            }

            neg_coeff = -alpha / diag;
            pos_coeff = -beta / diag;

            start = S->idx1[i];
            end = S->idx1[i+1];
            if (S->idx2[start] == i)
            {
                start++;
            }
            for (int j = start; j < end; j++)
            {
                col = S->idx2[j];
                if (states[col] == 1)
                {
                    val = sa[j];
                    P->idx2.push_back(col_to_new[col]);
                    if (val < 0)
                    {
                        P->vals.push_back(neg_coeff * val);
                    }
                    else
                    {
                        P->vals.push_back(pos_coeff * val);
                    }
                }
            }
        }
        P->idx1[i+1] = P->idx2.size();
    }
    P->nnz = P->idx2.size();

    return P;



}
