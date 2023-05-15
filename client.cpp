#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	int socket_;
	int len;
	struct sockaddr_in address;
	int result;
	char msg[80];

	socket_ = socket(AF_INET, SOCK_STREAM, 0);
	printf("Enter your message: \n");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(10000);

	len = sizeof(address);
	result = connect(socket_, (struct sockaddr *)&address, len);	
	if (result < 0)	
	{
		perror("connect");
		exit(1);
	}

	while(1){
		fgets(msg, sizeof msg, stdin);
		write(socket_, &msg, 80);
		if(!strcmp(msg, "-1")) break;
	}
	
	close(socket_);
	puts("Connection closed");
	exit(0);
}