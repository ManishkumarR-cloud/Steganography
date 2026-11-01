#ifndef DECODE_H
#define DECODE_H

#include "types.h"   // for Status, OperationType

/* Structure to hold decoding information */
typedef struct _DecodeInfo
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *output_fname;
    FILE *fptr_output;

    char extn_secret_file[10];  // extension of secret file
    long size_secret_file;      // size of secret file
} DecodeInfo;

/* Function prototypes */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);

Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);
char decode_byte_from_lsb(char *image_buffer);
int decode_size_from_lsb(char *image_buffer);

Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo);
Status decode_secret_file_extn(char *file_extn, int extn_size, DecodeInfo *decInfo);
Status decode_secret_file_size(long *file_size, DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);

#endif
