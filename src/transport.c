#include "transport.h"
#include "string.h"
static const char* pdu_type_name(u8 opcode)
{
    // 배열 인덱스는 0x0~0x09 정도만 쓰는 예시
    // 나머지(배열 범위 밖, 또는 미정의)는 "Unknown" 처리
    static const char* names[16] = {
        [0x00] = "ICReq",
        [0x01] = "ICResp",
        [0x02] = "H2CTermReq",
        [0x03] = "C2HTermReq",
        [0x04] = "CapsuleCmd",
        [0x05] = "CapsuleResp",
        [0x06] = "H2CData",
        [0x07] = "C2HData",
        // 0x08 자리 비어있음
        [0x09] = "R2T"
    };

    // 범위 초과하거나 매핑이 없는 경우 Unknown
    if (opcode < 16 && names[opcode] != NULL) {
        return names[opcode];
    }
    return "Unknown";
}


void print_pdu_header(const struct pdu_header *hdr)
{
    if (!hdr) {
        printf("pdu_header is NULL\n");
        return;
    }

	log_debug("Received PDU Header: type=0x%02x (%s), flags=0x%02x, hlen=%u, pdo=%u, plen=%u", hdr->type, pdu_type_name(hdr->type), hdr->flags, (unsigned)hdr->hlen, (unsigned)hdr->pdo, (unsigned)hdr->plen);

}

int recv_all(sock_t socket, void *buffer, int length) {
	int received = 0;
	int ret;

	while (received < length) {
		ret = recv(socket, (char*)buffer + received, length - received, 0);
		if (ret <= 0) {
			log_warn("recv_all failed (received=%d)", received);
			return -1;
		}
		received += ret;
	}
	return received;
}

/* 
 * Receives complete transport-level PDU and returns its type. psh_buffer and
 * data_buffer reference pointers that are set to buffers allocated for a
 * PDU-specific header and PDU data, or otherwise set to NULL. Returns -1
 * if an error occurs.
 */
int recv_pdu(sock_t socket, void** psh_buffer, void** data_buffer) {
	void *psh, *data;
	int received, len;
	struct pdu_header hdr;
	if (psh_buffer)
		*psh_buffer = NULL;
	if (data_buffer)
		*data_buffer = NULL;

	// receive common header
	if (recv_all(socket, &hdr, PDU_HDR_LEN) != PDU_HDR_LEN) {
		log_warn("recv_pdu failed (header)");
		return -1;
	}

	len = hdr.hlen - PDU_HDR_LEN;
	if (len > 0) {
		psh = malloc(len);
		if (!psh) {
			log_warn("malloc failed (psh)");
			return -1;
		}
		if (recv_all(socket, psh, len) != len) {
			log_warn("recv_pdu failed (psh)");
			free(psh);
			return -1;
		}
		if (psh_buffer)
			*psh_buffer = psh;
		else
			free(psh);
	}
	// get data
	len = hdr.plen - hdr.hlen;
	if (len > 0) {
		data = malloc(len);
		if (!data) {
			log_warn("malloc failed (data)");
			if (psh_buffer && *psh_buffer) free(*psh_buffer);
			return -1;
		}
		if (recv_all(socket, data, len) != len) {
			log_warn("recv_pdu failed (data)");
			free(data);
			if (psh_buffer && *psh_buffer) free(*psh_buffer);
			return -1;
		}
		if (data_buffer)
			*data_buffer = data;
		else
			free(data);
	}
	print_pdu_header(&hdr);
	return hdr.type;
}

/*
 * Sends a PDU with the provided common header referenced by hdr, plus PDU
 * -specific header and data if required. psh and data should be NULL if not
 * used, but if required the buffers they point to must match the size
 * implied by hdr->hlen and hdr->plen. Returns 0 on succes or -1 on error.
 */
int send_pdu(sock_t socket, struct pdu_header* hdr, void* psh, void* data) {
    int total_len = hdr->plen;
    char *buffer = malloc(total_len);
    if (!buffer) {
        log_warn("Memory allocation failed");
        return -1;
    }

    int offset = 0;

    // copy common header
    memcpy(buffer + offset, hdr, PDU_HDR_LEN);
    offset += PDU_HDR_LEN;

    // copy PDU-specific header if needed
    int len = hdr->hlen - PDU_HDR_LEN;
    if (len > 0) {
        memcpy(buffer + offset, psh, len);
        offset += len;
    }

    // copy data if needed
    len = hdr->plen - hdr->hlen;
    if (len > 0) {
        memcpy(buffer + offset, data, len);
        offset += len;
    }

    // send everything at once
    int sent_total = 0;
    int sent;
    while (sent_total < total_len) {
        sent = send(socket, buffer + sent_total, total_len - sent_total, 0);
        if (sent <= 0) {
            log_warn("send_pdu failed");
            free(buffer);
            return -1;
        }
        sent_total += sent;
    }

	log_debug("Send pdu type: 0x%02x (%s), length: %d\n", hdr->type, pdu_type_name(hdr->type), sent_total);
    free(buffer);
    return 0;
}

/*
 * Initializes a PDU-level connection by receiving a connection request PDU
 * and sending back a connection response. Returns 0 on success, -1 if error.
 */
int init_connection(sock_t socket) {
	int type;
	u8 psh[120] = {0};
	struct pdu_header hdr = {
		.type  = PDU_TYPE_ICRESP,
		.flags = 0,
		.hlen  = PDU_HDR_LEN + 120,
		.pdo   = 0,
		.plen  = PDU_HDR_LEN + 120,
	};

	// receive PDU and make sure its type is ICReq
	type = recv_pdu(socket, NULL, NULL);
	if (type != PDU_TYPE_ICREQ)
		return -1;

	// send ICResp PDU
	return send_pdu(socket, &hdr, psh, NULL);
}

/*
 * Receives an NVMe command and returns a pointer to a buffer allocated to
 * store the command. Returns NULL if the PDU received is the wrong type
 * or if another error occurs. Can take a pointer reference which is set to
 * a buffer allocated for command data, or set to NULL if there is none.
 */
struct nvme_cmd* recv_cmd(sock_t socket, void** data_buffer) {
	void* cmd_buffer;
	struct nvme_cmd* cmd;
	int type = recv_pdu(socket, &cmd_buffer, data_buffer);
	if (type != PDU_TYPE_CMD) {
		if (cmd_buffer) free(cmd_buffer);
		return NULL;
	}
	cmd = (struct nvme_cmd*) cmd_buffer;
	if (data_buffer && *data_buffer)
		log_debug("Received %d bytes of command data", cmd->sgl.length);
	return cmd;
}

/*
 * Sends a response PDU containing an NVMe completion queue entry. Returns
 * 0 on success or -1 if an error occurs.
 */
int send_status(sock_t socket, struct nvme_status* status) {
	log_debug("Sending status: 0x%x", status->sf);
	struct pdu_header hdr = {
		.type  = PDU_TYPE_RESP,
		.flags = 0,
		.hlen  = PDU_HDR_LEN + NVME_STATUS_LEN,
		.pdo   = 0,
		.plen  = PDU_HDR_LEN + NVME_STATUS_LEN,
	};
	return send_pdu(socket, &hdr, status, NULL);
}

/*
 * Sends a controller-to-host transfer PDU containing a data buffer. Takes
 * CID of associated command. Returns 0 on success or -1 on error.
 */
int send_data(sock_t socket, u16 cccid, void* data, int length) {
	struct pdu_header hdr = {
		.type  = PDU_TYPE_C2HDATA,
		.flags = 4, // last data PDU
		.hlen  = PDU_HDR_LEN + PSH_C2HDATA_LEN,
		.pdo   = 0,
		.plen  = PDU_HDR_LEN + PSH_C2HDATA_LEN + length,
	};
	struct psh_c2hdata psh = {
		.cccid	= cccid,
		.resvd1 = 0,
		.datao	= 0,
		.datal	= length,
		.resvd2 = 0,
	};
	log_debug("Sending %d bytes", length);
	return send_pdu(socket, &hdr, &psh, data);
}

