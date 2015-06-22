// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#include "ParMatrix.hpp"

ParMatrix::ParMatrix(int _globalRows, int _globalCols, Matrix* _diag, Matrix* _offd)
{
    this->globalRows = _globalRows;
    this->globalCols = _globalCols;
    this->diag = _diag;
    this->offd = _offd;
}

ParMatrix::ParMatrix(ParMatrix* A)
{
    this->globalRows = A->globalRows;
    this->globalCols = A->globalCols;
    this->diag = A->diag; // should we mark as not owning?
    this->offd = A->offd;
}

ParMatrix::~ParMatrix()
{
    delete this->diag;
    delete this->offd;
}
