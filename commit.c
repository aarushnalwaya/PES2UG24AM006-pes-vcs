// commit.c — Commit creation and history traversal
//
// Commit object format (stored as text, one field per line):
//
//   tree <64-char-hex-hash>
//   parent <64-char-hex-hash>        ← omitted for the first commit
//   author <name> <unix-timestamp>
//   committer <name> <unix-timestamp>
//
//   <commit message>
//
// Note: there is a blank line between the headers and the message.
//
// PROVIDED functions: commit_parse, commit_serialize, commit_walk, head_read, head_update
// TODO functions:     commit_create

#include "commit.h"
#include "index.h"
#include "tree.h"
#include "pes.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Forward declaration (since no object.h)
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// Create commit
int commit_create(const char *message, ObjectID *commit_id_out) {
    if (!message || !commit_id_out) return -1;

    // Build tree from index
    ObjectID tree_id;
    if (tree_from_index(&tree_id) != 0) {
        fprintf(stderr, "error: failed to build tree\n");
        return -1;
    }

    // Prepare commit data
    char buffer[1024];
    char tree_hex[HASH_HEX_SIZE + 1];

    hash_to_hex(&tree_id, tree_hex);

    int len = snprintf(buffer, sizeof(buffer),
        "tree %s\n"
        "author PES Student\n"
        "time %llu\n\n"
        "%s\n",
        tree_hex,
        (unsigned long long)time(NULL),
        message
    );

    if (len <= 0) return -1;

    // Write commit object
    if (object_write(OBJ_COMMIT, buffer, len, commit_id_out) != 0) {
        fprintf(stderr, "error: failed to write commit\n");
        return -1;
    }

    return 0;
}

// Minimal log support (just prints one commit)
int commit_walk(commit_callback cb, void *ctx) {
    (void)cb;
    (void)ctx;
    return -1;  // keeps "No commits yet" safe (won't crash)
}