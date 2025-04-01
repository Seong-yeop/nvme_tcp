#ifndef __NVME_H
#define __NVME_H

#include "log.h"
#include "types.h"

#define PORT 4420
#define PORT_ASCII "4420"
#define SUBSYS_NQN "nqn.2020-20.com.thirdmartini.nvme:uuid:edbb6aab-3194-493b-bb38-732f71fe966c"
#define IO_NQN "nqn.2020-20.com.thirdmartini.nvme:uuid:io"

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
	OPC_GET_LOG  = 0x2,
	OPC_IDENTIFY = 0x6,
	OPC_SET_FEATURES = 0x9,
	OPC_FABRICS  = 0x7f,
	OPC_KEEP_ALIVE = 0x18,
};

enum nvme_io_commands {
	IO_CMD_FLUSH = 0x0,
	IO_CMD_WRITE = 0x1,
	IO_CMD_READ  = 0x2,
};

enum fabrics_commands {
	FCTYPE_SET_PROP = 0x0,
	FCTYPE_CONNECT	= 0x1,
	FCTYPE_GET_PROP = 0x4,
};

enum identify_cns {
	CNS_ID_NS   = 0x0,
	CNS_ID_CTRL = 0x1,
	CNS_ID_ACTIVE_NSID = 0x2,
	CNS_ID_NS_LIST = 0x3,
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

enum nvme_log {
	LOG_HEALTH_INFO = 0x2,
	LOG_COMMANDS_SUPPORTED = 0x5,
	LOG_DISCOVERY = 0x70,
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

struct identify_active_namespace_list_data {
    unsigned int cns[1024];
};

struct identify_namespace_descriptor {
    u8 NIDT;          // offset: 0
    u8 NIDL;          // offset: 1
    u8 NID[16];       // offset: 4~19
};

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

struct nvmf_disc_rsp_page_entry  {
	u8   trtype;
	u8   adrfam;
	u8   subtype;
	u8   treq;
	u16  portid;
	u16  cntlid;
	u16  asqsz;
	u16  eflags;
	u8   resv10[20];
	char trsvcid[32];
	u8   resv64[192];
	char subnqn[256];
	char traddr[256];
	union tsas {
		char		common[256];
		struct rdma {
			u8	qptype;
			u8	prtype;
			u8	cms;
			u8	resv3[5];
			u16	pkey;
			u8	resv10[246];
		} rdma;
		struct tcp {
			u8	sectype;
		} tcp;
	} tsas;
};
#define NVME_DISCOVERY_LOG_ENTRY_LEN sizeof(struct nvme_discovery_log_entry)

struct nvme_discovery_log_page {
    u64 genctr;
    u64 numrec;
    u16 recfmt;
    u8  resv14[1006];
    struct nvmf_disc_rsp_page_entry entries[2];
};
#define NVME_DISCOVERY_LOG_PAGE_LEN sizeof(struct nvme_discovery_log_page)

// struct nvme_smart_log {
// 	u8			critical_warning;
// 	u8			temperature[2];
// 	u8			avail_spare;
// 	u8			spare_thresh;
// 	u8			percent_used;
// 	u8			endu_grp_crit_warn_sumry;
// 	u8			rsvd7[25];
// 	u8			data_units_read[16];
// 	u8			data_units_written[16];
// 	u8			host_reads[16];
// 	u8			host_writes[16];
// 	u8			ctrl_busy_time[16];
// 	u8			power_cycles[16];
// 	u8			power_on_hours[16];
// 	u8			unsafe_shutdowns[16];
// 	u8			media_errors[16];
// 	u8			num_err_log_entries[16];
// 	u32			warning_temp_time;
// 	u32			critical_comp_time;
// 	u16			temp_sensor[8];
// 	u32			thm_temp1_trans_count;
// 	u32			thm_temp2_trans_count;
// 	u32			thm_temp1_total_time;
// 	u32			thm_temp2_total_time;
// 	u8			rsvd232[280];
// };

/*
 * Returns a status field dword based on the given status code and type.
 */
u16 make_sf(u8 type, u8 status);

/*
 * Processes fabrics commands, i.e. get and set property.
 */
void fabric_cmd(struct nvme_properties* props, struct nvme_cmd* cmd, struct nvme_status* status);

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

static const char* nvme_io_opcode_name(u8 opcode) {
	switch (opcode) {
		case 0x00: return "Flush";
		case 0x01: return "Write";
		case 0x02: return "Read";
		case 0x04: return "Write Uncorrectable";
		case 0x08: return "Compare";
		case 0x09: return "Write Zeroes";
		case 0x0A: return "Dataset Management";
		default:   return "Unknown / Reserved";
	}
}

static const char* feature_name(u8 fid) {
    switch (fid) {
        case 0x01: return "Arbitration";
        case 0x02: return "Power Management";
        case 0x04: return "Temperature Threshold";
        case 0x06: return "Volatile Write Cache";
        case 0x07: return "Number of Queues";
        case 0x08: return "Interrupt Coalescing";
        case 0x09: return "Interrupt Vector Configuration";
        case 0x0B: return "Asynchronous Event Configuration";
        case 0x0C: return "Autonomous Power State Transition";
        case 0x0D: return "Host Memory Buffer";
        case 0x0E: return "Timestamp";
        case 0x0F: return "Keep Alive Timer";
        case 0x10: return "Host Controlled Thermal Management";
        case 0x11: return "Non-Operational Power State Config";
        case 0x12: return "Read Recovery Level Config";
        case 0x13: return "Predictable Latency Mode Config";
        case 0x14: return "Predictable Latency Mode Window";
        case 0x16: return "Host Behavior Support";
        case 0x17: return "Sanitize Config";
        case 0x18: return "Endurance Group Event Configuration";
        case 0x19: return "I/O Command Set Profile";
        case 0x1A: return "Spinup Control";
        case 0x7D: return "Enhanced Controller Metadata";
        case 0x7E: return "Controller Metadata";
        case 0x7F: return "Namespace Metadata";
        case 0x80: return "Software Progress Marker";
        case 0x81: return "Host Identifier";
        case 0x82: return "Reservation Notification Mask";
        case 0x83: return "Reservation Persistence";
        case 0x84: return "Namespace Write Protection Config";
        default:
            if (fid >= 0xC0 && fid <= 0xFF)
                return "Vendor Specific";
            return "Unknown Feature";
    }
}

static const char* identify_cns_name(u8 cns) {
    switch (cns) {
        case 0x00: return "Identify Namespace";
        case 0x01: return "Identify Controller";
        case 0x02: return "Active Namespace ID List";
        case 0x03: return "Namespace Identification Descriptor List";
        case 0x04: return "NVM Set List";
        case 0x05: return "I/O Cmd Set - Identify Namespace";
        case 0x06: return "I/O Cmd Set - Identify Controller";
        case 0x07: return "I/O Cmd Set - Namespace ID List";
        case 0x08: return "I/O Cmd Set Independent Identify NS";
        case 0x10: return "Allocated Namespace ID List";
        case 0x11: return "Identify Allocated Namespace";
        case 0x12: return "Controller List for NSID";
        case 0x13: return "Controller List in Subsystem";
        case 0x14: return "Primary Controller Capabilities";
        case 0x15: return "Secondary Controller List";
        case 0x16: return "Namespace Granularity List";
        case 0x17: return "UUID List";
        case 0x18: return "Domain List";
        case 0x19: return "Endurance Group List";
        case 0x1A: return "I/O Cmd Set - Allocated NSID List";
        case 0x1B: return "I/O Cmd Set - Identify Namespace";
        case 0x1C: return "I/O Cmd Set Data Structure";
        default:
            return "Reserved / Unknown CNS";
    }
}

static const char* log_page_name(u8 lid) {
    switch (lid) {
        case 0x00: return "Supported Log Pages";
        case 0x01: return "Error Information";
        case 0x02: return "SMART / Health Information";
        case 0x03: return "Firmware Slot Information";
        case 0x04: return "Changed Namespace List";
        case 0x05: return "Commands Supported and Effects";
        case 0x06: return "Device Self-test";
        case 0x07: return "Telemetry Host-Initiated";
        case 0x08: return "Telemetry Controller-Initiated";
        case 0x09: return "Endurance Group Information";
        case 0x0A: return "Predictable Latency Per NVM Set";
        case 0x0B: return "Predictable Latency Event Aggregate";
        case 0x0C: return "Asymmetric Namespace Access";
        case 0x0D: return "Persistent Event Log";
        case 0x0F: return "Endurance Group Event Aggregate";
        case 0x10: return "Media Unit Status";
        case 0x11: return "Supported Capacity Configuration List";
        case 0x12: return "Feature Identifiers Supported and Effects";
        case 0x13: return "NVMe-MI Commands Supported and Effects";
        case 0x14: return "Command and Feature Lockdown";
        case 0x15: return "Boot Partition";
        case 0x16: return "Rotational Media Information";
        case 0x70: return "Discovery Log Page";
        case 0x80: return "Reservation Notification";
        case 0x81: return "Sanitize Status";
        default:
            if (lid >= 0xC0 && lid <= 0xFF)
                return "Vendor Specific";
            if (lid >= 0x82 && lid <= 0xBE)
                return "I/O Command Set Specific";
            return "Reserved / Unknown Log Page";
    }
}

struct nvme_lbaf {
    u16 ms;
    u8 ds;
    u8 rp;
};


struct nvme_id_ns {
	u64			nsze;
	u64			ncap;
	u64			nuse;
	u8			nsfeat;
	u8			nlbaf;
	u8			flbas;
	u8			mc;
	u8			dpc;
	u8			dps;
	u8			nmic;
	u8			rescap;
	u8			fpi;
	u8			dlfeat;
	u16			nawun;
	u16			nawupf;
	u16			nacwu;
	u16			nabsn;
	u16			nabo;
	u16			nabspf;
	u16			noiob;
	u8			nvmcap[16];
	u16			npwg;
	u16			npwa;
	u16			npdg;
	u16			npda;
	u16			nows;
	u8			rsvd74[18];
	u32			anagrpid;
	u8			rsvd96[3];
	u8			nsattr;
	u16			nvmsetid;
	u16			endgid;
	u8			nguid[16];
	u8			eui64[8];
	struct nvme_lbaf	lbaf[64];
	u8			vs[3712];
};
#define NVME_ID_NS_LEN sizeof(struct nvme_id_ns)

#endif
