#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <zlib.h>
#include "sha1.h"
#include "commit.h"

void create_commit(const char *tree_hash, const char *parent_hash, const char *message, char *out_hex_hash) {
    char content[65536];
    int pos = 0;

    pos += sprintf(content + pos, "tree %s\n", tree_hash);

    if (parent_hash != NULL && strlen(parent_hash) > 0) {
        pos += sprintf(content + pos, "parent %s\n", parent_hash);
    }

    time_t now = time(NULL);
    pos += sprintf(content + pos, "author Satyaveer <satyaveer@example.com> %ld +0530\n", now);
    pos += sprintf(content + pos, "committer Satyaveer <satyaveer@example.com> %ld +0530\n", now);
    pos += sprintf(content + pos, "\n%s\n", message);

    char header[64];
    int header_len = snprintf(header, sizeof(header), "commit %d", pos);
    int store_len = header_len + 1 + pos;
    unsigned char *store = malloc(store_len);
    memcpy(store, header, header_len);
    store[header_len] = '\0';
    memcpy(store + header_len + 1, content, pos);

    unsigned char hash[20];
    sha1_hash(store, store_len, hash);
    for (int i = 0; i < 20; i++) {
        sprintf(out_hex_hash + (i*2), "%02x", hash[i]);
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

    free(store);
    free(compressed);
}