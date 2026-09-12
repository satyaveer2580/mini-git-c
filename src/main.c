#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/sha.h>

int main(int argc, char *argv[])
{
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./your_program.sh <command> [<args>]\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "init") == 0)
    {
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        // TODO: Uncomment the code below to pass the first stage

        if (mkdir(".git", 0755) == -1 ||
            mkdir(".git/objects", 0755) == -1 ||
            mkdir(".git/refs", 0755) == -1)
        {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }

        FILE *headFile = fopen(".git/HEAD", "w");
        if (headFile == NULL)
        {
            fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);

        printf("Initialized git directory\n");
    }
    else if (strcmp(command, "cat-file") == 0)
    {
        if (argc < 4 || strcmp(argv[2], "-p") != 0)
        {
            fprintf(stderr, "Usage: cat-file -p <blob_sha>\n");
            return 1;
        }

        else if (strcmp(command, "hash-object") == 0)
        {
            if (argc < 4 || strcmp(argv[2], "-w") != 0)
            {
                fprintf(stderr, "Usage: hash-object -w <file>\n");
                return 1;
            }

            const char *filename = argv[3];

            // File padho
            FILE *f = fopen(filename, "rb");
            if (f == NULL)
            {
                fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
                return 1;
            }
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            unsigned char *content = malloc(fsize);
            fread(content, 1, fsize, f);
            fclose(f);

            // Blob format banao: "blob <size>\0<content>"
            char header[64];
            int header_len = snprintf(header, sizeof(header), "blob %ld", fsize);
            long store_len = header_len + 1 + fsize; // +1 for null byte

            unsigned char *store = malloc(store_len);
            memcpy(store, header, header_len);
            store[header_len] = '\0';
            memcpy(store + header_len + 1, content, fsize);

            // SHA-1 hash nikalo
            unsigned char hash[SHA_DIGEST_LENGTH]; // 20 bytes
            SHA1(store, store_len, hash);

            // Hash ko hex string mein convert karo (40 characters)
            char hex_hash[41];
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
            {
                sprintf(hex_hash + (i * 2), "%02x", hash[i]);
            }
            hex_hash[40] = '\0';

            // Compress karo store karne ke liye
            unsigned long compressed_size = compressBound(store_len);
            unsigned char *compressed = malloc(compressed_size);
            compress(compressed, &compressed_size, store, store_len);

            // Path banao aur folder create karo: .git/objects/xx/yyyy...
            char dir_path[256];
            snprintf(dir_path, sizeof(dir_path), ".git/objects/%.2s", hex_hash);
            mkdir(dir_path, 0755);

            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, hex_hash + 2);

            FILE *out = fopen(file_path, "wb");
            fwrite(compressed, 1, compressed_size, out);
            fclose(out);

            // Hash print karo (bina newline ke)
            printf("%s", hex_hash);

            free(content);
            free(store);
            free(compressed);
        }

        const char *hash = argv[3];

        // Path banao: .git/objects/xx/yyyy...
        char path[256];
        snprintf(path, sizeof(path), ".git/objects/%.2s/%s", hash, hash + 2);

        FILE *f = fopen(path, "rb");
        if (f == NULL)
        {
            fprintf(stderr, "Failed to open object file: %s\n", strerror(errno));
            return 1;
        }

        // Poora compressed file read karo
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        unsigned char *compressed = malloc(fsize);
        fread(compressed, 1, fsize, f);
        fclose(f);

        // Decompress karne ke liye buffer (bada rakho)
        unsigned long decompressed_size = fsize * 20;
        unsigned char *decompressed = malloc(decompressed_size);

        int result = uncompress(decompressed, &decompressed_size, compressed, fsize);
        if (result != Z_OK)
        {
            fprintf(stderr, "Failed to decompress object\n");
            free(compressed);
            free(decompressed);
            return 1;
        }

        // Format hai: "blob <size>\0<content>"
        // '\0' dhoondo
        unsigned char *content_start = memchr(decompressed, '\0', decompressed_size);
        if (content_start == NULL)
        {
            fprintf(stderr, "Invalid object format\n");
            return 1;
        }
        content_start++; // null byte ke aage jaao

        // Content print karo (bina extra newline ke)
        long content_len = decompressed_size - (content_start - decompressed);
        fwrite(content_start, 1, content_len, stdout);

        free(compressed);
        free(decompressed);
    }

    else
    {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }

    return 0;
}
