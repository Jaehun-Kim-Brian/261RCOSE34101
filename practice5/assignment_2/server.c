#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 100
#define NP_SEND "./server_to_client"
#define NP_RECEIVE "./client_to_server"

int main(void) {
	char receive_msg[BUFFER_SIZE], send_msg[BUFFER_SIZE];
	int receive_fd, send_fd;
	int value;
	/* (1) make NP_RECEIVE pipe              */
	/* (2) make NP_SEND pipe                 */
	/* (3) init receive_fd and send_fd       */

	unlink(NP_RECEIVE);
	unlink(NP_SEND);
	mkfifo(NP_RECEIVE, 0666);
	//make client_to_server pipe
	mkfifo(NP_SEND, 0666);
	//make server_to_client pipe
	receive_fd = open(NP_RECEIVE, O_RDONLY);
	send_fd = open(NP_SEND, O_WRONLY);

	while (1) {
		/* TODO 4 : read msg                     */
		if (read(receive_fd, receive_msg, BUFFER_SIZE) <= 0) {
			break;
		}

		printf("server : receive %s\n", receive_msg);

		value = atoi(receive_msg);

		sprintf(send_msg, "%d", value*value);
		printf("server : send %s\n", send_msg);

		/* TODO 5 : write msg                    */
		write(send_fd, send_msg, BUFFER_SIZE);

	}
	return 0;
}
