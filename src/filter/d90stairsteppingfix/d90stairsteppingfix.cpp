#include "frei0r.hpp"

#include <stdio.h>

static int slices[] = {7,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,
                    9,8,9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,9,9,9,8,
                    9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,9,8,9,9,9,9,8,9,9,
                    9,9,9,8,9,9,7};

class D90StairsteppingFix : public frei0r::filter
{

public:
    
    
    D90StairsteppingFix(unsigned int width, unsigned int height)
    {
    }

    virtual void update()
    {
        std::copy(in, in + width*height, out);
    }

};



frei0r::construct<D90StairsteppingFix> plugin("Nikon D90 Stairstepping fix",
                "Removes the Stairstepping from Nikon D90 videos by interpolation",
                "Simon A. Eugster (Granjow)",
                0,1);
