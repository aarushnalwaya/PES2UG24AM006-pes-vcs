// commit.h — Commit object interface
//
// A commit ties together a tree snapshot, parent history, author info,
// and a human-readable message.

#ifndef COMMIT_H
#define COMMIT_H

#include "pes.h"

// Commit structure
typedef struct {
    ObjectID tree;
    char author[128];
    char message[512];
    uint64_t timestamp;
} Commit;

// 🔥 ADD THIS (VERY IMPORTANT)
typedef void (*commit_callback)(const ObjectID *id, const Commit *commit, void *ctx);

// Create commit
int commit_create(const char *message, ObjectID *commit_id_out);

// Walk commits
int commit_walk(commit_callback cb, void *ctx);

#endif