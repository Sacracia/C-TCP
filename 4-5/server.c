#include "erproc.h"

char stroka[6000];
int length;
int res[6000];

int workers[100];
int worker_count = 0;
int workers_alive = 0;

int server;

void PRINT() {
  for (int i = 0; i < length; ++i) {
    if (res[i] == 0) {
      printf("?? ");
    }
    else {
      printf("%#02x ", res[i]);
    }
  }
  printf("\n");
}

void EXIT(int sig) {
  if (sig == SIGINT) {
    close(server);
    printf("Server is shutting down...\n");
    exit(0);
  }
}

int pos = 0;
pthread_mutex_t lock;
pthread_mutex_t res_lock;
void* WorkerJob(void* args) {
  int workerId = ((int*)args)[0];
  int workerFd = ((int*)args)[1];
  free(args);
  struct t_package package;
  package.workerId = workerId;
  package.type = 0;
  while (1) {
    if (pos >= length) {
      package.type = -1;
      send(workerFd, &package, sizeof(package), 0);
      --workers_alive;
      if (workers_alive == 0) {
        printf("All workers are dead\n");
        EXIT(SIGINT);
      }
      return NULL;
    }
    pthread_mutex_lock(&lock);
    package.pos = pos;
    package.c1 = stroka[pos++];
    package.c2 = stroka[pos++];
    pthread_mutex_unlock(&lock);
    send(workerFd, &package, sizeof package, 0);
    //WAITING FOR RESPONSE WITH CODES
    recv(workerFd, &package, sizeof package, 0);
    //PRINT INTERMEDIATE RESULTS
    pthread_mutex_lock(&res_lock);
    res[package.pos] = package.code1;
    res[package.pos + 1] = package.code2;
    printf("After worker %d sent codes %x %x\n", package.workerId, package.code1, package.code2);
    PRINT();
    pthread_mutex_unlock(&res_lock);
    //IF EXIT PACKAGE GOT (WORKER INTERRUPTED)
    if (package.type < 0) {
      --workers_alive;
      return NULL;
    }
  }
}

struct sockaddr_in adr = {0};
void* Accepter(void* args) {
  socklen_t adrlen = sizeof adr;
  while (1) {
    int fd = Accept(server, (struct sockaddr *) &adr, &adrlen);
    int job;
    recv(fd, &job, sizeof job, 0);
    if (job == 0) { // worker
      ++workers_alive;
      workers[worker_count] = fd;
      int* args = malloc(sizeof(int) * 2);
      args[0] = worker_count;
      args[1] = fd;
      pthread_t tmp;
      pthread_create(&tmp, NULL, WorkerJob, args);
      printf("Worker %d connected\n", worker_count);
      ++worker_count;
    }
    else { //

    }
  }
}

int main(int argc, char* argv[]) {
  signal(SIGINT, EXIT);
  if (argc != 3) {
    printf("Usage: <Port> <File>\n");
    exit(-1);
  }

  int port = atoi(argv[1]);

  //READING INPUT STRING
  FILE* f = fopen(argv[2], "r");
  if (f == NULL) {
    printf("File not found!\n");
    exit(-1);
  }
  fgets(stroka, 6000, f);
  length = strlen(stroka);

  //INITS
  pthread_mutex_init(&lock, NULL);

  //CREATING SOCKET
  server = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  adr.sin_family = AF_INET;
  adr.sin_port = htons(port);
  Bind(server, (struct sockaddr *) &adr, sizeof adr);
  Listen(server, 5);
  pthread_t tmp;
  pthread_create(&tmp, NULL, Accepter, NULL);
  pthread_join(tmp, NULL);
  EXIT(SIGINT);
}