#include <stdio.h>
#include <string.h>

#include "log.h"
#include "transport.h"
#include "nvme.h"

/*
 * Starts command processing loop for the admin queue of the discovery
 * controller. Returns if the connection is broken.
 */
void start_discovery_queue(sock_t socket, struct nvme_cmd* conn_cmd);

/*
 * Processes an identify command.
 */
void discovery_identify(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);

/*
 * Processes a get log page command.
 */
void discovery_get_log(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);

