#ifndef BLOB_H
#define BLOB_H

void create_blob(const char *filename, char *out_hex_hash);
void create_blob_raw(const char *filename, unsigned char *out_raw_hash);

#endif