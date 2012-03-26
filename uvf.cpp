#include <cstdio>
#include <fstream>
#include <errno.h>
#include <iostream>
#include <stdexcept>
#include <boost/cstdint.hpp>
#include "mex.h"

#include "Controller/Controller.h"
#include "DebugOut/ConsoleOut.h"
#include "IO/RAWConverter.h"

static const char* raw_filename = NULL;
// ".uvf-intermediate-raw"; // FIXME

namespace {
  template<typename T>
  void to_disk(const T* array, size_t n_elems, const char* filename) {
    std::ofstream ofs(filename, std::ios::out|std::ios::binary);
    if(!ofs) {
      throw std::runtime_error("could not open file!");
    }

    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ofs.write(reinterpret_cast<const char*>(array), n_elems*sizeof(T));
    ofs.close();
  }
}

static void write_raw(const char* filename, const mxArray* input);
static void convert(const char* input, const char* uvf,
                    const size_t dimensions[3], mxClassID type);

// RAII for adding/removing a debug channel from the controller.
class AddADebugOut {
  public:
    AddADebugOut(tuvok::MasterController& ctl) : ctlr(ctl), debugOut(NULL) {
      this->debugOut = new ConsoleOut();
      this->debugOut->SetOutput(true, true, true, false);
      this->ctlr.AddDebugOut(this->debugOut);
    }
    ~AddADebugOut() {
      this->ctlr.RemoveDebugOut(this->debugOut);
      // controller will clean up the debugOut automagically; we don't need to
      // delete it.
    }
  private:
    tuvok::MasterController& ctlr;
    ConsoleOut* debugOut;
};

void mexFunction(int n_outputs, mxArray* /*unused outputs*/[],
                 int n_inputs, const mxArray* inputs[])
{
  mwSize n_dims;
  const mwSize *dims;

  AddADebugOut dbgout(tuvok::Controller::Instance());

  if(n_inputs != 1) {
    T_ERROR("'uvf' requires a single input.");
    return;
  }
  if(n_outputs > 0) {
    T_ERROR("Input from a UVF is not yet supported.");
    return;
  }

  dims = mxGetDimensions(inputs[0]);
  n_dims = mxGetNumberOfDimensions(inputs[0]);
  if(n_dims != 2 && n_dims != 3) {
    T_ERROR("'uvf' requires 2D or 3D data.\n");
    return;
  }

  raw_filename = tempnam(".", ".uvf-intermediate");
  write_raw(raw_filename, inputs[0]);
  const char* uvf_filename = "matlab.uvf"; // FIXME?
  {
    size_t dimensions[3] = {1,1,1};
    size_t i;
    for(i=0; i < n_dims; ++i) {
      dimensions[i] = static_cast<size_t>(dims[i]);
    }
    convert(raw_filename, uvf_filename, dimensions, mxGetClassID(inputs[0]));
  }
  if(remove(raw_filename) != 0) {
    T_ERROR("Error deleting temporary file %s: %d", raw_filename, errno);
  }
}

static void write_raw(const char* filename, const mxArray* input)
{
  mxClassID class_id = mxGetClassID(input);
  using namespace boost; // for int8_t, etc.
  switch(class_id) {
    case mxDOUBLE_CLASS:
      to_disk<double>(static_cast<const double*>(mxGetData(input)),
                      mxGetNumberOfElements(input), filename);
      break;
    case mxSINGLE_CLASS:
      to_disk<float>(static_cast<const float*>(mxGetData(input)),
                     mxGetNumberOfElements(input), filename);
      break;
    case mxINT8_CLASS:
      to_disk<int8_t>(static_cast<const int8_t*>(mxGetData(input)),
                      mxGetNumberOfElements(input), filename);
      break;
    case mxUINT8_CLASS:
      to_disk<uint8_t>(static_cast<const uint8_t*>(mxGetData(input)),
                       mxGetNumberOfElements(input), filename);
      break;
    case mxINT16_CLASS:
      to_disk<int16_t>(static_cast<const int16_t*>(mxGetData(input)),
                       mxGetNumberOfElements(input), filename);
      break;
    case mxUINT16_CLASS:
      to_disk<uint16_t>(static_cast<const uint16_t*>(mxGetData(input)),
                        mxGetNumberOfElements(input), filename);
      break;
    case mxINT32_CLASS:
      to_disk<int32_t>(static_cast<const int32_t*>(mxGetData(input)),
                       mxGetNumberOfElements(input), filename);
      break;
    case mxUINT32_CLASS:
      to_disk<uint32_t>(static_cast<const uint32_t*>(mxGetData(input)),
                        mxGetNumberOfElements(input), filename);
      break;
    case mxINT64_CLASS:
      to_disk<int64_t>(static_cast<const int64_t*>(mxGetData(input)),
                       mxGetNumberOfElements(input), filename);
      break;
    case mxUINT64_CLASS:
      to_disk<uint64_t>(static_cast<const uint64_t*>(mxGetData(input)),
                        mxGetNumberOfElements(input), filename);
      break;
    case mxUNKNOWN_CLASS:
    case mxCELL_CLASS:
    case mxSTRUCT_CLASS:
    case mxLOGICAL_CLASS:
    case mxCHAR_CLASS:
    case mxVOID_CLASS:
    case mxFUNCTION_CLASS:
    case mxOPAQUE_CLASS:
    case mxOBJECT_CLASS:
      T_ERROR("Do not know how to handle '%d' class ID", class_id);
      return;
  }
}

static uint64_t comp_size(mxClassID type) {
  switch(type) {
    case mxDOUBLE_CLASS: return 64;
    case mxSINGLE_CLASS: return 32;
    case mxINT8_CLASS: return 8;
    case mxUINT8_CLASS: return 8;
    case mxINT16_CLASS: return 16;
    case mxUINT16_CLASS: return 16;
    case mxINT32_CLASS: return 32;
    case mxUINT32_CLASS: return 32;
    case mxINT64_CLASS: return 64;
    case mxUINT64_CLASS: return 64;
    default:
      throw std::runtime_error("Invalid array type");
  }
  return 0;
}

static bool mx_fp(mxClassID type) {
  switch(type) {
    case mxDOUBLE_CLASS: /* FALL THROUGH */
    case mxSINGLE_CLASS:
      return true;
    default: return false;
  }
}
static bool mx_signed(mxClassID type) {
  switch(type) {
    case mxDOUBLE_CLASS: /* FALL THROUGH */
    case mxSINGLE_CLASS: /*  ... */
    case mxINT8_CLASS:
    case mxINT16_CLASS:
    case mxINT32_CLASS:
    case mxINT64_CLASS:
      return true;
    case mxUINT8_CLASS:  /* FALL THROUGH */
    case mxUINT16_CLASS: /* ... */
    case mxUINT32_CLASS:
    case mxUINT64_CLASS:
      return false;
    default:
      throw std::runtime_error("Unknown array type");
  }
}

static void convert(const char* input, const char* uvf,
                    const size_t dimensions[3], mxClassID type)
{
  const std::string temp_dir = ".";
  const uint64_t skip_bytes = 0;
  const uint64_t component_size = comp_size(type);
  const uint64_t component_count = 1;
  const uint64_t timesteps = 1;
  const bool convert_endianness = false;
  const bool is_signed = mx_signed(type);
  const bool is_float = mx_fp(type);
  UINT64VECTOR3 dims = UINT64VECTOR3(dimensions[0], dimensions[1],
                                     dimensions[2]);
  const FLOATVECTOR3 aspect = FLOATVECTOR3(1.0, 1.0, 1.0);
  const uint64_t brick_size = 256;
  const uint64_t overlap = 4;
  if(RAWConverter::ConvertRAWDataset(std::string(input),
                                     std::string(uvf),
                                     temp_dir,
                                     skip_bytes, component_size,
                                     component_count, timesteps,
                                     convert_endianness, is_signed, is_float,
                                     dims, aspect,
                                     std::string("Processed volume"),
                                     std::string("Matlab-UVF"),
                                     brick_size, overlap))
  {
    MESSAGE("Success!");
  } else {
    T_ERROR("Conversion failed!");
  }
}
