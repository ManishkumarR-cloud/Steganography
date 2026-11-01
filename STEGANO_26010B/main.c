/*Manish kumar R */
#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

/* Function prototype */
OperationType check_operation_type(char *);

/* Driver function */
int main(int argc, char *argv[])
{
    // Step 1: Check if enough arguments are passed
    if (argc < 2)
    {
        printf("Usage:\n");
        printf("  Encode: %s -e <src_image.bmp> <secret.txt> <stego_image.bmp>\n", argv[0]);
        printf("  Decode: %s -d <stego_image.bmp> <output_base_name>\n", argv[0]);
        return e_failure;
    }

    // Step 2: Identify the operation type (encode/decode)
    OperationType op = check_operation_type(argv[1]);

    switch (op)
    {
        case e_encode:
        {
            EncodeInfo encInfo;

            if (read_and_validate_encode_args(argv, &encInfo) == e_success)
            {
                printf("INFO: Arguments validated successfully.\n");

                if (do_encoding(&encInfo) == e_success)
                {
                    printf("INFO: Encoding completed successfully!\n");
                    return e_success;
                }
                else
                {
                    printf("ERROR: Encoding failed.\n");
                    return e_failure;
                }
            }
            else
            {
                printf("ERROR: Invalid arguments for encoding.\n");
                return e_failure;
            }
        }

        case e_decode:
        {
            DecodeInfo decInfo;

            if (read_and_validate_decode_args(argv, &decInfo) == e_success)
            {
                printf("INFO: Arguments validated successfully.\n");

                if (do_decoding(&decInfo) == e_success)
                {
                    printf("INFO: Decoding completed successfully!\n");
                    return e_success;
                }
                else
                {
                    printf("ERROR: Decoding failed.\n");
                    return e_failure;
                }
            }
            else
            {
                printf("ERROR: Invalid arguments for decoding.\n");
                return e_failure;
            }
        }

        default:
            printf("ERROR: Unsupported operation. Use -e for encode or -d for decode.\n");
            return e_failure;
    }
}

/* Function to check operation type based on symbol */
OperationType check_operation_type(char *symbol)
{
    if (strcmp(symbol, "-e") == 0)
        return e_encode;
    else if (strcmp(symbol, "-d") == 0)
        return e_decode;
    else
        return e_unsupported;
}
