#include "erproc.h"

char stroka[500];
int length;
int res[500];

int workers[100];
int worker_count = 0;
int workers_alive = 0;
int loggers[100];
int logger_count = 0;

int server;

pthread_mutex_t logger_lock;
void Log(char* log) {
  pthread_mutex_lock(&logger_lock);
  if (strncmp(log, "EXIT", 4) != 0)
    printf("%s",log);
  for (int i = 0; i < logger_count; ++i) {
    if (send(loggers[i], log, 1500, 0) < 0)
      printf("ERROR\n");
  }
  pthread_mutex_unlock(&logger_lock);
}

void PRINT() {
  char txt[1500];
  bzero(txt, sizeof txt);
  int txtp = 0;
  for (int i = 0; i < length; ++i) {
    if (res[i] == 0) {
      sprintf(txt + txtp, "?? ");
      txtp += 3;
    }
    else {
      sprintf(txt + txtp, "0x%x ", res[i]);
      txtp += 5;
    }
  }
  txt[txtp] = '\n';
  Log(txt);
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
  char buf[1500];
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
    bzero(buf, sizeof buf);
    sprintf(buf, "Sending \'%c\' \'%c\' to worker %d\n", package.c1, package.c2, package.workerId);
    Log(buf);
    send(workerFd, &package, sizeof package, 0);
    //WAITING FOR RESPONSE WITH CODES
    recv(workerFd, &package, sizeof package, 0);
    //PRINT INTERMEDIATE RESULTS
    pthread_mutex_lock(&res_lock);
    res[package.pos] = package.code1;
    res[package.pos + 1] = package.code2;
    bzero(buf, sizeof buf);
    sprintf(buf, "After worker %d sent codes %x %x\n", package.workerId, package.code1, package.code2);
    Log(buf);
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
  //printf("Server IP address = %s. Wait...\n", inet_ntoa(adr.sin_addr));
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
    else if (job == 1) { // logger
      loggers[logger_count] = fd;
      printf("Logger %d connected\n", logger_count);
      ++logger_count;
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
  fgets(stroka, 500, f);
  length = strlen(stroka);
  if (length > 500) {
    printf("Input string is too long!\n");
    exit(-1);
  }

  //INITS
  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&res_lock, NULL);
  pthread_mutex_init(&logger_lock, NULL);

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