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

#include "ompi_config.h"
#include <string.h>
#include <stdio.h>
#include "mpi.h"

#include "communicator/communicator.h"
#include "proc/proc.h"
#include "include/constants.h"
#include "mca/pml/pml.h"
#include "mca/ns/ns.h"
#include "mca/gpr/gpr.h"
#include "mca/oob/oob_types.h"

static ompi_mutex_t ompi_port_lock;

int ompi_open_port(char *port_name)
{
    ompi_proc_t **myproc=NULL;
    char *name=NULL;
    size_t size=0;
    orte_rml_tag_t lport_id=0;
    int rc;
    
    /*
     * The port_name is equal to the OOB-contact information
     * and an integer. The reason for adding the integer is
     * to make the port unique for multi-threaded scenarios.
     */
  
    myproc = ompi_proc_self (&size);
    if (ORTE_SUCCESS != (rc = orte_name_services.get_proc_name_string (name, &(myproc[0]->proc_name)))) {
        return rc;
    }

    OMPI_THREAD_LOCK(&ompi_port_lock);
    if (ORTE_SUCCESS != (rc = orte_name_services.assign_rml_tag(&lport_id, NULL))) {
        return rc;
    }
    OMPI_THREAD_UNLOCK(&ompi_port_lock);

    sprintf (port_name, "%s:%d", name, lport_id);
    free ( myproc );
    free ( name );
    
    return OMPI_SUCCESS;
}

/* takes a port_name and separates it into the process_name 
   and the tag
*/
char *ompi_parse_port (char *port_name, int *tag) 
{
    char tmp_port[MPI_MAX_PORT_NAME], *tmp_string;

    tmp_string = (char *) malloc (MPI_MAX_PORT_NAME);
    if (NULL ==  tmp_string ) {
	return NULL;
    }

    strncpy (tmp_port, port_name, MPI_MAX_PORT_NAME);
    strncpy (tmp_string, strtok(tmp_port, ":"), MPI_MAX_PORT_NAME);
    sscanf( strtok(NULL, ":"),"%d", tag);
    
    return tmp_string;
}

/* 
 * publish the port_name using the service_name as a token
 * jobid and vpid are used later to make
 * sure, that only this process can unpublish the information.
 */
int ompi_comm_namepublish ( char *service_name, char *port_name ) 
{

    char *token[2];
    orte_registry_keyval_t keyval;
    
    token[0] = service_name;
    token[1] = NULL;
    
    keyval.key = strdup("port_name");
    keyval.type = ORTE_STRING;
    keyval.value.strptr = strdup(port_name);
    
    return orte_registry.put(ORTE_REGISTRY_AND | ORTE_REGISTRY_OVERWRITE,
                             ORTE_NAMESPACE_SEGMENT,
                             token, 1, &keyval);
}

char* ompi_comm_namelookup ( char *service_name )
{
    char *token[2], *key[2];
    orte_registry_keyval_t *keyvals=NULL;
    int32_t cnt=0;
    char *stmp=NULL;
    int ret;
    
    token[0] = service_name;
    token[1] = NULL;
    
    key[0] = strdup("port_name");
    key[1] = NULL;
    
    ret = orte_registry.get(ORTE_REGISTRY_AND, ORTE_NAMESPACE_SEGMENT,
                            token, key, &cnt, keyvals);
    if (ORTE_SUCCESS != ret) {
        return NULL;
    }
    if ( 0 < cnt && NULL != keyvals ) {  /* should be only one, if any */
        stmp = strdup(keyvals->value.strptr);
        free(keyvals);
    }

    return (stmp);
}

/* 
 * delete the entry. Just the process who has published
 * the service_name, has the right to remove this 
 * service. Will be done later, by adding jobid and vpid
 * as tokens
 */
int ompi_comm_nameunpublish ( char *service_name )
{
    char *token[2];
    
    token[0] = service_name;
    token[1] = NULL;
    
    return orte_registry.delete_entries(ORTE_REGISTRY_AND,
                                        "ompi_name_publish",
                                        token, NULL); 
}
