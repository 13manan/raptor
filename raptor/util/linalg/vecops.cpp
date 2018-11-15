// Copyright (c) 2015-2017, RAPtor Developer Team
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "core/vector.hpp"

using namespace raptor;

/**************************************************************
*****   Vector AXPY
**************************************************************
***** Multiplies the vector x by a constant, alpha, and then
***** sums each element with corresponding local entry 
*****
***** Parameters
***** -------------
***** x : Vector&
*****    Vector to be summed with
***** alpha : data_t
*****    Constant value to multiply each element of vector by
**************************************************************/
void Vector::axpy(Vector& x, data_t alpha)
{
    for (index_t i = 0; i < num_values*b_vecs; i++)
    {
        values[i] += x.values[i]*alpha;
    }
}

/**************************************************************
*****   Vector Scale
**************************************************************
***** Multiplies each element of the vector by a constant value
*****
***** Parameters
***** -------------
***** alpha : data_t
*****    Constant value to set multiply element of vector by
***** alphas : data_t*
*****    Constant values to multiply element of each vector
*****    in block vector by  
**************************************************************/
void Vector::scale(data_t alpha, data_t* alphas)
{
    if (alphas == NULL)
    {
        for (index_t i = 0; i < num_values; i++)
        {
            values[i] *= alpha;
        }
    }
    else
    {
        index_t offset;
        for (index_t j = 0; j < b_vecs; j++)
        {
            offset = j * num_values;
            for (index_t i = 0; i < num_values; i++)
            {
                values[i + offset] *= alphas[j];
            }
        }
    }
}

/**************************************************************
*****   Vector Norm
**************************************************************
***** Calculates the P norm of the vector (for a given P)
*****
***** Parameters
***** -------------
***** p : index_t
*****    Determines which p-norm to calculate
**************************************************************/
data_t Vector::norm(index_t p, data_t* norms)
{
    data_t result = 0.0;
    double val;

    if (norms == NULL)
    {    
        for (index_t i = 0; i < num_values; i++)
        {
            val = values[i];
            if (fabs(val) > zero_tol)
                result += pow(val, p);
        }
        return pow(result, 1.0/p);
    }
    else
    {
        index_t offset;
        for (index_t j = 0; j < b_vecs; j++)
        {
            result = 0.0;
            offset = j * num_values;
            for (index_t i = 0; i < num_values; i++)
            {
                val = values[i + offset];
                if (fabs(val) > zero_tol)
                    result += pow(val, p);
            }
            norms[j] = pow(result, 1.0/p);
        }
        return 0;
    }
}

/**************************************************************
*****   Vector Inner Product
**************************************************************
***** Calculates the inner product of the given vector with 
***** input vector
*****
***** Parameters
***** -------------
***** ADD 
**************************************************************/
data_t Vector::inner_product(Vector& x, data_t* inner_prods)
{
    data_t result = 0.0;

    if (inner_prods == NULL)
    {
        for (int i = 0; i < num_values; i++)
        {
            result += values[i] * x[i];
        }
        return result;
    }
    else
    {
        index_t offset;
        if (x.b_vecs == 1)
        {
            for (index_t j = 0; j < b_vecs; j++)
            {
                result = 0.0;
                offset = j * num_values;
                for (index_t i = 0; i < num_values; i++)
                {
                    result += values[i + offset] * x[i];
                }
                inner_prods[j] = result;
            }
        }
        else
        {
            for (index_t j = 0; j < b_vecs; j++)
            {
                result = 0.0;
                offset = j * num_values;
                for (index_t i = 0; i < num_values; i++)
                {
                    result += values[i + offset] * x[i + offset];
                }
                inner_prods[j] = result;
            }
        }
        return 0;
    }
}

/**************************************************************
*****   BVector AXPY
**************************************************************
***** Multiplies the vector x by a constant, alpha, and then
***** sums each element with corresponding local entry for
***** each vector in the BVector 
*****
***** Parameters
***** -------------
***** x : Vector&
*****    Vector to be summed with
***** alpha : data_t
*****    Constant value to multiply each element of vector by
**************************************************************/
void BVector::axpy(Vector& x, data_t alpha)
{
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++) {
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            values[i + offset] += x.values[i]*alpha;
        }
    }
}

/**************************************************************
*****   BVector AXPY
**************************************************************
***** Multiplies each vector in y by a constant, alpha, and then
***** sums each entry in each vector with corresponding local entry
***** and vector in the BVector 
*****
***** Parameters
***** -------------
***** y : BVector&
*****    BVector to be summed with
***** alpha : data_t
*****    Constant value to multiply each element of each vector by
**************************************************************/
void BVector::axpy(BVector& y, data_t alpha)
{
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++)
    {
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            values[i + offset] += y.values[i + offset]*alpha;
        }
    }
}

/**************************************************************
*****   BVector Scale
**************************************************************
***** Multiplies each element of the vector by a constant value
*****
***** Parameters
***** -------------
***** alpha : data_t
*****    Constant value to set multiply element of vector by
***** alphas : data_t*
*****    Constant values to multiply element of each vector
*****    in block vector by  
**************************************************************/
void BVector::scale(data_t alpha, data_t* alphas)
{
    if (alphas == NULL)
    {
        for (index_t i = 0; i < num_values; i++)
        {
            values[i] *= alpha;
        }
    }
    else
    {
        index_t offset;
        for (index_t j = 0; j < b_vecs; j++)
        {
            offset = j * num_values;
            for (index_t i = 0; i < num_values; i++)
            {
                values[i + offset] *= alphas[j];
            }
        }
    }
}

/**************************************************************
*****   BVector Norm
**************************************************************
***** Calculates the P norm of each vector (for a given P)
***** in the block vector
*****
***** Parameters
***** -------------
***** p : index_t
*****    Determines which p-norm to calculate
**************************************************************/
data_t BVector::norm(index_t p, data_t* norms)
{
    data_t result;
    index_t offset;
    double val;

    for (index_t j = 0; j < b_vecs; j++)
    {
        result = 0.0;
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            val = values[i + offset];
            if (fabs(val) > zero_tol)
                result += pow(val, p);
        }
        norms[j] = pow(result, 1.0/p);
    }

    return 0;
}

/**************************************************************
*****   BVector Inner Product 
**************************************************************
***** Calculates the inner product of every vector in the
***** block vector with the vector x
*****
***** Parameters
***** -------------
***** x : Vector&
*****    Vector to calculate inner product with
**************************************************************/
data_t BVector::inner_product(Vector& x, data_t* inner_prods)
{
    data_t result;
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++)
    {
        result = 0.0;
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            result += values[i + offset] * x[i];
        }
        inner_prods[j] = result;
    }

    return 0;
}

/**************************************************************
*****   BVector Inner Product 
**************************************************************
***** Calculates the inner product of every vector in the
***** block vector with each corresponding vector in y
*****
***** Parameters
***** -------------
***** y : BVector&
*****    BVector to calculate inner product with
**************************************************************/
data_t BVector::inner_product(BVector& y, data_t* inner_prods)
{
    data_t result;
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++)
    {
        result = 0.0;
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            result += values[i + offset] * y[i + offset];
        }
        inner_prods[j] = result;
    }

    return 0;
}

/**************************************************************
*****   BVector Mult 
**************************************************************
***** Multiplies the BVector by the Vector x as if the 
***** the BVector were a dense matrix
*****
***** Parameters
***** -------------
***** x : Vector&
*****    Vector to multiply with
***** b : Vector&
*****    Vector to hold result
**************************************************************/
void BVector::mult(Vector& x, Vector& b)
{
    b.set_const_value(0.0);
    data_t result;
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++)
    {
        result = 0.0;
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            b[i] += values[i + offset] * x[j];
        }
    }
}

/**************************************************************
*****   BVector Mult 
**************************************************************
***** Multiplies the BVector by the Vector x as if the 
***** the BVector were a dense matrix
*****
***** Parameters
***** -------------
***** x : Vector&
*****    Vector to multiply with
***** b : Vector&
*****    Vector to hold result
**************************************************************/
void Vector::mult(Vector& x, Vector& b)
{
    b.set_const_value(0.0);
    data_t result;
    index_t offset;

    for (index_t j = 0; j < b_vecs; j++)
    {
        result = 0.0;
        offset = j * num_values;
        for (index_t i = 0; i < num_values; i++)
        {
            b[i] += values[i + offset] * x[j];
        }
    }
}
