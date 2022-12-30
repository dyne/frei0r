#include <random>

struct pixshift0r
{
    unsigned int m_width;
    unsigned int m_height;

    unsigned int m_shift_intensity;
    unsigned int m_block_height;
    
    // If m_block_height == 0, then these bound block heights
    unsigned int m_block_height_min;
    unsigned int m_block_height_max;

    // FIXME: It might be better to make it global variable.
    std::random_device m_rng_dev;

    std::uniform_int_distribution<long long int> m_shift_rng;
    std::uniform_int_distribution<unsigned int> m_block_height_rng;

    pixshift0r(unsigned int width, unsigned int height)
        : m_width(width), m_height(height), m_block_height(0) {}

    void process(const uint32_t *inframe, uint32_t *outframe)
    {
        for (unsigned int b = 0; b < m_height;)
        {
            unsigned int block_height = m_block_height ? m_block_height : m_block_height_rng(m_rng_dev);

            // Number of rows to shift is either block size or
            // what we can *safely* operate on (avoids corruption).
            block_height = std::min(block_height, m_height - b);

            long long shift = m_shift_rng(m_rng_dev);

            for (unsigned int j = 0; j < block_height; ++j)
            {
				// NOTE: Faulty implementations, such as FFmpeg don't
				// check for (width % 8 == 0 && height % 8 == 0).
				//
				// A possible workaround for all filters, is a scale filter:
				// (e.g "scale=round(in_w/8)*8:round(in_h/8)*8:flags=neighbor").
                //
                // https://ffmpeg.org/doxygen/trunk/vf__frei0r_8c_source.html#l00352
                size_t pitch = static_cast<size_t>(b + j) * m_width;

                const uint32_t *inrow = inframe + pitch;
                uint32_t *outrow = outframe + pitch;

                if (shift > 0)
                {
                    std::copy (inrow, inrow + m_width - shift, outrow + shift);
                    std::copy (inrow + m_width - shift, inrow + m_width, outrow);
                }
                else if (shift < 0)
                {
                    std::copy (inrow, inrow - shift, outrow + m_width + shift);
                    std::copy (inrow - shift, inrow + m_width, outrow);
                }
                else
                {
                    std::copy (inframe, inframe + m_width, outframe);
                }
            }

            b += block_height;
        }
    }

    void set_block_height_range(unsigned int min_bh, unsigned int max_bh)
    {
        this->m_block_height_min = min_bh;
        this->m_block_height_max = max_bh;

        auto rng_params = decltype(this->m_block_height_rng)::param_type(min_bh, max_bh);
        
        this->m_block_height_rng.param(rng_params);
    }

    void set_shift_intensity(unsigned int shift_intensity)
    {
        this->m_shift_intensity = shift_intensity;

        long long intensity = static_cast<long long>(shift_intensity);

        auto rng_params = decltype(this->m_shift_rng)::param_type(-intensity, intensity);
        
        this->m_shift_rng.param(rng_params);
    }
};

extern "C" {
#include <frei0r.h>

int f0r_init()
{
    return 0;
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    pixshift0r *instance = new pixshift0r(width, height);

    instance->set_shift_intensity(width / 100);
    instance->set_block_height_range(height / 100, height / 10);
    
    return instance;
}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
{
    info->name = "pixs0r";
    info->author = "xsbee";
    info->explanation = "Glitch image shifting rows to and fro";
    info->major_version = 1;
    info->minor_version = 0;
    info->frei0r_version = FREI0R_MAJOR_VERSION;

    info->color_model = F0R_COLOR_MODEL_PACKED32;
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->num_params = 4;
}

void f0r_get_param_info	(f0r_param_info_t *info, int param_index)
{
    switch (param_index)
    {
        case 0:
            info->name = "shift_intensity";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Agressiveness of row/column-shifting";
            break;
        
        case 1:
            info->name = "block_height";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Height of each block whose shift is invariant (0 = random)";
            break;

        case 2:
            info->name = "block_height_min";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Minimum height of block (if random mode)";
            break;

        case 3:
            info->name = "block_height_max";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Maximum height of block (if random mode)";
            break;
    }
}

void f0r_get_param_value(
    f0r_instance_t instance,
    f0r_param_t param,
    int param_index 
)
{
    pixshift0r* context = (pixshift0r*)instance;

    switch (param_index) {
        case 0:
            *static_cast<double*>(param) = static_cast<double>(context->m_shift_intensity) / context->m_width;
        break;
        case 1:
            *static_cast<double*>(param) = static_cast<double>(context->m_block_height) / context->m_height;
        break;
        case 2:
            *static_cast<double*>(param) = static_cast<double>(context->m_block_height_min) / context->m_height;
        break;
        case 3:
            *static_cast<double*>(param) = static_cast<double>(context->m_block_height_max) / context->m_height;
        break;
    }
}

void f0r_set_param_value(
    f0r_instance_t instance,
    f0r_param_t param,
    int param_index 
)
{
    pixshift0r* context = (pixshift0r*)instance;

    switch (param_index) {
        case 0:
            context->set_shift_intensity (*static_cast<double*>(param) * context->m_width);
        break;
        case 1:
            context->m_block_height = *static_cast<double*>(param) * context->m_height;
        break;
        case 2:
            context->set_block_height_range (*static_cast<double*>(param) * context->m_height, context->m_block_height_max);
        break;
        case 3:
            context->set_block_height_range (context->m_block_height_min, *static_cast<double*>(param) * context->m_height);
        break;
    }
}

void f0r_update	(
    f0r_instance_t instance,
    double,
    const uint32_t *inframe,
    uint32_t *outframe 
)
{
    static_cast<pixshift0r*>(instance)->process(inframe, outframe);
}

void f0r_deinit() {}

void f0r_destruct (f0r_instance_t instance)	
{
    delete static_cast<pixshift0r*>(instance);
}
}