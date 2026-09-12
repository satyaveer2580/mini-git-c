#ifndef TREE_H
#define TREE_H

void write_tree(const char *dir_path, char *out_hex_hash);
void ls_tree(const char *tree_hash);
void restore_tree(const char *tree_hash, const char *dest_dir);



#endif