#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "structures.h"

#define PORT_NUM "6543"

int connect_to_server(char *host, char *port_num) {
  int sock, err;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  err = getaddrinfo(host, port_num, &addr_hints, &addr_result);

  if (err != 0)
    printf("error in getaddrinfo\n");

  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if (sock < 0)
    printf("error in socket\n");

  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
    printf("error in connnect\n");

  freeaddrinfo(addr_result);

  return sock;
}

void list_request(int sock, uint32_t *dir_len, char **list) {
  struct RequestId request_id;
  struct DirList dir_list;
  ssize_t len;

  request_id.id = htons(1);
  if (write(sock, &request_id, sizeof(request_id)) != sizeof(request_id)) {
    printf("error in write\n");
  }

  len = read(sock, (char *) &dir_list, sizeof(dir_list));
  if (len < 0) {
    printf("error in read\n");
  }

  *(dir_len) = ntohl(dir_list.len);
//  printf("%d\n", *(dir_len));
  *(list) = (char *) realloc(*(list), *(dir_len) + 1);
  len = read(sock, *(list), *(dir_len));
  if (len < 0) {
    printf("error in read\n");
  }
//  printf("%.*s\n", (int) len, *(list));
}

int main(int argc, char *argv[]) {
  int sock;
  char *port_num = PORT_NUM;

  struct ClientRequest request;
  struct RequestId request_id;
  ssize_t len;

  char *list;
  uint32_t dir_len = 0;
  int file_number;
  uint32_t begin_address, length;
  uint16_t name_len;

  if (argc < 2) {
    printf("Not enough arguments\n");
    return 1;
  }
  else if (argc == 3)
    port_num = argv[2];

  sock = connect_to_server(argv[1], port_num);

  list_request(sock, &dir_len, &list);

//  printf("%.*s, %d\n", 21, list, dir_len);

  char *token = (char *) malloc(dir_len + 1);
  strcpy(token, list);
  token = strtok(token, "|");
  int i = 1;
  printf("Lista plików:\n");
  while (token != NULL) {
    printf("%d. %s, %ld\n", i++, token, strlen(token));
    token = strtok(NULL, "|");
  }

  printf("Wybierz plik do przesłania:\n");
  scanf("%d", &file_number);
  printf("Wybierz adres początku fragmentu:\n");
  scanf("%d", &begin_address);
  printf("Wybierz adres końca fragmentu:\n");
  scanf("%d", &length);
  length -= begin_address;

  printf("%d %" PRIu32 " %" PRIu32 "\n", file_number, begin_address, length);

  request_id.id = htons(2);
  len = sizeof(request_id);
  if (write(sock, &request_id, sizeof(request_id)) != len) {
    printf("error in write\n");
  }

  token = (char *) malloc(dir_len + 1);
  strcpy(token, list);
  token = strtok(list, "|");
  for (int k = 1; k < file_number; k++)
    token = strtok(NULL, "|");
  name_len = strlen(token);

  request.begin_address = htonl(begin_address);
  request.number_bytes = htonl(length);
  request.name_len = htons(name_len);
  len = sizeof(request);
  if (write(sock, &request, sizeof(request)) != len)
    printf("error in write\n");
  if (write(sock, token, name_len) != name_len)
    printf("error in write\n");
  free(list);

  if (close(sock) < 0)
    printf("error in close\n");
  return 0;
}