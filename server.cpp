#include <sys/types.h>
#include <sys/socket.h> // содержит определения флагов уровня сокета
#include <stdio.h>
#include <netinet/in.h> // Protocols are specified
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

volatile sig_atomic_t was_sig_hup = 0; //An integer type which can be accessed as an atomic entity even in the presence of asynchronous interrupts made by signals.	
//Объявление обработчика сигнала
void sig_hup_handler(int r)
{
	was_sig_hup = 1;
}

int main()
{
	int socket_ = 0;

	// Создание сокета
	// Первый параметр - домен
	// Второй параметр определяет тип канала связи с сокетом
	// Третий параметр протокол для канала связи. 0 - default for OS
	socket_ = socket(AF_INET, SOCK_STREAM, 0);	
	if(socket_ == -1){	
		perror("socket"); // Prints a textual description of the error code currently stored in the system variable errno to stderr.
		return -1;
	}
	
	// Установка настроек сокета
	// Первый параметр - сокет
	// Второй параметр - уровень, на которм определен параметр
	// Третий параметр - Параметр сокета
	// Четвертый параметр - указатель на буффер
	// Пятый параматр - размер буфера в байтах
	int buffer_pointer = 1;
	if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &buffer_pointer, sizeof(int)) == -1) {
		perror ("setsockopt");
		return -1;
	}

	// Установка адреса
	// Устанавливает первые n байтов области, начинающейся с s в нули (пустые байты).
	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));		
	addr.sin_family = AF_INET;	
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");	
	addr.sin_port = htons(10000);	

	// Присвоение имени сокету, возможно стоит использовать sockaddr_un (так как он для UNIX-доменов)
	if (bind(socket_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {	
		perror("bind");
		close(socket_);
		return -1;
	}
	
	// Ожидание запросов связи на сокете
	if (listen(socket_, SOMAXCONN) < 0) {	
		perror("listen");
		return -1;
	}

	// Регистрация обработчика событий
	struct sigaction sa;
	sigaction(SIGHUP, NULL, &sa);
	sa.sa_handler = sig_hup_handler;
	sa.sa_flags |= SA_RESTART;
	sigaction(SIGHUP, &sa, NULL);	

	// Блокировка сигнала
	sigset_t blockedMask, origMask;	
	sigemptyset(&blockedMask);
	sigaddset(&blockedMask, SIGHUP);
	sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

	char ch[80];
	int client_fd, res;
	int numberOfClients=0;

	
	while (!was_sig_hup)	
	{

		memset(ch, 0, sizeof(ch));	
		printf("---------------\n");
		printf("server waiting\n");

		int maxFd = -1;	

		fd_set fds;	
		FD_ZERO(&fds);	
		FD_SET(socket_, &fds);	


		// Подготовка списка фд
		if(socket_>maxFd) 
			maxFd = socket_;
		if(numberOfClients>0){
			FD_SET(client_fd, &fds);
			if(client_fd > maxFd) 
				maxFd = client_fd;
		}

		// pselect
		res = pselect (maxFd + 1, &fds, NULL, NULL, NULL, &origMask);	
		
		if(res == -1){
			if (errno == EINTR){	
				puts("Caught kill signal");
				return -1;
			}
		}

		if(res>=0)
		{
		    // Новый клиент
			if (FD_ISSET(socket_, &fds))	
			{
				
				int client_sockfd = accept(socket_, NULL, NULL);		
				if(client_sockfd == 0) perror("accept client_sockfd");

				if(numberOfClients>=1){
					close(client_sockfd);
                    printf("Another client accepted and disconnected\n");
					continue;
				}
				if(client_fd>=0){
					client_fd = client_sockfd;
					numberOfClients++;
					printf("Client accepted\n");
					printf("Num of clients %i accepted\n\n", numberOfClients);
				}
				else perror("accept");
				continue;
			}

            // События
			if (FD_ISSET(client_fd, &fds))
			{
				read(client_fd, &ch, 80);
				int k = strlen(ch);
				printf("Message from client: ");
				for(int i=0; i<k; i++) printf("%c", ch[i]);

				if(k>0){
					printf("\nsize = %d\n", k - 1);
					k=0;
				}
				else{
					close(client_fd);
					printf("Connection closed\n");
					client_fd=-1;
					numberOfClients--;
				}
			}
		}
	}
}