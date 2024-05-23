#include "erproc.h"

int fd;
struct t_package package;

struct sockaddr_in adr = {0};
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: <IP> <Port>\n");
    exit(-1);
  }

  char* servIP = argv[1];
  int port = atoi(argv[2]);

  fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  adr.sin_family = AF_INET;
  adr.sin_port = htons(port);
  Inet_pton(AF_INET, servIP, &adr.sin_addr);
  Connect(fd, (struct sockaddr *) &adr, sizeof adr);
  int type = 0;
  send(fd, &type, sizeof type, 0);
  while (1) {
    recv(fd, &package, sizeof package, 0);
    if (package.type < 0) {
      printf("Received exit package\n");
      close(fd);
      printf("EXIT\n");
      exit(0);
    }
    sleep(rand() % 2 + 1);
    package.code1 = (int)package.c1;
    package.code2 = (int)package.c2;
    send(fd, &package, sizeof package, 0);
  }
}