#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h> 

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512

void OnDataReceived(void *param) {
	int iResult;
	SOCKET soc = *((SOCKET*)param);
	char recvbuf[DEFAULT_BUFLEN] = { 0 };
	do {
		iResult = recv(soc, recvbuf, DEFAULT_BUFLEN, 0);
		if (iResult > 0)
			printf("\n[FRIEND] %s\n", recvbuf);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());
	} while (iResult > 0);
	_endthread();
}

int clientMode(char *ipServer, char *port) {
	WSADATA wsaData;
	int iResult;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	SOCKET ConnectSocket = INVALID_SOCKET;
	char sendbuf[DEFAULT_BUFLEN] = { 0 };
	HANDLE handle;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	iResult = getaddrinfo(ipServer, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server: %d!\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	handle = (HANDLE)_beginthread(OnDataReceived, 0, &ConnectSocket);
	printf("Chat started\n");
	while (fgets(sendbuf, DEFAULT_BUFLEN, stdin) != NULL) {
		iResult = send(ConnectSocket, sendbuf, DEFAULT_BUFLEN, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
	}
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	WaitForSingleObject(handle, INFINITE);
	closesocket(ConnectSocket);
	WSACleanup();
	return 0;
}

int serverMode(char *port) {
	WSADATA wsaData;
	int iResult;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	char sendbuf[DEFAULT_BUFLEN] = { 0 };
	HANDLE handle;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	iResult = listen(ListenSocket, 3);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ListenSocket);
	handle = (HANDLE)_beginthread(OnDataReceived, 0, &ClientSocket);
	printf("Chat started\n");
	while (fgets(sendbuf, DEFAULT_BUFLEN, stdin) != NULL) {
		iResult = send(ClientSocket, sendbuf, DEFAULT_BUFLEN, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	}
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	WaitForSingleObject(handle, INFINITE);
	closesocket(ClientSocket);
	WSACleanup();
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