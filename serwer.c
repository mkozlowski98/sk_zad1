#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT_NUM 6543
#define QUEUE_LENGTH 5

int main(int argc, char *argv[]) {
  int sock, msg_sock;
  short port_num = PORT_NUM;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_address_len;

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

    if (close(msg_sock))
      printf("error in close\n");
  }

  return 0;
}