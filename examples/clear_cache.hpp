// Copyright (c) 2015, Raptor Developer Team, University of Illinois at Urbana-Champaign
// License: Simplified BSD, http://opensource.org/licenses/BSD-2-Clause
#ifndef CLEAR_CACHE
#define CLEAR_CACHE

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>

void clear_cache(std::vector<double>& cache_list)
{
    srand(time(NULL));
    for (int i = 0; i < (int) cache_list.size(); i++)
    {
        cache_list[i] = rand()%10;
    }
}


#endif
