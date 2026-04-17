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
#include "tree.h"
#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Forward declaration
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out);

// Paths
#define HEAD_FILE ".pes/HEAD"
#define REF_MAIN ".pes/refs/heads/main"

// Helper: write HEAD ref
static void update_ref(const ObjectID *id) {
    FILE *f = fopen(REF_MAIN, "w");
    if (!f) return;

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    fprintf(f, "%s\n", hex);
    fclose(f);
}

// Create commit
int commit_create(const char *message, ObjectID *commit_id_out) {
    if (!message || !commit_id_out) return -1;

    // Build tree
    ObjectID tree_id;
    if (tree_from_index(&tree_id) != 0) {
        fprintf(stderr, "error: failed to build tree\n");
        return -1;
    }

    // Get parent (if exists)
    char parent[HASH_HEX_SIZE + 1] = {0};
    FILE *pf = fopen(REF_MAIN, "r");
    if (pf) {
        fgets(parent, sizeof(parent), pf);
        parent[strcspn(parent, "\n")] = 0;
        fclose(pf);
    }

    // Build commit text
    char buffer[2048];
    char tree_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&tree_id, tree_hex);

    int len;
    if (strlen(parent) > 0) {
        len = snprintf(buffer, sizeof(buffer),
            "tree %s\nparent %s\nauthor PES Student\ntime %llu\n\n%s\n",
            tree_hex,
            parent,
            (unsigned long long)time(NULL),
            message
        );
    } else {
        len = snprintf(buffer, sizeof(buffer),
            "tree %s\nauthor PES Student\ntime %llu\n\n%s\n",
            tree_hex,
            (unsigned long long)time(NULL),
            message
        );
    }

    if (len <= 0) return -1;

    // Store commit
    if (object_write(OBJ_COMMIT, buffer, len, commit_id_out) != 0) {
        fprintf(stderr, "error: failed to write commit\n");
        return -1;
    }

    // Update branch
    update_ref(commit_id_out);

    return 0;
}

// Walk commits (log)
int commit_walk(commit_callback cb, void *ctx) {
    FILE *f = fopen(".pes/refs/heads/main", "r");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];
    if (!fgets(hex, sizeof(hex), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    hex[strcspn(hex, "\n")] = 0;

    while (strlen(hex) > 0) {
        ObjectID id;
        hex_to_hash(hex, &id);

        ObjectType type;
        void *data;
        size_t len;

        if (object_read(&id, &type, &data, &len) != 0)
            break;

        Commit commit;
        memset(&commit, 0, sizeof(commit));

        char parent_hex[HASH_HEX_SIZE + 1] = {0};

        char *line = strtok(data, "\n");
        while (line) {
            if (strncmp(line, "author ", 7) == 0) {
                strcpy(commit.author, line + 7);
            } 
            else if (strncmp(line, "time ", 5) == 0) {
                commit.timestamp = strtoull(line + 5, NULL, 10);
            } 
            else if (strncmp(line, "parent ", 7) == 0) {
                strcpy(parent_hex, line + 7);
            } 
            else if (line[0] == '\0') {
                line = strtok(NULL, "\n");
                if (line) strcpy(commit.message, line);
                break;
            }
            line = strtok(NULL, "\n");
        }

        cb(&id, &commit, ctx);

        free(data);

        if (strlen(parent_hex) == 0)
            break;

        strcpy(hex, parent_hex);
    }

    return 0;
}