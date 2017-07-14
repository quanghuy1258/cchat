#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_BUFLEN 512

void * OnDataReceived(void *param) {
 int sockfd, ret;
 char buffer[DEFAULT_BUFLEN] = { 0 };
 sockfd = *((int*)param);
 do {
  ret = recvfrom(sockfd, buffer, DEFAULT_BUFLEN, 0, NULL, NULL);
  if (ret < 0)
   printf("Error receiving data: socket[ %d ]\n", sockfd);
  else if (ret == 0)
   printf("Connection closed: socket[ %d ]\n", sockfd);
  else
   printf("\n[FRIEND] %s\n", buffer);
 } while (ret > 0);
 return NULL;
}

int clientMode(char *ipServer, char *port) {
 struct sockaddr_in addr;
 int sockfd, ret;
 char buffer[DEFAULT_BUFLEN] = { 0 };
 pthread_t rThread;
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (sockfd < 0) {
  printf("Error creating socket: domain[ %d ] type[ %d ] protocol[ %d ]\n", AF_INET, SOCK_STREAM, 0);
  return 1;
 }
 else printf("Create socket: socket[ %d ] domain[ %d ] type[ %d ] protocol[ %d ]\n", sockfd, AF_INET, SOCK_STREAM, 0);
 memset(&addr, 0, sizeof(addr));
 addr.sin_family = AF_INET;
 addr.sin_addr.s_addr = inet_addr(ipServer);
 addr.sin_port = htons(port);
 ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
 if (ret < 0) {
  printf("Error connecting to the server: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
  return 1;
 }
 else printf("Connect server: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
 ret = pthread_create(&rThread, NULL, OnDataReceived, &sockfd);
 if (ret) {
  printf("ERROR: Return Code from pthread_create() is %d\n", ret);
  return 1;
 }
 printf("Chat started\n");
 while (fgets(buffer, DEFAULT_BUFLEN, stdin) != NULL) {
  ret = sendto(sockfd, buffer, DEFAULT_BUFLEN, 0, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0)
   printf("Error sending data\n");
 }
 close(sockfd);
 pthread_exit(NULL);
 return 0;
}

int serverMode(char *port) {
 struct sockaddr_in addr, cl_addr;
 int sockfd, len, ret, newsockfd;
 char buffer[DEFAULT_BUFLEN] = { 0 };
 pthread_t rThread;
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (sockfd < 0) {
  printf("Error creating socket: domain[ %d ] type[ %d ] protocol[ %d ]\n", AF_INET, SOCK_STREAM, 0);
  return 1;
 }
 else printf("Create socket: socket[ %d ]\n", sockfd);
 memset(&addr, 0, sizeof(addr));
 addr.sin_family = AF_INET;
 addr.sin_addr.s_addr = INADDR_ANY;
 addr.sin_port = htons(port);
 ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
 if (ret < 0) {
  printf("Error binding: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
  return 1;
 }
 else printf("Bind: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
 ret = listen(sockfd, 3);
 if (ret < 0) {
  printf("Error listening: socket[ %d ]\n", sockfd);
  return 1;
 }
 else printf("Listen: socket[ %d ]\n", sockfd);
 len = sizeof(cl_addr);
 newsockfd = accept(sockfd, (struct sockaddr *) &cl_addr, &len);
 if (newsockfd < 0) {
  printf("Error accepting connection: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", cl_addr.sin_family, cl_addr.sin_addr.s_addr, cl_addr.sin_port);
  return 1;
 }
 else printf("Accept connection: socket[ %d ] sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", newsockfd, cl_addr.sin_family, cl_addr.sin_addr.s_addr, cl_addr.sin_port);
 ret = pthread_create(&rThread, NULL, OnDataReceived, &newsockfd);
 if (ret) {
  printf("ERROR: Return Code from pthread_create() is %d\n", ret);
  return 1;
 }
 printf("Chat started\n");
 while (fgets(buffer, DEFAULT_BUFLEN, stdin) != NULL) {
  ret = sendto(newsockfd, buffer, DEFAULT_BUFLEN, 0, (struct sockaddr *) &cl_addr, len);
  if (ret < 0) {
   printf("Error sending data\n");
   return 1;
  }
 }
 close(newsockfd);
 close(sockfd);
 pthread_exit(NULL);
 return 0;
}

int main(int argc, char **argv) {
 printf("==============================================================\n");
 printf("usage:\n");
 printf("- client: %s client ip-server port\n", argv[0]);
 printf("- server: %s server port\n", argv[0]);
 printf("==============================================================\n");
 if (argc < 3) return 1;
 if (strcmp(argv[1], "client") == 0 && argc == 4) return clientMode(argv[2], argv[3]);
 if (strcmp(argv[1], "server") == 0 && argc == 3) return serverMode(argv[2]);
 return 1;
}
