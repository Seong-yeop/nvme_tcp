#include "discovery.h"

/*
 * Starts command processing loop for the admin queue of the discovery
 * controller. Returns if the connection is broken.
 */
void start_discovery_queue(sock_t socket, struct nvme_cmd* conn_cmd) {
	u16 qsize = conn_cmd->cdw11 & 0xffff;
	u16 sqhd = 2;
	struct nvme_properties props = {
		.cap  = ((u64)1<<37) | (4<<24) | (1<<16) | 63,
		.vs   = 0x10400,
		.cc   = 0,
		.csts = 0,
	};
	struct nvme_status status = {
		.dw0  = 1,
		.dw1  = 0,
		.sqhd = 1,
		.sqid = 0,
		.cid  = conn_cmd->cid,
		.sf   = 0,
	};
	int err = send_status(socket, &status);
	if (err) {
		printf("Failed to send response PDU\n");
		return;
	}

	// processing loop
	struct nvme_cmd* cmd;
	void *cmd_data, *resp_data;
	while (1) {
		resp_data = NULL;
		memset(&status, 0, NVME_STATUS_LEN);
		cmd = recv_cmd(socket, &cmd_data);
		if (!cmd) {
			printf("Failed to receive command\n");
			return;
		}
		status.cid = cmd->cid;
		status.sqhd = sqhd++;
		if (sqhd == qsize) sqhd = 0;

		if (cmd->opcode == OPC_FABRICS)
			fabric_cmd(&props, cmd, &status);
		else
			status.sf = make_sf(SCT_GENERIC, SC_INVALID_OPCODE);
		
		err = send_status(socket, &status);
		if (err) {
			printf("Failed to send response PDU\n");
			return;
		}
	}
}

