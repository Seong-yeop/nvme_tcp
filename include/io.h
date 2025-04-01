#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <string.h>

#include "admin.h"
#include "log.h"
#include "transport.h"
#include "nvme.h"


void start_io_queue(sock_t socket, struct nvme_cmd* conn_cmd);

void io_cmd_read(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);

void io_cmd_write(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status, void** data_buffer); 

#endif 