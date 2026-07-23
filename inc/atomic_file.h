#pragma once

#include "header.h"

// Writes a complete small text file through an exclusively-created sibling
// temporary file, flushes it to stable storage, then atomically replaces the
// destination. Concurrent writers cannot share/truncate a temporary file;
// the prior valid file remains intact if any earlier step fails.
bool atomic_write_text_file(const char *path, const char *contents, size_t length);
