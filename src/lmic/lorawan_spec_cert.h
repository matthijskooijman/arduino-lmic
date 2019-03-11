/*

Module:  lorawan_spec_cert.h

Function:
        Details from the LoRaWAN specification for certification.

Copyright notice and license info:
        See LICENSE file accompanying this project.
 
Author:
        Terry Moore, MCCI Corporation	March 2019

*/

#ifndef _lorawan_spec_cert_h_	/* prevent multiple includes */
#define _lorawan_spec_cert_h_

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
    // the port for the certification protocol
    LORAWAN_PORT_CERT = LORAWAN_PORT_RESERVED + 0u,
};

enum lowawan_cert_cmd_e {
    LORAWAN_CERT_CMD_DEACTIVATE = 0u,
    LORAWAN_CERT_CMD_ACTIVATE = 1u,
    LORAWAN_CERT_CMD_SET_CONFIRM = 2u,
    LORAWAN_CERT_CMD_SET_UNCONFIRM = 3u,
    LORAWAN_CERT_CMD_CRYPTO = 4u,
    LORAWAN_CERT_CMD_LINK = 5u,
    LORAWAN_CERT_CMD_JOIN = 6u,
    LORAWAN_CERT_CMD_CW = 7u,
    LORAWAN_CERT_CMD_MFG_BASE = 0x80u,
};

typedef unsigned char lorawan_cert_cmd_t;

// info on specific commands
enum {
    LORAWAN_CERT_CMD_ACTIVATE_LEN = 4u,
    LORAWAN_CERT_CMD_ACTIVATE_MAGIC = 1u,

    // Maximum crypto frame size; although the spec says 18, it
    // is also used for testing max packet size.
    LORAWAN_CERT_CMD_CRYPTO_LEN_MAX = 242u,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _lorawan_spec_cert_h_ */