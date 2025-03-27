#include <stdint.h>
#include <string.h>
#include "admin.h"
#include "nvme.h"

/* Forward declaration */
void response_keep_alive(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);

/*
 * 관리(Admin) 큐 처리 루프.
 * - 최초 연결 시 전달받은 conn_cmd를 이용하여 초기 응답(Present Completion Status 등)을 전송하고,
 *   이후 반복문 내에서 명령을 수신하여 각 커맨드 핸들러를 호출한 후 상태 정보를 채워 최종적으로
 *   send_status()를 통해 응답을 전송합니다.
 */
void start_admin_queue(sock_t socket, struct nvme_cmd* conn_cmd) {
    log_info("Starting new admin queue");
    u16 qsize = conn_cmd->cdw11 & 0xffff;
    u16 sqhd = 2;
    struct nvme_properties props = {
        .cap  = ((u64)1 << 37) | (4 << 24) | (1 << 16) | 127,
        .vs   = 0x10400,
        .cc   = 0x460001,
        .csts = 0,
    };

    /* 초기 응답 전송 (예: Admin Queue 생성 완료) */
    struct nvme_status status = {
        .dw0  = 1,
        .dw1  = 0,
        .sqhd = 1,
        .sqid = 0,
        .cid  = conn_cmd->cid,
        .sf   = 0,
    };
    if (send_status(socket, &status)) {
        log_warn("Failed to send initial response");
        return;
    }

    /* 명령 처리 루프 */
    struct nvme_cmd* cmd;
    while (1) {
        /* 상태 구조체 초기화 및 큐 헤드 업데이트 */
        memset(&status, 0, sizeof(status));
        status.sqhd = sqhd++;
        if (sqhd >= qsize)
            sqhd = 0;
        cmd = recv_cmd(socket, NULL);
        if (!cmd) {
            log_warn("Failed to receive command");
            return;
        }
        status.cid = cmd->cid;
        log_debug("Got command: 0x%02x (%s)", cmd->opcode, nvme_opcode_name(cmd->opcode));

        if (cmd->opcode == OPC_FABRICS) {
            /* Fabrics 전용 처리 */
            fabric_cmd(&props, cmd, &status);
        }
        else if (props.cc & 0x1) {
            /* 일반 NVMe Admin 명령 처리 */
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
                case OPC_KEEP_ALIVE:  // Keep Alive
                    response_keep_alive(socket, cmd, &status);
                    break;
                default:
                    status.sf = make_sf(SCT_GENERIC, SC_INVALID_OPCODE);
                    break;
            }
        }
        else {
            status.sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);
        }

        free(cmd);
        if (send_status(socket, &status)) {
            log_warn("Failed to send response");
            return;
        }
    }
}

/*
 * Identify 명령 처리.
 * - CNS 값에 따라 Controller, Namespace 등 구분.
 * - 데이터 단계가 필요한 경우 send_data()로 전송한 후,
 *   최종 상태 정보는 admin queue 루프에서 send_status()로 전송됩니다.
 */
void admin_identify(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
	log_debug("Admin Identify: CNS=0x%02x (%s), NSID=0x%08x", cmd->cdw10 & 0xFF, identify_cns_name(cmd->cdw10 & 0xFF), cmd->nsid);

    switch (cmd->cdw10) {
        case CNS_ID_NS: {
            /* Namespace Identify 처리 - 더미 구현 */
			if(cmd->nsid == 1){

            struct nvme_id_ns id_ns;
            memset(&id_ns, 0, sizeof(id_ns));

            /* 예시 값: 1GB (블록 크기 512바이트 기준) */
            id_ns.nsze   = 1024 * 1024 * 1024 / 4096;  // 총 논리 블록 수
            id_ns.ncap   = id_ns.nsze;                // capacity는 size와 동일
            id_ns.nuse   = id_ns.nsze;                // 사용된 블록 수 (예시)
            id_ns.nsfeat = 0;                        // 추가 기능 없음
            id_ns.nlbaf  = 9;                        // 지원하는 LBA 포맷 수 (예: 1)
            id_ns.flbas  = 0;                        // 기본 LBA 포맷 사용
            id_ns.mc     = 0;                        // 메타데이터 없음
            id_ns.dpc    = 0;
            id_ns.dps    = 0;
            id_ns.nmic   = 0;
            memset(&(id_ns.rescap), 0, sizeof(id_ns.rescap));
            id_ns.fpi    = 0;
			id_ns.lbaf[0].ds = 12;  // 2^11 = 4k
	
            /* 실제 NVMe Identify Namespace 응답은 NVME_ID_NS_LEN (예: 4096바이트)만큼 전송되어야 함 */
            send_data(socket, cmd->cid, &id_ns, NVME_ID_NS_LEN);
			}
			else{
				log_warn("Namespace ID %d is not supported", cmd->nsid);
				status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
			}
            break;
        }
        case CNS_ID_CTRL: {
            struct nvme_identify_ctrl id_ctrl = {0};
			strcpy(id_ctrl.sn, "SN1234567890");      // 여기에 실제 Serial Number를 입력
			strcpy(id_ctrl.mn, "CSL NVMe-TCP Model");     // 여기에 실제 Model 값을 입력
            strcpy(id_ctrl.fr, "0.0.1");
            strcpy(id_ctrl.subnqn, SUBSYS_NQN);
            id_ctrl.mdts   = 1;
            id_ctrl.cntlid = 1;
            id_ctrl.maxcmd = 128;
            id_ctrl.nn     = 1;
            id_ctrl.ver    = 0x10400;
            id_ctrl.kas    = 0x1111;
			id_ctrl.sqes = 0x66;
			id_ctrl.cqes = 0x44;
			id_ctrl.sgls = 1;
            send_data(socket, cmd->cid, &id_ctrl, NVME_ID_CTRL_LEN);
            break;
        }
        case CNS_ID_ACTIVE_NSID: {
            struct identify_active_namespace_list_data id_active_ns = {0};
            id_active_ns.cns[0] = htole32(0x1); // NSID=1
            send_data(socket, cmd->cid, &id_active_ns, sizeof(id_active_ns));
            break;
        }
		case CNS_ID_NS_LIST: {
			struct identify_namespace_descriptor id_ns_desc = {0};
			id_ns_desc.NIDT = htole32(0x3);
			id_ns_desc.NIDL = htole32(16);
			memcpy(id_ns_desc.NID, SUBSYS_NQN, sizeof(SUBSYS_NQN));
			send_data(socket, cmd->cid, &id_ns_desc, sizeof(id_ns_desc));
			break;
		}
        default:
            status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
            break;
    }
}

/* Feature ID 정의 */
#define FEATURE_NUMBER_OF_QUEUES    0x07
#define FEATURE_ASYNC_EVENT_CONFIG  0x0b
#define FEATURE_CONTROLLER_RESET    0x20

/*
 * Set Features 명령 처리.
 * - 전달받은 NVMe 명령에서 Feature ID와 관련 값을 추출하고,
 *   지원하는 경우 status->sf에 결과값을 채워 넣습니다.
 * - 여기서는 Number of Queues, Async Event Config, Controller Reset을 예시로 함.
 */
void admin_set_features(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
    uint8_t fid = cmd->cdw10 & 0xff;
    uint32_t result = 0;
    
    log_debug("Set Features: FID=0x%02x (%s), CDW11=0x%08x, CDW12=0x%08x", 
              fid, feature_name(fid), cmd->cdw11, cmd->cdw12);

    switch (fid) {
        case FEATURE_CONTROLLER_RESET:
            log_debug("Controller Reset requested (FID=0x20).");
            // 추가적인 Reset 처리가 필요하면 이곳에 구현
            break;

        case FEATURE_NUMBER_OF_QUEUES: {
            /* NVMe 스펙상, 설정 값은 (큐 개수 - 1)로 표현됨.
               CDW11의 하위 16비트: Submission Queues Requested,
               상위 16비트: Completion Queues Requested */
            uint16_t NSQR = cmd->cdw11 & 0xffff;
            uint16_t NCQR = (cmd->cdw11 >> 16) & 0xffff;
            log_debug("Number of Queues requested: NSQR=%u, NCQR=%u", NSQR, NCQR);

            /* 예시: 1개의 Submission/Completion 큐만 지원하는 경우 */
            NSQR = 0x0;  // 0은 1개 큐 의미
            NCQR = 0x0;
            result = ((uint32_t)NCQR << 16) | NSQR;
            status->sf = result;
            break;
        }

        case FEATURE_ASYNC_EVENT_CONFIG: {
            uint32_t AEC = cmd->cdw11;  // Async Event Config 값
            log_debug("Async Event Configuration requested: 0x%x", AEC);
            if (AEC & (1U << 31)) {
                // Async Notification 활성화 처리 (필요 시 구현)
                log_debug("Async Discovery Notification Enabled.");
            }
            result = AEC;
            status->sf = result;
            break;
        }

        default:
            log_debug("Unsupported Feature ID requested: 0x%x", fid);
            status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
            break;
    }
}

/*
 * Keep Alive 명령 처리.
 * - 데이터 전송 단계는 없으며, 단순히 명령을 수신했음을 확인하는 용도입니다.
 * - 상태 구조체에 별도의 변경 없이, 이후 admin queue 루프에서 응답(status)이 전송됩니다.
 */
void response_keep_alive(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {
    log_debug("Keep Alive requested.");
    /* 특별한 처리 없이 상태(status)는 그대로 유지 */
}
