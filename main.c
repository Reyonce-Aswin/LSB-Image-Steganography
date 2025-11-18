/*
Name: Reyonce Aswin T
Student ID: 25021_181
Description: This project is a console-based LSB Image Steganography application designed for Linux terminal environments.
             It enables users to hide and retrieve secret data (such as text or files) within a BMP image using the 
             Least Significant Bit (LSB) technique — a simple yet effective form of steganography.
             Users can easily encode a secret file into a cover image and later decode it back to retrieve the hidden information.
             The program ensures data security and integrity by validating input files, checking image capacity, and handling errors gracefully.

             It provides a menu-driven command-line interface that allows users to perform encoding, decoding, and verification operations with ease.
             All encoding and decoding processes maintain the visual quality of the image, 
             ensuring that the hidden message remains undetectable to the naked eye.

             This project demonstrates the practical implementation of data hiding techniques using bit-level manipulation and file I/O operations in C, 
             making it an excellent learning exercise in image processing, binary operations, and information security fundamentals.
*/
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "encode.h"
#include "decode.h"
#include "colour.h"

/* Function Declarations */
OperationType check_operation_type(char *argv[]);
void print_usage();

/* Main function */
int main(int argc, char *argv[])
{
    if (argc > 5 || argc < 3)
    {
        print_usage();
        return 1;
    }

    OperationType op_type = check_operation_type(argv);

    if (op_type == encode)
    {
        if(argc < 4){
            print_usage();
            return 1;
        }
        printf(CYAN BOLD"Selected operation: Encoding\n"RESET);

        EncodeInfo encInfo;
        if (read_and_validate_encode_args(argv, &encInfo) == failure)
        {
            printf(RED"ERROR: Invalid encoding arguments.\n"RESET);
            return 1;
        }

        if (do_encoding(&encInfo) == failure)
        {
            printf(RED"ERROR: Encoding failed.\n"RESET);
            return 1;
        }

        printf(GREEN BOLD"Encoding successful!\n\n"RESET);
    }
    else if (op_type == decode)
    {
        printf(CYAN BOLD"Selected operation: Decoding\n"RESET);

        DecodeInfo decInfo;
        if (read_and_validate_decode_args(argv, &decInfo) == failure)
        {
            printf(RED"ERROR: Invalid decoding arguments.\n"RESET);
            return 1;
        }

        if (do_decoding(&decInfo) == failure)
        {
            printf(RED"ERROR: Decoding failed.\n"RESET);
            return 1;
        }

        printf(GREEN BOLD"Decoding successful!\n\n"RESET);
    }
    else
    {
        printf(RED BOLD"ERROR: Unsupported operation.\n"RESET);
        print_usage();
        return 1;
    }

    return 0;
}

/* Identify encode/decode from argv[1] */
OperationType check_operation_type(char *argv[])
{
    if (strcmp(argv[1], "-e") == 0)
        return encode;
    else if (strcmp(argv[1], "-d") == 0)
        return decode;
    else
        return unsupported;
}

/* Print usage instructions */
void print_usage()
{
    printf(YELLOW"-------------------------------------------------------------\n");
    printf("Usage:\n");
    printf("  Encoding: ./steg.exe -e <source.bmp> <secret.txt> [output.bmp]\n");
    printf("  Decoding: ./steg.exe -d <stego.bmp> [output.txt]\n");
    printf("-------------------------------------------------------------\n"RESET);
}