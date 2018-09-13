extern "C" 
{
  #include "frei0r.h"
}

#include <list>
#include <vector>
#include <string>
#include <iostream>


namespace frei0r
{
  class fx;
  
  // remember me
  static std::string s_name;
  static std::string s_author;
  static std::string s_explanation;
  static std::pair<int,int> s_version;
  static unsigned int s_effect_type;
  static unsigned int s_color_model;

  static  fx* (*s_build) (unsigned int, unsigned int);

  struct param_info
  {
    param_info(const std::string& name, const std::string& desc, int type)
      : m_name(name), m_desc(desc), m_type(type) {}
    
    std::string m_name;
    std::string m_desc;
    int m_type;
  };
  
  static std::vector<param_info> s_params;

  
  class fx
  {
  public:
    
    unsigned int width;
    unsigned int height;
    unsigned int size; // = width * height
    std::vector<void*> param_ptrs;

    fx()
    {
      s_params.clear(); // reinit static params 
    }
    
    virtual unsigned int effect_type()=0;
    
    void register_param(f0r_param_color& p_loc,
			const std::string& name,
			const std::string& desc)
    {
      param_ptrs.push_back(&p_loc);
      s_params.push_back(param_info(name,desc,F0R_PARAM_COLOR));
    }
    
    void register_param(double& p_loc,
			const std::string& name,
			const std::string& desc)
    {
      param_ptrs.push_back(&p_loc);
      s_params.push_back(param_info(name,desc,F0R_PARAM_DOUBLE));
    }

    void register_param(bool& p_loc,
			const std::string& name,
			const std::string& desc)
    {
      param_ptrs.push_back(&p_loc);
      s_params.push_back(param_info(name,desc,F0R_PARAM_BOOL));
    }

    void register_param(f0r_param_position& p_loc,
			const std::string& name,
			const std::string& desc)
    {
      param_ptrs.push_back(&p_loc);
      s_params.push_back(param_info(name,desc,F0R_PARAM_POSITION));
    }
    
    void register_param(std::string& p_loc,
			const std::string& name,
			const std::string& desc)
    {
      param_ptrs.push_back(&p_loc);
      s_params.push_back(param_info(name,desc,F0R_PARAM_STRING));
    }
    
    
    void get_param_value(f0r_param_t param, int param_index)
    {
      void* ptr = param_ptrs[param_index];
      
      switch (s_params[param_index].m_type)
	{
	case F0R_PARAM_BOOL :
	  *static_cast<f0r_param_bool*>(param)
	    = *static_cast<f0r_param_bool*>(ptr) > 0.5 ? 1.0 : 0.0;
	  break;
	case F0R_PARAM_DOUBLE:
	  *static_cast<f0r_param_double*>(param)
	    = *static_cast<f0r_param_double*>(ptr);
	  break;
	case F0R_PARAM_COLOR:
	  *static_cast<f0r_param_color*>(param)
	    = *static_cast<f0r_param_color*>(ptr);
	    break;
	case F0R_PARAM_POSITION:
	  *static_cast<f0r_param_position*>(param)
	    = *static_cast<f0r_param_position*>(ptr);
	    break;
	case F0R_PARAM_STRING:
	  *static_cast<f0r_param_string*>(param)
	    = const_cast<f0r_param_string>(static_cast<std::string*>(ptr)->c_str());
	  break;
	}
    }


    void set_param_value(f0r_param_t param, int param_index)
    {
      void* ptr = param_ptrs[param_index];
      
      switch (s_params[param_index].m_type)
	{
	case F0R_PARAM_BOOL :
	  *static_cast<bool*>(ptr)
	    = (*static_cast<f0r_param_bool*>(param) > 0.5) ;
	  break;
	case F0R_PARAM_DOUBLE:
	  *static_cast<f0r_param_double*>(ptr)
	    = *static_cast<f0r_param_double*>(param);
	  break;
	case F0R_PARAM_COLOR:
	  *static_cast<f0r_param_color*>(ptr)
	    = *static_cast<f0r_param_color*>(param);
	  break;
	case F0R_PARAM_POSITION:
	  *static_cast<f0r_param_position*>(ptr)
	    =  *static_cast<f0r_param_position*>(param);
	    break;
	case F0R_PARAM_STRING:
	    *static_cast<std::string*>(ptr)
	      = *static_cast<f0r_param_string*>(param);
	    break;
	}
      
    }
      
    virtual void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2,
              const uint32_t* in3) = 0;
    
    virtual ~fx()
    {
    }
  };
  
  class source : public fx
    {
    protected:
      source() {}
      
    public:
      virtual unsigned int effect_type(){ return F0R_PLUGIN_TYPE_SOURCE; }
      virtual void update(double time, uint32_t* out) = 0;

    private:
      virtual void update(double time,
                uint32_t* out,
                const uint32_t* in1,
                const uint32_t* in2,
                const uint32_t* in3) {
          (void)in1; // unused
          (void)in2; // unused
          (void)in3; // unused
          update(time, out);
      }
  };

  class filter : public fx
  {
  protected:
    filter() {}
    
  public:
    virtual unsigned int effect_type(){ return F0R_PLUGIN_TYPE_FILTER; }
    virtual void update(double time, uint32_t* out, const uint32_t* in1) = 0;

  private:
    virtual void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2,
              const uint32_t* in3) {
        (void)in2; // unused
        (void)in3; // unused
        update(time, out, in1);
    }
  };

  class mixer2 : public fx
  {
  protected:
    mixer2() {}
      
  public:
    virtual unsigned int effect_type(){ return F0R_PLUGIN_TYPE_MIXER2; }
    virtual void update(double time, uint32_t* out, const uint32_t* in1, const uint32_t* in2) = 0;

  private:
    virtual void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2,
              const uint32_t* in3) {
        (void)in3; // unused
        update(time, out, in1, in2);
    }
  };

  
  class mixer3 : public fx
  {
  protected:
    mixer3() {}
      
  public:
    virtual unsigned int effect_type(){ return F0R_PLUGIN_TYPE_MIXER3; }
  };

  
  // register stuff
  template<class T>
  class construct
  {
  public:
    construct(const std::string& name,
              const std::string& explanation,
              const std::string& author,
              const int& major_version,
              const int& minor_version,
              unsigned int color_model = F0R_COLOR_MODEL_BGRA8888)
    {
      T a(0,0);
      
      s_name=name; 
      s_explanation=explanation;
      s_author=author;
      s_version=std::make_pair(major_version,minor_version);
      s_build=build;
      
      s_effect_type=a.effect_type();
      s_color_model=color_model;
    }

  private:
    static fx* build(unsigned int width, unsigned int height)
    {
      return new T(width,height);
    }
  };
}


// the exported frei0r functions

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = frei0r::s_name.c_str();
  info->author = frei0r::s_author.c_str();
  info->plugin_type = frei0r::s_effect_type;
  info->color_model = frei0r::s_color_model;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = frei0r::s_version.first; 
  info->minor_version = frei0r::s_version.second; 
  info->explanation = frei0r::s_explanation.c_str();
  info->num_params =  frei0r::s_params.size(); 
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  info->name=frei0r::s_params[param_index].m_name.c_str();
  info->type=frei0r::s_params[param_index].m_type;
  info->explanation=frei0r::s_params[param_index].m_desc.c_str();
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  frei0r::fx* nfx = frei0r::s_build(width, height);
  nfx->width=width;
  nfx->height=height;
  nfx->size=width*height;
  return nfx;
}

void f0r_destruct(f0r_instance_t instance)
{
  delete static_cast<frei0r::fx*>(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
  static_cast<frei0r::fx*>(instance)->set_param_value(param, param_index);
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
  static_cast<frei0r::fx*>(instance)->get_param_value(param, param_index);
}

void f0r_update2(f0r_instance_t instance, double time,
		 const uint32_t* inframe1,
		 const uint32_t* inframe2,
		 const uint32_t* inframe3,
		 uint32_t* outframe)
{
  static_cast<frei0r::fx*>(instance)->update(time,
                                             outframe,
                                             inframe1,
                                             inframe2,
                                             inframe3);
}

// compability for frei0r 1.0 
void f0r_update(f0r_instance_t instance, 
		double time, const uint32_t* inframe, uint32_t* outframe)
{
  f0r_update2(instance, time, inframe, 0, 0, outframe);
}

