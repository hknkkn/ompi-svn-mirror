//
// $HEADER$
//

#include "ompi_config.h"

#include <iostream>
#include <string>

#include <stdlib.h>
#include <string.h>

#include "mca/base/base.h"
#include "mca/allocator/allocator.h"
#include "mca/allocator/base/base.h"
#include "mca/mpool/mpool.h"
#include "mca/mpool/base/base.h"
#include "mca/pcm/pcm.h"
#include "mca/pcm/base/base.h"
#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"
#include "mca/pml/pml.h"
#include "mca/pml/base/base.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/base.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/base.h"
#include "mca/ns/ns.h"
#include "mca/ns/base/base.h"
#include "tools/ompi_info/ompi_info.h"

using namespace std;
using namespace ompi_info;


//
// Public variables
//

ompi_info::module_map_t ompi_info::module_map;


//
// Private variables
//

static bool opened_modules = false;


//
// Open all MCA modules so that they can register their MCA
// parameters.  Take a shotgun approach here and indiscriminately open
// all modules -- don't be selective.  To this end, we need to clear
// out the environment of all OMPI_MPI_mca_<type> variables to ensure
// that the open algorithms don't try to only open one module.
//
void ompi_info::open_modules()
{
  ompi_info::type_vector_t::size_type i;
  string env;
  char *target;

  if (opened_modules)
    return;

  // Clear out the environment.  Use strdup() to orphan the resulting
  // strings because items are placed in the environment by reference,
  // not by value.

  for (i = 0; i < mca_types.size(); ++i) {
    env = "OMPI_MPI_MCA_" + mca_types[i];
    if (NULL != getenv(env.c_str())) {
      env += "=";
      target = strdup(env.c_str());
      putenv(target);
    }
  }

  // Find / open all components

  mca_base_open();
  module_map["base"] = NULL;

  mca_allocator_base_open();
  module_map["allocator"] = &mca_allocator_base_components;

  mca_coll_base_open();
  module_map["coll"] = &mca_coll_base_components_opened;

#if 0
  // common component framework not implemented yet
  mca_common_base_open();
  module_map["common"] = &mca_common_base_components_opened;
#else
  module_map["common"] = NULL;
#endif

#if 0
  // waiting for gpr to be implemented
  mca_gpr_base_open();
  module_map["gpr"] = &mca_ns_base_components_available;
#else
  module_map["gpr"] = NULL;
#endif

#if 0
  // io module opening not implemented yet
  mca_io_base_open();
  module_map["io"] = &mca_io_base_modules_available;
#else
  module_map["io"] = NULL;
#endif

  mca_ns_base_open();
  module_map["ns"] = &mca_ns_base_components_available;

  mca_mpool_base_open();
  module_map["mpool"] = &mca_mpool_base_components;

#if 0
  // one module opening not implemented yet
  mca_one_base_open();
  module_map["one"] = &mca_one_base_modules_available;
#else
  module_map["one"] = NULL;
#endif

  mca_oob_base_open();
  module_map["oob"] = &mca_oob_base_components;

#if 0
  // op component framework not yet implemented
  mca_op_base_open();
  module_map["op"] = &mca_oob_base_components;
#else
  module_map["op"] = NULL;
#endif

  mca_pcm_base_open();
  module_map["pcm"] = &mca_pcm_base_modules_available;

  mca_pml_base_open();
  module_map["pml"] = &mca_pml_base_modules_available;

  mca_ptl_base_open();
  module_map["ptl"] = &mca_ptl_base_modules_available;

#if 0
  // topo module opening not implemented yet
  mca_topo_base_open();
  module_map["topo"] = &mca_topo_base_modules_available;
#else
  module_map["topo"] = NULL;
#endif

  // All done

  opened_modules = true;
}


void ompi_info::close_modules()
{
  if (opened_modules) {
    mca_pcm_base_close();
    mca_oob_base_close();
    mca_ns_base_close();
    mca_coll_base_close();
    mca_pml_base_close();
    mca_ptl_base_close();

    mca_base_close();
    module_map.clear();
  }

  opened_modules = false;
}
