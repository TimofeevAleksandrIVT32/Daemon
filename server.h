#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>

#define CONFIG_NAME "config.txt"
#define STR_SIZE 100
#define MAX_EVENTS 100
#define EMPTY_DESC 0

void start_logging(char *program);
int daemonize();
int server_socket();
int read_config(char *config_name, char *soc, char *file_name);
int file_size(char *file_name);
int settings_server_socket(char *soc);
int listen_socket(int connection_socket, char *soc);
int settings_inotify(int connection_socket, char *soc);
int settings_epoll(int connection_socket, char *soc, int inotify_fd);
int wait_epoll(int connection_socket, char *soc, int epoll_fd, struct epoll_event *events, int inotify_fd);
int accept_connect(int connection_socket, char *soc, int inotify_fd);
int read_data_soc(int data_socket, int connection_socket, char *soc, char *buffer, int *end_flag, int size, int inotify_fd);
int write_data_soc(int data_socket, int connection_socket, char *soc, char *buffer, int inotify_fd);
void closing(int data_socket, int connection_socket, char *soc, int inotify_fd);
void finish_logging();
#endif /* SERVER_H */
