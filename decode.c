#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"
#include "colour.h"

/* Function Definitions */

/* Read and validate decode arguments */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    // Validate stego image filename
    if (strlen(argv[2]) < 4)
    {
        printf(RED"ERROR: Invalid source file name length.\n"RESET);
        return failure;
    }

    char *src_ext = argv[2] + strlen(argv[2]) - 4;
    if (strcmp(src_ext, ".bmp") != 0)
    {
        printf(RED"ERROR: Invalid source file. Use .bmp files.\n"RESET);
        return failure;
    }

    decInfo->src_image_fname = argv[2];

    // Handle optional output argument
    if (argv[3] != NULL)
    {
        strcpy(decInfo->secret_fname, argv[3]);
        decInfo->fptr_secret = NULL;
    }
    else
    {
        decInfo->secret_fname[0] = '\0'; // Leave empty for now
        decInfo->fptr_secret = NULL;
    }

    return success;
}

/* Open required files */
Status open_files_decode(DecodeInfo *decInfo)
{
    decInfo->fptr_src_image = fopen(decInfo->src_image_fname, "r");

    printf(YELLOW"INFO: Opening source image file\n"RESET);
    if (decInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, RED"ERROR: Unable to open source image file %s\n"RESET, decInfo->src_image_fname);
        return failure;
    }
    printf(GREEN"SUCCESS: Opened source image file\n"RESET);
    return success;
}

/* Decode 1 byte from 8 LSBs */
Status decode_byte_from_lsb(char *image_buffer, char *data)
{
    *data = 0;
    for (int i = 0; i < 8; i++)
    {
        // Get the LSB (0 or 1) from the current image byte
        char bit = image_buffer[i] & 0x01;
        // Shift the bit to its correct position (7-i) to reconstruct the original byte
        *data = (*data << 1) | bit; // MSB first reconstruction
    }
    return success;
}

/* Decode N bytes of data from image */
Status decode_data_from_image(int size, FILE *fptr_src_image, char *data, DecodeInfo *decInfo)
{
    for (int i = 0; i < size; i++)
    {
        // Use a temporary buffer on the stack to read the 8 image bytes
        char buffer[8];
        if (fread(buffer, 1, 8, fptr_src_image) != 8)
            return failure;

        if (decode_byte_from_lsb(buffer, &data[i]) == failure)
            return failure;
    }
    return success;
}

/* Decode 4-byte size (from 32 image bytes) */
Status decode_size_from_lsb(char *image_buffer, long *size)
{
    // Decode 32 bits MSB-first (as they were encoded) into the 32-bit size value.
    *size = 0;
    for (int i = 0; i < 32; i++)
    {
        // Get the LSB (0 or 1) from the current image byte (image_buffer[i])
        char bit = image_buffer[i] & 0x01;
        // Shift the reconstructed size MSB-first.
        *size = (*size << 1) | bit;
    }
    return success;
}

/* Decode Magic String */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)
{
    int len = strlen(magic_string);
    char decoded_ms[len + 1];
    decoded_ms[len] = '\0';

    fseek(decInfo->fptr_src_image, 54, SEEK_SET); // Skip BMP header

    if (decode_data_from_image(len, decInfo->fptr_src_image, decoded_ms, decInfo) == failure)
        return failure;

    if (strcmp(decoded_ms, magic_string) == 0)
        return success;
    else
        return failure;
}

/* Decode secret file extn size */
Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    long extn_size;
    // Read 32 image bytes
    if (fread(decInfo->image_data, 1, 32, decInfo->fptr_src_image) != 32)
        return failure;

    if (decode_size_from_lsb(decInfo->image_data, &extn_size) == failure)
        return failure;

    // The maximum size for extn_secret_file is 5, including the null terminator.
    if (extn_size <= 0 || extn_size > 4) 
    {
        fprintf(stderr, RED"ERROR: Decoded extn size invalid: %ld\n"RESET, extn_size);
        return failure;
    }

    decInfo->extn_size = (int)extn_size; // Store the size for later use
    return success;
}

/* Decode secret file extn */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    // Decode 'extn_size' bytes for the extension
    if (decode_data_from_image(decInfo->extn_size, decInfo->fptr_src_image, decInfo->extn_secret_file, decInfo) == failure)
        return failure;

    decInfo->extn_secret_file[decInfo->extn_size] = '\0'; // Null-terminate

    // Always construct the output filename based on user's input, but use decoded extension
    char base_name[100];

    if (decInfo->secret_fname[0] == '\0')
    {
        // No user-provided output filename → use "output"
        strcpy(base_name, "output");
    }
    else
    {
        // Copy user-provided filename and strip extension if present
        strncpy(base_name, decInfo->secret_fname, sizeof(base_name) - 1);
        base_name[sizeof(base_name) - 1] = '\0';
        char *dot = strrchr(base_name, '.');
        if (dot != NULL)
            *dot = '\0'; // remove extension
    }

    // Copy base_name into final filename buffer
    strncpy(decInfo->secret_fname, base_name, sizeof(decInfo->secret_fname) - 1);
    decInfo->secret_fname[sizeof(decInfo->secret_fname) - 1] = '\0';

    // Append the decoded extension (e.g. ".c", ".sh", ".txt")
    if (strlen(decInfo->secret_fname) + strlen(decInfo->extn_secret_file) < sizeof(decInfo->secret_fname))
        strcat(decInfo->secret_fname, decInfo->extn_secret_file);
    else
    {
        fprintf(stderr, RED"ERROR: Output filename too long after adding extension.\n"RESET);
        return failure;
    }

    // Open output file for writing
    decInfo->fptr_secret = fopen(decInfo->secret_fname, "w");
    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, RED"ERROR: Unable to open %s\n"RESET, decInfo->secret_fname);
        return failure;
    }

    printf(MAGENTA"INFO: Output file created as "RESET);
    printf(BOLD"%s\n"RESET, decInfo->secret_fname);
    return success;
}

/* Decode secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    long file_size;
    // Read 32 image bytes
    if (fread(decInfo->image_data, 1, 32, decInfo->fptr_src_image) != 32)
        return failure;

    if (decode_size_from_lsb(decInfo->image_data, &file_size) == failure)
        return failure;

    if (file_size < 0)
    {
         fprintf(stderr, RED"ERROR: Decoded file size is negative: %ld\n"RESET, file_size);
         return failure;
    }
    
    decInfo->size_secret_file = file_size;
    printf(MAGENTA"INFO: Secret file size: "RESET);
    printf(BOLD"%ld bytes\n"RESET, file_size);
    return success;
}

/* Decode and write secret data */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char secret_byte;
    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        // Use a temporary buffer on the stack to read the 8 image bytes
        char buffer[8];
        if (fread(buffer, 1, 8, decInfo->fptr_src_image) != 8)
            return failure;

        if (decode_byte_from_lsb(buffer, &secret_byte) == failure)
            return failure;

        fwrite(&secret_byte, 1, 1, decInfo->fptr_secret);
    }
    return success;
}

/* Master decode process */
Status do_decoding(DecodeInfo *decInfo)
{
    printf(CYAN"Starting decoding...\n"RESET);
    
    // 1. Opening files
    printf(YELLOW"INFO: Opening files\n"RESET);
    if (open_files_decode(decInfo) == failure)
        return failure;
    printf(GREEN"SUCCESS: Opened required files\n"RESET);

    // 2. Decoding magic string
    printf(YELLOW"INFO: Decoding magic string\n"RESET);
    if (decode_magic_string(MAGIC_STRING, decInfo) == failure)
    {
        fprintf(stderr, RED"ERROR: Magic string not found! Not a stego image.\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Magic string verified\n"RESET);

    // 3. Decoding secret file extension size
    printf(YELLOW"INFO: Decoding secret file extension size\n"RESET);
    if (decode_secret_file_extn_size(decInfo) == failure)
        return failure;
    printf(GREEN"SUCCESS: Decoded extn size\n"RESET);

    // 4. Decoding secret file extension
    printf(YELLOW"INFO: Decoding secret file extension\n"RESET);
    if (decode_secret_file_extn(decInfo) == failure)
        return failure;
    printf(GREEN"SUCCESS: Decoded extn\n"RESET);

    // 5. Decoding secret file size
    printf(YELLOW"INFO: Decoding secret file size\n"RESET);
    if (decode_secret_file_size(decInfo) == failure)
        return failure;
    printf(GREEN"SUCCESS: Decoded secret file size\n"RESET);

    // 6. Decoding secret file data
    printf(YELLOW"INFO: Decoding secret file data\n"RESET);
    if (decode_secret_file_data(decInfo) == failure)
        return failure;
    printf(GREEN"SUCCESS: Decoded secret file data\n"RESET);

    printf(YELLOW"INFO: Closing files\n"RESET);
    fclose(decInfo->fptr_secret);
    fclose(decInfo->fptr_src_image);
    return success;
}