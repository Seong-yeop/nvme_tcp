#ifndef __NVME_H
#define __NVME_H

#include "types.h"

struct nvme_sgl_desc {
	u64 address;
	u32 length;
	u8  resvd[3];
	u8  sglid;
};

struct nvme_cmd {
	u8  opcode;
	u8  flags;
	u16 cid;
	u32 nsid;
	u64 resvd1;
	u64 mptr;
	struct nvme_sgl_desc sgl;
	u32 cdw10;
	u32 cdw11;
	u32 cdw12;
	u32 cdw13;
	u32 cdw14;
	u32 cdw15;
};
#define NVME_CMD_LEN sizeof(struct nvme_cmd)

struct nvme_status {
	u32 dw0;
	u32 dw1;
	u16 sqhd;
	u16 sqid;
	u16 cid;
	u16 sf;
};
#define NVME_STATUS_LEN sizeof(struct nvme_status)

enum nvme_opcodes {
	OPC_IDENTIFY = 0x6,
	OPC_FABRICS  = 0x7f,
};

enum fabrics_commands {
	FCTYPE_SET_PROP = 0x0,
	FCTYPE_CONNECT  = 0x1,
	FCTYPE_GET_PROP = 0x4,
};

enum nvme_sc_type {
	SCT_GENERIC  = 0,
	SCT_CMD_SPEC = 1,
};

enum nvme_sc {
	SC_SUCCESS         = 0x0,
	SC_INVALID_OPCODE  = 0x1,
	SC_INVALID_FIELD   = 0x2,
	SC_COMMAND_SEQ     = 0xC,
	SC_CONNECT_INVALID = 0x82,
};

struct nvme_connect_params {
	char hostid[16];
	u16  cntlid;
	u8   resvd1[238];
	char subnqn[256];
	char hostnqn[256];
	u8   resvd2[256];
};
#define NVME_CONNPARAMS_LEN sizeof(struct nvme_connect_params)

struct nvme_properties {
	u64 cap;
	u32 vs;
	u32 cc;
	u32 csts;
};

/*
 * Returns a status field dword based on the given status code and type.
 */
u16 make_sf(u8 type, u8 status);

/*
 * Processes fabrics commands, i.e. get and set property.
 */
void fabric_cmd(struct nvme_properties* props, struct nvme_cmd* cmd, struct nvme_status* status);

#endif
