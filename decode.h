#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h" // Contains user defined types (Status, uint, OperationType)

/*
 * Structure to store information required for
 * decoding secret file from source Image
 */
typedef struct _DecodeInfo
{
    /* Source Image info */
    char *src_image_fname;      // To store the src image name (stego image)
    FILE *fptr_src_image;       // To store the address of the src image

    /* Secret File Info */
    char secret_fname[100]; // To store the secret file name
    FILE *fptr_secret;          // To store the secret file address
    char extn_secret_file[5]; // To store the secret file extn (e.g., ".txt")
    long size_secret_file;      // To store the size of the secret data
    int extn_size;              // To store the actual length of the extension (e.g., 4 for ".txt")

    /* Other Data */
    char image_data[100 * 8]; // To hold image data during decoding

} DecodeInfo;

/* Function prototypes */

/* Read and validate Decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Get File pointers for i/p and o/p files */
Status open_files_decode(DecodeInfo *decInfo);

/* Decode Magic String */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);

/* Decode data from LSBs of image data */
Status decode_data_from_image(int size, FILE *fptr_src_image, char *data, DecodeInfo *decInfo);

/* Decode a byte from LSBs of image data */
Status decode_byte_from_lsb(char *image_buffer, char *data);

/* Decode size from LSBs of image data */
Status decode_size_from_lsb(char *image_buffer, long *size);

/* Decode secret file extn size */
Status decode_secret_file_extn_size(DecodeInfo *decInfo);

/* Decode secret file extn */
Status decode_secret_file_extn(DecodeInfo *decInfo);

/* Decode secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo);

/* Decode secret file data and write to file */
Status decode_secret_file_data(DecodeInfo *decInfo);

#endif