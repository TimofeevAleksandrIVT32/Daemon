#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <syslog.h>
#include "server.h"

//функция создания демона
int daemonize() {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        syslog(LOG_CRIT,"Cannot get maximum descriptor number");
        return 1;
    }

    pid_t pid;
    if ((pid = fork()) < 0) {
        syslog(LOG_CRIT,"Error calling fork function");
        return 1;
    } else if (pid) {
        exit(0);
    }

    if (setsid() < 0) {
        syslog(LOG_CRIT,"Error creating a new session");
        return 1;
    }

    if ((pid = fork()) < 0) {
        syslog(LOG_CRIT,"Error calling fork function");
        return 1;
    } else if (pid) {
        exit(0);
    }

    if (rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }
    for (unsigned int i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_CRIT,"Error when working with file descriptors");
        return 1;
    }
    return 0;
}

//функция сервера
int server_socket() {
    char soc[STR_SIZE];
    if (read_config(CONFIG_NAME, soc)) {
        return 1;
    }
    int connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (connection_socket == -1) {
        syslog(LOG_CRIT,"Socket creation error");
        return 1;
    }
    struct sockaddr_un socket_name;
    memset(&socket_name, 0, sizeof(socket_name));
    socket_name.sun_family = AF_UNIX;
    strncpy(socket_name.sun_path, soc, sizeof(socket_name.sun_path) - 1);
    if ((bind(connection_socket,(const struct sockaddr *) &socket_name, sizeof(socket_name))) == -1) {
        syslog(LOG_CRIT,"Bind socket error");
        unlink(soc);
        return 1;
    }
    if ((listen(connection_socket, SOMAXCONN)) == -1) {
        syslog(LOG_CRIT,"Listen error");
        close(connection_socket);
        unlink(soc);
        return 1;
    }
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        syslog(LOG_CRIT,"Epoll create error");
        close(connection_socket);
        unlink(soc);
        return 1;
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = connection_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_socket, &event) == -1) {
        syslog(LOG_CRIT,"Epoll ctl error");
        close(connection_socket);
        unlink(soc);
        return 1;
    }
    while(1) {
        int end_flag = 0;
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            syslog(LOG_CRIT,"Epoll wait error");
            close(connection_socket);
            unlink(soc);
            return 1;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == connection_socket) {
                int data_socket = accept(connection_socket, NULL, NULL);
                if (data_socket == -1) {
                    syslog(LOG_CRIT,"Accept error");
                    close(data_socket);
                    close(connection_socket);
                    unlink(soc);
                    return 1;
                }
                char buffer[STR_SIZE];
                if ((read(data_socket, buffer, sizeof(buffer))) == -1) {
                    syslog(LOG_CRIT,"Read error");
                    close(data_socket);
                    close(connection_socket);
                    unlink(soc);
                    return 1;
                }
                buffer[sizeof(buffer) - 1] = 0;
                if (!strcmp(buffer, "END")) {
                    end_flag = 1;
                } else {
                    int result = file_size(buffer);
                    sprintf(buffer, "%d", result);
                }
                if (write(data_socket, buffer, sizeof(buffer)) == -1) {
                    syslog(LOG_CRIT,"Write error");
                    close(data_socket);
                    close(connection_socket);
                    unlink(soc);
                    return 1;
                }
                close(data_socket);
            }
        }
        if (end_flag) {
            break;
        }
    }
    close(connection_socket);
    unlink(soc);
    return 0;
}

//функция чтения имени сокета из конфигурации
int read_config(char *file_name, char *soc) {
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        syslog(LOG_CRIT,"Failed to open file");
        return 1;
    }
    if (!fgets(soc, sizeof(soc), file)) {
        syslog(LOG_CRIT,"Error read socket");
        return 1;
    }
    fclose(file);
    return 0;
}

//функция нахождения размера файла
int file_size(char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        syslog(LOG_CRIT,"Failed to open file");
        return -1;
    }
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    fclose(file);
    return size;
}
