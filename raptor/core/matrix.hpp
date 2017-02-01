// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef RAPTOR_CORE_MATRIX_HPP
#define RAPTOR_CORE_MATRIX_HPP

#include "vector.hpp"
#include <map>
#include <numeric>
#include <algorithm>
#include <functional>
#include <set>
#include <map>

/**************************************************************
 *****   Matrix Base Class
 **************************************************************
 ***** This class constructs a sparse matrix, supporting simple linear
 ***** algebra operations.
 *****
 ***** Attributes
 ***** -------------
 ***** n_rows : int
 *****    Number of rows
 ***** n_cols : int
 *****    Number of columns
 ***** nnz : int
 *****    Number of nonzeros
 ***** idx1 : std::vector<int>
 *****    List of position indices, specific to type of matrix
 ***** idx2 : std::vector<int>
 *****    List of position indices, specific to type of matrix
 ***** vals : std::vector<double>
 *****    List of values in matrix
 ***** row_list : std::vector<int>
 *****    List of rows containing nonzeros.  Only initialized
 *****    for condensed matrices.
 ***** col_list : std::vector<int>
 *****    List of columns containing nonzeros.  Only initialized
 *****    for condensed matrices.
 *****
 ***** Methods
 ***** -------
 ***** print()
 *****    Prints the nonzeros in the sparse matrix, along with 
 *****    the row and column of the nonzero
 ***** resize(int n_rows, int n_cols)
 *****    Resizes dimension of matrix to passed parameters
 ***** mult(Vector* x, Vector* b)
 *****    Sparse matrix-vector multiplication b = A * x
 ***** residual(Vector* x, Vector* b, Vector* r)
 *****    Calculates the residual r = b - A * x
 *****
 ***** Virtual Methods
 ***** -------
 ***** format() 
 *****    Returns the format of the sparse matrix (COO, CSR, CSC)
 ***** sort()
 *****    Sorts the matrix by position.  Whether row-wise or 
 *****    column-wise depends on matrix format.
 ***** add_value(int row, int col, double val)
 *****     Adds val to position (row, col)
 *****     TODO -- make sure this is working for CSR/CSC
 ***** condense_rows()
 *****     Removes zeros rows from sparse matrix, decreasing the indices
 *****     of remaining rows as needed.  Initializes row_list to contain
 *****     the original rows of the matrix (row_list[i] = orig_row[i])
 ***** condense_cols()
 *****     Removes zeros cols from sparse matrix, decreasing the indices
 *****     of remaining cols as needed.  Initializes col_list to contain
 *****     the original cols of the matrix (col_list[i] = orig_col[i])
 ***** apply_func (std::function<void(int, int, double)> func_ptr)
 *****     Applys function passed as paramter to each position of matrix.
 *****     For example call to this function, see method print()
 ***** apply_func (double* x, double* b, std::function<void(int, int, double ...)>)
 *****     Applys function passed as parameter to each position of matrix,
 *****     where function depends on double* x and double* b.
 *****     For example call to this function, see mult(Vector* x, Vector* b)
 **************************************************************/
namespace raptor
{
  // Forward Declaration of classes so objects can be used
  class COOMatrix;
  class CSRMatrix;
  class CSCMatrix;

  class Matrix
  {

  public:

    /**************************************************************
    *****   Matrix Base Class Constructor
    **************************************************************
    ***** Sets matrix dimensions, and sets nnz to 0
    *****
    ***** Parameters
    ***** -------------
    ***** _nrows : int
    *****    Number of rows in matrix
    ***** _ncols : int
    *****    Number of cols in matrix
    **************************************************************/
    Matrix(int _nrows, int _ncols)
    {
        n_rows = _nrows;
        n_cols = _ncols;
        nnz = 0;
    }

    /**************************************************************
    *****   Matrix Base Class Constructor
    **************************************************************
    ***** Sets matrix dimensions and nnz based on Matrix* A
    *****
    ***** Parameters
    ***** -------------
    ***** A : Matrix*
    *****    Matrix to be copied
    **************************************************************/
    Matrix(const Matrix* A)
    {
        n_rows = A->n_rows;
        n_cols = A->n_cols;
        nnz = A->nnz;
    }

    virtual format_t format() = 0;
    virtual void sort() = 0;
    virtual void add_value(int row, int col, double val) = 0;
    virtual void condense_rows() = 0;
    virtual void condense_cols() = 0;
    virtual void apply_func( std::function<void(int, int, double)> func_ptr) = 0;
    virtual void apply_func( double* x, double* b, 
            std::function<void(int, int, double, double*, double*)> func_ptr) = 0;

    void print();
    void mult(Vector* x, Vector* b);
    void mult(double* x_data, double* b_data);
    void residual(Vector* x, Vector* b, Vector* r);
    void resize(int _n_rows, int _n_cols);

    std::vector<int> idx1;
    std::vector<int> idx2;
    std::vector<double> vals;

    // Lists of rows with nonzeros
    // Only initialized when matrix is condensed
    std::vector<int> row_list;
    std::vector<int> col_list;

    int n_rows;
    int n_cols;
    int nnz;
  };


/**************************************************************
 *****   COOMatrix Class (Inherits from Matrix Base Class)
 **************************************************************
 ***** This class constructs a sparse matrix in COO format.
 *****
 ***** Methods
 ***** -------
 ***** format() 
 *****    Returns the format of the sparse matrix (COO)
 ***** sort()
 *****    Sorts the matrix by row, and by column within each row.
 ***** add_value(int row, int col, double val)
 *****     Adds val to position (row, col)
 ***** condense_rows()
 *****     Removes zeros rows from sparse matrix, decreasing the indices
 *****     of remaining rows as needed.  Initializes row_list to contain
 *****     the original rows of the matrix (row_list[i] = orig_row[i])
 ***** condense_cols()
 *****     Removes zeros cols from sparse matrix, decreasing the indices
 *****     of remaining cols as needed.  Initializes col_list to contain
 *****     the original cols of the matrix (col_list[i] = orig_col[i])
 ***** apply_func (std::function<void(int, int, double)> func_ptr)
 *****     Applys function passed as paramter to each position of matrix.
 *****     For example call to this function, see method print()
 ***** apply_func (double* x, double* b, std::function<void(int, int, double ...)>)
 *****     Applys function passed as parameter to each position of matrix,
 *****     where function depends on double* x and double* b.
 *****     For example call to this function, see mult(Vector* x, Vector* b)
 ***** rows()
 *****     Returns std::vector<int>& containing the rows corresponding
 *****     to each nonzero
 ***** cols()
 *****     Returns std::vector<int>& containing the cols corresponding
 *****     to each nonzero
 ***** data()
 *****     Returns std::vector<double>& containing the nonzero values
 **************************************************************/
  class COOMatrix : public Matrix
  {

  public:

    /**************************************************************
    *****   COOMatrix Class Constructor
    **************************************************************
    ***** Initializes an empty COOMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** _nrows : int
    *****    Number of rows in Matrix
    ***** _ncols : int
    *****    Number of columns in Matrix
    ***** nnz_per_row : int
    *****    Prediction of (approximately) number of nonzeros 
    *****    per row, used in reserving space
    **************************************************************/
    COOMatrix(int _nrows, int _ncols, int nnz_per_row = 1): Matrix(_nrows, _ncols)
    {
        if (nnz_per_row)
        {
            int _nnz = nnz_per_row * _nrows;
            idx1.reserve(_nnz);
            idx2.reserve(_nnz);
            vals.reserve(_nnz);
        }
    }

    /**************************************************************
    *****   COOMatrix Class Constructor
    **************************************************************
    ***** Constructs a COOMatrix from a CSRMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSRMatrix*
    *****    CSRMatrix A, from which to copy data
    **************************************************************/
    explicit COOMatrix(const CSRMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;
        idx1.reserve(mat->nnz);
        idx2.reserve(mat->nnz);
        vals.reserve(mat->nnz);
        for (int i = 0; i < mat->n_rows; i++)
        {
            int row_start = mat->idx1[i];
            int row_end = mat->idx1[i+1];
            for (int j = row_start; j < row_end; j++)
            {
                idx1.push_back(i);
                idx2.push_back(mat->idx2[j]);
                vals.push_back(mat->vals[j]);
            }
        }
    }

    /**************************************************************
    *****   COOMatrix Class Constructor
    **************************************************************
    ***** Copies matrix, constructing new COOMatrix from 
    ***** another COOMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const COOMatrix*
    *****    COOMatrix A, from which to copy data
    **************************************************************/
    explicit COOMatrix(const COOMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;
        idx1.reserve(mat->nnz);
        idx2.reserve(mat->nnz);
        vals.reserve(mat->nnz);
        for (int i = 0; i < mat->nnz; i++)
        {
            idx1.push_back(mat->idx1[i]);
            idx2.push_back(mat->idx2[i]);
            vals.push_back(mat->vals[i]);
        }
    }

    /**************************************************************
    *****   COOMatrix Class Constructor
    **************************************************************
    ***** Constructs a COOMatrix from a CSCMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSCMatrix*
    *****    CSCMatrix A, from which to copy data
    **************************************************************/
    explicit COOMatrix(const CSCMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;
        idx1.reserve(mat->nnz);
        idx2.reserve(mat->nnz);
        vals.reserve(mat->nnz);
        for (int i = 0; i < mat->n_cols; i++)
        {
            int col_start = mat->idx1[i];
            int col_end = mat->idx1[i+1];
            for (int j = col_start; j < col_end; j++)
            {
                idx1.push_back(mat->idx2[j]);
                idx2.push_back(i);
                vals.push_back(mat->vals[j]);
            }
        }
    }

    void add_value(int row, int col, double value);
    void condense_rows();
    void condense_cols();
    void sort();
    void apply_func(std::function<void(int, int, double)> func_ptr);
    void apply_func(double* xd, double* bd, 
            std::function<void(int, int, double, double*, double*)> func_ptr);

    format_t format()
    {
        return COO;
    }

    std::vector<int>& rows()
    {
        return idx1;
    }

    std::vector<int>& cols()
    {
        return idx2;
    }

    std::vector<double>& data()
    {
        return vals;
    }

  };

/**************************************************************
 *****   CSRMatrix Class (Inherits from Matrix Base Class)
 **************************************************************
 ***** This class constructs a sparse matrix in CSR format.
 *****
 ***** Methods
 ***** -------
 ***** format() 
 *****    Returns the format of the sparse matrix (CSR)
 ***** sort()
 *****    Sorts the matrix.  Already in row-wise order, but sorts
 *****    the columns in each row.
 ***** add_value(int row, int col, double val)
 *****     TODO -- add this functionality
 ***** condense_rows()
 *****     Removes zeros rows from sparse matrix, decreasing the indices
 *****     of remaining rows as needed.  Initializes row_list to contain
 *****     the original rows of the matrix (row_list[i] = orig_row[i])
 ***** condense_cols()
 *****     Removes zeros cols from sparse matrix, decreasing the indices
 *****     of remaining cols as needed.  Initializes col_list to contain
 *****     the original cols of the matrix (col_list[i] = orig_col[i])
 ***** apply_func (std::function<void(int, int, double)> func_ptr)
 *****     Applys function passed as paramter to each position of matrix.
 *****     For example call to this function, see method print()
 ***** apply_func (double* x, double* b, std::function<void(int, int, double ...)>)
 *****     Applys function passed as parameter to each position of matrix,
 *****     where function depends on double* x and double* b.
 *****     For example call to this function, see mult(Vector* x, Vector* b)
 ***** indptr()
 *****     Returns std::vector<int>& row pointer.  The ith element points to
 *****     the index of indices() corresponding to the first column to lie on 
 *****     row i.
 ***** indices()
 *****     Returns std::vector<int>& containing the cols corresponding
 *****     to each nonzero
 ***** data()
 *****     Returns std::vector<double>& containing the nonzero values
 **************************************************************/
  class CSRMatrix : public Matrix
  {

  public:

    /**************************************************************
    *****   CSRMatrix Class Constructor
    **************************************************************
    ***** Initializes an empty CSRMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** _nrows : int
    *****    Number of rows in Matrix
    ***** _ncols : int
    *****    Number of columns in Matrix
    ***** nnz_per_row : int
    *****    Prediction of (approximately) number of nonzeros 
    *****    per row, used in reserving space
    **************************************************************/
    CSRMatrix(int _nrows, int _ncols, int _nnz): Matrix(_nrows, _ncols)
    {
        idx1.reserve(_nrows + 1);
        idx2.reserve(_nnz);
        vals.reserve(_nnz);
        nnz = _nnz;
    }

    /**************************************************************
    *****   CSRMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSRMatrix from a COOMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const COOMatrix*
    *****    COOMatrix A, from which to copy data
    **************************************************************/
    explicit CSRMatrix(COOMatrix* A) : Matrix( (Matrix*) A)
    {
        idx1.resize(n_rows + 1);
        idx2.resize(nnz);
        vals.resize(nnz);

        // Calculate indptr
        for (int i = 0; i < n_rows + 1; i++)
        {
            idx1[i] = 0;
        }
        for (int i = 0; i < A->nnz; i++)
        {
            int row = A->idx1[i];
            idx1[row+1]++;
        }
        for (int i = 0; i < A->n_rows; i++)
        {
            idx1[i+1] += idx1[i];
        }

        // Add indices and data
        int ctr[n_rows] = {0};
        for (int i = 0; i < A->nnz; i++)
        {
            int row = A->idx1[i];
            int col = A->idx2[i];
            double val = A->vals[i];
            int index = idx1[row] + ctr[row]++;
            idx2[index] = col;
            vals[index] = val;
        }
    }

    /**************************************************************
    *****   CSRMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSRMatrix from a CSCMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSCMatrix*
    *****    CSCMatrix A, from which to copy data
    **************************************************************/
    explicit CSRMatrix(CSCMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;

        // Resize vectors to appropriate dimensions
        idx1.resize(mat->n_rows);
        idx2.resize(mat->nnz);
        vals.resize(mat->nnz);

        // Create indptr, summing number times row appears in CSC
        for (int i = 0; i <= mat->n_rows; i++) idx1[i] = 0;
        for (int i = 0; i < mat->nnz; i++)
        {
            idx1[mat->idx2[i] + 1]++;
        }
        for (int i = 1; i <= mat->n_rows; i++)
        {
            idx1[i] += idx1[i-1];
        }

        // Add values to indices and data
        int ctr[n_rows] = {0};
        for (int i = 0; i < mat->n_cols; i++)
        {
            int col_start = mat->idx1[i];
            int col_end = mat->idx1[i+1];
            for (int j = col_start; j < col_end; j++)
            {
                int row = mat->idx2[j];
                int idx = idx1[row] + ctr[row]++;
                idx2[idx] = i;
                vals[idx] = mat->vals[j];
            }
        }
    }

    /**************************************************************
    *****   CSRMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSRMatrix from a CSRMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSRMatrix*
    *****    CSRMatrix A, from which to copy data
    **************************************************************/
    explicit CSRMatrix(const CSRMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;
        idx1.resize(mat->n_rows + 1);
        idx2.reserve(mat->nnz);
        vals.reserve(mat->nnz);
        idx1[0] = 0;
        for (int i = 0; i < mat->n_rows; i++)
        {
            idx1[i+1] = mat->idx1[i+1];
            int row_start = idx1[i];
            int row_end = idx1[i+1];
            for (int j = row_start; j < row_end; j++)
            {
                idx2[j] = mat->idx2[j];
                vals[j] = mat->vals[j];
            }
        }
    }

    void add_value(int row, int col, double value);
    void condense_rows();
    void condense_cols();
    void sort();
    void apply_func(std::function<void(int, int, double)> func_ptr);
    void apply_func(double* xd, double* bd, 
            std::function<void(int, int, double, double*, double*)> func_ptr);

    format_t format()
    {
        return CSR;
    }

    std::vector<int>& indptr()
    {
        return idx1;
    }

    std::vector<int>& indices()
    {
        return idx2;
    }

    std::vector<double> data()
    {
        return vals;
    }

  };

/**************************************************************
 *****   CSCMatrix Class (Inherits from Matrix Base Class)
 **************************************************************
 ***** This class constructs a sparse matrix in CSC format.
 *****
 ***** Methods
 ***** -------
 ***** format() 
 *****    Returns the format of the sparse matrix (CSC)
 ***** sort()
 *****    Sorts the matrix.  Already in col-wise order, but sorts
 *****    the rows in each column.
 ***** add_value(int row, int col, double val)
 *****     TODO -- add this functionality
 ***** condense_rows()
 *****     Removes zeros rows from sparse matrix, decreasing the indices
 *****     of remaining rows as needed.  Initializes row_list to contain
 *****     the original rows of the matrix (row_list[i] = orig_row[i])
 ***** condense_cols()
 *****     Removes zeros cols from sparse matrix, decreasing the indices
 *****     of remaining cols as needed.  Initializes col_list to contain
 *****     the original cols of the matrix (col_list[i] = orig_col[i])
 ***** apply_func (std::function<void(int, int, double)> func_ptr)
 *****     Applys function passed as paramter to each position of matrix.
 *****     For example call to this function, see method print()
 ***** apply_func (double* x, double* b, std::function<void(int, int, double ...)>)
 *****     Applys function passed as parameter to each position of matrix,
 *****     where function depends on double* x and double* b.
 *****     For example call to this function, see mult(Vector* x, Vector* b)
 ***** indptr()
 *****     Returns std::vector<int>& column pointer.  The ith element points to
 *****     the index of indices() corresponding to the first row to lie on 
 *****     column i.
 ***** indices()
 *****     Returns std::vector<int>& containing the rows corresponding
 *****     to each nonzero
 ***** data()
 *****     Returns std::vector<double>& containing the nonzero values
 **************************************************************/
  class CSCMatrix : public Matrix
  {

  public:

    CSCMatrix(int _nrows, int _ncols, int _nnz): Matrix(_nrows, _ncols)
    {
        idx1.reserve(_ncols + 1);
        idx2.reserve(_nnz);
        vals.reserve(_nnz);
        nnz = _nnz;
    }

    /**************************************************************
    *****   CSCMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSCMatrix from a COOMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const COOMatrix*
    *****    COOMatrix A, from which to copy data
    **************************************************************/
    explicit CSCMatrix(COOMatrix* A) : Matrix( (Matrix*) A)
    {
        idx1.resize(n_cols + 1);
        idx2.resize(nnz);
        vals.resize(nnz);

        // Calculate indptr
        for (int i = 0; i < n_cols + 1; i++)
        {
            idx1[i] = 0;
        }
        for (int i = 0; i < A->nnz; i++)
        {
            int col = A->idx2[i];
            idx1[col+1]++;
        }
        for (int i = 0; i < A->n_cols; i++)
        {
            idx1[i+1] += idx1[i];
        }

        // Add indices and data
        int ctr[n_cols] = {0};
        for (int i = 0; i < A->nnz; i++)
        {
            int row = A->idx1[i];
            int col = A->idx2[i];
            double val = A->vals[i];
            int index = idx1[col] + ctr[col]++;
            idx2[index] = row;
            vals[index] = val;
        }
    }

    /**************************************************************
    *****   CSCMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSCMatrix from a CSRMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSRMatrix*
    *****    CSRMatrix A, from which to copy data
    **************************************************************/
    explicit CSCMatrix(CSRMatrix* A) : Matrix( (Matrix*) A)
    {
        Matrix* mat = (Matrix*) A;

        // Resize vectors to appropriate dimensions
        idx1.resize(mat->n_cols);
        idx2.resize(mat->nnz);
        vals.resize(mat->nnz);

        // Create indptr, summing number times col appears in CSR
        for (int i = 0; i <= mat->n_cols; i++) idx1[i] = 0;
        for (int i = 0; i < mat->nnz; i++)
        {
            idx1[mat->idx2[i] + 1]++;
        }
        for (int i = 1; i <= mat->n_cols; i++)
        {
            idx1[i] += idx1[i-1];
        }

        // Add values to indices and data
        int ctr[mat->n_cols] = {0};
        for (int i = 0; i < mat->n_rows; i++)
        {
            int row_start = mat->idx1[i];
            int row_end = mat->idx1[i+1];
            for (int j = row_start; j < row_end; j++)
            {
                int col = mat->idx2[j];
                int idx = idx1[col] + ctr[col]++;
                idx2[idx] = i;
                vals[idx] = mat->vals[j];
            }
        }
    }

    /**************************************************************
    *****   CSCMatrix Class Constructor
    **************************************************************
    ***** Constructs a CSCMatrix from a CSCMatrix
    *****
    ***** Parameters
    ***** -------------
    ***** A : const CSCMatrix*
    *****    CSCMatrix A, from which to copy data
    **************************************************************/
    explicit CSCMatrix(const CSCMatrix* A) : Matrix( (Matrix*) A )
    {
        Matrix* mat = (Matrix*) A;
        idx1.resize(mat->n_cols + 1);
        idx2.reserve(mat->nnz);
        vals.reserve(mat->nnz);
        idx1[0] = 0;
        for (int i = 0; i < mat->n_cols; i++)
        {
            idx1[i+1] = mat->idx1[i+1];
            int col_start = idx1[i];
            int col_end = idx1[i+1];
            for (int j = col_start; j < col_end; j++)
            {
                idx2[j] = mat->idx2[j];
                vals[j] = mat->vals[j];
            }
        }
    }

    void sort();
    void add_value(int row, int col, double value);
    void condense_rows();
    void condense_cols();
    void apply_func(std::function<void(int, int, double)> func_ptr);
    void apply_func(double* xd, double* bd, 
            std::function<void(int, int, double, double*, double*)> func_ptr);

    format_t format()
    {
        return CSR;
    }

    std::vector<int>& indptr()
    {
        return idx1;
    }

    std::vector<int>& indices()
    {
        return idx2;
    }

    std::vector<double>& data()
    {
        return vals;
    }

  };


}

#endif

