# MinI Git Implementation in C 

A simplified version control system built from scratch in C, implementing core Git concepts.

## Features 
- `init` - Initialize a new repository
- `cat-file -p <hash>` - Read a blob object
- `hash-object -w <file>` - Create a blob object

## Concepts Used 
- Content-addressable storage (SHA-1 hashing)
- zlib compression 
- File I/O binary data handling

## Build 
Requires zlib and OpenSSL libraries.