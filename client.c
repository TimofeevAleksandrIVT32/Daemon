#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "client.h"

//функция клиента
int client_socket(char *file_name) {
    char soc[STR_SIZE];
    if (read_config(CONFIG_NAME, soc)) {
        return 1;
    }
    int data_socket = settings_client_socket(soc);
    if (data_socket == -1) {
        return 1;
    }
    if (write_data_soc(data_socket, file_name)) {
        return 1;
    }
    if (read_data_soc(data_socket)) {
        return 1;
    }
    close(data_socket);
    return 0;
}

//функция чтения имени сокета из конфигурации
int read_config(char *file_name, char *soc) {
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }
    if (!fgets(soc, sizeof(soc), file)) {
        perror("Error read socket");
        fclose(file);
        return 1;
    }
    soc[strcspn(soc, "\n")] = 0;
    fclose(file);
    return 0;
}

//функция создания и настройки клиентского сокета
int settings_client_socket(char *soc) {
    int data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (data_socket == -1) {
        perror("Socket creation error");
        return data_socket;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, soc, sizeof(addr.sun_path) - 1);
    if (connect(data_socket, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("The server is down\n");
        close(data_socket);
        return -1;
    }
    return data_socket;
}

//функция записи данных в сокет
int write_data_soc(int data_socket, char *signal) {
    if (write(data_socket, signal, strlen(signal) + 1) == -1) {
        perror("Write error");
        close(data_socket);
        return 1;
    }
    return 0;

}

//функция чтения данных из сокета
int read_data_soc(int data_socket) {
    char buffer[STR_SIZE];
    if (read(data_socket, buffer, sizeof(buffer)) == -1) {
        perror("Read error");
        close(data_socket);
        return 1;
    }
    buffer[sizeof(buffer) - 1] = 0;
    print_result(buffer);
    return 0;
}

//функция вывода результата
void print_result(char *buffer) {
   if (!strcmp(buffer,"end")) {
        printf("Server shut down successfully\n");
    } else if (!strcmp(buffer,"-1")) {
        printf("File does not exist\n");
    } else {
        printf("File size = %s\n", buffer);
    } 
}
