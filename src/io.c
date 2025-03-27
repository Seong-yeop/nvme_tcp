#include <stdint.h>
#include <string.h>
#include "nvme.h"
#include "io.h"

/* Forward declaration */
void response_keep_alive(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status);


void start_io_queue(sock_t socket, struct nvme_cmd* conn_cmd) {
    log_info("Starting io queue");
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
            switch (cmd->opcode) {
                case IO_CMD_FLUSH:
                    break;
                case IO_CMD_WRITE:
                    break;
                case IO_CMD_READ:
                    io_cmd_read(socket, cmd, &status);
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

void io_cmd_read(sock_t socket, struct nvme_cmd* cmd, struct nvme_status* status) {

    log_debug("IO Read command");
    u64 lba = cmd->cdw10 | ((u64)cmd->cdw11 << 32);
    u64 lba_count = cmd->cdw12 + 1;
    u32 payload_len = lba_count * 4096;

    log_debug("IO Read command: LBA=0x%lx, LBA Count=%lu, Payload Length=%u", lba, lba_count, payload_len);

    char *buffer = (char*) malloc(payload_len);

    send_data(socket, cmd->cid, buffer, payload_len);


    free(buffer);
}