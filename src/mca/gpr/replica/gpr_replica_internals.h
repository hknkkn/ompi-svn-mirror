/*
 * $HEADER$
 */
/** @file:
 *
 * The Open MPI general purpose registry - support functions.
 *
 */

/** Retrieve a registry key value for a given token string.
 * The ompi_registry_getkey() function is used to translate a token string for a particular
 * segment of the registry into its associated (integer) key value.
 *
 * @param segment Pointer to a character string defining the segment of the registry being queried.
 * @param token Pointer to a character string containing the token to be translated. If token=NULL,
 * the function returns the key value corresponding to the specified segment itself.
 *
 * @retval key Unsigned long integer value corresponding to the specified token within the specified segment.
 * @retval -1 Indicates that the segment and/or token could not be found.
 */
mca_gpr_replica_key_t gpr_replica_get_key(char *segment, char *token);

/** Add a token to a segment's dictionary.
 * The gpr_replica_definekey() function allows the addition of a new definition to
 * the registry's token-key dictionaries. The specified token is assigned an integer
 * value within the specified segment, and the entry is added to the segment's token-key
 * dictionary.
 *
 * @param segment Pointer to a character string defining the segment of the registry being queried.
 * @param token Pointer to a character string containing the token to be defined. If token=NULL,
 * the function adds the token to the segment dictionary, thus defining a new segment name.
 *
 * @retval key Unsigned long integer value corresponding to the specified token within the specified segment.
 * @retval 0 Indicates that the entry could not be created.
 */
mca_gpr_replica_key_t gpr_replica_define_key(char *segment, char *token);

/** Delete a token from a segment's dictionary.
 * The gpr_replica_deletekey() function allows the removal of a definition from the
 * registry's token-key dictionaries. This should be used with caution! Deletion of
 * a token-key pair causes the registry to search through all entries within that segment
 * for objects that include the specified token-key pair in their description. The reference
 * is subsequently removed, and any object for which this was the SOLE key will also
 * be removed from the registry!
 *
 * @param segment Pointer to a character string defining the segment of the registry.
 * @param token Pointer to a character string containing the token to be deleted. If token=NULL,
 * the function deletes the specified segment name from the segment dictionary.
 *
 * @retval OMPI_SUCCESS Indicating that the operation was successful.
 * @retval OMPI_ERROR Indicates that the operation failed - most likely caused by specifying
 * a token that did not exist within the specified segment, or a non-existent segment.
 */
int gpr_replica_delete_key(char *segment, char *token);

/** Find a requested registry segment.
 * The gpr_replica_findseq() function finds the registry segment corresponding to
 * the specified name.
 *
 * @param segment Pointer to a string containing the name of the segment to be found.
 *
 * @retval *seg Pointer to the segment
 * @retval NULL Indicates that the specified segment could not be found
 */
mca_gpr_registry_segment_t *gpr_replica_find_seg(char *segment);

mca_gpr_keytable_t *gpr_replica_find_dict_entry(char *segment, char *token);

int gpr_replica_empty_segment(mca_gpr_registry_segment_t *seg);
