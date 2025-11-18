# LSB Image Steganography

A C-based implementation of the **Least Significant Bit (LSB)**
technique for hiding and extracting secret data inside a 24-bit BMP
images.\
This project provides a modular, reliable, and beginner-friendly
approach to understanding image steganography at the bit level.

## Features

- Encode any secret file into a BMP image\
- Decode and extract hidden data from a stego-image\
- Validates image type, file size, and secret file compatibility\
- Modular code structure for easy understanding\
- Console-based interface with detailed logs

## How It Works

The program modifies the **least significant bits** of the image's pixel
data to embed secret information.\
Since LSB changes minimally affect pixel intensity, the image looks
visually identical after encoding.

## Project Structure

```filestruct
    ├── encode.c
    ├── decode.c
    ├── encode.h
    ├── decode.h
    ├── common.h
    ├── types.h
    ├── main.c
    └── README.md
```

## Compilation

``` bash
gcc *.c -o steg

```

## Usage

### Encoding

``` bash
./steg -e source_image.bmp secret_file output_stego.bmp [linux]
./steg.exe -e source_image.bmp secret_file output_stego.bmp [Windows]

```

### Decoding

``` bash
./steg -d stego_image.bmp output_file [linux]
./steg.exe -d stego_image.bmp output_file [Windows]

```

## Example

``` bash
./steg -e image.bmp secret.txt output.bmp
./steg -d output.bmp output_file
```

## Requirements

- GCC or any C compiler\
- 24-bit BMP images\
- Linux terminal recommended
