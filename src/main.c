#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>

#include "transport.h"
#include "nvme.h"
#include "discovery.h"

#define PORT 4420
char* DISCOVERY_NQN = "nqn.2014-08.org.nvmexpress.discovery";
char* SUBSYS_NQN    = "nqn.2019-11.fun.adamdjudge.tcpnvme";

/*
 * Procedure launched in its own thread which takes a client connection socket
 * and establishes an NVMe transport connection, then begins the command
 * processing loop of the appropriate queue pair.
 */
void* handle_connection(void* client_sock) {
	sock_t socket = *((sock_t*) client_sock);
	int err;
	struct nvme_cmd* conn_cmd;
	struct nvme_connect_params* params;
	struct nvme_status cqe = {0};
	
	// establish PDU-level connection
	err = init_connection(socket);
	if (err) {
		printf("Failed to initialize PDU-level connection\n");
		goto exit;
	}
	printf("PDU-level connection successful!\n");
	
	// receive commands until valid connection request is received
	while (1) {
		conn_cmd = recv_cmd(socket, (void**) &params);
		if (!conn_cmd) {
			printf("Failed to receive command\n");
			goto exit;
		}
		cqe.cid = conn_cmd->cid;

		// check parameters, break loop if they're valid
		if (conn_cmd->opcode != OPC_FABRICS || conn_cmd->nsid != FCTYPE_CONNECT) {
			printf("Received wrong command type!\n");
			cqe.sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);
		}
		else if (strcmp(SUBSYS_NQN, (char*) &(params->subnqn))
				&& strcmp(DISCOVERY_NQN, (char*) &(params->subnqn))) {
			printf("Received request for invalid SUBNQN: %s\n", SUBSYS_NQN);
			cqe.sf = make_sf(SCT_CMD_SPEC, SC_CONNECT_INVALID);
		}
		else break;

		// send status and loop
		err = send_status(socket, &cqe);
		if (err) {
			printf("Failed to send status\n");
			goto exit;
		}
	}

	// start appropriate queue processing based on connect request
	if (!strcmp(DISCOVERY_NQN, (char*) &(params->subnqn)))
		start_discovery_queue(socket, conn_cmd);

exit:
	printf("Closing connection and terminating thread\n");
	close(socket);
	pthread_exit(NULL);
	return 0;
}

/*
 * Initializes a virtual namespace file and sets up a listener socket, then
 * launches new threads to handle each client connection.
 */
int main(int argc, char** argv) {
	int sockfd, err, client;
	pthread_t thread;

	// create listener socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd <= 0) {
		perror("Socket creation error");
		return -1;
	}
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};
	err = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));
	if (err) {
		perror("Socket binding error");
		return -1;
	}
	listen(sockfd, 16);

	// accept new connections
	while (1) {
		client = accept(sockfd, NULL, NULL);
		pthread_create(&thread, NULL, handle_connection, &client);
		sleep(1);
	}
}

