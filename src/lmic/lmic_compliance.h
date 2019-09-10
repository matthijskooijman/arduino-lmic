/*

Module:  lmic_compliance.h

Function:
        Internal header file for compliance-related work.

Copyright notice and license info:
        See LICENSE file accompanying this project.

Author:
        Terry Moore, MCCI Corporation	March 2019

Description:
        This header file allows us to break up the compliance
        functions into multiple .c files if we wish.

*/

#ifndef _lmic_compliance_h_	/* prevent multiple includes */
#define _lmic_compliance_h_

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _lmic_h_
# include "lmic.h"
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct lmic_compliance_s lmic_compliance_t;

// concrete type for the state enumeration for the compliance engine.
typedef uint8_t lmic_compliance_state_t;

enum lmic_compliance_state_e {
    LMIC_COMPLIANCE_STATE_IDLE = 0,       // app state
    LMIC_COMPLIANCE_STATE_STOPPING = 1,   // transitioning back to app
    LMIC_COMPLIANCE_STATE_ACTIVATING = 2, // transitioning to compliance state
    LMIC_COMPLIANCE_STATE_ACTIVE = 3,     // in compliance state
};

// return true if a state value indicates that the FSM is active.
static inline bool
lmic_compliance_state_IsActive(lmic_compliance_state_t s) {
    return s >= LMIC_COMPLIANCE_STATE_ACTIVATING;
}

// events from the outside world to the FSM
typedef uint8_t lmic_compliance_eventflags_t;

enum lmic_compliance_eventflags_e {
    LMIC_COMPLIANCE_EVENT_ACTIVATE      = 1u << 0,
    LMIC_COMPLIANCE_EVENT_DEACTIVATE    = 1u << 1,
    LMIC_COMPLIANCE_EVENT_TIMER_EXPIRED = 1u << 2,
    LMIC_COMPLIANCE_EVENT_UPLINK_COMPLETE = 1u << 3,
    LMIC_COMPLIANCE_EVENT_ECHO_REQUEST  = 1u << 4,
};

typedef uint8_t lmic_compliance_fsmflags_t;
enum lmic_compliance_fsmflags_e {
    LMIC_COMPLIANCE_FSM_ACTIVE          = 1u << 0,
    LMIC_COMPLIANCE_FSM_REENTERED       = 1u << 1,
    LMIC_COMPLIANCE_FSM_CONFIRM         = 1u << 2,
};

typedef uint8_t lmic_compliance_fsmstate_t;
enum lmic_compliance_fsmstate_e {
    LMIC_COMPLIANCE_FSMSTATE_INITIAL = 0,
    LMIC_COMPLIANCE_FSMSTATE_NOCHANGE = 1,
    LMIC_COMPLIANCE_FSMSTATE_ACTIVE = 2,
    LMIC_COMPLIANCE_FSMSTATE_INACTIVE = 3,
    LMIC_COMPLIANCE_FSMSTATE_TESTMODE = 4,   // sending test uplinks
    LMIC_COMPLIANCE_FSMSTATE_ECHOING = 5,
    LMIC_COMPLIANCE_FSMSTATE_REPORTING = 6,
    LMIC_COMPLIANCE_FSMSTATE_RECOVERY = 7,
    LMIC_COMPLIANCE_FSMSTATE_TXBUSY = 8,
};

#define LMIC_COMPLIANCE_FSMSTATE__NAMES   \
    "INITIAL", "NOCHANGE", "ACTIVE", "INACTIVE", "TESTMODE",   \
    "ECHOING", "REPORTING", "RECOVERY", "TXBUSY"

typedef struct lmic_compliance_eventcb_s lmic_compliance_eventcb_t;
struct lmic_compliance_eventcb_s {
	// save the user's event CB while active.
	lmic_event_cb_t		*pEventCb;
	// save the user's event data while active.
	void			*pUserData;
};

// structure for saving band settings during test
typedef struct lmic_compliance_band_s lmic_compliance_band_t;
struct lmic_compliance_band_s {
    u2_t    txcap;  // saved 1/duty cycle
};

// the state of the compliance engine.
struct lmic_compliance_s {
        // uint64
        // uintptr
        osjob_t                     timerJob;       // the job for driving uplinks
        osjob_t                     fsmJob;         // job for reevaluating the FSM.
        lmic_compliance_eventcb_t   saveEvent;      // the user's event handler.

        // uint32

        // uint16
#if CFG_LMIC_EU_like
        lmic_compliance_band_t      saveBands[MAX_BANDS];
#endif // CFG_LMIC_EU_like

        // we are required to maintain a downlink count
        // that is reset on join/test entry and incremented for
        // each valid test message.
        uint16_t                    downlinkCount;

        // uint8

        lmic_compliance_state_t         state;      // current state of compliance engine.
        lmic_compliance_eventflags_t    eventflags; // incoming events.
        lmic_compliance_fsmflags_t      fsmFlags;   // FSM operational flags
        lmic_compliance_fsmstate_t      fsmState;   // FSM current state

        uint8_t                 uplinkSize;
        uint8_t                 uplinkMessage[MAX_LEN_PAYLOAD];
};

extern lmic_compliance_t LMIC_Compliance;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _lmic_compliance_h_ */