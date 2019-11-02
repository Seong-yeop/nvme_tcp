#include "transport.h"

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
	received = recv(socket, &hdr, PDU_HDR_LEN, 0);
	if (received < PDU_HDR_LEN) {
		printf("recv_pdu failed\n");
		return -1;
	}

	// get PDU-specific header
	len = hdr.hlen - PDU_HDR_LEN;
	if (len > 0) {
		psh = malloc(len);
		received = recv(socket, psh, len, 0);
		if (received < len) {
			printf("recv_pdu failed\n");
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
		received = recv(socket, data, len, 0);
		if (received < len) {
			printf("recv_pdu failed\n");
			free(data);
			if (psh) free(psh);
			return -1;
		}
		if (data_buffer)
			*data_buffer = data;
		else if (data)
			free(data);
	}

	return hdr.type;
}

/*
 * Sends a PDU with the provided common header referenced by hdr, plus PDU
 * -specific header and data if required. psh and data should be NULL if not
 * used, but if required the buffers they point to must match the size
 * implied by hdr->hlen and hdr->plen. Returns 0 on succes or -1 on error.
 */
int send_pdu(sock_t socket, struct pdu_header* hdr, void* psh, void* data) {
	int sent, len;

	// send common header
	sent = send(socket, hdr, PDU_HDR_LEN, 0);
	if (sent < PDU_HDR_LEN) {
		printf("send_pdu failed\n");
		return -1;
	}
	
	// send PDU-specific header if needed
	len = hdr->hlen - PDU_HDR_LEN;
	if (len > 0) {
		sent = send(socket, psh, len, 0);
		if (sent < len) {
			printf("send_pdu failed\n");
			return -1;
		}
	}

	// send data if needed
	len = hdr->plen - hdr->hlen;
	if (len > 0) {
		sent = send(socket, data, len, 0);
		if (sent < len) {
			printf("send_pdu failed\n");
			return -1;
		}
	}

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
	int type = recv_pdu(socket, &cmd_buffer, data_buffer);
	if (type != PDU_TYPE_CMD) {
		if (cmd_buffer) free(cmd_buffer);
		return NULL;
	}
	return (struct nvme_cmd*) cmd_buffer;
}

/*
 * Sends a response PDU containing an NVMe completion queue entry. Returns
 * 0 on success or -1 if an error occurs.
 */
int send_status(sock_t socket, struct nvme_status* status) {
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
		.cccid  = cccid,
		.resvd1 = 0,
		.datao  = 0,
		.datal  = length,
		.resvd2 = 0,
	};
	return send_pdu(socket, &hdr, &psh, data);
}

