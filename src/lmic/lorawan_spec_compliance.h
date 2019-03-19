/*

Module:  lorawan_spec_compliance.h

Function:
        Details from the LoRaWAN specification for compliance.

Copyright notice and license info:
        See LICENSE file accompanying this project.
 
Author:
        Terry Moore, MCCI Corporation	March 2019

*/

#ifndef _lorawan_spec_COMPLIANCE_H_	/* prevent multiple includes */
#define _lorawan_spec_COMPLIANCE_H_

#ifdef __cplusplus
extern "C"{
#endif

enum {
    // the port for MAC commands
    LORAWAN_PORT_MAC = 0u,
    // the first port available for applications
    LORAWAN_PORT_USER_MIN = 1u,
    // the last port available for applications
    LORAWAN_PORT_USER_MAX = 223u,
    // the base of the reserved port numbers
    LORAWAN_PORT_RESERVED = 224u,
    // the port for the compliance protocol
    LORAWAN_PORT_COMPLIANCE = LORAWAN_PORT_RESERVED + 0u,
};

enum lowawan_compliance_cmd_e {
    LORAWAN_COMPLIANCE_CMD_DEACTIVATE = 0u,
    LORAWAN_COMPLIANCE_CMD_ACTIVATE = 1u,
    LORAWAN_COMPLIANCE_CMD_SET_CONFIRM = 2u,
    LORAWAN_COMPLIANCE_CMD_SET_UNCONFIRM = 3u,
    LORAWAN_COMPLIANCE_CMD_ECHO = 4u,
    LORAWAN_COMPLIANCE_CMD_LINK = 5u,
    LORAWAN_COMPLIANCE_CMD_JOIN = 6u,
    LORAWAN_COMPLIANCE_CMD_CW = 7u,
    LORAWAN_COMPLIANCE_CMD_MFG_BASE = 0x80u,
};

typedef unsigned char lorawan_compliance_cmd_t;

// info on specific commands
enum {
    LORAWAN_COMPLIANCE_CMD_ACTIVATE_LEN = 4u,
    LORAWAN_COMPLIANCE_CMD_ACTIVATE_MAGIC = 1u,

    // Maximum crypto frame size; although the spec says 18, it
    // is also used for testing max packet size.
    LORAWAN_COMPLIANCE_CMD_ECHO_LEN_MAX = 242u,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _lorawan_spec_COMPLIANCE_H_ */