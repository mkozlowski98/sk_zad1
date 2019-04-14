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
#define QUEUE_LENGTH 5

int main(int argc, char *argv[]) {
  int sock, msg_sock;
  short port_num = PORT_NUM;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_address_len;

  struct ClientRequest client_request;
  struct RequestId request_id;
  struct DirList dir_list;
  ssize_t len;

  struct dirent *dir;
  DIR *d;
  uint32_t dir_len, file_len;
  char *list;

  if (argc == 3)
    port_num = (short) strtol(argv[2], NULL, 0);

  sock = socket(PF_INET, SOCK_STREAM, 0);

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(port_num);

  if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    printf("error in bind\n");

  if (listen(sock, QUEUE_LENGTH) < 0)
    printf("error in listen\n");

  for (;;) {
    client_address_len = sizeof(client_address);
    msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

    if (msg_sock < 0)
      printf("error in accept\n");

    printf("client connected\n");
    len = read(msg_sock, (char *) &request_id, sizeof(request_id));
    dir_len = 0;
    if (len < 0)
      printf("error in read\n");
    if (ntohs(request_id.id) == 1) {
      printf("listing dir\n");
      int i = 0;
      d = opendir(argv[1]);
      list = (char *) calloc(1, sizeof(char));
      if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
          if (dir->d_type != DT_DIR) {
            file_len = strlen(dir->d_name);
            dir_len += file_len;
            if (i == 0) {
              list = (char *) realloc(list, dir_len + 1);
              strcpy(list, dir->d_name);
            }
            else {
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
        len = read(msg_sock, (char *) &request_id, sizeof(request_id));
        if (len < 0)
          printf("error in read\n");
        else {
          printf("id: %" PRIu16 "\n", ntohs(request_id.id));
          len = read(msg_sock, (char *) &client_request, sizeof(client_request));
          if (len < 0)
            printf("error in read\n");
          else {
            printf("begin address: %" PRIu32 ", length: %" PRIu32 ", len_name: %" PRIu16 "\n",
              ntohl(client_request.begin_address), ntohl(client_request.number_bytes), ntohs(client_request.name_len));
          }
        }
        free(list);
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