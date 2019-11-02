#include "nvme.h"

/*
 * Returns a status field dword based on the given status code and type.
 */
u16 make_sf(u8 type, u8 status) {
	u16 sf = 0;
	if (type || status)
		sf |= 0x8000;
	sf |= (type & 0x7) << 9;
	sf |= status << 1;
	return sf;
}

/*
 * Processes fabrics commands, i.e. get and set property.
 */
void fabric_cmd(struct nvme_properties* props, struct nvme_cmd* cmd, struct nvme_status* status) {
	u64 val;
	if (cmd->nsid == FCTYPE_GET_PROP) {
		switch (cmd->cdw11) {
			case 0x0:  val = props->cap;  break;
			case 0x8:  val = props->vs;   break;
			case 0x14: val = props->cc;   break;
			case 0x1c: val = props->csts; break;
			default:
				val = 0;
				status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
		}
		status->dw0 = val & 0xffffffff;
		if (cmd->cdw10 == 1)
			status->dw1 = val >> 32;
	}
	else if (cmd->nsid == FCTYPE_SET_PROP) {
		switch (cmd->cdw11) {
			case 0x14: props->cc = cmd->cdw12;
			case 0x0:
			case 0x8:
			case 0x1c:
				status->dw0 = cmd->cdw12;
				props->csts = (props->csts & 0xfffffffe) | (props->cc & 1);
				if (((props->cc >> 14) & 0xfffffffe) > 0)
					props->csts = (props->csts & 0xfffffff3) | 0x8;
			default:
				status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
		}
	}
	else if (cmd->nsid == FCTYPE_CONNECT)
		status->sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);
	else
		status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
}

