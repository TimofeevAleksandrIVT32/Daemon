#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <syslog.h>
#include "server.h"

//функция открытия журнала
void start_logging(char *program) {
    openlog(program, LOG_PERROR, LOG_USER);
}

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
    char file_name[STR_SIZE];
    if (read_config(CONFIG_NAME, soc, file_name)) {
        return 1;
    }
    int connection_socket = settings_server_socket(soc);
    if (connection_socket == -1) {
        return 1;
    }
    if (listen_socket(connection_socket, soc)) {
        return 1;
    }
    int inotify_fd = settings_inotify(connection_socket, soc);
    if (inotify_fd == -1) {
        return 1;
    }
    int epoll_fd = settings_epoll(connection_socket, soc, inotify_fd);
    if (epoll_fd == -1) {
        return 1;
    }
    int size = file_size(file_name);
    while(1) {
        int end_flag = 0;
        struct epoll_event events[MAX_EVENTS];
        int nfds = wait_epoll(connection_socket, soc, epoll_fd, events, inotify_fd);
        if (nfds == -1) {
            return 1;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == connection_socket) {
                int data_socket = accept_connect(connection_socket, soc, inotify_fd);
                if (data_socket == -1) {
                    return 1;
                }
                char buffer[STR_SIZE];
                if (read_data_soc(data_socket, connection_socket, soc, buffer, &end_flag, size, inotify_fd)) {
                    return 1;
                }
                if (write_data_soc(data_socket, connection_socket, soc, buffer, inotify_fd)) {
                    return 1;
                }
                close(data_socket);
            } else if (events[i].data.fd == inotify_fd) {
                size = file_size(file_name);
            }
        }
        if (end_flag) {
            break;
        }
    }
    closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
    return 0;
}

//функция чтения имени сокета из конфигурации
int read_config(char *config_name, char *soc, char *file_name) {
    FILE *file = fopen(config_name, "rb");
    if (!file) {
        syslog(LOG_CRIT,"Failed to open file");
        return 1;
    }
    if (!fgets(soc, STR_SIZE, file)) {
        syslog(LOG_CRIT,"Error read socket");
        return 1;
    }
    soc[strcspn(soc, "\n")] = 0;
    if (!fgets(file_name, STR_SIZE, file)) {
        syslog(LOG_CRIT,"Error read file name");
        return 1;
    }
    file_name[strcspn(file_name, "\n")] = 0;
    fclose(file);
    return 0;
}

//функция создания и настройки серверного сокета
int settings_server_socket(char *soc) {
    int connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (connection_socket == -1) {
        syslog(LOG_CRIT,"Socket creation error");
        return connection_socket;
    }
    struct sockaddr_un socket_name;
    memset(&socket_name, 0, sizeof(socket_name));
    socket_name.sun_family = AF_UNIX;
    strncpy(socket_name.sun_path, soc, sizeof(socket_name.sun_path) - 1);
    if ((bind(connection_socket,(const struct sockaddr *) &socket_name, sizeof(socket_name))) == -1) {
        syslog(LOG_CRIT,"Bind socket error");
        unlink(soc);
        return -1;
    }
    return connection_socket;
}

//функция прослушивания сокета
int listen_socket(int connection_socket, char *soc) {
    if ((listen(connection_socket, SOMAXCONN)) == -1) {
        syslog(LOG_CRIT,"Listen error");
        closing(EMPTY_DESC, connection_socket, soc, EMPTY_DESC);
        return 1;
    }
    return 0;
}

//функция создания и настройки дескриптора inotify
int settings_inotify(int connection_socket, char *soc) {
    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        syslog(LOG_CRIT,"Inotify initialization error");
        closing(EMPTY_DESC, connection_socket, soc, EMPTY_DESC);
        return inotify_fd;
    }

    int watch_descriptor = inotify_add_watch(inotify_fd, ".", IN_MODIFY);
    if (watch_descriptor == -1) {
        syslog(LOG_CRIT,"Error adding to inotify");
        closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
        return -1;
    }
    return inotify_fd;
}

//функция создания и настройки epoll
int settings_epoll(int connection_socket, char *soc, int inotify_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        syslog(LOG_CRIT,"Epoll create error");
        closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
        return epoll_fd;
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = connection_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_socket, &event) == -1) {
        syslog(LOG_CRIT,"Epoll ctl error");
        closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
        return -1;
    }
    event.events = EPOLLIN;
    event.data.fd = inotify_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inotify_fd, &event) == -1) {
        syslog(LOG_CRIT,"Error adding inotify to epoll");
        closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
        return -1;
    }
    return epoll_fd;
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

//функция ожидания событий
int wait_epoll(int connection_socket, char *soc, int epoll_fd, struct epoll_event *events, int inotify_fd) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        syslog(LOG_CRIT,"Epoll wait error");
        closing(EMPTY_DESC, connection_socket, soc, inotify_fd);
    }
    return nfds;
}

//функция установления соединения
int accept_connect(int connection_socket, char *soc, int inotify_fd) {
    int data_socket = accept(connection_socket, NULL, NULL);
    if (data_socket == -1) {
        syslog(LOG_CRIT,"Accept error");
        closing(data_socket, connection_socket, soc, inotify_fd);
    }
    return data_socket;
}

//функция чтения данных из сокета
int read_data_soc(int data_socket, int connection_socket, char *soc, char *buffer, int *end_flag, int size, int inotify_fd) {
    if ((read(data_socket, buffer, sizeof(buffer))) == -1) {
    syslog(LOG_CRIT,"Read error");
    closing(data_socket, connection_socket, soc, inotify_fd);
        return 1;
    }
    buffer[sizeof(buffer) - 1] = 0;
    if (!strcmp(buffer, "end")) {
        *end_flag = 1;
    } else {
        sprintf(buffer, "%d", size);
    }
    return 0;
}

//функция записи данных в сокет
int write_data_soc(int data_socket, int connection_socket, char *soc, char *buffer, int inotify_fd) {
    if (write(data_socket, buffer, sizeof(buffer)) == -1) {
        syslog(LOG_CRIT,"Write error");
        closing(data_socket, connection_socket, soc, inotify_fd);
        return 1;
    }
    return 0;
}

//функция закрытия сокета и inotify
void closing(int data_socket, int connection_socket, char *soc, int inotify_fd) {
    if (data_socket) {
        close(data_socket);
    }
    if (connection_socket) {
        close(connection_socket);
    }
    if (soc) {
        unlink(soc);
    }
    if (inotify_fd) {
        close(inotify_fd);
    }
}

//функция закрытия журнала
void finish_logging() {
    closelog();
}
