//
// Created by Patrick Li on 1/7/2023.
//


#ifndef PL_PL_CLASS_H
#define PL_PL_CLASS_H

#include "stddef.h"

/*-----------------------------------------------------------------------------
 |  Class metadata
 ----------------------------------------------------------------------------*/

/// Total number of classes.
#define PL_NUM_CLASS 6

/// Enum of classes.
/// @details Classes should follow the format "PL_CLASS_{NAME}".
enum {
    PL_CLASS_CHAR = 0,
    PL_CLASS_INT = 1,
    PL_CLASS_LONG = 2,
    PL_CLASS_DOUBLE = 3,
    PL_CLASS_LIST = 4,
    PL_CLASS_EXTERNAL = 5
};

/// Class names.
static const char *const PL_CLASS_NAME[PL_NUM_CLASS] = {
        [PL_CLASS_CHAR]     = "CHAR",
        [PL_CLASS_INT]      = "INT",
        [PL_CLASS_LONG]     = "LONG",
        [PL_CLASS_DOUBLE]   = "DOUBLE",
        [PL_CLASS_LIST]     = "LIST",
        [PL_CLASS_EXTERNAL] = "EXTERNAL"};

/// Element size of each class.
static const size_t PL_CLASS_ELEMENT_SIZE[PL_NUM_CLASS] = {
        [PL_CLASS_CHAR]     = sizeof(char),
        [PL_CLASS_INT]      = sizeof(int),
        [PL_CLASS_LONG]     = sizeof(long),
        [PL_CLASS_DOUBLE]   = sizeof(double),
        [PL_CLASS_LIST]     = sizeof(void *),
        [PL_CLASS_EXTERNAL] = sizeof(void *)};

/// Parent of each class.
static const int PL_CLASS_INHERIT[PL_NUM_CLASS] = {
        [PL_CLASS_CHAR]     = -1,
        [PL_CLASS_INT]      = -1,
        [PL_CLASS_LONG]     = -1,
        [PL_CLASS_DOUBLE]   = -1,
        [PL_CLASS_LIST]     = -1,
        [PL_CLASS_EXTERNAL] = -1};

/*-----------------------------------------------------------------------------
 |  Class namespace
 ----------------------------------------------------------------------------*/

/// Namespace of class.
typedef struct pl_class_ns {
    /// Check if one class is inherited from another class.
    /// @param derived (int). The derived class.
    /// @param base (int). The base class.
    /// @return 0 or 1.
    int (*const inherit)(int derived, int base);

    /// Get the underlying type of a class.
    /// @param derived (int). The derived class.
    /// @return The base class.
    int (*const type)(int derived);

#ifdef PL_TEST

    /// Test the namespace.
    void (*const test)(void);

#endif//PL_TEST

} pl_class_ns;

/// Get namespace of class.
/// @return Namespace of class.
pl_class_ns pl_class_get_ns(void);

#endif//PL_PL_CLASS_H
