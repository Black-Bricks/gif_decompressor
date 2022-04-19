# GIF decompressor
Decompress .gif files.

Requires: gcc compiler

To build the project open terminal at project directory and run command:
> make all

To rebuild the project run command:
> make clean
And then run:
> make all

To run the program go to the "build" directory and run "gifdec.exe" file.
Options:
* `-h`: `Help`
* `-i <fileName>`: `Input file name`
* `-o <fileName>`: `Output file name`
* `--rgct`: `Remove global color table`
* `--rlct`: `Remove local color table`

Example:
> gifdec.exe -i C:/file_to_decompress.gif -o C:/decompressed_file.gif --rgct

This project based on [lecram gifdec](https://github.com/lecram/gifdec) project which source code released into public domain and provided without warranty of any kind.