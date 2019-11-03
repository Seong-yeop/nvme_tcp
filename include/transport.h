#ifndef __TRANSPORT_H
#define __TRANSPORT_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>

#include "log.h"
#include "types.h"
#include "nvme.h"

/*
 * PDU types
 */
enum {
	PDU_TYPE_ICREQ   = 0,
	PDU_TYPE_ICRESP  = 1,
	PDU_TYPE_CMD     = 4,
	PDU_TYPE_RESP    = 5,
	PDU_TYPE_C2HDATA = 7,
};

/*
 * PDU common header
 */
struct pdu_header {
	u8  type;
	u8  flags;
	u8  hlen;
	u8  pdo;
	u32 plen;
};
#define PDU_HDR_LEN sizeof(struct pdu_header)

/*
 * PDU-specific header for C2H data
 */
struct psh_c2hdata {
	u16 cccid;
	u16 resvd1;
	u32 datao;
	u32 datal;
	u32 resvd2;
};
#define PSH_C2HDATA_LEN sizeof(struct psh_c2hdata)

/*
 * Receives complete transport-level PDU and returns its type. psh_buffer and
 * data_buffer reference pointers that are set to buffers allocated for a
 * PDU-specific header and PDU data, or otherwise set to NULL. Returns -1
 * if an error occurs.
 */
int recv_pdu(sock_t socket, void** psh_buffer, void** data_buffer);

/*
 * Sends a PDU with the provided common header referenced by hdr, plus PDU
 * -specific header and data if required. psh and data should be NULL if not
 * used, but if required the buffers they point to must match the size
 * implied by hdr->hlen and hdr->plen. Returns 0 on succes or -1 on error.
 */
int send_pdu(sock_t socket, struct pdu_header* hdr, void* psh, void* data);

/*
 * Initializes a PDU-level connection by receiving a connection request PDU
 * and sending back a connection response. Returns 0 on success, -1 if error.
 */
int init_connection(sock_t socket);

/*
 * Receives an NVMe command and returns a pointer to a buffer allocated to
 * store the command. Returns NULL if the PDU received is the wrong type
 * or if another error occurs. Can take a pointer reference which is set to
 * a buffer allocated for command data, or set to NULL if there is none.
 */
struct nvme_cmd* recv_cmd(sock_t socket, void** data_buffer);

/*
 * Sends a response PDU containing an NVMe completion queue entry. Returns
 * 0 on success or -1 if an error occurs.
 */
int send_status(sock_t socket, struct nvme_status* status);

/*
 * Sends a controller-to-host transfer PDU containing a data buffer. Takes
 * CID of associated command. Returns 0 on success or -1 on error.
 */
int send_data(sock_t socket, u16 cccid, void* data, int length);

#endif
