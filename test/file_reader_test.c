#include "file_reader.h"

int main(int argc, char *argv[]) {
    char *file_name = argc > 1 ? argv[1] : "dnsrelay.txt";
    struct file_reader *fr = file_reader_alloc(file_name);
    char domain[512];
    printf("input domain: \n");
    while(scanf("%s", domain)) {
        char ip[16];
        char *res = file_reader_get_a_record(fr, domain, ip);
        if (res == NULL) {
            printf("%s not found\n", domain);
        } else {
            printf("%s %s\n", domain, ip);
        }
        printf("input domain: \n");
    }
    return 0;
}