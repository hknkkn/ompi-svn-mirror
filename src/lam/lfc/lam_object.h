/*
 * $HEADER$
 */

/**
 * @file:
 *
 * A simple C-language object-oriented system with single inheritance
 * and ownership-based memory management using a retain/release model.
 *
 * A class consists of a struct and singly-instantiated class
 * descriptor.  The first element of the struct must be the parent
 * class's struct.  The class descriptor must be given a well-known
 * name based upon the class struct name (if the struct is sally_t,
 * the class descriptor should be sally_t_class) and must be
 * statically initialized as discussed below.
 *
 * (a) To define a class
 *
 * In a interface (.h) file, define the class.  The first element
 * should always be the parent class, for example
 * @code
 *   typedef struct sally_t sally_t;
 *   struct sally_t
 *   {
 *     parent_t parent;
 *     void *first_member;
 *     ...
 *   };
 *
 *   OBJ_CLASS_DECLARATION(sally_t_class);
 * @endcode
 * All classes must have a parent which is also class.
 * 
 * In an implementation (.c) file, instantiate a class descriptor for
 * the class like this:
 * @code
 *   OBJ_CLASS_INSTANCE(sally_t, parent_t, sally_construct, sally_destruct);
 * @endcode
 * This macro actually expands to 
 * @code
 *   lam_class_t sally_t_class = {
 *     "sally_t",
 *     OBJ_CLASS(parent_t),  // pointer to parent_t_class
 *     sally_construct,
 *     sally_destruct,
 *     0, 0, NULL, NULL
 *   };
 * @endcode
 * This variable should be declared in the interface (.h) file using
 * the OBJ_CLASS_DECLARATION macro as shown above.
 *
 * sally_construct, and sally_destruct are function pointers to the
 * constructor and destructor for the class and are best defined as
 * static functions in the implementation file.  NULL pointers maybe
 * supplied instead.
 *
 * Other class methods may be added to the struct.
 *
 * (b) Class instantiation: dynamic
 *
 * To create a instance of a class (an object) use OBJ_NEW:
 * @code
 *   sally_t *sally = OBJ_NEW(sally_t);
 * @endcode
 * which allocates memory of sizeof(sally_t) and runs the class's
 * constructors.
 *
 * Use OBJ_RETAIN, OBJ_RELEASE to do reference-count-based
 * memory management:
 * @code
 *   OBJ_RETAIN(sally);
 *   OBJ_RELEASE(sally);
 *   OBJ_RELEASE(sally);
 * @endcode
 * When the reference count reaches zero, the class's "fini" method
 * is run and the memory is freed.
 *
 * N.B. There is no explicit free/delete method for dynamic objects in
 * this model.
 *
 * (c) Class instantiation: static
 *
 * For an object with static (or stack) allocation, it is only
 * necessary to initialize the memory, which is done using
 * OBJ_CONSTRUCT:
 * @code
 *   sally_t sally;
 *
 *   OBJ_CONSTRUCT(&sally, sally_t);
 * @endcode
 * The retain/release model is not necessary here, but before the
 * object goes out of scope, OBJ_DESTRUCT should be run to release
 * initialized resources:
 * @code
 *   OBJ_DESTRUCT(&sally);
 * @endcode
 */

#ifndef LAM_OBJECT_H
#define LAM_OBJECT_H

#include <assert.h>
#include <stdlib.h>

#include "lam_config.h"
#include "lam/types.h"
#include "lam/atomic.h"

/*
 * BEGIN_C_DECLS should be used at the beginning of your declarations,
 * so that C++ compilers don't mangle their names.  Use END_C_DECLS at
 * the end of C declarations.
 */
#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
# define BEGIN_C_DECLS extern "C" {
# define END_C_DECLS }
#else
# define BEGIN_C_DECLS /* empty */
# define END_C_DECLS /* empty */
#endif


/* macros ************************************************************/

/**
 * Return a pointer to the class descriptor associated with a
 * class type.
 *
 * @param NAME          Name of class
 * @return              Pointer to class descriptor
 */
#define OBJ_CLASS(NAME)     (&(NAME ## _class))


/**
 * Static initializer for a class descriptor
 *
 * @param NAME          Name of class
 * @param PARENT        Name of parent class
 * @param CONSTRUCTOR   Pointer to constructor
 * @param DESTRUCTOR    Pointer to destructor
 *
 * Put this in NAME.c
 */
#define OBJ_CLASS_INSTANCE(NAME, PARENT, CONSTRUCTOR, DESTRUCTOR)       \
    lam_class_t NAME ## _class = {                                      \
        # NAME,                                                         \
        OBJ_CLASS(PARENT),                                              \
        (lam_construct_t)CONSTRUCTOR,                                   \
        (lam_destruct_t)DESTRUCTOR,                                     \
        0, 0, NULL, NULL                                                \
    }


/**
 * Declaration for class descriptor
 *
 * @param NAME          Name of class
 *
 * Put this in NAME.h
 */
#define OBJ_CLASS_DECLARATION(NAME) \
    extern lam_class_t NAME ## _class


/**
 * Create an object: dynamically allocate storage and run the class
 * constructor.
 *
 * @param type          Type (class) of the object
 * @return              Pointer to the object 
 */
#define OBJ_NEW(type)                                   \
    ((type *) lam_obj_new(sizeof(type), OBJ_CLASS(type)))


/**
 * Retain an object (by incrementing its reference count)
 *
 * @param object        Pointer to the object
 */
#define OBJ_RETAIN(object)                              \
    do {                                                \
        if (object) {                                   \
            lam_obj_retain((lam_object_t *) object);    \
        }                                               \
    } while (0)


/**
 * Release an object (by decrementing its reference count).  If the
 * reference count reaches zero, destruct (finalize) the object and
 * free its storage.
 *
 * @param object        Pointer to the object
 */
#define OBJ_RELEASE(object)                             \
    do {                                                \
        if (object) {                                   \
            lam_obj_release((lam_object_t *) object);   \
        }                                               \
    } while (0)


/**
 * Construct (initialize) objects that are not dynamically allocated.
 *
 * @param object        Pointer to the object
 * @param type          The object type
 */
#define OBJ_CONSTRUCT(object, type)                                     \
    do {                                                                \
        if (0 == OBJ_CLASS(type)->cls_initialized) {                    \
            lam_class_initialize(OBJ_CLASS(type));                      \
        }                                                               \
        if (object) {                                                   \
            ((lam_object_t *) object)->obj_class = OBJ_CLASS(type);     \
            ((lam_object_t *) object)->obj_reference_count = 1;         \
            lam_obj_run_constructors((lam_object_t *) object);          \
        }                                                               \
    } while (0)


/**
 * Destruct (finalize) an object that is not dynamically allocated.
 *
 * @param object        Pointer to the object
 */
#define OBJ_DESTRUCT(object)                                    \
    do {                                                        \
        if (object) {                                           \
            lam_obj_run_destructors((lam_object_t *) object);   \
        }                                                       \
    } while (0)


/* typedefs ***********************************************************/

typedef struct lam_object_t lam_object_t;
typedef struct lam_class_t lam_class_t;
typedef void (*lam_construct_t) (lam_object_t *);
typedef void (*lam_destruct_t) (lam_object_t *);


/* types **************************************************************/

/**
 * Class descriptor.
 *
 * There should be a single instance of this descriptor for each class
 * definition.
 */
struct lam_class_t {
    const char *cls_name;          /**< symbolic name for class */
    lam_class_t *cls_parent;       /**< parent class descriptor */
    lam_construct_t cls_construct; /**< class constructor */
    lam_destruct_t cls_destruct;   /**< class destructor */
    int cls_initialized;           /**< is class initialized */
    int cls_depth;                 /**< depth of class hierarchy tree */
    lam_construct_t *cls_construct_array;
                                   /**< array of parent class constructors */
    lam_destruct_t *cls_destruct_array;
                                   /**< array of parent class destructors */
};


/**
 * Base object.
 *
 * This is special and does not follow the pattern for other classes.
 */
struct lam_object_t {
    lam_class_t *obj_class;        /**< class descriptor */
    int obj_reference_count;       /**< reference count */
};

OBJ_CLASS_DECLARATION(lam_object_t);


/* declarations *******************************************************/

BEGIN_C_DECLS

/**
 * Lazy initialization of class descriptor.
 *
 * Specifically cache arrays of function pointers for the constructor
 * and destructor hierarchies for this class.
 *
 * @param class    Pointer to class descriptor
 */
void lam_class_initialize(lam_class_t *);

END_C_DECLS

/**
 * Run the hierarchy of class constructors for this object, in a
 * parent-first order.
 *
 * Do not use this function directly: use OBJ_CONSTRUCT() instead.
 *
 * WARNING: This implementation relies on a hardwired maximum depth of
 * the inheritance tree!!!
 *
 * Hardwired for fairly shallow inheritance trees
 * @param size          Pointer to the object.
 */
static inline void lam_obj_run_constructors(lam_object_t *object)
{
    lam_class_t *cls;
    int i;

    assert(NULL != object);
    assert(NULL != object->obj_class);

    cls = object->obj_class;
    for (i = cls->cls_depth - 1; i >= 0; i--) {
        if (cls->cls_construct_array[i]) {
            (cls->cls_construct_array[i])(object);
        }
    }
}


/**
 * Run the hierarchy of class destructors for this object, in a
 * parent-last order.
 *
 * Do not use this function directly: use OBJ_DESTRUCT() instead.
 *
 * @param size          Pointer to the object.
 */
static inline void lam_obj_run_destructors(lam_object_t *object)
{
    lam_class_t *cls;
    int i;

    assert(NULL != object);
    assert(NULL != object->obj_class);

    cls = object->obj_class;
    for (i = 0; i < cls->cls_depth; i++) {
        if (cls->cls_destruct_array[i]) {
            (cls->cls_destruct_array[i])(object);
        }
    }
}


/**
 * Create new object: dynamically allocate storage and run the class
 * constructor.
 *
 * Do not use this function directly: use OBJ_NEW() instead.
 *
 * @param size          Size of the object
 * @param cls           Pointer to the class descriptor of this object
 * @return              Pointer to the object 
 */
static inline lam_object_t *lam_obj_new(size_t size,
                                        lam_class_t *cls)
{
    lam_object_t *object;

    assert(size >= sizeof(lam_object_t));

    object = (lam_object_t *) malloc(size);
    if (0 == cls->cls_initialized) {
        lam_class_initialize(cls);
    }
    if (NULL != object) {
	object->obj_class = cls;
        object->obj_reference_count = 1;
        lam_obj_run_constructors(object);
    }
    return object;
}


/*
 * This function is used by inline functions later in this file, and
 * it must be defined by other header files later (eg., one of the
 * atomic.h's).
 */
static inline int fetchNadd(volatile int *addr, int inc);


/**
 * Retain an object (by incrementing its reference count)
 *
 * Do not use this function directly: use OBJ_RETAIN instead.
 *
 * @param object        Pointer to the object
 */
static inline void lam_obj_retain(lam_object_t *object)
{
    assert(NULL != object);
    assert(NULL != object->obj_class);

    fetchNadd(&(object->obj_reference_count), 1);

    assert(object->obj_reference_count >= 0);
}


/**
 * Release an object (by decrementing its reference count).  If the
 * reference count reaches zero, destruct (finalize) the object and
 * free its storage.
 *
 * Do not use this function directly: use OBJ_RELEASE instead.
 *
 * @param object        Pointer to the object
 */
static inline void lam_obj_release(lam_object_t *object)
{
    assert(NULL != object);
    assert(NULL != object->obj_class);

    if (fetchNadd(&object->obj_reference_count, -1) == 1) {
	object->obj_class->cls_destruct(object);
	free(object);
    }
}

/**********************************************************************/

#endif				/* LAM_OBJECT_H */
