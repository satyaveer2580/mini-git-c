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

#include <string.h>

void show_log(const char *commit_hash) {
    char current_hash[41];
    strcpy(current_hash, commit_hash);

    while (strlen(current_hash) > 0) {
        char path[256];
        snprintf(path, sizeof(path), ".git/objects/%.2s/%s", current_hash, current_hash + 2);

        FILE *f = fopen(path, "rb");
        if (f == NULL) {
            fprintf(stderr, "Failed to open commit: %s\n", current_hash);
            return;
        }
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

        char *content = (char*)memchr(decompressed, '\0', decompressed_size);
        content++;

        printf("commit %s\n", current_hash);

        char parent_hash[41] = "";
        char *line = strtok(content, "\n");
        int in_message = 0;
        while (line != NULL) {
            if (strncmp(line, "parent ", 7) == 0) {
                strcpy(parent_hash, line + 7);
            } else if (strncmp(line, "author ", 7) == 0) {
                printf("%s\n", line);
            } else if (strlen(line) == 0) {
                in_message = 1;
            } else if (in_message) {
                printf("\n    %s\n", line);
            }
            line = strtok(NULL, "\n");
        }
        printf("\n");

        free(decompressed);
        strcpy(current_hash, parent_hash);
    }
}