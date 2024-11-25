#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

// Global variables
static _Atomic unsigned int cli_count = 0; // Tracks the number of connected clients
static int uid = 10; // Unique identifier for each client

// Structure to represent a client
typedef struct {
	struct sockaddr_in address; // Client's address
	int sockfd;                 // Socket file descriptor
	int uid;                    // Unique ID
	char name[32];              // Client's name
} client_t;

client_t *clients[MAX_CLIENTS]; // Array to store connected clients

// Mutex to synchronize client array access
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Utility function: Overwrite stdout prompt
void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

// Utility function: Trim newline character from a string
void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

// Utility function: Print a client's IP address
void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add a client to the client queue */
void queue_add(client_t *cl) {
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (!clients[i]) { // Find an empty spot
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove a client from the client queue */
void queue_remove(int uid) {
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i]) {
			if (clients[i]->uid == uid) {
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Broadcast a message to all clients except the sender */
void send_message(char *s, int uid) {
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i]) {
			if (clients[i]->uid != uid) { // Skip the sender
				if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Handle communication with an individual client */
void *handle_client(void *arg) {
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++; // Increment client count
	client_t *cli = (client_t *)arg;

	// Receive client's name
	if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1) {
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else {
		strcpy(cli->name, name); // Store the name
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid); // Notify others
	}

	bzero(buff_out, BUFFER_SZ);

	while (1) {
		if (leave_flag) break;

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0) {
			if (strlen(buff_out) > 0) {
				send_message(buff_out, cli->uid); // Broadcast message

				str_trim_lf(buff_out, strlen(buff_out)); // Remove newline
				printf("%s -> %s\n", buff_out, cli->name);
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid); // Notify others
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

	/* Remove client from queue and cleanup */
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--; // Decrement client count
	pthread_detach(pthread_self()); // Detach thread

	return NULL;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Server IP address
	int port = atoi(argv[1]); // Convert port to integer
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	/* Create socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	/* Ignore SIGPIPE signals */
	signal(SIGPIPE, SIG_IGN);

	/* Set socket options */
	if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0) {
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* Bind socket to address */
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	/* Start listening for incoming connections */
	if (listen(listenfd, 10) < 0) {
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("===== CHATROOM =====\n");

	while (1) {
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if ((cli_count + 1) == MAX_CLIENTS) {
			printf("No more places, please retry later: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Setup client structure */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to queue and create a thread for them */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
