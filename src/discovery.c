#include "discovery.h"

static const char* nvme_opcode_name(u8 opcode) {
    switch (opcode) {
        case 0x02: return "Get Log Page";
        case 0x06: return "Identify";
        case 0x09: return "Set Features";
        case 0x0A: return "Get Features";
        case 0x0C: return "Asynchronous Event Request";
        case 0x18: return "Keep Alive";
		case 0x7F: return "Fabrics";
        default:   return "Unknown / Reserved";
    }
}


/*
 * Starts command processing loop for the admin queue of the discovery
 * controller. Returns if the connection is broken.
 */
void start_discovery_queue(sock_t socket, struct nvme_cmd* conn_cmd) {
	u16 qsize = conn_cmd->cdw11 & 0xffff;
	log_debug("Received NVME_CONNECT command, qsize=%u", qsize);
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
		log_warn("Failed to send response");
		return;
	}

	// processing loop
	struct nvme_cmd* cmd;
	while (1) {
		memset(&status, 0, NVME_STATUS_LEN);
		status.sqhd = sqhd++;
		if (sqhd >= qsize) sqhd = 0;
		cmd = recv_cmd(socket, NULL);
		if (!cmd) {
			log_warn("Failed to receive command");
			return;
		}
		status.cid = cmd->cid;
		log_debug("Got command: 0x%02x (%s)", cmd->opcode, nvme_opcode_name(cmd->opcode));

		if (cmd->opcode == OPC_FABRICS)
			fabric_cmd(&props, cmd, &status);
		else if (props.cc & 0x1) {
			switch (cmd->opcode) {
				case OPC_IDENTIFY:
					discovery_identify(socket, cmd, &status);
					break;
				case OPC_GET_LOG:
					discovery_get_log(socket, cmd, &status);
					break;
				default:
					status.sf = make_sf(SCT_GENERIC, SC_INVALID_OPCODE);
			}
		}
		else
			status.sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);

		free(cmd);
		err = send_status(socket, &status);
		if (err) {
			log_warn("Failed to send response");
			return;
		}
	}
}

/*
 * Processes an identify command.
 */
void discovery_identify(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
	log_debug("Identify: CNS=0x%x, NSID=0x%x", cmd->cdw10, cmd->nsid);
	if (cmd->cdw10 != CNS_ID_CTRL) {
		status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
		return;
	}
	struct nvme_identify_ctrl id_ctrl = {0};
	strcpy(id_ctrl.fr, "0.0.1");
	strcpy(id_ctrl.subnqn, DISCOVERY_NQN);
	id_ctrl.mdts = 1;
	id_ctrl.cntlid = 1;
	id_ctrl.maxcmd = 128;
	id_ctrl.ver = 0x10400;

	send_data(socket, cmd->cid, &id_ctrl, NVME_ID_CTRL_LEN);
}

/*
 * Processes a get log page command.
 */
void discovery_get_log(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
	int size;
	struct nvme_discovery_log_page* page;
	u8 lid = cmd->cdw10 & 0xff;
	int bytes = ((cmd->cdw10 >> 16) & 0x0fff) * 4 + 4;
	log_debug("Get log page: LID=0x%x, %d bytes", lid, bytes);
	if (lid != 0x70) {
		status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
		return;
	}

	// allocate the right amount of space for length requested
	if (bytes > NVME_DISCOVERY_LOG_PAGE_LEN)
		size = bytes;
	else
		size = NVME_DISCOVERY_LOG_PAGE_LEN;
	page = (struct nvme_discovery_log_page*) calloc(1, size);

	// fill out the data structure
	page->genctr = 0;
	page->numrec = 1;
	page->recfmt = 0;
	page->entry.trtype = 3; // tcp
	page->entry.adrfam = 1; // ipv4
	page->entry.subtype = 2;
	page->entry.treq = 0;
	page->entry.portid = 0;
	page->entry.cntlid = 1;
	page->entry.asqsz = 64;
	strcpy(page->entry.trsvcid, PORT_ASCII);
	strcpy(page->entry.subnqn, SUBSYS_NQN);
	strcpy(page->entry.traddr, "127.0.0.1");

	send_data(socket, cmd->cid, page, bytes);
	free(page);
}

