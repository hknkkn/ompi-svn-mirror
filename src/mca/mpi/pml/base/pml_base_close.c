/*
 * $HEADER$
 */

#include "lam_config.h"

#include <stdio.h>

#include "lam/constants.h"
#include "mca/mca.h"
#include "mca/lam/base/base.h"
#include "mca/mpi/pml/pml.h"
#include "mca/mpi/pml/base/base.h"


int mca_pml_base_close(void)
{
  extern lam_list_t mca_pml_base_modules_available;

  /* Close all remaining available modules (may be one if this is a
     LAM RTE program, or [possibly] multiple if this is laminfo) */

  mca_base_modules_close(mca_pml_base_output, 
                         &mca_pml_base_modules_available, NULL);

  /* All done */

  return LAM_SUCCESS;
}

