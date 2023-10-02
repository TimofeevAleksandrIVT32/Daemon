#ifndef CLIENT_H
#define CLIENT_H

#define CONFIG_NAME "config.txt"
#define STR_SIZE 100

int client_socket(char *file_name);
int read_config(char *file_name, char *soc);
#endif /* CLIENT_H */
