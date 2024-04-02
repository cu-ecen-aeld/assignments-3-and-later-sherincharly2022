#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define PORT 9000

void signal_handler(int sig)
{
	int save_err = errno; 

	if (sig == SIGINT) 
		printf("Signal interrupt happened");

	if (sig == SIGTERM)
		printf("signal termination happened");
	
	syslog(LOG_DEBUG, "Caught signal, exiting");

	errno = save_err;
	exit(0);
}

int main(int argc, char *argv[])
{
	int socketfd, newfd, opt = 1; 
	struct sockaddr_in address = {}, res = {};

	// openlog(NULL, 0, LOG_USER);

	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
	{
		syslog(LOG_DEBUG, "socket failed to open, %d, %s\n", errno, strerror(errno));
		return -1;
	}

	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		printf("set socketopt error, %d:%s\n", errno, strerror(errno));
		return -1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(socketfd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		printf("socket bind failed. %d: %s\n", errno, strerror(errno));
		close(socketfd);
	}

	if (listen(socketfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	FILE *fp = NULL;

	if ((fp = fopen("/var/tmp/aesdsocketdata", "w")) != NULL)
	{
		fclose(fp);
	}

	pid_t pid = 0;
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		 pid = fork();
	}

	if (pid == 0)
	{
		for (;;)
		{
			socklen_t address_len = (socklen_t)sizeof(struct sockaddr_in);
			if ((newfd = accept(socketfd, (struct sockaddr*)&res, &address_len)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			char dst[INET_ADDRSTRLEN] = {};
			syslog(LOG_DEBUG, "Accepted connection from %s", inet_ntop(AF_INET, &(res.sin_addr), dst, INET_ADDRSTRLEN));
			printf("connected IP: %s\n", inet_ntop(AF_INET, &(res.sin_addr), dst, INET_ADDRSTRLEN));

			unsigned char buffer[1024] = {};

			for(int j=0;;)
			{
				FILE *fp = NULL;
				unsigned char c;
				ssize_t size_read = read(newfd, &c, sizeof(c));
				if (size_read < 0) {
					perror("read erro");
					break;
				}
				else if (size_read == 0)
				{
					break;
				}

				buffer[j++] = c; 
				buffer[j] = 0;
				if (c == '\n' || (j == sizeof(buffer)-1))
				{
					j = 0;

					if ((fp = fopen("/var/tmp/aesdsocketdata", "a+")) != NULL)
					{
						fwrite(buffer, 1, strlen((char *)buffer), fp); 
						if (c == '\n')
						{
							long size_of_file = ftell(fp);
							printf("size of file: %ld\n", size_of_file);
							char *file_buffer = calloc( 1, size_of_file + 1);
							if (file_buffer)
							{
								rewind(fp);
								if (fread(file_buffer, 1, size_of_file, fp))
								{
									send(newfd, file_buffer, size_of_file, 0);
								}
								free(file_buffer);
							}
						}
						fclose(fp);
					}
				}
			}
			close(newfd);
		}
	}
//	else
//	{
//		close(socketfd);
//	}
	return 0;
}
