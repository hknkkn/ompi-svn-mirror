/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

/**
 * @file
 *
 * Parse command line options for the Open Run Time Environment. This program MUST be called before
 * any call to orte_init, but after orte_parse_environ to ensure that variables are correctly set !!!
 *
 */
#include "orte_config.h"

#include <stdlib.h>
#include <string.h>

#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"

#include "mca/errmgr/errmgr.h"

#include "util/cmd_line.h"
#include "util/output.h"

#include "runtime/orte_context_value_tbl.h"
#include "runtime/runtime.h"

static int orte_parse_context_setup_cmd(ompi_cmd_line_t *cmd_line,
                                        orte_context_value_names_t *context_tbl);

int orte_parse_context(orte_context_value_names_t *context_tbl, ompi_cmd_line_t *cmd_line,
                       int argc, char **argv)
{
    char *tmp;
    int i, j, k, num_inst, id, rc;
    
    if (ORTE_SUCCESS != (rc = orte_parse_context_setup_cmd(cmd_line, context_tbl))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = ompi_cmd_line_parse(cmd_line, true, argc, argv))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    for (i=0; NULL != context_tbl[i].name.prime ||
              NULL != context_tbl[i].cmd_line_name; i++) {
        /* if command_line option supported and the option is present
         * on command line, then get value from there
         */
        if (NULL != cmd_line && NULL != context_tbl[i].cmd_line_name &&
            ompi_cmd_line_is_taken(cmd_line, context_tbl[i].cmd_line_name)) {
            
            num_inst = ompi_cmd_line_get_ninsts(cmd_line, context_tbl[i].cmd_line_name);
            for (j=0; j < num_inst; j++) {
                k = 0;
                do {
                    switch (context_tbl[i].type) {
                        case ORTE_STRING:
                            if (NULL == (tmp = ompi_cmd_line_get_param(cmd_line, context_tbl[i].cmd_line_name, j, k))) {
                                ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                                return ORTE_ERR_BAD_PARAM;
                            }
                            *((char**)context_tbl[i].dest) = strdup(tmp);
                            if (NULL == *((char**)context_tbl[i].dest)) {
                                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                                return ORTE_ERR_OUT_OF_RESOURCE;
                            }
                            break;
                        
                        case ORTE_INT:
                            if (NULL == (tmp = ompi_cmd_line_get_param(cmd_line, context_tbl[i].cmd_line_name, j, k))) {
                                ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                                return ORTE_ERR_BAD_PARAM;
                            }
                            *((int*)context_tbl[i].dest) = (int)strtoul(tmp, NULL, 0);
                            break;
                    
                        case ORTE_BOOL:
                            *((bool*)context_tbl[i].dest) = true;
                            break;
                            
                        default:
                            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                            return ORTE_ERR_BAD_PARAM;
                            break;
                    }
        
                    /* if specified, execute the requested callback function */
                    if (NULL != context_tbl[i].cbfunc) {
                       context_tbl[i].cbfunc(context_tbl[i].dest, j, k);
                    }
                    k++;
                } while (k < context_tbl[i].num_params);
            }
        } else if (NULL != context_tbl[i].name.prime) { /* otherwise get MCA parameter, if present */
    
            switch (context_tbl[i].type) {
                case ORTE_STRING:
                    if (0 > (id = mca_base_param_register_string(context_tbl[i].name.prime,
                                                context_tbl[i].name.second,
                                                context_tbl[i].name.third,
                                                NULL, (char*)(context_tbl[i].def)))) {
                        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                        return ORTE_ERR_BAD_PARAM;
                    }
                    if (ORTE_SUCCESS != (rc = mca_base_param_lookup_string(id, &tmp))) {
                        ORTE_ERROR_LOG(rc);
                        return rc;
                    }
                    if (NULL != tmp) {
                        *((char**)(context_tbl[i].dest)) = strdup(tmp);
                        free(tmp);
                        tmp = NULL;
                    }
                    break;
                
                case ORTE_INT:
                case ORTE_BOOL:
                    if (0 > (id = mca_base_param_register_int(context_tbl[i].name.prime,
                                                context_tbl[i].name.second,
                                                context_tbl[i].name.third,
                                                NULL, (int)(context_tbl[i].def)))) {
                        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                        return ORTE_ERR_BAD_PARAM;
                    }
                    if (ORTE_SUCCESS != (rc = mca_base_param_lookup_int(id,
                                            (int*)context_tbl[i].dest))) {
                        ORTE_ERROR_LOG(rc);
                        return rc;
                    }
                    break;

                default:
                    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                    return ORTE_ERR_BAD_PARAM;
                    break;
            }
            /* if specified, execute the requested callback function */
            if (NULL != context_tbl[i].cbfunc) {
               context_tbl[i].cbfunc(context_tbl[i].dest, 0, 0);
            }

        }
    }
    
    return ORTE_SUCCESS;
}

static int orte_parse_context_setup_cmd(ompi_cmd_line_t *cmd_line,
                                        orte_context_value_names_t *context_tbl)
{
    int i, rc;
    
    for (i=0; NULL != context_tbl[i].name.prime ||
              NULL != context_tbl[i].cmd_line_name; i++) {
        if (NULL != context_tbl[i].cmd_line_name) {
            if (ORTE_SUCCESS != (rc = ompi_cmd_line_make_opt3(cmd_line, '\0',
                            context_tbl[i].cmd_line_name, context_tbl[i].cmd_line_name,
                            context_tbl[i].num_params, NULL))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        }
    }
    return ORTE_SUCCESS;
}
