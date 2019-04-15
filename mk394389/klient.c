#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/stat.h>

#include "structures.h"
#include "err.h"

#define PORT_NUM "6543"
#define BUFF_SIZE 1024*512

int connect_to_server(char *host, char *port_num) {
  int sock, err;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  err = getaddrinfo(host, port_num, &addr_hints, &addr_result);

  if (err == EAI_SYSTEM)
    syserr("getaddrinfo: %s", gai_strerror(err));
  else if (err != 0)
    fatal("getaddrinfo: %s", gai_strerror(err));

  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if (sock < 0)
    syserr("socket");

  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
    syserr("connect");

  freeaddrinfo(addr_result);

  return sock;
}

void list_request(int sock, uint32_t *dir_len) {
  struct RequestId request_id;
  struct ServerAnswer server_answer;
  ssize_t len;

  request_id.id = htons(1);
  if (write(sock, &request_id, sizeof(request_id)) != sizeof(request_id))
    syserr("partial / failed write");

  len = read(sock, (char *) &server_answer, sizeof(server_answer));
  if (len < 0)
    syserr("read");

  *(dir_len) = ntohl(server_answer.len);
}

void get_list(int sock, uint32_t dir_len, char **list, uint32_t *begin_address, int *file_number, uint32_t *length) {
  char c;
  ssize_t len = read(sock, *list, dir_len);
  if (len < 0)
    syserr("read");

  char *token = (char *) malloc(dir_len + 1);
  strcpy(token, *list);
  token = strtok(token, "|");
  int i = 1;
  printf("Lista plików:\n");
  while (token != NULL) {
    printf("%d. %s, %ld\n", i++, token, strlen(token));
    token = strtok(NULL, "|");
  }

  do {
    printf("Wybierz plik do przesłania:\n");
    if (scanf("%d", file_number) < 0)
      syserr("scanf");
    c = getchar();
    if (c != '\n') {
      while (getchar() != '\n');
      printf("Każda wartość powinna być w osobnym wierszu!\n");
    }
  } while (c != '\n');

  do {
    printf("Wybierz adres początku fragmentu:\n");
    if (scanf("%d", begin_address) < 0)
      syserr("scanf");
    c = getchar();
    if (c != '\n') {
      while (getchar() != '\n');
      printf("Każda wartość powinna być w osobnym wierszu!\n");
    }
  } while (c != '\n');

  do {
    printf("Wybierz adres końca fragmentu:\n");
    if (scanf("%d", length) < 0)
      syserr("scanf");
    c = getchar();
    if (c != '\n') {
      while (getchar() != '\n');
      printf("Każda wartość powinna być w osobnym wierszu!\n");
    }
  } while (c != '\n');

  *length -= *begin_address;

  if (*length < 0)
    printf("Adres końca jest mniejszy niż początek!\n");
}

void write_file(int sock, char *token, uint16_t name_len, uint32_t begin_address) {
  ssize_t len;
  char *dir_name;
  char buffer[BUFF_SIZE];
  struct stat sb;
  struct ServerAnswer server_answer;
  FILE *fp;
  uint32_t remains;

  dir_name = (char *) malloc(5);
  strcpy(dir_name, "tmp/");

  len = read(sock, (char *) &server_answer, sizeof(server_answer));
  if (len < 0)
    syserr("read");

  switch (ntohs(server_answer.id)) {
    case 2:
      switch (ntohl(server_answer.len)) {
        case 1:
          printf("Zła nazwa pliku!\n");
          break;
        case 2:
          printf("Nieprawidłowy adres początku fragmentu!\n");
          break;
        case 3:
          printf("Zerowy rozmiar fragmentu!\n");
          break;
      }
      break;
    case 3:
      remains = ntohl(server_answer.len);
      if (stat(dir_name, &sb) == -1)
      mkdir(dir_name, S_IRWXU);

      dir_name = (char *) realloc(dir_name, strlen(dir_name) + name_len + 1);
      strcat(dir_name, token);

      fp = fopen(dir_name, "r+");
      if (fp == NULL) {
        fp = fopen(dir_name, "w+");
      }

      while (remains > 0) {
        memset(buffer, 0, sizeof(buffer));
        len = read(sock, buffer, sizeof(buffer) - 1);
        if (len < 0)
          syserr("read");

        fseek(fp, begin_address, SEEK_SET);
        fprintf(fp, "%.*s", (int) len, buffer);
        fseek(fp, len, SEEK_CUR);
        remains -= len;
      }
      fclose(fp);
      break;
  }
}

void send_file_request(int sock, char *list, uint32_t dir_len, uint32_t begin_address, uint32_t length, int file_number) {
  struct ClientRequest request;
  struct RequestId request_id;
  char *token;
  uint16_t name_len;

  request_id.id = htons(2);
  if (write(sock, &request_id, sizeof(request_id)) != sizeof(request_id))
    syserr("partial / failed write");

  token = (char *) malloc(dir_len + 1);
  strcpy(token, list);
  token = strtok(list, "|");
  for (int k = 1; k < file_number; k++)
    token = strtok(NULL, "|");
  name_len = strlen(token);

  request.begin_address = htonl(begin_address);
  request.number_bytes = htonl(length);
  request.name_len = htons(name_len);
  if (write(sock, &request, sizeof(request)) != sizeof(request))
    syserr("partial / failed write");
  if (write(sock, token, name_len) != name_len)
    syserr("partial / failed write");

  write_file(sock, token, name_len, begin_address);
}


int main(int argc, char *argv[]) {
  int sock;
  char *port_num = PORT_NUM;

  char *list;
  uint32_t dir_len = 0;
  int file_number;
  uint32_t begin_address, length;

  if (argc < 2)
    fatal("Usage: host [port]\n");

  if (argc == 3)
    port_num = argv[2];

  sock = connect_to_server(argv[1], port_num);

  list_request(sock, &dir_len);

  list = (char *) malloc(dir_len + 1);

  get_list(sock, dir_len, &list, &begin_address, &file_number, &length);

  send_file_request(sock, list, dir_len, begin_address, length, file_number);

  free(list);

  if (close(sock) < 0)
    syserr("close");
  return 0;
}