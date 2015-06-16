#ifndef DIFFUSION_HPP
#define DIFFUSION_HPP


#include <mpi.h>
#include <math.h>
#include <Eigen/Dense>
using Eigen::VectorXd;

#include "ParMatrix.hpp"

double* diffusion_stencil_2d(double eps = 1.0, double theta = 0.0)
{
    double* stencil = (double*) malloc (sizeof(double) * 9);

    double C = cos(theta);
    double S = sin(theta);
    double CS = C*S;
    double CC = C*C;
    double SS = S*S;
   
    double val1 =  ((-1*eps - 1)*CC + (-1*eps - 1)*SS + ( 3*eps - 3)*CS) / 6.0;
    double val2 =  (( 2*eps - 4)*CC + (-4*eps + 2)*SS) / 6.0;
    double val3 =  ((-1*eps - 1)*CC + (-1*eps - 1)*SS + (-3*eps + 3)*CS) / 6.0;
    double val4 =  ((-4*eps + 2)*CC + ( 2*eps - 4)*SS) / 6.0;
    double val5 =  (( 8*eps + 8)*CC + ( 8*eps + 8)*SS) / 6.0;

    stencil[0] = val1;
    stencil[1] = val2;
    stencil[2] = val3;
    stencil[4] = val4;
    stencil[5] = val5;
    stencil[6] = val4;
    stencil[7] = val3;
    stencil[8] = val2;
    stencil[9] = val1;

    return stencil;
}

#endif

