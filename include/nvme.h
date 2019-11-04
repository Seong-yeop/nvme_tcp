#ifndef __NVME_H
#define __NVME_H

#include "log.h"
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

enum identify_cns {
    CNS_ID_NS   = 0x0,
    CNS_ID_CTRL = 0x1,
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

struct nvme_identify_ctrl {
    u16  vid;
    u16  ssvid;
    char sn[20];
    char mn[40];
    char fr[8];
    u8   rab;
    u8   ieee[3];
    u8   cmic;
    u8   mdts;
    u16  cntlid;
    u32  ver;
    u32  rtd3r;
    u32  rtd3e;
    u32  oaes;
    u32  ctratt;
    u16  rrls;
    u8   revsd1[9];
    u8   cntrltype;
    char fguid[16];
    u16  crdt1;
    u16  crdt2;
    u16  crdt3;
    u8   resvd2[122];
    u16  oacs;
    u8   acl;
    u8   aerl;
    u8   frmw;
    u8   apl;
    u8   elpe;
    u8   npss;
    u8   avscc;
    u8   apsta;
    u16  wctemp;
    u16  cctemp;
    u16  mtfa;
    u32  hmpre;
    u32  hmmin;
    u64  tnvmcap;
    u64  tnvmcap_hi;
    u64  unvmcap;
    u64  unvmcap_hi;
    u32  rpmbs;
    u16  edstt;
    u8   dsto;
    u8   fwug;
    u16  kas;
    u16  hctma;
    u16  mntmt;
    u16  mxtmt;
    u32  sanicap;
    u32  hmminds;
    u16  hmmaxd;
    u16  nsetidmax;
    u16  endgidmax;
    u8   anatt;
    u8   anacap;
    u32  anagrpmax;
    u32  nanagrpid;
    u32  pels;
    u8   resvd3[156];
    u8   sqes;
    u8   cqes;
    u16  maxcmd;
    u32  nn;
    u16  oncs;
    u16  fuses;
    u8   fna;
    u8   vwc;
    u16  awun;
    u16  awupf;
    u8   nvscc;
    u8   nwpc;
    u16  acwu;
    u8   resvd4[2];
    u32  sgls;
    u32  mnan;
    u8   resvd5[224];
    char subnqn[256];
    u8   resvd6[768];
    u32  ioccsz;
    u32  iorcsz;
    u16  icdoff;
    u8   ctrattr;
    u8   msdbd;
    u8   resvd7[244];
    u8   psd[1024];
    u8   vs[1024];
};
#define NVME_ID_CTRL_LEN sizeof(struct nvme_identify_ctrl)

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
