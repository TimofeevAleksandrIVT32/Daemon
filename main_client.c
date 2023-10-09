#include <stdio.h>
#include <string.h>
#include "client.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Invalid number of arguments\n");
        return 1;
    } else if ((strcmp(argv[1],"end")) && (strcmp(argv[1],"size"))) {
        printf("Invalid argument\n");
        return 1;
    }
    if (client_socket(argv[1])) {
        return 1;
    }
    return 0;
}
