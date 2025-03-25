#include <stdint.h>
#include <string.h>
#include "admin.h"
#include "nvme.h"

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
void start_admin_queue(sock_t socket, struct nvme_cmd* conn_cmd) {
	log_info("Starting new admin queue");
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
					admin_identify(socket, cmd, &status);
					break;
				case OPC_GET_LOG:
					// discovery_get_log(socket, cmd, &status);
					break;

				case OPC_SET_FEATURES:
					admin_set_features(socket, cmd, &status);
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
void admin_identify(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
	log_debug("Identify: CNS=0x%x, NSID=0x%x", cmd->cdw10, cmd->nsid);

	switch (cmd->cdw10) {
		case CNS_ID_NS: {

			break;
		}
		case CNS_ID_CTRL: {
			struct nvme_identify_ctrl id_ctrl = {0};
			strcpy(id_ctrl.fr, "0.0.1");
			strcpy(id_ctrl.subnqn, SUBSYS_NQN);
			id_ctrl.mdts = 1;
			id_ctrl.cntlid = 1;
			id_ctrl.maxcmd = 11;
			id_ctrl.nn = 1;
			id_ctrl.ver = 0x10400;
			
			send_data(socket, cmd->cid, &id_ctrl, NVME_ID_CTRL_LEN);
			break;
		}
		case CNS_ID_ACTIVE_NSID: {
			struct identify_active_namespace_list_data id_active_ns = {0};
			memset(&id_active_ns, 0, sizeof(id_active_ns));
			id_active_ns.cns[0] = htole32(0x1); // NSID=1
			send_data(socket, cmd->cid, &id_active_ns, sizeof(id_active_ns));
			break;
		}
		default:
			status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
			break;
	}

}



#define FEATURE_NUMBER_OF_QUEUES 0x07
#define FEATURE_ASYNC_EVENT_CONFIG 0x0b
#define FEATURE_CONTROLLER_RESET 0x20

void admin_set_features(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
    uint8_t fid = cmd->cdw10 & 0xff;
    uint32_t result = 0;  
    
    log_debug("Set Features: FID=0x%x, CDW11=0x%x, CDW12=0x%x", fid, cmd->cdw11, cmd->cdw12);

    switch (fid) {
        case FEATURE_CONTROLLER_RESET:
            log_debug("Controller Reset requested (FID=0x20).");
            break;

        case FEATURE_NUMBER_OF_QUEUES: {
            // CDW11에서 Queue의 수를 설정
            uint16_t NCQR = (cmd->cdw11 >> 16) & 0xffff; // Completion Queues Requested
            uint16_t NSQR = cmd->cdw11 & 0xffff;         // Submission Queues Requested

            log_debug("Number of Queues requested: NCQR=%u, NSQR=%u", NCQR, NSQR);

            NCQR = 0x0;  // 실제 값은 (큐 개수 - 1)이므로, 0은 1개 큐를 의미
            NSQR = 0x0;

            result = ((uint32_t)NCQR << 16) | NSQR;

            send_data(socket, cmd->cid, &result, sizeof(result));
            break;
        }

        case FEATURE_ASYNC_EVENT_CONFIG: {
            uint32_t AEC = cmd->cdw11;  // Async Event Config value

            log_debug("Async Event Configuration requested: 0x%x", AEC);

            if (AEC & (1U << 31)) {
                // TODO: Async Notification 활성화 처리 구현
                log_debug("Async Discovery Notification Enabled.");
            }

            result = AEC;

            send_data(socket, cmd->cid, &result, sizeof(result));
            break;
        }

        default:
            log_debug("Unsupported Feature ID requested: 0x%x", fid);
            status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
            break;
    }
}
