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

#define PORT_NUM 6543

void send_list(int msg_sock, char *dir_name) {
  struct DirList dir_list;
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

    printf("%s, %ld\n", list, strlen(list));
    closedir(d);
    dir_list.id = htons(1);
    dir_list.len = htonl(dir_len);
    if (write(msg_sock, &dir_list, sizeof(dir_list)) != sizeof(dir_list))
      printf("error in write\n");
    if (write(msg_sock, list, dir_len) != strlen(list))
      printf("error in write with list\n");
    free(list);
  }
}

int main(int argc, char *argv[]) {
  int sock, msg_sock;
  short port_num = PORT_NUM;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_address_len;

  struct ClientRequest client_request;
  struct RequestId request_id;
  ssize_t len;

  uint32_t begin_address, number_bytes;
  uint16_t name_len;
  char *file_name;

  if (argc == 3)
    port_num = (short) strtol(argv[2], NULL, 0);

  sock = socket(PF_INET, SOCK_STREAM, 0);

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(port_num);

  if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    printf("error in bind\n");

  if (listen(sock, SOMAXCONN) < 0)
    printf("error in listen\n");

  for (;;) {
    client_address_len = sizeof(client_address);
    msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

    if (msg_sock < 0)
      printf("error in accept\n");

    printf("client connected\n");

    for (int i = 0; i < 2; i++) {
      len = read(msg_sock, (char *) &request_id, sizeof(request_id));
      if (len < 0)
        printf("error in read\n");

      switch (ntohs(request_id.id)) {
        case 1:
          printf("listing dir\n");
          send_list(msg_sock, argv[1]);
          break;
        case 2:
          printf("send file\n");
          len = read(msg_sock, (char *) &client_request, sizeof(client_request));
          if (len < 0) {
            printf("error in read\n");
            break;
          }
          begin_address = ntohl(client_request.begin_address);
          number_bytes = ntohl(client_request.number_bytes);
          name_len = ntohs(client_request.name_len);
          file_name = (char *) malloc(name_len + 1);
          len = read(msg_sock, file_name, name_len);
          if (len < 0) {
            printf("error in read\n");
            break;
          }
          printf("file name is: %.*s\n", (int) len, file_name);
          free(file_name);
          i = 2;
          break;
      }
    }
//    prev_len = 0;
//
//    do {
//      remains = sizeof(client_request) - prev_len;
//
//      if (len < 0)
//        printf("error in read\n");
//      else if (len > 0) {
//        printf("read from socket: %zd bytes\n", len);
//        prev_len += len;
//
//        if (prev_len == sizeof(client_request)) {
//          printf("received id %" PRIu16 ", begin address %" PRIu32 ", length %" PRIu32 ", name length %" PRIu16 "\n", ntohs(client_request.id),
//            ntohl(client_request.begin_address), ntohl(client_request.number_bytes), ntohs(client_request.name_len));
//        }
//      }
//    } while (len > 0);

    if (close(msg_sock) < 0)
      printf("error in close\n");
    printf("ending connection\n");
  }

  return 0;
}