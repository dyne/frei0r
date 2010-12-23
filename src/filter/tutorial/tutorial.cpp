#include "frei0r.hpp"

/**
This is a sample filter for easy copy/pasting.
CMakeLists.txt needs to be adjusted as well.
*/

class Tutorial : public frei0r::filter
{

public:

    Tutorial(unsigned int width, unsigned int height)

    {
        register_param(m_barSize, "barSize", "Size of the black bar");
        m_barSize = 0.1;
    }

    ~Tutorial()
    {
        // Delete member variables if necessary.
    }

    virtual void update()
    {
        // Just copy input to output.
        std::copy(in, in + width*height, out);

        // Then fill the given amount of the top part with black.
        std::fill(&out[0], &out[(int) (m_barSize*width*height)], 0);
    }

private:
    // The various f0r_params are adjustable parameters.
    // This one determines the size of the black bar in this example.
    f0r_param_double m_barSize;

};



frei0r::construct<Tutorial> plugin("Tutorial filter",
                "This is an example filter, kind of a quick howto showing how to add a frei0r filter.",
                "Your Name",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
