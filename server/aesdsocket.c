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
#include <pthread.h>
#include <time.h>

#define PORT 9000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//static void handler(int sig, siginfo_t *si, void *uc)
void timer_expiry_cb(union sigval timer_data)
{
	printf("timer cb\n");
	pthread_mutex_lock(&mutex);
	FILE *fp = NULL;
	char outstr[200];
	time_t t;
	struct tm *tmp;
	t = time(NULL);
	tmp = localtime(&t);
	strftime(outstr, sizeof(outstr), "timestamp:%Y-%m-%d %H:%M:%S\n", tmp);

	if ((fp = fopen("/var/tmp/aesdsocketdata", "a+")) != NULL)
	{
		fwrite(outstr, 1, strlen(outstr), fp); 
		fclose(fp);
	}
	pthread_mutex_unlock(&mutex);
}

struct thread_data
{
	int client_fd;
	pthread_t tid;
	FILE *fp;
};

struct node
{
	struct thread_data td;
	struct node *next;
} *head = NULL;

void* thread_func(void *args)
{
	struct node *thread_node = args;

	unsigned char buffer[1024] = {};

	for(int j=0;;)
	{
		FILE *fp = NULL;
		unsigned char c;
		ssize_t size_read = read(thread_node->td.client_fd, &c, sizeof(c));
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
			char *file_buffer = NULL;
			long size_of_file = 0;

			pthread_mutex_lock(&mutex);
			if ((fp = fopen("/var/tmp/aesdsocketdata", "a+")) != NULL)
			{
				fwrite(buffer, 1, strlen((char *)buffer), fp); 

				if (c == '\n')
				{
					size_of_file = ftell(fp);
					printf("size of file: %ld\n", size_of_file);
					if((file_buffer = calloc( 1, size_of_file + 1)) != NULL) {
						rewind(fp);
						fread(file_buffer, 1, size_of_file, fp);
					}
				}
				fclose(fp);
			}
			if (file_buffer)
			{
				send(thread_node->td.client_fd, file_buffer, size_of_file, 0);
				free(file_buffer);
			}
			pthread_mutex_unlock(&mutex);

		}
	}

	close(thread_node->td.client_fd);
	
	pthread_exit(pthread_self);
}

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

void timer_func(void)
{
	timer_t timerId = 0;
	struct sigevent sev = {0};
	struct itimerspec its = {   
		.it_value.tv_sec  = 10,
		.it_value.tv_nsec = 0,
		.it_interval.tv_sec  = 10,
		.it_interval.tv_nsec = 0
	};

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &timer_expiry_cb;
    sev.sigev_value.sival_ptr = NULL;

	timer_create(CLOCK_REALTIME, &sev, &timerId);

	/* start timer */
	timer_settime(timerId, 0, &its, NULL);
	printf("what is this\n");
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
		timer_func();	
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
			// Thread creation
			struct node *c = malloc(sizeof(struct node));
			struct node *h = head;
			while (h != NULL)
			{
				h = h->next;
			}
			c->next = NULL;
			h = c;

			if (head == NULL) head = c;

			c->td.client_fd = newfd; 
			pthread_create(&c->td.tid, NULL, &thread_func, c); 
			// Thread done

		}
		struct node *h = head;
		while (h != NULL)
		{
			pthread_join(h->td.tid, NULL);
			head = h = h->next;
			free(head);
		}
	}
	return 0;
}
