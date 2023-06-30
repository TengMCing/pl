//
// Created by Patrick Li on 29/6/2023.
//

#ifndef PL_PL_OPTIONAL_H
#define PL_PL_OPTIONAL_H

#include "pl_error.h"

/*-----------------------------------------------------------------------------
 |  Optional types
 ----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 |  Standard optional types
 ----------------------------------------------------------------------------*/

typedef struct pl_optional_void
{
 const pl_error error;
} pl_optional_void;

typedef struct pl_optional_char
{
    const char value;
    const pl_error error;
} pl_optional_char;

typedef struct pl_optional_int
{
    const int value;
    const pl_error error;
} pl_optional_int;

typedef struct pl_optional_double
{
    const double value;
    const pl_error error;
} pl_optional_double;

typedef struct pl_optional_char_p
{
    char *const value;
    const pl_error error;
} pl_optional_char_p;

typedef struct pl_optional_int_p
{
    int *const value;
    const pl_error error;
} pl_optional_int_p;

typedef struct pl_optional_double_p
{
    double *const value;
    const pl_error error;
} pl_optional_double_p;

typedef struct pl_optional_void_p
{
    void *const value;
    const pl_error error;
} pl_optional_void_p;

typedef struct pl_optional_const_char_p
{
    const char *const value;
    const pl_error error;
} pl_optional_const_char_p;

typedef struct pl_optional_const_int_p
{
    const int *const value;
    const pl_error error;
} pl_optional_const_int_p;

typedef struct pl_optional_const_double_p
{
    const double *const value;
    const pl_error error;
} pl_optional_const_double_p;

typedef struct pl_optional_const_void_p
{
    const void *const value;
    const pl_error error;
} pl_optional_const_void_p;


#endif//PL_PL_OPTIONAL_H
