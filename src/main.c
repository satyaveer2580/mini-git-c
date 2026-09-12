#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "sha1.h"
#include "blob.h"
#include "tree.h"
#include "commit.h"

int main(int argc, char *argv[])
{
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
        if (mkdir(".git") == -1 ||
            mkdir(".git/objects") == -1 ||
            mkdir(".git/refs") == -1)
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

        const char *hash = argv[3];

        char path[256];
        snprintf(path, sizeof(path), ".git/objects/%.2s/%s", hash, hash + 2);

        FILE *f = fopen(path, "rb");
        if (f == NULL)
        {
            fprintf(stderr, "Failed to open object file: %s\n", strerror(errno));
            return 1;
        }

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        unsigned char *compressed = malloc(fsize);
        fread(compressed, 1, fsize, f);
        fclose(f);

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

        unsigned char *content_start = memchr(decompressed, '\0', decompressed_size);
        if (content_start == NULL)
        {
            fprintf(stderr, "Invalid object format\n");
            return 1;
        }
        content_start++;

        long content_len = decompressed_size - (content_start - decompressed);
        fwrite(content_start, 1, content_len, stdout);

        free(compressed);
        free(decompressed);
    }
    else if (strcmp(command, "hash-object") == 0)
    {
        if (argc < 4 || strcmp(argv[2], "-w") != 0)
        {
            fprintf(stderr, "Usage: hash-object -w <file>\n");
            return 1;
        }
        char hex_hash[41];
        create_blob(argv[3], hex_hash);
        printf("%s", hex_hash);
    }
    else if (strcmp(command, "write-tree") == 0)
    {
        char hex_hash[41];
        write_tree(".", hex_hash);
        printf("%s", hex_hash);
    }

    else if (strcmp(command, "commit-tree") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: commit-tree <tree_sha> [-p <parent_sha>] -m <message>\n");
            return 1;
        }
        const char *tree_hash = argv[2];
        const char *parent_hash = NULL;
        const char *message = "";

        for (int i = 3; i < argc; i++)
        {
            if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            {
                parent_hash = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            {
                message = argv[i + 1];
                i++;
            }
        }

        char hex_hash[41];
        create_commit(tree_hash, parent_hash, message, hex_hash);
        printf("%s", hex_hash);
    }

    else if (strcmp(command, "ls-tree") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: ls-tree <tree_sha>\n");
            return 1;
        }
        ls_tree(argv[2]);
    }

    else if (strcmp(command, "restore") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: restore <tree_sha> [dest_dir]\n");
            return 1;
        }
        const char *dest = (argc >= 4) ? argv[3] : ".";
        mkdir(dest);
        restore_tree(argv[2], dest);
        printf("Restored to %s\n", dest);
    }

    else if (strcmp(command, "log") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: log <commit_sha>\n");
            return 1;
        }
        show_log(argv[2]);
    }

    else if (strcmp(command, "diff") == 0)
    {
        if (argc < 4)
        {
            fprintf(stderr, "Usage: diff <hash1> <hash2>\n");
            return 1;
        }
        diff_blobs(argv[2], argv[3]);
    }
    else if (strcmp(command, "search") == 0)
    {
        if (argc < 4)
        {
            fprintf(stderr, "Usage: search <commit_sha> <keyword>\n");
            return 1;
        }
        search_log(argv[2], argv[3]);
    }
    else if (strcmp(command, "remove") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: remove <filename>\n");
            return 1;
        }
        if (remove(argv[2]) == 0)
        {
            printf("Removed %s\n", argv[2]);
        }
        else
        {
            fprintf(stderr, "Failed to remove %s: %s\n", argv[2], strerror(errno));
            return 1;
        }
    }

    else
    {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }

    return 0;
}