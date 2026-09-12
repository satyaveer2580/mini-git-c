#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <zlib.h>
#include "blob.h"
#include "sha1.h"
#include "tree.h"

typedef struct {
    char mode[10];
    char name[256];
    unsigned char hash[20];
} Entry;

int compare_entries(const void *a, const void *b) {
    return strcmp(((Entry*)a)->name, ((Entry*)b)->name);
}

void write_tree(const char *dir_path, char *out_hex_hash) {
    DIR *dir = opendir(dir_path);
    struct dirent *entry;
    Entry entries[1024];
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        stat(full_path, &st);

        strcpy(entries[count].name, entry->d_name);

        if (S_ISDIR(st.st_mode)) {
            strcpy(entries[count].mode, "40000");
            char sub_hex[41];
            write_tree(full_path, sub_hex);
            for (int i = 0; i < 20; i++) {
                sscanf(sub_hex + i*2, "%2hhx", &entries[count].hash[i]);
            }
        } else {
            strcpy(entries[count].mode, "100644");
            create_blob_raw(full_path, entries[count].hash);
        }
        count++;
    }
    closedir(dir);

    qsort(entries, count, sizeof(Entry), compare_entries);

    unsigned char buffer[65536];
    int pos = 0;
    for (int i = 0; i < count; i++) {
        pos += sprintf((char*)buffer + pos, "%s %s", entries[i].mode, entries[i].name);
        buffer[pos++] = '\0';
        memcpy(buffer + pos, entries[i].hash, 20);
        pos += 20;
    }

    char header[64];
    int header_len = snprintf(header, sizeof(header), "tree %d", pos);
    int store_len = header_len + 1 + pos;
    unsigned char *store = malloc(store_len);
    memcpy(store, header, header_len);
    store[header_len] = '\0';
    memcpy(store + header_len + 1, buffer, pos);

    unsigned char hash[20];
    sha1_hash(store, store_len, hash);
    for (int i = 0; i < 20; i++) {
        sprintf(out_hex_hash + (i*2), "%02x", hash[i]);
    }
    out_hex_hash[40] = '\0';

    unsigned long compressed_size = compressBound(store_len);
    unsigned char *compressed = malloc(compressed_size);
    compress(compressed, &compressed_size, store, store_len);

    char dir_path2[256];
    snprintf(dir_path2, sizeof(dir_path2), ".git/objects/%.2s", out_hex_hash);
    mkdir(dir_path2);

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path2, out_hex_hash + 2);
    FILE *out = fopen(file_path, "wb");
    fwrite(compressed, 1, compressed_size, out);
    fclose(out);

    free(store);
    free(compressed);
}

static unsigned char* read_and_decompress(const char *hash, unsigned long *out_size) {
    char path[256];
    snprintf(path, sizeof(path), ".git/objects/%.2s/%s", hash, hash + 2);

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Failed to open object: %s\n", hash);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *compressed = malloc(fsize);
    fread(compressed, 1, fsize, f);
    fclose(f);

    unsigned long decompressed_size = fsize * 30;
    unsigned char *decompressed = malloc(decompressed_size);
    uncompress(decompressed, &decompressed_size, compressed, fsize);
    free(compressed);

    *out_size = decompressed_size;
    return decompressed;
}

void ls_tree(const char *tree_hash) {
    unsigned long size;
    unsigned char *data = read_and_decompress(tree_hash, &size);

    unsigned char *ptr = memchr(data, '\0', size);
    ptr++; // skip header

    while (ptr < data + size) {
        char mode[10], name[256];
        sscanf((char*)ptr, "%s %s", mode, name);
        ptr += strlen(mode) + 1 + strlen(name) + 1;

        printf("%s %s\n", mode, name);
        ptr += 20; // skip 20-byte hash
    }
    free(data);
}

void restore_tree(const char *tree_hash, const char *dest_dir) {
    unsigned long size;
    unsigned char *data = read_and_decompress(tree_hash, &size);

    unsigned char *ptr = memchr(data, '\0', size);
    ptr++;

    while (ptr < data + size) {
        char mode[10], name[256];
        sscanf((char*)ptr, "%s %s", mode, name);
        ptr += strlen(mode) + 1 + strlen(name) + 1;

        unsigned char raw_hash[20];
        memcpy(raw_hash, ptr, 20);
        ptr += 20;

        char hex_hash[41];
        for (int i = 0; i < 20; i++) sprintf(hex_hash + i*2, "%02x", raw_hash[i]);
        hex_hash[40] = '\0';

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_dir, name);

        if (strcmp(mode, "40000") == 0) {
            mkdir(full_path);
            restore_tree(hex_hash, full_path);
        } else {
            unsigned long blob_size;
            unsigned char *blob_data = read_and_decompress(hex_hash, &blob_size);
            unsigned char *content = memchr(blob_data, '\0', blob_size);
            content++;
            long content_len = blob_size - (content - blob_data);

            FILE *out = fopen(full_path, "wb");
            fwrite(content, 1, content_len, out);
            fclose(out);
            free(blob_data);
        }
    }
    free(data);
}