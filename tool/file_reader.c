#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "clog.h"
#include "hash.h"

static void file_reader_indexing(struct file_reader *fr) {
    char ip[16], domain[512];
    long cur;
    fseek(fr->fd, 0, SEEK_SET);
    while(cur = ftell(fr->fd), fscanf(fr->fd, "%s %s", ip, domain) != EOF) {
        insert(fr->item_index, domain, &cur, sizeof(cur));
    }
}

struct file_reader *file_reader_alloc(const char *file_name) {
    struct file_reader *fr = calloc(1, sizeof(struct file_reader));
    log_check(fr != NULL, "calloc failed");

    fr->fd = fopen(file_name, "r");
    log_check(fr->fd != NULL, "fopen failed");

    fr->item_index = NewHashTab();
    log_check(fr->item_index != NULL, "NewHashTab failed");

    file_reader_indexing(fr);

    return fr;
}

void file_reader_free(struct file_reader *fr) {
    log_check(fr != NULL && fr->fd != NULL && fr->item_index != NULL, "invalid file_reader");

    fclose(fr->fd);
    DestroyHashTab(fr->item_index);
    free(fr);
}

char *file_reader_get_a_record(struct file_reader *fr, char *domain, char *ip) {
    long *pcur = search(fr->item_index, domain);
    if (pcur == NULL) {
        return NULL;
    }

    char tdomain[512];
    fseek(fr->fd, *pcur, SEEK_SET);
    fscanf(fr->fd, "%s %s", ip, tdomain);

    if (strcmp(domain, tdomain) != 0) {
        file_reader_indexing(fr);
        return NULL;
    }

    return ip;
}