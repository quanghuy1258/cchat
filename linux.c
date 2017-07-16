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

int isConnected = 0;

void * OnDataReceived(void *param) {
 int sockfd, ret;
 char buffer[DEFAULT_BUFLEN] = { 0 };
 sockfd = *((int*)param);
 do {
  ret = recvfrom(sockfd, buffer, DEFAULT_BUFLEN, 0, NULL, NULL);
  if (ret < 0)
   printf("error receiving data: socket[ %d ]\n", sockfd);
  else if (ret == 0)
   printf("connection closed: socket[ %d ]\n", sockfd);
  else
   printf("\n[FRIEND] %s\n", buffer);
 } while (ret > 0);
 isConnected = 0;
 return NULL;
}

int clientMode(char *ipServer, char *port) {
 struct sockaddr_in addr;
 int sockfd, ret;
 char buffer[DEFAULT_BUFLEN] = { 0 };
 pthread_t rThread;
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (sockfd < 0) {
  printf("error creating socket: domain[ %d ] type[ %d ] protocol[ %d ]\n", AF_INET, SOCK_STREAM, 0);
  return 1;
 }
 else printf("create socket: socket[ %d ] domain[ %d ] type[ %d ] protocol[ %d ]\n", sockfd, AF_INET, SOCK_STREAM, 0);
 memset(&addr, 0, sizeof(addr));
 addr.sin_family = AF_INET;
 addr.sin_addr.s_addr = inet_addr(ipServer);
 addr.sin_port = htons(atoi(port));
 ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
 if (ret < 0) {
  printf("error connecting to the server: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
  close(sockfd);
  return 1;
 }
 else printf("connect server: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
 ret = pthread_create(&rThread, NULL, OnDataReceived, &sockfd);
 if (ret) {
  printf("error: return Code from pthread_create() is %d\n", ret);
  close(sockfd);
  return 1;
 }
 isConnected = 1;
 printf("chat started\n\n");
 while (fgets(buffer, DEFAULT_BUFLEN, stdin) != NULL) {
  if (isConnected == 0) break;
  ret = sendto(sockfd, buffer, DEFAULT_BUFLEN, 0, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
   printf("error sending data\n");
   close(sockfd);
   pthread_exit(NULL);
   return 1;
  }
 }
 ret = shutdown(sockfd, SHUT_WR);
 if (ret < 0) 
  printf("shutdown failed\n");
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
  printf("error creating socket: domain[ %d ] type[ %d ] protocol[ %d ]\n", AF_INET, SOCK_STREAM, 0);
  return 1;
 }
 else printf("create socket: socket[ %d ]\n", sockfd);
 memset(&addr, 0, sizeof(addr));
 addr.sin_family = AF_INET;
 addr.sin_addr.s_addr = INADDR_ANY;
 addr.sin_port = htons(atoi(port));
 ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
 if (ret < 0) {
  printf("error binding: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
  close(sockfd);
  return 1;
 }
 else printf("bind: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
 ret = listen(sockfd, 3);
 if (ret < 0) {
  printf("error listening: socket[ %d ]\n", sockfd);
  close(sockfd);
  return 1;
 }
 else printf("listen: socket[ %d ]\n", sockfd);
 len = sizeof(cl_addr);
 newsockfd = accept(sockfd, (struct sockaddr *) &cl_addr, &len);
 if (newsockfd < 0) {
  printf("error accepting connection: sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", cl_addr.sin_family, cl_addr.sin_addr.s_addr, cl_addr.sin_port);
  close(sockfd);
  return 1;
 }
 else printf("accept connection: socket[ %d ] sin_family[ %hd ] s_addr[ %lu ] sin_port[ %hu ]\n", newsockfd, cl_addr.sin_family, cl_addr.sin_addr.s_addr, cl_addr.sin_port);
 ret = pthread_create(&rThread, NULL, OnDataReceived, &newsockfd);
 if (ret) {
  printf("error: return Code from pthread_create() is %d\n", ret);
  close(newsockfd);
  close(sockfd);
  return 1;
 }
 isConnected = 1;
 printf("chat started\n\n");
 while (fgets(buffer, DEFAULT_BUFLEN, stdin) != NULL) {
  if (isConnected == 0) break;
  ret = sendto(newsockfd, buffer, DEFAULT_BUFLEN, 0, (struct sockaddr *) &cl_addr, len);
  if (ret < 0) {
   printf("error sending data\n");
   close(newsockfd);
   close(sockfd);
   pthread_exit(NULL);
   return 1;
  }
 }
 ret = shutdown(newsockfd, SHUT_WR);
 if (ret < 0) 
  printf("shutdown failed\n");
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
