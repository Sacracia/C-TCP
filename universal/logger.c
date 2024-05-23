#include "erproc.h"

int fd;
void EXIT(int sig) {
  if (sig == SIGINT) {
    close(fd);
    printf("EXIT\n");
    exit(0);
  }
}

struct sockaddr_in adr = {0};
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: <IP> <Port>\n");
    exit(-1);
  }
  signal(SIGINT, EXIT);

  char* servIP = argv[1];
  int port = atoi(argv[2]);

  fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  adr.sin_family = AF_INET;
  adr.sin_port = htons(port);
  Inet_pton(AF_INET, servIP, &adr.sin_addr);
  Connect(fd, (struct sockaddr *) &adr, sizeof adr);
  int job = 1;
  send(fd, &job, sizeof job, 0);
  char buf[1500];
  while (1) {
    bzero(buf, sizeof buf);
    recv(fd, buf, sizeof buf, 0);
    if (buf[0] == 'E') {
      printf("Received exit\n");
      EXIT(SIGINT);
    }
    printf("%s", buf);
  }
}