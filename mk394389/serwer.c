#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <dirent.h>

#include "structures.h"
#include "err.h"

#define PORT_NUM 6543
#define BUFF_SIZE 1024*512

int create_socket(short port_num) {
  struct sockaddr_in server_address;
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    syserr("socket");

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(port_num);

  if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    syserr("bind");
  if (listen(sock, SOMAXCONN) < 0)
    syserr("listen");

  return sock;
}

void send_list(int msg_sock, char *dir_name) {
  struct ServerAnswer server_answer;
  struct dirent *dir;
  DIR *d;
  uint32_t dir_len = 0, file_len;
  char *list;
  int i = 0;

  d = opendir(dir_name);
  list = (char *) calloc(1, sizeof(char));
  if (d != NULL) {
    while ((dir = readdir(d)) != NULL) {
      if (dir->d_type != DT_DIR) {
        file_len = strlen(dir->d_name);
        dir_len += file_len;
        if (i == 0) {
          list = (char *) realloc(list, dir_len + 1);
          strcpy(list, dir->d_name);
        } else {
          dir_len++;
          list = (char *) realloc(list, dir_len + 3);
          strcat(list, "|");
          strcat(list, dir->d_name);
        }
        i++;
      }
    }

    closedir(d);
    server_answer.id = htons(1);
    server_answer.len = htonl(dir_len);
    if (write(msg_sock, &server_answer, sizeof(server_answer)) != sizeof(server_answer))
      syserr("writing to client socket");
    if (write(msg_sock, list, dir_len) != strlen(list))
      syserr("writing to client socket");
    free(list);
  }
}

void get_file(int msg_sock, char *dir_name) {
  char *file_name, *path;
  uint32_t begin_address, number_bytes;
  uint16_t name_len;
  struct ClientRequest client_request;
  struct ServerAnswer server_answer;
  ssize_t len;
  int size;

  FILE *fp;
  char buffer[BUFF_SIZE];

  len = read(msg_sock, (char *) &client_request, sizeof(client_request));
  if (len < 0)
    syserr("reading from client socket");

  begin_address = ntohl(client_request.begin_address);
  number_bytes = ntohl(client_request.number_bytes);
  name_len = ntohs(client_request.name_len);

  if (number_bytes == 0) {
    server_answer.id = htons(2);
    server_answer.len = htonl(3);
    if (write(msg_sock, &server_answer, sizeof(server_answer)) != sizeof(server_answer))
      syserr("writing to client socket");
  } else {
    file_name = (char *) malloc(name_len + 1);
    len = read(msg_sock, file_name, name_len);
    if (len < 0) {
      syserr("reading from client socket");
    }
    path = (char *) malloc(strlen(dir_name) + name_len + 2);
    strcpy(path, dir_name);
    strcat(path, "/");
    strncat(path, file_name, name_len);

    fp = fopen(path, "r");
    if (fp == NULL) {
      server_answer.id = htons(2);
      server_answer.len = htonl(1);
      if (write(msg_sock, &server_answer, sizeof(server_answer)) != sizeof(server_answer))
        syserr("writing to client socket");
    } else {
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      if (begin_address > size - 1) {
        server_answer.id = htons(2);
        server_answer.len = htonl(2);
        if (write(msg_sock, &server_answer, sizeof(server_answer)) != sizeof(server_answer))
          syserr("writing to client socket");
      } else {
        server_answer.id = htons(3);
        if (begin_address + number_bytes + 1 > size)
          number_bytes = size - begin_address;
        server_answer.len = htonl(number_bytes);
        if (write(msg_sock, &server_answer, sizeof(server_answer)) != sizeof(server_answer))
          syserr("writing to client socket");
        fseek(fp, begin_address, SEEK_SET);
        while (number_bytes > 0) {
          int bytes_write = number_bytes < BUFF_SIZE ? number_bytes : BUFF_SIZE;
          if (fread(buffer, 1, bytes_write, fp) < bytes_write)
            syserr("fread");
          if (write(msg_sock, buffer, bytes_write) != bytes_write)
            syserr("writing to client socket");
          number_bytes -= bytes_write;
        }
      }
      fclose(fp);
    }

    free(path);
    free(file_name);
  }

}

int main(int argc, char *argv[]) {
  int sock, msg_sock;
  short port_num = PORT_NUM;
  struct sockaddr_in client_address;
  socklen_t client_address_len;

  struct RequestId request_id;
  ssize_t len;

  if (argc < 2)
    fatal("Usage: name of directory [host]\n");

  if (argc == 3)
    port_num = (short) strtol(argv[2], NULL, 0);

  sock = create_socket(port_num);

  for (;;) {
    client_address_len = sizeof(client_address);
    msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

    if (msg_sock < 0)
      syserr("accept");

    printf("client connected\n");

    for (int i = 0; i < 2; i++) {
      len = read(msg_sock, (char *) &request_id, sizeof(request_id));
      if (len < 0)
        syserr("reading from client socket");

      switch (ntohs(request_id.id)) {
        case 1:
          printf("listing files from directory\n");
          send_list(msg_sock, argv[1]);
          break;
        case 2:
          printf("sending file\n");
          get_file(msg_sock, argv[1]);
          i = 2;
          break;
      }
    }

    if (close(msg_sock) < 0)
      syserr("close");
    printf("ending connection\n");
  }

  return 0;
}