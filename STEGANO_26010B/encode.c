#include <stdio.h>
#include "encode.h"
#include "types.h"
#include "common.h"
/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}




/*
 * Function: get_file_size
 * -----------------------
 * Finds the size of a file in bytes.
 *
 * Parameters:
 *   fptr - file pointer to the file
 *
 * Returns:
 *   size of the file in bytes (unsigned int)
 */
uint get_file_size(FILE *fptr)
{
    // Step 1: Move file pointer to the end
    fseek(fptr, 0, SEEK_END);

    // Step 2: Use ftell() to get current position (file size in bytes)
    uint size = ftell(fptr);

    // Step 3: Reset file pointer back to the beginning
    rewind(fptr);

    // Step 4: Return the file size
    return size;
}






/*
 * Function: read_and_validate_encode_args
 * ---------------------------------------
 * Validates the command-line arguments for encoding and
 * stores them into the EncodeInfo structure.
 *
 * Expected arguments:
 *   argv[2] -> source BMP image
 *   argv[3] -> secret file (with extension)
 *   argv[4] -> stego BMP image (optional, defaults to "stego.bmp")
 *
 * Returns:
 *   e_success if arguments are valid
 *   e_failure if validation fails
 */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    // Step 1: Validate source image (argv[2])
    if (argv[2] != NULL && strstr(argv[2], ".bmp") != NULL)
    {
        encInfo->src_image_fname = argv[2];
    }
    else
    {
        printf("ERROR: Source image must be a .bmp file.\n");
        return e_failure;
    }

    // Step 2: Validate secret file (argv[3])
    if (argv[3] != NULL)
    {
        

        // Extract extension of secret file
        char *dot = strrchr(argv[3], '.');  // find last dot
        if (dot != NULL)
        {
            encInfo->secret_fname = argv[3];
            strcpy(encInfo->extn_secret_file, dot); // store extension (skip dot)
        }
        else
        {
            printf("ERROR: Secret file must have an extension.\n");
            return e_failure;
        }
    }
    else
    {
        printf("ERROR: Secret file not provided.\n");
        return e_failure;
    }

    // Step 3: Validate stego image (argv[4])
    if (argv[4] != NULL)
    {
        if (strstr(argv[4], ".bmp") != NULL)
        {
            encInfo->stego_image_fname = argv[4];
        }
        else
        {
            printf("ERROR: Stego image must be a .bmp file.\n");
            return e_failure;
        }
    }
    else
    {
        // Default stego image name if not provided
        encInfo->stego_image_fname = "stego.bmp";
    }

    // Step 4: Return success after all validations
    return e_success;
}






Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

        return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

        return e_failure;
    }

    // No failure return e_success
    return e_success;
}






/*
 * Function: check_capacity
 * ------------------------
 * Checks if the source BMP image has enough capacity to store:
 *   - Magic string
 *   - Secret file extension size
 *   - Secret file extension
 *   - Secret file size
 *   - Secret file data
 *
 * Parameters:
 *   encInfo - pointer to EncodeInfo structure containing file pointers
 *             and secret file details
 *
 * Returns:
 *   e_success if image has enough capacity
 *   e_failure if not enough space
 */
Status check_capacity(EncodeInfo *encInfo)
{
    // Step 1: Calculate image capacity
    // Get total number of bytes available for encoding in the BMP
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    // Step 2: Get size of secret file
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    // Step 3: Calculate required bits for encoding
    // Magic string length * 8 bits
    int magic_bits = strlen(MAGIC_STRING) * 8;

    // Extension size (stored as 32 bits)
    int extn_size_bits = 32;

    // Extension characters * 8 bits
    int extn_bits = strlen(encInfo->extn_secret_file) * 8;

    // Secret file size (stored as 32 bits)
    int file_size_bits = 32;

    // Secret file data * 8 bits
    long secret_bits = encInfo->size_secret_file * 8;

    // Total required bits
    long total_required_bits = magic_bits + extn_size_bits + extn_bits + file_size_bits + secret_bits;

    // Step 4: Compare image capacity with required bits
    if (encInfo->image_capacity > total_required_bits)
    {
        // Enough capacity
        return e_success;
    }
    else
    {
        // Not enough capacity
        printf("ERROR: Image does not have enough capacity.\n");
        return e_failure;
    }
}





/*
 * Function: copy_bmp_header
 * -------------------------
 * Copies the 54-byte BMP header from the source image
 * to the destination (stego) image.
 *
 * Parameters:
 *   fptr_src_image  - file pointer to source BMP
 *   fptr_dest_image - file pointer to destination BMP
 *
 * Returns:
 *   e_success if header is copied correctly
 */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    // Step 1: Create a buffer to hold the BMP header (54 bytes)
    char header[54];

    // Step 2: Move source file pointer to the beginning
    rewind(fptr_src_image);

    // Step 3: Read 54 bytes (BMP header) from source image
    if (fread(header, 1, 54, fptr_src_image) != 54)
    {
        printf("ERROR: Failed to read BMP header.\n");
        return e_failure;
    }

    // Step 4: Write the same 54 bytes into destination image
    if (fwrite(header, 1, 54, fptr_dest_image) != 54)
    {
        printf("ERROR: Failed to write BMP header.\n");
        return e_failure;
    }

    // Step 5: Return success after copying header
    return e_success;
}






/*
 * Function: encode_magic_string
 * -----------------------------
 * Encodes the magic string (e.g., "#*") into the BMP image.
 * This acts as a marker to identify whether the image contains
 * hidden data during decoding.
 *
 * Parameters:
 *   magic_string - the string to be hidden (defined in common.h)
 *   encInfo      - pointer to EncodeInfo structure containing file pointers
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    // Step 1: Loop through each character of the magic string
    for (int i = 0; i < strlen(magic_string); i++)
    {
        char buffer[8];  // Temporary buffer to hold 8 bytes of image data

        // Step 2: Read 8 bytes from source image
        if (fread(buffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            printf("ERROR: Failed to read image data while encoding magic string.\n");
            return e_failure;
        }

        // Step 3: Encode one character of magic string into these 8 bytes
        encode_byte_to_lsb(magic_string[i], buffer);

        // Step 4: Write the modified 8 bytes into stego image
        if (fwrite(buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to write image data while encoding magic string.\n");
            return e_failure;
        }
    }

    // Step 5: Return success after encoding all characters
    return e_success;
}






/*
 * Function: encode_secret_file_extn_size
 * --------------------------------------
 * Encodes the size (length) of the secret file extension into the BMP image.
 * Example: ".txt" → length = 3 → stored as 32 bits.
 *
 * Parameters:
 *   size    - length of the secret file extension
 *   encInfo - pointer to EncodeInfo structure containing file pointers
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    // Step 1: Create a buffer to hold 32 bytes of image data
    char buffer[32];

    // Step 2: Read 32 bytes from source image
    if (fread(buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        printf("ERROR: Failed to read image data while encoding extension size.\n");
        return e_failure;
    }

    // Step 3: Encode the extension size into these 32 bytes
    encode_size_to_lsb(size, buffer);

    // Step 4: Write the modified 32 bytes into stego image
    if (fwrite(buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to write image data while encoding extension size.\n");
        return e_failure;
    }

    // Step 5: Return success after encoding extension size
    return e_success;
}







/*
 * Function: encode_secret_file_extn
 * ---------------------------------
 * Encodes the secret file extension (e.g., "txt") into the BMP image.
 * Each character of the extension is stored using LSB substitution.
 *
 * Parameters:
 *   file_extn - the extension string (without the dot)
 *   encInfo   - pointer to EncodeInfo structure containing file pointers
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    // Step 1: Loop through each character of the file extension
    for (int i = 0; i < strlen(file_extn); i++)
    {
        char buffer[8];  // Temporary buffer to hold 8 bytes of image data

        // Step 2: Read 8 bytes from source image
        if (fread(buffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            printf("ERROR: Failed to read image data while encoding file extension.\n");
            return e_failure;
        }

        // Step 3: Encode one character of extension into these 8 bytes
        encode_byte_to_lsb(file_extn[i], buffer);

        // Step 4: Write the modified 8 bytes into stego image
        if (fwrite(buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to write image data while encoding file extension.\n");
            return e_failure;
        }
    }

    // Step 5: Return success after encoding all extension characters
    return e_success;
}






/*
 * Function: encode_secret_file_size
 * ---------------------------------
 * Encodes the size of the secret file (in bytes) into the BMP image.
 * The size is stored as a 32-bit integer using LSB substitution.
 *
 * Parameters:
 *   file_size - size of the secret file in bytes
 *   encInfo   - pointer to EncodeInfo structure containing file pointers
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    // Step 1: Create a buffer to hold 32 bytes of image data
    char buffer[32];

    // Step 2: Read 32 bytes from source image
    if (fread(buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        printf("ERROR: Failed to read image data while encoding file size.\n");
        return e_failure;
    }

    // Step 3: Encode the file size into these 32 bytes
    encode_size_to_lsb((int)file_size, buffer);

    // Step 4: Write the modified 32 bytes into stego image
    if (fwrite(buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to write image data while encoding file size.\n");
        return e_failure;
    }

    // Step 5: Return success after encoding file size
    return e_success;
}







/*
 * Function: encode_secret_file_data
 * ---------------------------------
 * Reads the secret file data and encodes it into the BMP image
 * using LSB substitution.
 *
 * Parameters:
 *   encInfo - pointer to EncodeInfo structure containing file pointers
 *             and secret file details
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    // Step 1: Reset secret file pointer to start
    rewind(encInfo->fptr_secret);

    // Step 2: Read the entire secret file into encInfo->secret_data
    fread(encInfo->secret_data, 1, encInfo->size_secret_file, encInfo->fptr_secret);

    // Step 3: Loop through each character of the secret data
    for (long i = 0; i < encInfo->size_secret_file; i++)
    {
        char buffer[8];  // Temporary buffer for 8 bytes of image data

        // Step 4: Read 8 bytes from source image
        fread(buffer, 1, 8, encInfo->fptr_src_image);

        // Step 5: Encode one character into these 8 bytes
        encode_byte_to_lsb(encInfo->secret_data[i], buffer);

        // Step 6: Write the modified 8 bytes into stego image
        fwrite(buffer, 1, 8, encInfo->fptr_stego_image);
    }

    // Step 7: Return success after encoding all characters
    return e_success;
}







/*
 * Function: copy_remaining_img_data
 * ---------------------------------
 * Copies the remaining image data (after encoding is done)
 * from the source BMP to the destination (stego) BMP.
 *
 * Parameters:
 *   fptr_src  - file pointer to source BMP
 *   fptr_dest - file pointer to destination BMP
 *
 * Returns:
 *   e_success if copying is done
 */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    // Step 1: Create a buffer for reading chunks of data
    char ch;

    // Step 2: Read until end of source file
    while (fread(&ch, 1, 1, fptr_src) == 1)
    {
        // Step 3: Write each byte directly to destination file
        if (fwrite(&ch, 1, 1, fptr_dest) != 1)
        {
            printf("ERROR: Failed to write remaining image data.\n");
            return e_failure;
        }
    }

    // Step 4: Return success after copying all remaining data
    return e_success;
}







/* 
 * Function: encode_byte_to_lsb
 * ----------------------------
 * Encodes a single character (1 byte = 8 bits) into the 
 * least significant bits (LSBs) of 8 bytes of image data.
 *
 * Parameters:
 *   data         - the character to be hidden
 *   image_buffer - array of 8 bytes from the BMP image
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    // Loop through each bit of the character (8 bits total)
    for (int i = 0; i < 8; i++)
    {
        // Extract the i-th bit from 'data'
        // (Shift right and mask with 1)
        int bit = (data >> (7 - i)) & 1;

        // Clear the LSB of the current image byte (mask with 0xFE)
        // Then set it to the extracted bit
        image_buffer[i] = (image_buffer[i] & 0xFE) | bit;
    }

    // Return success after all 8 bits are encoded
    return e_success;
}








/*
 * Function: encode_size_to_lsb
 * ----------------------------
 * Encodes a 32-bit integer (size) into the least significant bits (LSBs)
 * of 32 bytes of image data.
 *
 * Parameters:
 *   size         - the integer value to be hidden (e.g., file size, extn length)
 *   imageBuffer  - array of 32 bytes from the BMP image
 *
 * Returns:
 *   e_success if encoding is done
 */
Status encode_size_to_lsb(int size, char *imageBuffer)
{
    // Loop through each bit of the integer (32 bits total)
    for (int i = 0; i < 32; i++)
    {
        // Extract the i-th bit from 'size'
        // Shift right and mask with 1
        int bit = (size >> (31 - i)) & 1;

        // Clear the LSB of the current image byte (mask with 0xFE)
        // Then set it to the extracted bit
        imageBuffer[i] = (imageBuffer[i] & 0xFE) | bit;
    }

    // Return success after all 32 bits are encoded
    return e_success;
}







/*
 * Function: do_encoding
 * ---------------------
 * Performs the encoding process step by step:
 *   1. Open files
 *   2. Check capacity
 *   3. Copy BMP header
 *   4. Encode magic string
 *   5. Encode secret file extension size
 *   6. Encode secret file extension
 *   7. Encode secret file size
 *   8. Encode secret file data
 *   9. Copy remaining image data
 *
 * Parameters:
 *   encInfo - pointer to EncodeInfo structure containing filenames and file pointers
 *
 * Returns:
 *   e_success if encoding is completed successfully
 *   e_failure if any step fails
 */
Status do_encoding(EncodeInfo *encInfo)
{
    printf("INFO: Starting encoding process...\n");

    // Step 1: Open files
    if (open_files(encInfo) == e_failure)
    {
        printf("ERROR: Failed to open required files.\n");
        return e_failure;
    }
    printf("INFO: Files opened successfully.\n");

    // Step 2: Check capacity
    if (check_capacity(encInfo) == e_failure)
    {
        printf("ERROR: Image does not have enough capacity to store secret data.\n");
        return e_failure;
    }
    printf("INFO: Image has enough capacity.\n");

    // Step 3: Copy BMP header
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy BMP header.\n");
        return e_failure;
    }
    printf("INFO: BMP header copied successfully.\n");

    // Step 4: Encode magic string
    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode magic string.\n");
        return e_failure;
    }
    printf("INFO: Magic string encoded successfully.\n");

    // Step 5: Encode secret file extension size
    if (encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file extension size.\n");
        return e_failure;
    }
    printf("INFO: Secret file extension size encoded successfully.\n");

    // Step 6: Encode secret file extension
    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file extension.\n");
        return e_failure;
    }
    printf("INFO: Secret file extension encoded successfully.\n");

    // Step 7: Encode secret file size
    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file size.\n");
        return e_failure;
    }
    printf("INFO: Secret file size encoded successfully.\n");

    // Step 8: Encode secret file data
    if (encode_secret_file_data(encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file data.\n");
        return e_failure;
    }
    printf("INFO: Secret file data encoded successfully.\n");

    // Step 9: Copy remaining image data
    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy remaining image data.\n");
        return e_failure;
    }
    printf("INFO: Remaining image data copied successfully.\n");

    // Final Step: Encoding completed
    printf("INFO: Encoding process completed successfully!\n");
    return e_success;
}
