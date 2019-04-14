//#include <sys/types.h>
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

int main(int argc, char *argv[]) {
  int sock;
  char *port_num = PORT_NUM;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  int err;
  struct RequestId request_id;
  struct DirList dir_list;
  ssize_t len;

  if (argc < 2) {
    printf("Not enough arguments\n");
    return 1;
  }
  else if (argc == 3)
    port_num = argv[2];

  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  err = getaddrinfo(argv[1], port_num, &addr_hints, &addr_result);

  if (err != 0)
    printf("error in getaddrinfo\n");

  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if (sock < 0)
    printf("error in socket\n");

  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
    printf("error in connnect\n");

  freeaddrinfo(addr_result);

  request_id.id = htons(1);
  len = sizeof(request_id);

  if (write(sock, &request_id, sizeof(request_id)) != len) {
    printf("error in write\n");
  }

  len = read(sock, (char *) &dir_list, sizeof(dir_list));
  if (len < 0)
    printf("error in read\n");
  else {
    printf("id: %" PRIu16 ", len: %" PRIu32 "\n", ntohs(dir_list.id), ntohl(dir_list.len));

  }

  if (close(sock) < 0)
    printf("error in close\n");
  return 0;
}