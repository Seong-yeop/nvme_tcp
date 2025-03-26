#include "nvme.h"

static const char* fabrics_cmd_type_name(u8 cmd)
{
    switch (cmd) {
        case 0x00:
            return "Property Set";
        case 0x01:
            return "Connect";
        case 0x04:
            return "Property Get";
        case 0x05:
            return "Authentication Send";
        case 0x06:
            return "Authentication Receive";
        case 0x08:
            return "Disconnect";
        default:
            if (cmd >= 0xC0 && cmd <= 0xFF)
                return "Vendor Specific";
            return "Unknown";
    }
    return "Unknown";
}

static const char* property_name(u32 offset) {
    switch (offset) {
        case 0x00: return "CAP (Controller Capabilities)";
        case 0x08: return "VS (Version)";
        case 0x0C: return "INTMS (Reserved)";
        case 0x10: return "INTMC (Reserved)";
        case 0x14: return "CC (Controller Configuration)";
        case 0x18: return "Reserved";
        case 0x1C: return "CSTS (Controller Status)";
        case 0x20: return "NSSR (NVM Subsystem Reset)";
        case 0x24: return "AQA (Reserved)";
        case 0x28: return "ASQ (Reserved)";
        case 0x30: return "ACQ (Reserved)";
        case 0x38: return "CMBLOC (Reserved)";
        case 0x3C: return "CMBSZ (Reserved)";
        default:
            if (offset >= 0xF00 && offset <= 0xFFF)
                return "Vendor Specific";
            if (offset >= 0x1000 && offset <= 0x12FF)
                return "Fabrics Definition Area";
            return "Unknown Property";
    }
}

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
    switch (cmd->nsid) {
        case FCTYPE_GET_PROP:
        log_debug("Get Property: offset=0x%02x (%s)", cmd->cdw11, property_name(cmd->cdw11));
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
            break;
        case FCTYPE_SET_PROP:
        log_debug("Set Property: offset=0x%02x (%s), value=0x%08x", cmd->cdw11, property_name(cmd->cdw11), cmd->cdw12);
        switch (cmd->cdw11) {
                case 0x14: props->cc = cmd->cdw12;
                case 0x0:
                case 0x8:
                case 0x1c:
                    status->dw0 = cmd->cdw12;
                    props->csts = (props->csts & 0xfffffffe) | (props->cc & 1);
                    if (((props->cc >> 14) & 0x3) > 0)
                        props->csts = (props->csts & 0xfffffff3) | 0x8;
                    break;
                default:
                    status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
            }
            break;
        case FCTYPE_CONNECT:
            status->sf = make_sf(SCT_GENERIC, SC_COMMAND_SEQ);
            break;
        default:
            status->sf = make_sf(SCT_GENERIC, SC_INVALID_FIELD);
    }
}

