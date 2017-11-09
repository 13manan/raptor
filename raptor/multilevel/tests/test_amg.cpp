// EXPECT_EQ and ASSERT_EQ are macros
// EXPECT_EQ test execution and continues even if there is a failure
// ASSERT_EQ test execution and aborts if there is a failure
// The ASSERT_* variants abort the program execution if an assertion fails 
// while EXPECT_* variants continue with the run.


#include "gtest/gtest.h"
#include "core/types.hpp"
#include "core/matrix.hpp"
#include "multilevel/multilevel.hpp"
#include "gallery/matrix_IO.hpp"
#include "gallery/diffusion.hpp"
#include "gallery/laplacian27pt.hpp"
#include "gallery/stencil.hpp"

using namespace raptor;

int argc;
char **argv;

int main(int _argc, char** _argv)
{
    ::testing::InitGoogleTest(&_argc, _argv);
    argc = _argc;
    argv = _argv;
    return RUN_ALL_TESTS();

} // end of main() //

TEST(AMGTest, TestsInMultilevel)
{
    int dim;
    int n = 5;
    int system = 0;

    if (argc > 1)
    {
        system = atoi(argv[1]);
    }

    Multilevel* ml;
    CSRMatrix* A;
    Vector x;
    Vector b;

    double strong_threshold = 0.25;
    

    if (system < 2)
    {
        double* stencil = NULL;
        std::vector<int> grid;
        if (argc > 2)
        {
            n = atoi(argv[2]);
        }

        if (system == 0)
        {
            dim = 3;
            grid.resize(dim, n);
            stencil = laplace_stencil_27pt();
        }
        else if (system == 1)
        {
            dim = 2;
            grid.resize(dim, n);
            double eps = 0.001;
            double theta = M_PI/8.0;
            strong_threshold = 0.0;
            if (argc > 3)
            {
                eps = atof(argv[3]);
                if (argc > 4)
                {
                    theta = atof(argv[4]);
                }
            }
            stencil = diffusion_stencil_2d(eps, theta);
        }
        A = stencil_grid(stencil, grid.data(), dim);
        delete[] stencil;
    }
    else if (system == 2)
    {
        int sym = 1;
        char* file = (char *) "../../../../examples/LFAT5.mtx";
        if (argc > 2)
        {
            sym = atoi(argv[2]);
            if (argc > 3)
            {
                strong_threshold = atof(argv[3]);
            }
        }
        A = readMatrix(file, sym);
    }

    x.resize(A->n_rows);
    b.resize(A->n_rows);
    x.set_const_value(1.0);
    A->mult(x, b);
    x.set_const_value(0.0);
    
    ml = new Multilevel(A, strong_threshold);

    printf("Num Levels = %d\n", ml->num_levels);
	printf("A\tNRow\tNCol\tNNZ\n");
    for (int i = 0; i < ml->num_levels; i++)
    {
        CSRMatrix* Al = ml->levels[i]->A;
        printf("%d\t%d\t%d\t%d\n", i, Al->n_rows, Al->n_cols, Al->nnz);
    }
	printf("\nP\tNRow\tNCol\tNNZ\n");
    for (int i = 0; i < ml->num_levels-1; i++)
    {
        CSRMatrix* Pl = ml->levels[i]->P;
        printf("%d\t%d\t%d\t%d\n", i, Pl->n_rows, Pl->n_cols, Pl->nnz);
    }
    
    printf("\nSolve Phase Relative Residuals:\n");
    ml->solve(x, b);

    delete ml;
    delete A;

} // end of TEST(AMGTest, TestsInMultilevel) //
