#ifndef COMMIT_H
#define COMMIT_H

void create_commit(const char *tree_hash, const char *parent_hash, const char *message, char *out_hex_hash);
void show_log(const char *commit_hash);
void search_log(const char *commit_hash, const char *keyword);


#endif