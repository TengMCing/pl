//
// Created by Patrick Li on 1/7/2023.
//

#ifndef PL_PL_VECTOR_H
#define PL_PL_VECTOR_H

#include "pl_class.h"
#include "pl_misc.h"

#define PL_TYPE_NUM 4

typedef enum pl_type
{
    PL_TYPE_CHAR   = 0,
    PL_TYPE_INT    = 1,
    PL_TYPE_DOUBLE = 2,
    PL_TYPE_ANY    = 3
} pl_type;

typedef struct pl_name
{
    const int length;
    const char *const data;
} pl_name;

typedef struct pl_char_vector
{
    const pl_class class;
    const pl_type type;
    pl_name name;
    int capacity;
    int length;
    char *data;
} pl_char_vector;

typedef struct pl_int_vector
{
    const pl_class class;
    const pl_type type;
    pl_name name;
    int capacity;
    int length;
    int *data;
} pl_int_vector;

typedef struct pl_double_vector
{
    const pl_class class;
    const pl_type type;
    pl_name name;
    int capacity;
    int length;
    double *data;
} pl_double_vector;

typedef struct pl_any_vector
{
    const pl_class class;
    const pl_type type;
    pl_name name;
    int capacity;
    int length;
    void *data;
} pl_any_vector;

typedef struct pl_vector_ns
{
    struct pl_vector_name_ns
    {
        pl_name (*const new)(const char *string);
        pl_name (*const duplicate)(pl_name name);
        pl_name (*const get)(void *v);
        void (*const update)(void *v, pl_name name);
    } name;

    struct pl_vector_char_ns
    {
        pl_char_vector (*const new)(int capacity);
        pl_char_vector (*const new_from_array)(int capacity, const char *array);
        pl_char_vector (*const new_from_variadic)(int capacity, ...);

        void (*const reserve)(pl_char_vector v, int capacity);
        char (*const get)(pl_char_vector v, int index);
        void (*const assign)(pl_char_vector v, int index, char item);
        void (*const append)(pl_char_vector v, char item);
        void (*const extend)(pl_char_vector v1, pl_char_vector v2);
        pl_char_vector (*const copy)(pl_char_vector v);
        pl_char_vector (*const subset)(pl_char_vector v, int start, int end);

        pl_int_vector (*const as_int)(pl_char_vector v);
        pl_double_vector (*const as_double)(pl_char_vector v);
        pl_any_vector (*const as_any)(pl_char_vector v);
    } char_;

    struct pl_vector_int_ns
    {
        pl_int_vector (*const new)(int capacity);
        pl_int_vector (*const new_from_array)(int capacity, const char *array);
        pl_int_vector (*const new_from_variadic)(int capacity, ...);

        void (*const reserve)(pl_int_vector v, int capacity);
        char (*const get)(pl_int_vector v, int index);
        void (*const assign)(pl_int_vector v, int index, int item);
        void (*const append)(pl_int_vector v, int item);
        void (*const extend)(pl_int_vector v1, pl_int_vector v2);
        pl_int_vector (*const copy)(pl_int_vector v);
        pl_int_vector (*const subset)(pl_int_vector v, int start, int end);

        pl_char_vector (*const as_char)(pl_int_vector v);
        pl_double_vector (*const as_double)(pl_int_vector v);
        pl_any_vector (*const as_any)(pl_int_vector v);
    } int_;

    struct pl_vector_double_ns
    {
        pl_double_vector (*const new)(int capacity);
        pl_double_vector (*const new_from_array)(int capacity, const char *array);
        pl_double_vector (*const new_from_variadic)(int capacity, ...);

        void (*const reserve)(pl_double_vector v, int capacity);
        char (*const get)(pl_double_vector v, int index);
        void (*const assign)(pl_double_vector v, int index, double item);
        void (*const append)(pl_double_vector v, double item);
        void (*const extend)(pl_double_vector v1, pl_double_vector v2);
        pl_double_vector (*const copy)(pl_double_vector v);
        pl_double_vector (*const subset)(pl_double_vector v, int start, int end);

        pl_char_vector (*const as_char)(pl_double_vector v);
        pl_int_vector (*const as_int)(pl_double_vector v);
        pl_any_vector (*const as_any)(pl_double_vector v);
    } double_;

    struct pl_vector_any_ns
    {
        pl_any_vector (*const new)(int capacity);
        pl_any_vector (*const new_from_array)(int capacity, const char *array);
        pl_any_vector (*const new_from_variadic)(int capacity, ...);

        void (*const reserve)(pl_any_vector v, int capacity);
        char (*const get)(pl_any_vector v, int index);
        void (*const assign)(pl_any_vector v, int index, ...);
        void (*const append)(pl_any_vector v, ...);
        void (*const extend)(pl_any_vector v1, pl_any_vector v2);
        pl_any_vector (*const copy)(pl_any_vector v);
        pl_any_vector (*const subset)(pl_any_vector v, int start, int end);

        pl_char_vector (*const as_char)(pl_any_vector v);
        pl_int_vector (*const as_int)(pl_any_vector v);
        pl_double_vector (*const as_double)(pl_any_vector v);
    } any;

} pl_vector_ns;


#endif//PL_PL_VECTOR_H
