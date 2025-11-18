// types.h
#ifndef TYPES_H
#define TYPES_H

/* User defined types */
typedef unsigned int uint;

/* Status will be used in fn. return type */
typedef enum
{
    failure,
    success
} Status;

/* Operation type for CLI selection */
typedef enum
{
    encode,
    decode,
    unsupported
} OperationType;

#endif // TYPES_H
