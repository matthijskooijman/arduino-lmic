/*

Module:  lmic_cert.h

Function:
        Internal header file for certification-related work.

Copyright notice and license info:
        See LICENSE file accompanying this project.
 
Author:
        Terry Moore, MCCI Corporation	March 2019

Description:
        This header file allows us to break up the certification
        functions into multiple .c files if we wish.

*/

#ifndef _lmic_cert_h_	/* prevent multiple includes */
#define _lmic_cert_h_

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _lmic_h_
# include "lmic.h"
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct lmic_cert_s lmic_cert_t;

// concrete type for the state enumeration for the certification engine.
typedef uint8_t lmic_cert_state_t;

enum lmic_cert_state_e {
    LMIC_CERT_STATE_IDLE = 0,       // app state
    LMIC_CERT_STATE_STOPPING = 1,   // transitioning back to app
    LMIC_CERT_STATE_ACTIVATING = 2, // transitioning to cert state
    LMIC_CERT_STATE_ACTIVE = 3,     // in certification state
};

// return true if a state value indicates that the FSM is active.
static inline bool
lmic_cert_state_IsActive(lmic_cert_state_t s) {
    return s >= LMIC_CERT_STATE_ACTIVATING;
}

// events from the outside world to the FSM
typedef uint8_t lmic_cert_eventflags_t;

enum lmic_cert_eventflags_e {
    LMIC_CERT_EVENT_ACTIVATE    = 1u << 0,
    LMIC_CERT_EVENT_DEACTIVATE  = 1u << 1,
    LMIC_CERT_EVENT_SEND_UPLINK = 1u << 2,
    LMIC_CERT_EVENT_UPLINK_COMPLETE = 1u << 3,
    LMIC_CERT_EVENT_JOIN_CMD	= 1u << 4,
    LMIC_CERT_EVENT_JOINED	    = 1u << 5,
    LMIC_CERT_EVENT_ECHO_REQUEST = 1u << 6,
};

typedef uint8_t lmic_cert_fsmflags_t;
enum lmic_cert_fsmflags_e {
    LMIC_CERT_FSM_ACTIVE        = 1u << 0,
    LMIC_CERT_FSM_REENTERED     = 1u << 1,
    LMIC_CERT_FSM_CONFIRM       = 1u << 2,
    LMIC_CERT_FSM_UPLINK_BUSY	= 1u << 3,
    LMIC_CERT_FSM_JOB_BUSY      = 1u << 4,
};

typedef uint8_t lmic_cert_fsmstate_t;
enum lmic_cert_fsmstate_e {
    LMIC_CERT_FSMSTATE_INITIAL = 0,
    LMIC_CERT_FSMSTATE_NOCHANGE = 1,
    LMIC_CERT_FSMSTATE_ACTIVE = 2,
    LMIC_CERT_FSMSTATE_INACTIVE = 3,
    LMIC_CERT_FSMSTATE_TESTMODE = 4,   // sending test uplinks
    LMIC_CERT_FSMSTATE_JOINING = 5,    // joining (under command)
    LMIC_CERT_FSMSTATE_ECHOING = 6,
    LMIC_CERT_FSMSTATE_REPORTING = 7,
};

#define LMIC_CERT_FSMSTATE__NAMES   \
    "INITIAL", "NOCHANGE", "ACTIVE", "INACTIVE", "TESTMODE", "JOINING",  \
    "ECHOING", "REPORTING"

typedef struct lmic_cert_eventcb_s lmic_cert_eventcb_t;
struct lmic_cert_eventcb_s {
	// save the user's event CB while active.
	lmic_event_cb_t		*pEventCb;
	// save the user's event data while active.
	void			*pUserData;
};

// the state of the certification engine.
struct lmic_cert_s {
        // uint64
        // uintptr
        osjob_t                 uplinkJob;  // the job for driving uplinks
        osjob_t                 fsmJob;     // job for reevaluating the FSM.
        lmic_cert_eventcb_t	    saveEvent;  // the user's event handler.

        // uint32
        // uint16

        // uint8

        lmic_cert_state_t       state;      // current state of cert engine.
        lmic_cert_eventflags_t  eventflags; // incoming events.
        lmic_cert_fsmflags_t    fsmFlags;   // FSM operational flags
        lmic_cert_fsmstate_t    fsmState;   // FSM current state

        uint8_t                 uplinkSize;
        uint8_t                 uplinkMessage[MAX_LEN_PAYLOAD];
};

extern lmic_cert_t LMIC_Cert;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _lmic_cert_h_ */