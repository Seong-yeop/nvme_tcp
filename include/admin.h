#include <stdio.h>
#include <string.h>

#include "log.h"
#include "transport.h"
#include "nvme.h"



#define FEATURE_NUMBER_OF_QUEUES 0x07
#define FEATURE_ASYNC_EVENT_CONFIG 0x0b
#define FEATURE_CONTROLLER_RESET 0x20

/*
 * Starts command processing loop for the admin queue of the subsystem
 * controller. Returns if the connection is broken.
 */
void start_admin_queue(sock_t socket, struct nvme_cmd* conn_cmd);

void admin_identify(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);

void admin_set_features(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);