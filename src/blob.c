#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <zlib.h>
#include "sha1.h"
#include "blob.h"

void create_blob(const char *filename, char *out_hex_hash)
{
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *content = malloc(fsize);
    fread(content, 1, fsize, f);
    fclose(f);

    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %ld", fsize);
    long store_len = header_len + 1 + fsize;

    unsigned char *store = malloc(store_len);
    memcpy(store, header, header_len);
    store[header_len] = '\0';
    memcpy(store + header_len + 1, content, fsize);

    unsigned char hash[20];
    sha1_hash(store, store_len, hash);

    for (int i = 0; i < 20; i++)
    {
        sprintf(out_hex_hash + (i * 2), "%02x", hash[i]);
    }
    out_hex_hash[40] = '\0';

    unsigned long compressed_size = compressBound(store_len);
    unsigned char *compressed = malloc(compressed_size);
    compress(compressed, &compressed_size, store, store_len);

    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), ".git/objects/%.2s", out_hex_hash);
    mkdir(dir_path);

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, out_hex_hash + 2);

    FILE *out = fopen(file_path, "wb");
    fwrite(compressed, 1, compressed_size, out);
    fclose(out);

    free(content);
    free(store);
    free(compressed);
}

void create_blob_raw(const char *filename, unsigned char *out_raw_hash)
{
    char hex_hash[41];
    create_blob(filename, hex_hash);
    for (int i = 0; i < 20; i++)
    {
        sscanf(hex_hash + i * 2, "%2hhx", &out_raw_hash[i]);
    }
}

static unsigned char *get_blob_content(const char *hash, unsigned long *out_len)
{
    char path[256];
    snprintf(path, sizeof(path), ".git/objects/%.2s/%s", hash, hash + 2);

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *compressed = malloc(fsize);
    fread(compressed, 1, fsize, f);
    fclose(f);

    unsigned long decompressed_size = fsize * 20;
    unsigned char *decompressed = malloc(decompressed_size);
    uncompress(decompressed, &decompressed_size, compressed, fsize);
    free(compressed);

    unsigned char *content = memchr(decompressed, '\0', decompressed_size);
    content++;
    *out_len = decompressed_size - (content - decompressed);

    unsigned char *result = malloc(*out_len + 1);
    memcpy(result, content, *out_len);
    result[*out_len] = '\0';
    free(decompressed);
    return result;
}

void diff_blobs(const char *hash1, const char *hash2)
{
    unsigned long len1, len2;
    unsigned char *content1 = get_blob_content(hash1, &len1);
    unsigned char *content2 = get_blob_content(hash2, &len2);

    if (content1 == NULL || content2 == NULL)
    {
        fprintf(stderr, "Failed to read one or both blobs\n");
        return;
    }

    if (len1 == len2 && memcmp(content1, content2, len1) == 0)
    {
        printf("No differences\n");
    }
    else
    {
        printf("--- %s\n+++ %s\n", hash1, hash2);
        printf("- %s\n", content1);
        printf("+ %s\n", content2);
    }

    free(content1);
    free(content2);
}