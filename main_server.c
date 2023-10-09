#include <stdio.h>
#include <string.h>
#include "server.h"

int main(int argc, char *argv[]) {
    int log_flag = 1;
    if (argc > 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }else if (argc == 2) {
        if (!strcmp(argv[1],"-d")) {
            log_flag = 0;
            if (daemonize()) {
                finish_logging();
                return 1;
            }
        }
        else {
            printf ("Invalid argument\n");
            return 1;
        }
    }
    if (log_flag) {
        start_logging(argv[0]);
    }
    int er = server_socket();
    if (log_flag) {
        finish_logging();
    }
    if (er) {
        return 1;
    }
    return 0;
}
