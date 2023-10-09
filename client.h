#ifndef CLIENT_H
#define CLIENT_H

#define CONFIG_NAME "config.txt"
#define STR_SIZE 100

int client_socket(char *file_name);
int read_config(char *file_name, char *soc);
int settings_client_socket(char *soc);
int write_data_soc(int data_socket, char *file_name);
int read_data_soc(int data_socket);
void print_result(char *buffer);
#endif /* CLIENT_H */
