#include <stdio.h>
#include "server.h"

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }else if (argc == 2) {
        if (argv[1][0] == 'd') {
            if (daemonize()) {
                return 1;
            }
        }
        else {
            printf ("Invalid argument\n");
            return 1;
        }
    }
    if (server_socket()) {
        return 1;
    }
    return 0;
}
