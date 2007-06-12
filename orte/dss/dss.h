/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/**
 * @file
 *
 * Data packing subsystem.
 */

#ifndef ORTE_DSS_H_
#define ORTE_DSS_H_

#include "orte_config.h"

#include "orte/orte_constants.h"
#include "orte/orte_types.h"

#include "orte/dss/dss_types.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * Set the buffer type.
 *
 * The pack/unpack functions can work with two types of buffer - fully
 * described (i.e., every data object is preceeded by an identifier as
 * to the type of the object) and non-described (i.e., no type
 * identifier included). This function allows the caller to set the
 * buffer type to the specified type - the function first checks to
 * ensure the buffer is empty as the type cannot be changed once
 * data has already entered the buffer.
 *
 * @param *buffer A pointer to the buffer
 *
 * @param type The new buffer type
 *
 * @retval ORTE_SUCCESS Operation successfully executed
 *
 * @retval ORTE_ERROR_VALUE An appropriate error code
 */
typedef int (*orte_dss_set_buffer_type_fn_t)(orte_buffer_t *buffer, orte_dss_buffer_type_t type);

/**
 * Top-level itnerface function to pack one or more values into a
 * buffer.
 *
 * The pack function packs one or more values of a specified type into
 * the specified buffer.  The buffer must have already been
 * initialized via an OBJ_NEW or OBJ_CONSTRUCT call - otherwise, the
 * pack_value function will return an error. Providing an unsupported
 * type flag will likewise be reported as an error.
 *
 * Note that any data to be packed that is not hard type cast (i.e.,
 * not type cast to a specific size) may lose precision when unpacked
 * by a non-homogeneous recipient.  The DSS will do its best to deal
 * with heterogeneity issues between the packer and unpacker in such
 * cases. Sending a number larger than can be handled by the recipient
 * will return an error code (generated by the DSS upon unpacking) via
 * the RML upon transmission - the DSS cannot detect such errors
 * during packing.
 *
 * @param *buffer A pointer to the buffer into which the value is to
 * be packed.
 *
 * @param *src A void* pointer to the data that is to be packed. Note
 * that strings are to be passed as (char **) - i.e., the caller must
 * pass the address of the pointer to the string as the void*. This
 * allows the DSS to use a single interface function, but still allow
 * the caller to pass multiple strings in a single call.
 *
 * @param num A size_t value indicating the number of values that are
 * to be packed, beginning at the location pointed to by src. A string
 * value is counted as a single value regardless of length. The values
 * must be contiguous in memory. Arrays of pointers (e.g., string
 * arrays) should be contiguous, although (obviously) the data pointed
 * to need not be contiguous across array entries.
 *
 * @param type The type of the data to be packed - must be one of the
 * DSS defined data types.
 *
 * @retval ORTE_SUCCESS The data was packed as requested.
 *
 * @retval ORTE_ERROR(s) An appropriate ORTE error code indicating the
 * problem encountered. This error code should be handled
 * appropriately.
 *
 * @code
 * orte_buffer_t *buffer;
 * int32_t src;
 *
 * status_code = orte_dss.pack(buffer, &src, 1, ORTE_INT32);
 * @endcode
 */
typedef int (*orte_dss_pack_fn_t)(orte_buffer_t *buffer, const void *src,
                                  orte_std_cntr_t num_values,
                                  orte_data_type_t type);

/**
 * Unpack values from a buffer.
 *
 * The unpack function unpacks the next value (or values) of a
 * specified type from the specified buffer.
 *
 * The buffer must have already been initialized via an OBJ_NEW or
 * OBJ_CONSTRUCT call (and assumedly filled with some data) -
 * otherwise, the unpack_value function will return an
 * error. Providing an unsupported type flag will likewise be reported
 * as an error, as will specifying a data type that DOES NOT match the
 * type of the next item in the buffer. An attempt to read beyond the
 * end of the stored data held in the buffer will also return an
 * error.
 *
 * NOTE: it is possible for the buffer to be corrupted and that
 * the DSS will *think* there is a proper variable type at the
 * beginning of an unpack region - but that the value is bogus (e.g., just
 * a byte field in a string array that so happens to have a value that
 * matches the specified data type flag). Therefore, the data type error check
 * is NOT completely safe. This is true for ALL unpack functions.
 *
 *
 * Unpacking values is a "destructive" process - i.e., the values are
 * removed from the buffer, thus reducing the buffer size. It is
 * therefore not possible for the caller to re-unpack a value from the
 * same buffer.
 *
 * Warning: The caller is responsible for providing adequate memory
 * storage for the requested data. The orte_dss_peek() function is
 * provided to assist in meeting this requirement. As noted below, the user
 * must provide a parameter indicating the maximum number of values that
 * can be unpacked into the allocated memory. If more values exist in the
 * buffer than can fit into the memory storage, then the dss will unpack
 * what it can fit into that location and return an error code indicating
 * that the buffer was only partially unpacked.
 *
 * Note that any data that was not hard type cast (i.e., not type cast
 * to a specific size) when packed may lose precision when unpacked by
 * a non-homogeneous recipient.  The DSS will do its best to deal with
 * heterogeneity issues between the packer and unpacker in such
 * cases. Sending a number larger than can be handled by the recipient
 * will return an error code (generated by the DSS upon unpacking) via
 * the RML upon transmission - the DSS cannot detect such errors
 * during packing.
 *
 * @param *buffer A pointer to the buffer from which the value will be
 * extracted.
 *
 * @param *dest A void* pointer to the memory location into which the
 * data is to be stored. Note that these values will be stored
 * contiguously in memory. For strings, this pointer must be to (char
 * **) to provide a means of supporting multiple string
 * operations. The DSS unpack function will allocate memory for each
 * string in the array - the caller must only provide adequate memory
 * for the array of pointers.
 *
 * @param *num A pointer to a orte_std_cntr_t value indicating the maximum
 * number of values that are to be unpacked, beginning at the location
 * pointed to by src. This is provided to help protect the caller from
 * memory overrun. Note that a string
 * value is counted as a single value regardless of length.
 *
 * @note The unpack function will return the actual number of values
 * unpacked in this location.
 *
 * @param type The type of the data to be unpacked - must be one of
 * the DSS defined data types.
 *
 * @retval *max_num_values The number of values actually unpacked. In
 * most cases, this should match the maximum number provided in the
 * parameters - but in no case will it exceed the value of this
 * parameter.  Note that if you unpack fewer values than are actually
 * available, the buffer will be in an unpackable state - the dss will
 * return an error code to warn of this condition.
 *
 * @retval ORTE_SUCCESS The next item in the buffer was successfully
 * unpacked.
 *
 * @retval ORTE_ERROR(s) The unpack function returns an error code
 * under one of several conditions: (a) the number of values in the
 * item exceeds the max num provided by the caller; (b) the type of
 * the next item in the buffer does not match the type specified by
 * the caller; or (c) the unpack failed due to either an error in the
 * buffer or an attempt to read past the end of the buffer.
 *
 * @code
 * orte_buffer_t *buffer;
 * int32_t dest;
 * char **string_array;
 * orte_std_cntr_t num_values;
 *
 * num_values = 1;
 * status_code = orte_dss.unpack(buffer, (void*)&dest, &num_values, ORTE_INT32);
 *
 * num_values = 5;
 * string_array = malloc(num_values*sizeof(char *));
 * status_code = orte_dss.unpack(buffer, (void*)(string_array), &num_values, ORTE_STRING);
 *
 * @endcode
 */
typedef int (*orte_dss_unpack_fn_t)(orte_buffer_t *buffer, void *dest,
                                    orte_std_cntr_t *max_num_values,
                                    orte_data_type_t type);

/**
 * Get the type and number of values of the next item in the buffer.
 *
 * The peek function looks at the next item in the buffer and returns
 * both its type and the number of values in the item. This is a
 * non-destructive function call that does not disturb the buffer, so
 * it can be called multiple times if desired.
 *
 * @param buffer A pointer to the buffer in question.
 *
 * @param type A pointer to an orte_data_type_t variable where the
 * type of the next item in the buffer is to be stored. Caller must
 * have memory backing this location.
 *
 * @param number A pointer to a orte_std_cntr_t variable where the number of
 * data values in the next item is to be stored. Caller must have
 * memory backing this location.
 *
 * @retval ORTE_SUCCESS Requested info was successfully returned.
 * @retval ORTE_ERROR(s) An appropriate error code indicating the
 * problem will be returned. This should be handled appropriately by
 * the caller.
 *
 */
typedef int (*orte_dss_peek_next_item_fn_t)(orte_buffer_t *buffer,
                                            orte_data_type_t *type,
                                            orte_std_cntr_t *number);

/**
 * Unload the data payload from a buffer.
 *
 * The unload function provides the caller with a pointer to the data
 * payload within the buffer and the size of that payload. This allows
 * the user to directly access the payload - typically used in the RML
 * to unload the payload from the buffer for transmission.
 *
 * @note This is a destructive operation. While the payload is
 * undisturbed, the function will clear the buffer's pointers to the
 * payload. Thus, the buffer and the payload are completely separated,
 * leaving the caller free to OBJ_RELEASE the buffer.
 *
 * @param buffer A pointer to the buffer whose payload is to be
 * unloaded.
 *
 * @param payload The address to a void* pointer that is to be loaded
 * with the address of the data payload in the buffer.
 *
 * @param size The size (in bytes) of the data payload in the buffer.
 *
 * @retval ORTE_SUCCESS The request was succesfully completed.
 *
 * @retval ORTE_ERROR(s) An appropriate error code indicating the
 * problem will be returned. This should be handled appropriately by
 * the caller.
 *
 * @code
 * orte_buffer_t *buffer;
 * uint8_t *bytes;
 * orte_std_cntr_t size;
 *
 * status_code = orte_dss.unload(buffer, (void**)(&bytes), &size);
 * OBJ_RELEASE(buffer);
 * @endcode
 */
typedef int (*orte_dss_unload_fn_t)(orte_buffer_t *buffer,
                                    void **payload,
                                    orte_std_cntr_t *size);

/**
 * Load a data payload into a buffer.
 *
 * The load function allows the caller to replace the payload in a
 * buffer with one provided by the caller. If a payload already exists
 * in the buffer, the function will "free" the existing data to
 * release it, and then replace the data payload with the one provided
 * by the caller.
 *
 * @note The buffer must be allocated in advance via the OBJ_NEW
 * function call - failing to do so will cause the load function to
 * return an error code.
 *
 * @note The caller is responsible for pre-packing the provided
 * payload - the load function cannot convert to network byte order
 * any data contained in the provided payload.
 *
 * @param buffer A pointer to the buffer into which lthe payload is to
 * be loaded.
 *
 * @param payload A void* pointer to the payload to be loaded into the
 * buffer.
 *
 * @param size The size (in bytes) of the provided payload.
 *
 * @retval ORTE_SUCCESS The request was successfully completed
 *
 * @retval ORTE_ERROR(s) An appropriate error code indicating the
 * problem will be returned. This should be handled appropriately by
 * the caller.
 *
 * @code
 * orte_buffer_t *buffer;
 * uint8_t bytes;
 * orte_std_cntr_t size;
 *
 * buffer = OBJ_NEW(orte_buffer_t);
 * status_code = orte_dss.load(buffer, (void*)(&bytes), size);
 * @endcode
 */
typedef int (*orte_dss_load_fn_t)(orte_buffer_t *buffer,
                                  void *payload,
                                  orte_std_cntr_t size);


/**
 * Transfer a payload from one buffer to another
 * This function is a convenience shortcut that basically unloads the
 * payload from one buffer and loads it into another. This is a destructive
 * action - see the unload and load descriptions above.
 */
typedef int (*orte_dss_xfer_payload_fn_t)(orte_buffer_t *dest,
                                          orte_buffer_t *src);

/**
 * Copy a payload from one buffer to another
 * This function will append a copy of the payload in one buffer into
 * another buffer. If the destination buffer is NOT empty, then the
 * type of the two buffers MUST match or else an
 * error will be returned. If the destination buffer IS empty, then
 * its type will be set to that of the source buffer.
 * NOTE: This is NOT a destructive procedure - the
 * source buffer's payload will remain intact, as will any pre-existing
 * payload in the destination's buffer.
 */
typedef int (*orte_dss_copy_payload_fn_t)(orte_buffer_t *dest,
                                          orte_buffer_t *src);

/**
 * DSS initialization function.
 *
 * In dynamic libraries, declared objects and functions don't get
 * loaded until called. We need to ensure that the orte_dss function
 * structure gets loaded, so we provide an "open" call that is
 * executed as part of the program startup.
 */
ORTE_DECLSPEC int orte_dss_open(void);

/**
 * DSS finalize function
 */
ORTE_DECLSPEC int orte_dss_close(void);


/**
 * Copy a data value from one location to another.
 *
 * Since registered data types can be complex structures, the system
 * needs some way to know how to copy the data from one location to
 * another (e.g., for storage in the registry). This function, which
 * can call other copy functions to build up complex data types, defines
 * the method for making a copy of the specified data type.
 *
 * @param **dest The address of a pointer into which the
 * address of the resulting data is to be stored.
 *
 * @param *src A pointer to the memory location from which the
 * data is to be copied.
 *
 * @param type The type of the data to be copied - must be one of
 * the DSS defined data types.
 *
 * @retval ORTE_SUCCESS The value was successfully copied.
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 *
 */
typedef int (*orte_dss_copy_fn_t)(void **dest, void *src, orte_data_type_t type);

/**
 * Compare two data values.
 *
 * Since registered data types can be complex structures, the system
 * needs some way to know how to compare two data values (e.g., when
 * trying to order them in some fashion). This function, which
 * can call other compare functions to build up complex data types, defines
 * the method for comparing two values of the specified data type.
 *
 * @retval -1 Indicates first value is greater than second value
 * @retval 0 Indicates two values are equal
 * @retval +1 Indicates second value is greater than first value
 */
typedef int (*orte_dss_compare_fn_t)(void *value1, void *value2,
                                     orte_data_type_t type);


/**
 * Compute size of data value.
 *
 * Since registered data types can be complex structures, the system
 * needs some way to compute its size. Some of these types, however, involve
 * variable amounts of storage (e.g., a string!). Hence, a pointer to the
 * actual object being "sized" needs to be passed as well.
 *
 * @param size Address of a size_t value where the size of the data value
 * (in bytes) will be stored - set to zero in event of error.
 *
 * @param *src A pointer to the memory location of the data object. It is okay
 * for this to be NULL - if NULL, the function must return the size of the object
 * itself, not including any data contained in its fields.
 *
 * @param type The type of the data value - must be one of
 * the DSS defined data types or an error will be returned.
 *
 * @retval ORTE_SUCCESS The value was successfully copied.
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_size_fn_t)(size_t *size, void *src, orte_data_type_t type);


/**
 * Print a data value.
 *
 * Since registered data types can be complex structures, the system
 * needs some way to know how to print them (i.e., convert them to a string
 * representation).
 *
 * @retval ORTE_SUCCESS The value was successfully printed.
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_print_fn_t)(char **output, char *prefix, void *src, orte_data_type_t type);


/**
 * Print a data value to an output stream for debugging purposes
 *
 * Uses the dss.print command to obtain a string version of the data value
 * and prints it to the designated output stream.
 *
 * @retval ORTE_SUCCESS The value was successfully printed.
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_dump_fn_t)(int output_stream, void *src, orte_data_type_t type);

/**
 * Set a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can set the data value to a specific value. This
 * is the equivalent to a C++ access function.
 *
 * NOTE: this function does NOT allocate any memory. It only sets the value pointer
 * and type to the specified location and type. Use "copy" if you want dynamic allocation
 * of storage.
 *
 * @retval ORTE_SUCCESS The value was successfully stored
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_set_fn_t)(orte_data_value_t *value, void *new_value, orte_data_type_t type);

/**
 * Get a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can get the data value from within the data_value object. This
 * is the equivalent to a C++ access function.
 *
 * NOTE: this function does NOT allocate any memory. It simply points the "data" location
 * to that of the value, after ensuring that the value's type matches the specified one.
 * Use "copy" if you want dynamic allocation of memory.
 *
 * @retval ORTE_SUCCESS The value was successfully retrieved
 *
 * @retval ORTE_ERROR(s) An appropriate error code - usually caused by the specified type
 * not matching the data type within the stored object.
 */
typedef int (*orte_dss_get_fn_t)(void **data, orte_data_value_t *value, orte_data_type_t type);

/**
 * Perform an arithemetic operation on a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can manipulate the data value within the data_value object. This
 * is the equivalent to a C++ access function.
 *
 * @retval ORTE_SUCCESS The value was successfully retrieved
 *
 * @retval ORTE_ERROR(s) An appropriate error code - usually caused by the specified type
 * not matching the data type within the stored object.
 */
typedef int (*orte_dss_arith_fn_t)(orte_data_value_t *value, orte_data_value_t *operand, orte_dss_arith_op_t operation);

/**
 * Increment a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can manipulate the data value within the data_value object. This
 * is the equivalent to a C++ access function.
 *
 * @retval ORTE_SUCCESS The value was successfully retrieved
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_increment_fn_t)(orte_data_value_t *value);

/**
 * Decrement a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can manipulate the data value within the data_value object. This
 * is the equivalent to a C++ access function.
 *
 * @retval ORTE_SUCCESS The value was successfully retrieved
 *
 * @retval ORTE_ERROR(s) An appropriate error code.
 */
typedef int (*orte_dss_decrement_fn_t)(orte_data_value_t *value);

/**
 * Release the storage used by a data value
 *
 * Since the data values are stored in an opaque manner, the system needs
 * a function by which it can release the storage associated with a value
 * stored in a data value object.
 */
typedef void (*orte_dss_release_fn_t)(orte_data_value_t *value);

/**
 * Register a set of data handling functions.
 *
 *  * This function registers a set of data type functions for a specific
 * type.  An integer is returned that should be used a an argument to
 * future invocations of orte_dss.pack(), orte_dss.unpack(), orte_dss.copy(),
 * and orte_dss.compare, which
 * will trigger calls to the appropriate functions.  This
 * is most useful when extending the datatypes that the dss can
 * handle; pack and unpack functions can nest calls to orte_dss.pack()
 * / orte_dss.unpack(), so defining small pack/unpack functions can be
 * used recursively to build larger types (e.g., packing/unpacking
 * structs can use calls to orte_dss.pack()/unpack() to serialize /
 * deserialize individual members). This is likewise true for the copy
 * and compare functions.
 *
 * @param release_fn [IN] Function pointer to the release routine
 * @param pack_fn [IN] Function pointer to the pack routine
 * @param unpack_fn [IN] Function pointer to the unpack routine
 * @param copy_fn [IN] Function pointer to copy routine
 * @param compare_fn [IN] Function pointer to compare routine
 * @param size_fn [IN] Function pointer to size routine
 * @param print_fn [IN] Function pointer to print routine
 * @param structured [IN] Boolean indicator as to whether or not the data is structured. A true
 * value indicates that this data type is always passed via reference (i.e., a pointer to the
 * object is passed) as opposed to directly (e.g., the way an int32_t would appear)
 * @param name [IN] String name for this pair (mainly for debugging)
 * @param type [OUT] Type number for this registration
 *
 * @returns ORTE_SUCCESS upon success
 *
 */
typedef int (*orte_dss_register_fn_t)(orte_dss_pack_fn_t pack_fn,
                                    orte_dss_unpack_fn_t unpack_fn,
                                    orte_dss_copy_fn_t copy_fn,
                                    orte_dss_compare_fn_t compare_fn,
                                    orte_dss_size_fn_t size_fn,
                                    orte_dss_print_fn_t print_fn,
                                    orte_dss_release_fn_t release_fn,
                                    bool structured,
                                    const char *name, orte_data_type_t *type);
/*
 * This function looks up the string name corresponding to the identified
 * data type - used for debugging messages.
 */
typedef char* (*orte_dss_lookup_data_type_fn_t)(orte_data_type_t type);

/*
 * Dump the data type list - used for debugging to see what has been registered
 */
typedef void (*orte_dss_dump_data_types_fn_t)(int output);

/**
 * Base structure for the DSS
 *
 * Base module structure for the DSS - presents the required function
 * pointers to the calling interface.
 */
struct orte_dss_t {
    orte_dss_set_fn_t 				set;
    orte_dss_get_fn_t 				get;
    orte_dss_arith_fn_t 			arith;
    orte_dss_increment_fn_t 		increment;
    orte_dss_decrement_fn_t 		decrement;
    orte_dss_set_buffer_type_fn_t 	set_buffer_type;
    orte_dss_pack_fn_t 				pack;
    orte_dss_unpack_fn_t 			unpack;
    orte_dss_copy_fn_t 				copy;
    orte_dss_compare_fn_t 			compare;
    orte_dss_size_fn_t 				size;
    orte_dss_print_fn_t 			print;
    orte_dss_release_fn_t 			release;
    orte_dss_peek_next_item_fn_t 	peek;
    orte_dss_unload_fn_t 			unload;
    orte_dss_load_fn_t 				load;
    orte_dss_xfer_payload_fn_t      xfer_payload;
    orte_dss_copy_payload_fn_t      copy_payload;
    orte_dss_register_fn_t 			register_type;
    orte_dss_lookup_data_type_fn_t 	lookup_data_type;
    orte_dss_dump_data_types_fn_t 	dump_data_types;
    orte_dss_dump_fn_t				dump;
};
typedef struct orte_dss_t orte_dss_t;

ORTE_DECLSPEC extern orte_dss_t orte_dss;  /* holds dss function pointers */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* ORTE_DSS_H */
