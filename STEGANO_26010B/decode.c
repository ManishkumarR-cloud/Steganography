#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"
/* Validate decode arguments */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    // Step 1: Check stego image (.bmp compulsory)
    if (argv[2] != NULL && strstr(argv[2], ".bmp") != NULL)
    {
        decInfo->stego_image_fname = argv[2];
    }
    else
    {
        printf("ERROR: Stego image must be a .bmp file\n");
        return e_failure;
    }

    // Step 2: Output file name (without extension, default if missing)
    if (argv[3] != NULL)
    {
        decInfo->output_fname = argv[3];
    }
    else
    {
        decInfo->output_fname = "decoded_secret";
    }

    return e_success;
}

/* Helper: decode one byte from 8 LSBs */
char decode_byte_from_lsb(char *image_buffer)
{
    char decoded_byte = 0;

    for (int i = 0; i < 8; i++)
    {
        char bit = image_buffer[i] & 0x01;
        decoded_byte |= (bit << (7 - i));
    }

    return decoded_byte;
}


/* Helper: decode 32 bits (size) from LSBs */
int decode_size_from_lsb(char *image_buffer)
{
    int decoded_size = 0;

    for (int i = 0; i < 32; i++)
    {
        char bit = image_buffer[i] & 0x01;
        decoded_size |= (bit << (31 - i));
    }

    return decoded_size;
}

/* Decode the magic string */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)
{
    char buffer[8];
    char decoded_char;

    // Skip BMP header (54 bytes)
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    for (int i = 0; i < strlen(magic_string); i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read image data for magic string\n");
            return e_failure;
        }

        decoded_char = decode_byte_from_lsb(buffer);

        if (decoded_char != magic_string[i])
        {
            printf("ERROR: Magic string mismatch at position %d\n", i);
            return e_failure;
        }
    }

    printf("INFO: Magic string decoded successfully\n");
    return e_success;
}

/* Decode the size of the secret file extension */
Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo)
{
    char buffer[32];

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unable to read image data for extension size\n");
        return e_failure;
    }

    *size = decode_size_from_lsb(buffer);
    printf("INFO: Extension size decoded = %d\n", *size);

    return e_success;
}

/* Decode the secret file extension */
Status decode_secret_file_extn(char *file_extn, int extn_size, DecodeInfo *decInfo)
{
    char buffer[8];
    char decoded_char;

    for (int i = 0; i < extn_size; i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read image data for extension\n");
            return e_failure;
        }

        decoded_char = decode_byte_from_lsb(buffer);
        file_extn[i] = decoded_char;
    }

    file_extn[extn_size] = '\0';
    printf("INFO: Secret file extension decoded = %s\n", file_extn);
    return e_success;
}

//* Decode the size of the secret file (in bytes) */
Status decode_secret_file_size(long *file_size, DecodeInfo *decInfo)
{
    char buffer[32];

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unable to read image data for secret file size\n");
        return e_failure;
    }

    *file_size = (long)decode_size_from_lsb(buffer);
    printf("INFO: Secret file size decoded = %ld bytes\n", *file_size);

    return e_success;
}



/* Decode the actual secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char buffer[8];       // 8 image bytes per secret byte
    char decoded_char;

    // Loop through each byte of the secret file
    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        // Read 8 bytes from stego image
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read image data for secret file byte %ld\n", i);
            return e_failure;
        }

        // Decode one byte from 8 LSBs
        decoded_char = decode_byte_from_lsb(buffer);

        // Write to output file
        fputc(decoded_char, decInfo->fptr_output);
    }

    printf("INFO: Secret file data decoded successfully\n");
    return e_success;
}




/* Main decoding driver */
Status do_decoding(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Unable to open stego image %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    if (decode_magic_string(MAGIC_STRING, decInfo) == e_failure)
    {
        printf("ERROR: Magic string mismatch\n");
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    int extn_size;
    if (decode_secret_file_extn_size(&extn_size, decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    char file_extn[extn_size + 1];
    if (decode_secret_file_extn(file_extn, extn_size, decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    char output_fname[50];
    sprintf(output_fname, "%s.%s", decInfo->output_fname, file_extn);
    decInfo->fptr_output = fopen(output_fname, "w");
    if (decInfo->fptr_output == NULL)
    {
        printf("ERROR: Unable to create output file %s\n", output_fname);
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    long file_size;
    if (decode_secret_file_size(&file_size, decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }
    decInfo->size_secret_file = file_size;

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_output);

    printf("INFO: Decoding completed successfully!\n");
    return e_success;
}
