#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 4096

void handle_get(int client_socket) {
  FILE *fp = popen("wl-paste", "r");
  if (!fp) {
    write(client_socket, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
    return;
  }

  char buffer[BUFFER_SIZE];
  fread(buffer, 1, BUFFER_SIZE, fp);
  pclose(fp);

  char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
  write(client_socket, header, strlen(header));
  write(client_socket, buffer, strlen(buffer));
}

void handle_post(int client_socket, char *body) {
  char command[BUFFER_SIZE + 100];
  snprintf(command, sizeof(command), "echo \"%s\" | wl-copy", body);
  system(command);

  char response[] = "HTTP/1.1 200 OK\r\n\r\nClipboard updated";
  write(client_socket, response, strlen(response));
}

int main() {
  int server_fd, client_socket;
  struct sockaddr_in address;
  char buffer[BUFFER_SIZE];
  socklen_t addrlen = sizeof(address);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 5);

  printf("Clipboard server running on port %d...\n", PORT);

  while (1) {
    client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);

    if (strncmp(buffer, "GET /clip", 9) == 0) {
      handle_get(client_socket);
    } else if (strncmp(buffer, "POST /clip", 10) == 0) {
      char *body = strstr(buffer, "\r\n\r\n");
      if (body) {
        body += 4;
        handle_post(client_socket, body);
      }
    } else {
      write(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n", 26);
    }

    close(client_socket);
  }

  return 0;
}
