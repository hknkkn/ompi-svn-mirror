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
#include "orte_config.h"

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "include/orte_constants.h"
#include "rds_resfile.h"

static void
process_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            printf("node type: Element, parent: %s name: %s\n",
                    a_node->name, cur_node->name);
        }

        process_element_names(cur_node->children);
    }
}


static int orte_rds_resfile_query(void)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    /*
     * this initializes the library and checks potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile(argv[1], NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", argv[1]);
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    /* traverse the tree, adding data to registry as we go */
    process_element_names(root_element);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();


    return ORTE_SUCCESS;
}


static int orte_rds_resfile_finalize(void)
{
    return ORTE_SUCCESS;
}
