#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>

#include "log.h"
#include "transport.h"
#include "nvme.h"
#include "discovery.h"

#define PORT 4420
#define DISCOVERY_NQN "nqn.2014-08.org.nvmexpress.discovery"
#define SUBSYS_NQN    "nqn.2019-11.fun.adamdjudge.tcpnvme"

/*
 * Procedure launched in its own thread which takes a client connection socket
 * and establishes an NVMe transport connection, then begins the command
 * processing loop of the appropriate queue pair.
 */
void* handle_connection(void* client_sock) {
	sock_t socket = *((sock_t*) client_sock);
	int err, type;
	struct nvme_cmd* cmd;
	struct nvme_connect_params* params;
	struct nvme_status status = {0};
	log_info("Starting thread to handle new connection");
	
	// establish PDU-level connection
	err = init_connection(socket);
	if (err) {
		log_warn("Failed to initialize PDU-level connection");
		goto exit;
	}
	log_info("PDU-level connection established");
	
	// receive commands until valid connection request is received
	while (1) {
		cmd = recv_cmd(socket, (void**) &params);
		if (!cmd) {
			log_warn("Failed to receive command");
			goto exit;
		}
		status.cid = cmd->cid;
		type = 0;

		// check parameters, break loop if they're valid
		if (cmd->opcode == OPC_FABRICS && cmd->nsid == FCTYPE_CONNECT) {
			log_info("Connect subnqn: %s", (char*) &(params->subnqn));
			if (!strcmp(DISCOVERY_NQN, (char*) &(params->subnqn))) {
				type = 1;
				break;
			}
			else if (!strcmp(SUBSYS_NQN, (char*) &(params->subnqn))) {
				type = 2;
				break;
			}
			else {
				log_warn("Invalid subnqn: %s", (char*) &(params->subnqn));
				status.sf = make_sf(SCT_CMD_SPEC, SC_CONNECT_INVALID);
			}
		}
		else {
			log_warn("Received wrong command");
			status.sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);
		}

		// send status and loop
		err = send_status(socket, &status);
		if (err) {
			log_warn("Failed to send status");
			goto exit;
		}
	}

	// start appropriate queue processing based on connect request
	if (type == 1)
		start_discovery_queue(socket, cmd);

exit:
	log_warn("Closing connection and terminating thread");
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
		log_error("Socket creation error: %s", strerror(errno));
		return -1;
	}
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};
	err = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));
	if (err) {
		log_error("Socket binding error: %s", strerror(errno));
		return -1;
	}
	listen(sockfd, 16);
	log_info("Listening on port %d", PORT);

	// accept new connections
	while (1) {
		client = accept(sockfd, NULL, NULL);
		pthread_create(&thread, NULL, handle_connection, &client);
		sleep(1);
	}
}

