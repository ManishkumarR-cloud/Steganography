#ifndef TYPES_H
#define TYPES_H

/* User defined types */
typedef unsigned int uint;

/* Status will be used in fn. return type */
typedef enum
{
    e_failure,  //0
    e_success   //1
} Status;

typedef enum
{
    e_encode,  //0
    e_decode,  //1
    e_unsupported  //2
} OperationType;

#endif
