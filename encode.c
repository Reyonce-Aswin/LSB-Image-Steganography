#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "common.h"
#include "colour.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
// Get image size for BMP
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf(MAGENTA"     Width = "RESET BOLD"%u pxls\n"RESET, width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf(MAGENTA"     Height = "RESET BOLD"%u pxls\n"RESET, height);

    // Return image capacity
    return width * height * 3;
}

// Get file size
uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    return ftell(fptr);
}

// Validate and read arguments
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    /* Validate source image (argv[2]) */
    printf(YELLOW "INFO: Checking source image extension\n" RESET);

    if (argv[2] == NULL) {
        printf(RED "ERROR: Source image not provided\n" RESET);
        return failure;
    }

    char *img_dot = strrchr(argv[2], '.'); // last dot in filename
    if (img_dot == NULL || strcmp(img_dot, ".bmp") != 0) {
        printf(RED "ERROR: Source image file must be .bmp\n" RESET);
        return failure;
    }

    printf(GREEN "SUCCESS: Valid extension\n" RESET);
    encInfo->src_image_fname = argv[2];

    /* Validate secret file (argv[3]) */
    printf(YELLOW "INFO: Checking for secret message file\n" RESET);

    if (argv[3] == NULL) {
        printf(RED "ERROR: Secret file not provided\n" RESET);
        return failure;
    }

    /* Acceptable extensions */
    char *secret_dot = strrchr(argv[3], '.');
    if (secret_dot == NULL) {
        printf(RED "ERROR: Secret file has no extension\n" RESET);
        return failure;
    }

    if (strcmp(secret_dot, ".txt") != 0 &&
        strcmp(secret_dot, ".c")   != 0 &&
        strcmp(secret_dot, ".h")   != 0 &&
        strcmp(secret_dot, ".sh")  != 0 &&
        strcmp(secret_dot, ".py")  != 0
    )
    {
        printf(RED "ERROR: Secret file should have a valid extension ('.txt', '.c', '.h', '.sh', '.py')\n" RESET);
        return failure;
    }
    encInfo->secret_fname = argv[3];
    printf(GREEN "SUCCESS: Secret message found\n" RESET);

    /* Extract and store extension safely */
    printf(YELLOW "INFO: Checking for secret file extension\n" RESET);
    if (secret_dot != NULL) {
        /* copy extension into fixed buffer safely */
        strncpy(encInfo->extn_secret_file, secret_dot, sizeof(encInfo->extn_secret_file) - 1);
        encInfo->extn_secret_file[sizeof(encInfo->extn_secret_file) - 1] = '\0';
        printf(GREEN "SUCCESS: Secret file extension verified: %s\n" RESET, encInfo->extn_secret_file);
    } else {
        encInfo->extn_secret_file[0] = '\0';
        fprintf(stderr, RED "ERROR: Secret file has no extension. Verification failed.\n" RESET);
        return failure;
    }

    /* Output stego filename (optional argv[4]) */
    if (argv[4] != NULL) {
        char *o_dot = strrchr(argv[4], '.'); // last dot in filename
        if (o_dot == NULL || strcmp(o_dot, ".bmp") != 0) {
            printf(RED "ERROR: Destination image file must be .bmp\n" RESET);
            return failure;
        }
        printf(GREEN "SUCCESS: Valid extension\n" RESET);
        encInfo->stego_image_fname = argv[4];
    }
    else
        encInfo->stego_image_fname = "steg.bmp";

    return success;
}

// Open required files
Status open_files(EncodeInfo *encInfo)
{
    printf(YELLOW"INFO: Opening source file\n"RESET);
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (!encInfo->fptr_src_image)
    {
        perror(RED"ERROR: Unable to open source image file"RED);
        return failure;
    }
    printf(GREEN"SUCCESS: Source file opened:"RESET BOLD"%s\n"RESET,encInfo -> src_image_fname);

    printf(YELLOW"INFO: Opening secret file\n"RESET);
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (!encInfo->fptr_secret)
    {
        perror(RED"ERROR: Unable to open secret file"RED);
        return failure;
    }
    printf(GREEN"SUCCESS: Secret file opened:"RESET BOLD"%s\n"RESET,encInfo -> secret_fname);

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (!encInfo->fptr_stego_image)
    {
        perror(RED"ERROR: Unable to open output file"RED);
        return failure;
    }

    return success;
}

// Check capacity
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    /* Calculate total number of bits required to embed: BMP header (54 bytes),
    magic string, extension size, file extension, secret file size, and the
    entire secret file data */
    uint file_capacity = (54 + strlen(MAGIC_STRING) * 8 + sizeof(int) * 8 + strlen(encInfo->extn_secret_file) * 8 + 
                            sizeof(long) * 8 + encInfo->size_secret_file * 8);
    if (encInfo->image_capacity < file_capacity)
    {
        printf(RED"ERROR: Image does not have enough capacity: "RESET);
        printf("%u bytes/%u bytes",file_capacity, encInfo -> image_capacity);
        return failure;
    }
    return success;
}

// Copy BMP header (first 54 bytes)
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];
    rewind(fptr_src_image);
    fread(header, 54, 1, fptr_src_image);
    fwrite(header, 54, 1, fptr_dest_image);
    return success;
}

// Encode a byte into 8 bytes (LSB)
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);
    }
    return success;
}

// Encode integer size to LSB
Status encode_size_to_lsb(int size, char *image_buffer)
{
    for (int i = 0; i < 32; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((size >> (31 - i)) & 1);
    }
    return success;
}

// Encode magic string
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    char buffer[8];
    for (int i = 0; i < strlen(magic_string); i++)
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }
    return success;
}

// Encode secret file extn size
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    char buffer[32];
    fread(buffer, 32, 1, encInfo->fptr_src_image);
    encode_size_to_lsb(size, buffer);
    fwrite(buffer, 32, 1, encInfo->fptr_stego_image);
    return success;
}

// Encode secret file extn
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    char buffer[8];
    for (int i = 0; i < strlen(file_extn); i++)
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }
    return success;
}

// Encode secret file size
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char buffer[32];
    fread(buffer, 32, 1, encInfo->fptr_src_image);
    encode_size_to_lsb(file_size, buffer);
    fwrite(buffer, 32, 1, encInfo->fptr_stego_image);
    return success;
}

// Encode secret file data
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch, buffer[8];
    rewind(encInfo->fptr_secret);

    while (fread(&ch, 1, 1, encInfo->fptr_secret) > 0)
    {
        fread(buffer, 8, 1, encInfo->fptr_src_image);
        encode_byte_to_lsb(ch, buffer);
        fwrite(buffer, 8, 1, encInfo->fptr_stego_image);
    }

    return success;
}

// Copy remaining data after encoding
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;
    while (fread(&ch, 1, 1, fptr_src) > 0)
        fwrite(&ch, 1, 1, fptr_dest);
    return success;
}

// Main encoding function
Status do_encoding(EncodeInfo *encInfo)
{
    // 1. Open files
    printf(YELLOW"INFO: Opening files\n"RESET);
    if (open_files(encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Opening files failed\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Opening files done\n"RESET);

    // 2. Check Capacity
    printf(YELLOW"INFO: Checking capacity\n"RESET);
    if (check_capacity(encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Image capacity is insufficient to hold the secret data\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Check capacity done\n"RESET);

    // 3. Copy BMP Header (54 bytes)
    printf(YELLOW"INFO: Copying BMP header\n"RESET);
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == failure) {
        fprintf(stderr, RED"ERROR: Failed to copy BMP header\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Copying BMP header done\n"RESET);

    // 4. Encode Magic String (e.g., "#*")
    printf(YELLOW"INFO: Encoding Magic String\n"RESET);
    if (encode_magic_string(MAGIC_STRING, encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Failed to encode magic string\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Encoding Magic String done\n"RESET);

    // 5. Encode Secret File Extension Size
    printf(YELLOW"INFO: Encoding secret file extension size\n"RESET);
    int extn_size = strlen(encInfo->extn_secret_file);
    if (encode_secret_file_extn_size(extn_size, encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Failed to encode secret file extension size\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Encoding Secret File Extension Size done\n"RESET);

    // 6. Encode Secret File Extension (e.g., ".txt")
    printf(YELLOW"INFO: Encoding secret file extension\n"RESET);
    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Failed to encode secret file extension\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Encoding Secret File Extension done\n"RESET);

    // 7. Encode Secret File Size
    printf(YELLOW"INFO: Encoding secret file size\n"RESET);
    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Failed to encode secret file size\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Encoding Secret File Size done\n"RESET);

    // 8. Encode Secret File Data
    printf(YELLOW"INFO: Encoding secret file data\n"RESET);
    if (encode_secret_file_data(encInfo) == failure) {
        fprintf(stderr, RED"ERROR: Failed to encode secret file data\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Encoding Secret File Data done\n"RESET);

    // 9. Copy Remaining Image Data
    printf(YELLOW"INFO: Copying remaining Image data\n"RESET);
    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == failure) {
        fprintf(stderr, RED"ERROR: Failed to copy remaining image data\n"RESET);
        return failure;
    }
    printf(GREEN"SUCCESS: Copying remaining Image data done\n"RESET);

    // All steps successful
    printf(YELLOW"INFO: Closing files\n"RESET);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_src_image);
    return success;
}