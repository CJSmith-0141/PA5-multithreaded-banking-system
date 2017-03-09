#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define PORT "108141"
#define MESSAGE_SIZE 1024

static pthread_attr_t attr;

void* command_input_thread(void* arg)
{
  int fd;
  char request[MESSAGE_SIZE];
  size_t len;
  
  fd = *((int *)arg);
  free(arg);
  
  while (1) {
    memset(request, 0, MESSAGE_SIZE);
    printf("Enter command: \n");
    if (fgets(request, sizeof(request), stdin) == NULL) {
      printf("Invalid input.\n");
      continue;
    }
    //if the character right before the null is a newline the code scrolls annoyingly. So it needs removing!
    len = strlen(request);
    if (request[len-1] == '\n') {
      request[len-1] = '\0';
    }
    if (send(fd, request, len, 0) == -1) {
      perror("client: failed to send");
      exit(-1);
    }
    if (strncmp(request, "exit", 4) == 0) {
      pthread_exit(NULL);
    }
    sleep(2);
  }
}

void* response_output_thread(void* arg)
{
  int fd;
  size_t bytes_count;
  char response[MESSAGE_SIZE];
  
  fd = *((int *)arg);
  free(arg);
  while (1) {
    memset(response, 0, MESSAGE_SIZE);
    if ((bytes_count = recv(fd, response, MESSAGE_SIZE, 0)) == -1) {
      perror("client: failed to recv");
      exit(-1);
    }
    else if (bytes_count == 0) {
      printf("server has shutdown\n");
      exit(0);
    }
    if (strncmp(response, "exit", 4) == 0) {
      pthread_exit(NULL);
    }
    printf("%s\n", response);
  }
}

int main(int argc, char *argv[])
{
  int connection_fd;
  int *connection_fd_ptr;
  struct addrinfo hints, *res, *p;
  int status;
  pthread_t command, response;
 
  if (argc != 2) {
    fprintf(stderr,"usage: ./client remote_address\n");
    exit(-1);
  }
  hints.ai_flags = 0;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_addrlen = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL; //ALL THIS SOCKET STUFF IS COMPLETE MUMBO JUMBO TO ME. . .  LETS BE REAL 
  if ((status = getaddrinfo(argv[1], PORT, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(1);
  }
  printf("client: connecting to server\n");
  while (1) {
    for (p = res; p != NULL; p = p->ai_next) {
      if ((connection_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        continue;
      }
      
      if (connect(connection_fd, p->ai_addr, p->ai_addrlen) == -1) {
        close(connection_fd);
        continue;
      }
      break;
    }
    if (p != NULL) {
      break;
    }
    sleep(3);
  }
  freeaddrinfo(res);
  printf("client: connected to server.\n");
  if (pthread_attr_init(&attr) != 0) {
    perror("client: failed to initialize thread attr");
    exit(SIGTERM);
  }
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) {
    perror("client: failed to set thread attr");
    exit(SIGTERM);
  }
  connection_fd_ptr = (int *)malloc(sizeof(int));
  *connection_fd_ptr = connection_fd;
  if (pthread_create(&command, &attr, command_input_thread, connection_fd_ptr) != 0) {
    perror("client: failed to create command_input_thread");
    exit(SIGTERM);
  }
  connection_fd_ptr = (int *)malloc(sizeof(int));
  *connection_fd_ptr = connection_fd;
  if (pthread_create(&response, &attr, response_output_thread, connection_fd_ptr) != 0) {
    perror("client: failed to create response_output_thread");
    exit(SIGTERM);
  }
  pthread_join(command, NULL);
  pthread_join(response, NULL);
  printf("client: disconnected from server.\n");
  close(connection_fd);
  return 0;
}

