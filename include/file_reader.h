#pragma once

#include <stdio.h>

#include "hash.h"

struct file_reader {
    FILE *fd;
    HashTab *item_index;
};

struct file_reader *file_reader_alloc(const char *file_name);

void file_reader_free(struct file_reader *fr);

char *file_reader_get_a_record(struct file_reader *fr, char *domain, char *ip);
