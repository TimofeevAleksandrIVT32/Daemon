#ifndef SERVER_H
#define SERVER_H

#define CONFIG_NAME "config.txt"
#define STR_SIZE 100
#define MAX_EVENTS 100

int daemonize();
int server_socket();
int read_config(char *file_name, char *soc);
int file_size(char *file_name);
#endif /* SERVER_H */
