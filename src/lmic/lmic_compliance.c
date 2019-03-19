/*

Module:  lmic_compliance.c

Function:
        Implementation of the compliance engine.

Copyright notice and license info:
        See LICENSE file accompanying this project.

Author:
        Terry Moore, MCCI Corporation	March 2019

Description:
        See function descriptions.

*/

#include "lmic.h"
#include "lmic_compliance.h"
#include "lorawan_spec_compliance.h"
#include <stdbool.h>
#include <string.h>

#if defined(LMIC_PRINTF_TO)
# include <stdio.h>
# define LMIC_COMPLIANCE_PRINTF(f, ...) printf(f, ## __VA_ARGS__)
#else
# define LMIC_COMPLIANCE_PRINTF(f, ...) do { ; } while (0)
#endif

/****************************************************************************\
|
|   Manifest constants and local declarations.
|
\****************************************************************************/

static void acDoJoin(void);
static void acEnterActiveMode(void);
static void acEnterTestMode(void);
static void acExitActiveMode(void);
static void acSendUplink(void);
static void acSetTimer(ostime_t);
static void acSendUplinkBuffer(void);
static void evActivate(void);
static void evDeactivate(void);
static void evJoinCommand(void);
static void evMessage(const uint8_t *pMessage, size_t nMessage);
static lmic_compliance_fsmstate_t fsmDispatch(lmic_compliance_fsmstate_t, bool);
static void fsmEval(void);
static void fsmEvalDeferred(void);
static osjobcbfn_t fsmJobCb;
static bool isActivateMessage(const uint8_t *pMessage, size_t nMessage);
static void evEchoCommand(const uint8_t *pMessage, size_t nMessage);
static lmic_event_cb_t lmicEventCb;
static lmic_txmessage_cb_t sendUplinkCompleteCb;
static osjobcbfn_t timerExpiredCb;
static const char *txSuccessToString(int fSuccess);

/****************************************************************************\
|
|   Read-only data.
|
\****************************************************************************/

/****************************************************************************\
|
|   Variables.
|
\****************************************************************************/

lmic_compliance_t LMIC_Compliance;

/*

Name:   LMIC_complianceRxMessage()

Function:
        Add compliance-awareness to LMIC applications by filtering messages.

Definition:
        lmic_compliance_rx_action_t LMIC_complianceRxMessage(
                                u1_t        port,
                                const u1_t *pMessage,
                                size_t      nMessage
                                );

Description:
        Clients who want to handle the LoRaWAN compliance protocol on
        port 224 should call this routine each time a downlink message is
        received. This function will update the internal compliance state,
        and return an appropriate action to the user.

        If the result is `LMIC_COMPLIANCE_RX_ACTION_PROCESS`, then the client should
        process the message as usual. Otherwise, the client should discard the
        message. The other values further allow the client to track entry into,
        and exit from, compliance state.  `LMIC_COMPLIANCE_RX_ACTION_START` signals
        entry into compliance state; `LMIC_COMPLIANCE_RX_ACTION_END` signals exit
        from compliance state; and `LMIC_COMPLIANCE_RX_ACTION_IGNORE` indicates
        a mesage that should be discarded while already in compliance
        state.

Returns:
        See description.

*/

lmic_compliance_rx_action_t
LMIC_complianceRxMessage(
    uint8_t port,
    const uint8_t *pMessage,
    size_t nMessage
) {
    lmic_compliance_state_t const complianceState = LMIC_Compliance.state;

    // update the counter used by the status message.
    ++LMIC_Compliance.downlinkCount;

    // filter normal messages.
    if (port != LORAWAN_PORT_COMPLIANCE) {
        return lmic_compliance_state_IsActive(complianceState)
                    ? LMIC_COMPLIANCE_RX_ACTION_PROCESS
                    : LMIC_COMPLIANCE_RX_ACTION_IGNORE
                    ;
    }

    // it's a message to port 224.
    // if we're not active, ignore everything but activation messages
    if (! lmic_compliance_state_IsActive(complianceState)) {
        if (isActivateMessage(pMessage, nMessage)) {
            evActivate();
        } // else ignore.
    } else {
        evMessage(pMessage, nMessage);
    }
    if (lmic_compliance_state_IsActive(complianceState) == lmic_compliance_state_IsActive(LMIC_Compliance.state))
        return LMIC_COMPLIANCE_RX_ACTION_IGNORE;
    else if (! lmic_compliance_state_IsActive(complianceState))
        return LMIC_COMPLIANCE_RX_ACTION_START;
    else
        return LMIC_COMPLIANCE_RX_ACTION_END;
}

/*

Name: isActivateMessage()

Function:
        See whether a message is a LoRaWAN activate test mode message.

Definition:
        static bool isActivateMessage(
                        const uint8_t *pMessage,
                        size_t nMessage
                        );

Description:
        The message body is compared to an activate message (per the
        LoRa Alliance End Device Certification spec).

Returns:
        The result is `true` if the message is an activation message;
        it's `false` otherwise.

*/

static bool
isActivateMessage(
        const uint8_t *pMessage,
        size_t nMessage
) {
    const uint8_t body[LORAWAN_COMPLIANCE_CMD_ACTIVATE_LEN] = {
        LORAWAN_COMPLIANCE_CMD_ACTIVATE,
        LORAWAN_COMPLIANCE_CMD_ACTIVATE_MAGIC,
        LORAWAN_COMPLIANCE_CMD_ACTIVATE_MAGIC,
        LORAWAN_COMPLIANCE_CMD_ACTIVATE_MAGIC,
        };

    if (nMessage != sizeof(body))
        return false;

    if (memcmp(pMessage, body, sizeof(body)) == 0)
        return true;
    else
        return false;
}

/*

Name:   evActivate()

Function:
        Report an activation event to the finite state machine.

Definition:
        void evActivate(void);

Description:
        We report an activation event, and re-evaluate the FSM.

Returns:
        No explicit result.

*/

static void evActivate(void) {
    if (! lmic_compliance_state_IsActive(LMIC_Compliance.state)) {
        LMIC_Compliance.downlinkCount = 0;
        LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_ACTIVATE;
        LMIC_Compliance.state = LMIC_COMPLIANCE_STATE_ACTIVATING;

        LMIC_Compliance.saveEvent.pEventCb = LMIC.client.eventCb;
        LMIC_Compliance.saveEvent.pUserData = LMIC.client.eventUserData;

        LMIC_registerEventCb(lmicEventCb, NULL);

        fsmEvalDeferred();
    } else {
        LMIC_COMPLIANCE_PRINTF("Redundant ActivateTM message ignored.\n");
    }
}

/*

Name:   evMessage()

Function:
        Process an inbound message while active.

Definition:
        void evMessage(const uint8_t *pMessage, size_t nMessage);

Description:
        The event is parsed, and the appropriate event(s) are sent into
        the finite state machine.  Note that because of the way the LMIC
        works, we can assume that no uplink event is pending; so it's safe
        to launch a send from here.

Returns:
        No explicit result.

*/

static void evMessage(
    const uint8_t *pMessage,
    size_t nMessage
) {
    if (nMessage == 0)
        return;

    const uint8_t cmd = pMessage[0];
    switch (cmd) {
        case LORAWAN_COMPLIANCE_CMD_DEACTIVATE: {
            evDeactivate();
            break;
        }
        case LORAWAN_COMPLIANCE_CMD_ACTIVATE: {
            if (isActivateMessage(pMessage, nMessage))
                evActivate();
            break;
        }
        case LORAWAN_COMPLIANCE_CMD_SET_CONFIRM: {
            LMIC_Compliance.fsmFlags |= LMIC_COMPLIANCE_FSM_CONFIRM;
            break;
        }
        case LORAWAN_COMPLIANCE_CMD_SET_UNCONFIRM: {
            LMIC_Compliance.fsmFlags &= ~LMIC_COMPLIANCE_FSM_CONFIRM;
            break;
        }
        case LORAWAN_COMPLIANCE_CMD_ECHO: {
            evEchoCommand(pMessage, nMessage);
            break;
            }
        case LORAWAN_COMPLIANCE_CMD_LINK: {
            // not clear what theis request does.
            break;
        }
        case LORAWAN_COMPLIANCE_CMD_JOIN: {
            evJoinCommand();
            break;
        }
        default:
            break;
    }
}

/*

Name:   evDeactivate()

Function:
        Report an deactivation event to the finite state machine.

Definition:
        void evDectivate(void);

Description:
        We report a deactivation event, and re-evaluate the FSM.

Returns:
        No explicit result.

*/

static void evDeactivate(void) {
    LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_DEACTIVATE;
    LMIC_Compliance.state = LMIC_COMPLIANCE_STATE_STOPPING;

    // restore user's event handler.
    LMIC_registerEventCb(LMIC_Compliance.saveEvent.pEventCb, LMIC_Compliance.saveEvent.pUserData);

    fsmEvalDeferred();
}


/*

Name:   evJoinCommand()

Function:
        Report that a join has been commanded.

Definition:
        void evJoinCommand(void);

Description:
        We report a join-command event, and the kick the FSM. This
        will cause the FSM to coordinate sending and the other activities.

Returns:
        No explicit result.

*/

static void evJoinCommand(
    void
) {
    LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_JOIN_CMD;
    fsmEvalDeferred();
}

/*

Name:   evEchoCommand()

Function:
        Format and transmit the response to an echo downlink (aka echo request).

Definition:
        void evEchoCommand(
                const uint8_t *pMessage,
                size_t nMessage
                );

Description:
        The echo response is formatted and transmitted. Since we just received
        a downlink, it's always safe to do this.

Returns:
        No explicit result.

*/

static lmic_txmessage_cb_t evEchoCommandCb;

static void evEchoCommand(
    const uint8_t *pMessage,
    size_t nMessage
) {
    uint8_t *pResponse;

    if (nMessage > sizeof(LMIC_Compliance.uplinkMessage))
        return;

    // create the echo message.
    pResponse = LMIC_Compliance.uplinkMessage;

    // copy the command byte unchanged.
    *pResponse++ = *pMessage++;
    --nMessage;

    // each byte in the body has to be incremented by one.
    for (; nMessage > 0; --nMessage) {
        *pResponse++ = (uint8_t)(*pMessage++ + 1);
    }

    // now that the message is formatted, tell the fsm to send it;
    // need to use a separate job.
    LMIC_Compliance.uplinkSize = (uint8_t) (pResponse - LMIC_Compliance.uplinkMessage);
    LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_ECHO_REQUEST;
    fsmEvalDeferred();
}


/*

Name:   fsmEval()

Function:
        Evaluate the FSM, preventing recursion.

Definition:
        void fsmEval(void);

Description:
        We check for a nested call to evaluate the FSM;
        if detected, the processing is deferred until the
        current evaluation completes. Otherwise, we start
        a new FSM evaluation, which proceeds until the FSM
        returns a "no-change" result.

Returns:
        No explicit result.

*/

static const char * lmic_compliance_fsmstate_Getname(lmic_compliance_fsmstate_t state) {
    const char * const names[] = { LMIC_COMPLIANCE_FSMSTATE__NAMES };

    if ((unsigned) state > sizeof(names)/sizeof(names[0]))
        return "<<unknown>>";
    else
        return names[state];
}

static void fsmEvalDeferred(void) {
    os_setCallback(&LMIC_Compliance.fsmJob, fsmJobCb);
}

static void fsmJobCb(osjob_t *j) {
    LMIC_API_PARAMETER(j);
    fsmEval();
}

static void fsmEval(void) {
    bool fNewState;

    // check for reentry.
    do {
        lmic_compliance_fsmflags_t const fsmFlags = LMIC_Compliance.fsmFlags;

        if (fsmFlags & LMIC_COMPLIANCE_FSM_ACTIVE) {
            LMIC_Compliance.fsmFlags = fsmFlags | LMIC_COMPLIANCE_FSM_REENTERED;
            return;
        }

        // record that we're active
        LMIC_Compliance.fsmFlags = fsmFlags | LMIC_COMPLIANCE_FSM_ACTIVE;
    } while (0);

    // evaluate and change state
    fNewState = false;
    for (;;) {
        lmic_compliance_fsmstate_t const oldState = LMIC_Compliance.fsmState;
        lmic_compliance_fsmstate_t newState;

        newState = fsmDispatch(oldState, fNewState);

        if (newState == LMIC_COMPLIANCE_FSMSTATE_NOCHANGE) {
            lmic_compliance_fsmflags_t const fsmFlags = LMIC_Compliance.fsmFlags;

            if ((fsmFlags & LMIC_COMPLIANCE_FSM_REENTERED) == 0) {
                // not reentered, no change: get out.
                LMIC_Compliance.fsmFlags = fsmFlags & ~LMIC_COMPLIANCE_FSM_ACTIVE;
                return;
            } else {
                // reentered. reset reentered flag and keep going.
                LMIC_Compliance.fsmFlags = fsmFlags & ~LMIC_COMPLIANCE_FSM_REENTERED;
                fNewState = false;
            }
        } else {
            // state change!
            LMIC_COMPLIANCE_PRINTF("%s: change state %s(%u) => %s(%u)\n",
                __func__,
                lmic_compliance_fsmstate_Getname(oldState), (unsigned) oldState,
                lmic_compliance_fsmstate_Getname(newState), (unsigned) newState
                );
            fNewState = true;
            LMIC_Compliance.fsmState = newState;
        }
    }
}

/*

Name:   fsmDispatch()

Function:
        Dispatch to the appropriate event handler.

Definition:
        lmic_compliance_fsmstate_t fsmDispatch(
                                lmic_compliance_fsmstate_t state,
                                bool fEntry
                                );

Description:
        This function is called by the evalutator as needed. `state`
        is set to the current state of the FSM, and `fEntry` is
        true if and only if this state has just been entered via a
        transition arrow. (Might be a transition to self.)

Returns:
        This function returns LMIC_COMPLIANCE_FSMSTATE_NOCHANGE if
        the FSM is to remain in this state until an event occurs.
        Otherwise it returns the new state.

*/

static inline lmic_compliance_eventflags_t
eventflags_TestAndClear(lmic_compliance_eventflags_t flag) {
    const lmic_compliance_eventflags_t old = LMIC_Compliance.eventflags;
    const lmic_compliance_eventflags_t result = old & flag;

    if (result != 0)
        LMIC_Compliance.eventflags = old ^ result;

    return result;
}

static lmic_compliance_fsmstate_t
fsmDispatch(
    lmic_compliance_fsmstate_t state,
    bool fEntry
) {
    lmic_compliance_fsmstate_t newState;

    // currently, this is a stub.
    newState = LMIC_COMPLIANCE_FSMSTATE_NOCHANGE;

    switch (state) {
        case LMIC_COMPLIANCE_FSMSTATE_INITIAL: {
            newState = LMIC_COMPLIANCE_FSMSTATE_INACTIVE;
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_INACTIVE: {
            if (fEntry) {
                acExitActiveMode();
            }

            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_ACTIVATE)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_ACTIVE;
            }
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_ACTIVE: {
            if (fEntry) {
                acEnterActiveMode();
                acSetTimer(sec2osticks(1));
            }
            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_TIMER_EXPIRED))
                newState = LMIC_COMPLIANCE_FSMSTATE_TESTMODE;
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_TESTMODE: {
            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_DEACTIVATE)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_INACTIVE;
            } else if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_JOIN_CMD)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_JOINING;
            } else if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_ECHO_REQUEST)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_ECHOING;
            } else {
                newState = LMIC_COMPLIANCE_FSMSTATE_REPORTING;
            }
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_JOINING: {
            if (fEntry)
                acDoJoin();

            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_JOINED)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_RECOVERY;
            }
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_ECHOING: {
            if (fEntry)
                acSendUplinkBuffer();

            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_UPLINK_COMPLETE)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_RECOVERY;
            }
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_REPORTING: {
            if (fEntry)
                acSendUplink();

            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_UPLINK_COMPLETE)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_RECOVERY;
            }
            break;
        }

        case LMIC_COMPLIANCE_FSMSTATE_RECOVERY: {
            if (fEntry) {
                if (LMIC_Compliance.eventflags & (LMIC_COMPLIANCE_EVENT_DEACTIVATE |
                                                  LMIC_COMPLIANCE_EVENT_ECHO_REQUEST |
                                                  LMIC_COMPLIANCE_EVENT_JOIN_CMD)) {
                    acSetTimer(sec2osticks(1));
                } else {
                    acSetTimer(sec2osticks(5));
                }
            }

            if (eventflags_TestAndClear(LMIC_COMPLIANCE_EVENT_TIMER_EXPIRED)) {
                newState = LMIC_COMPLIANCE_FSMSTATE_TESTMODE;
            }
        }

        default: {
            break;
        }
    }

    return newState;
}

static void acEnterActiveMode(void) {
    // indicate to the outer world that we're active.
    LMIC_Compliance.state = LMIC_COMPLIANCE_STATE_ACTIVE;
}

static void acEnterTestMode(void) {
}

void acSetTimer(ostime_t delay) {
    os_setTimedCallback(&LMIC_Compliance.timerJob, os_getTime() + delay, timerExpiredCb);
}

static void timerExpiredCb(osjob_t *j) {
    LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_TIMER_EXPIRED;
    fsmEval();
}

static void lmicEventCb(
    void *pUserData,
    ev_t ev
) {
    LMIC_API_PARAMETER(pUserData);

    // pass to user handler
    if (LMIC_Compliance.saveEvent.pEventCb) {
        LMIC_Compliance.saveEvent.pEventCb(
            LMIC_Compliance.saveEvent.pUserData, ev
            );
    }

    // if it's a EV_JOINED, we should tell the FSM.
    if (ev == EV_JOINED) {
        LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_JOINED;
        fsmEvalDeferred();
    }
}


static void acExitActiveMode(void) {
    LMIC_Compliance.state = LMIC_COMPLIANCE_STATE_IDLE;
    os_clearCallback(&LMIC_Compliance.timerJob);
    LMIC_clrTxData();
}


static void acSendUplink(void) {
    uint8_t payload[2];
    uint32_t const downlink = LMIC_Compliance.downlinkCount;

    // build the uplink message
    payload[0] = (uint8_t) (downlink >> 8);
    payload[1] = (uint8_t) downlink;

    // reset the flags
    LMIC_Compliance.eventflags &= ~LMIC_COMPLIANCE_EVENT_UPLINK_COMPLETE;

    // don't try to send if busy; might be sending echo message.
    if ((LMIC.opmode & OP_TXRXPEND) == 0 &&
        LMIC_sendWithCallback(
            LORAWAN_PORT_COMPLIANCE,
            payload, sizeof(payload),
            /* confirmed? */
            !! (LMIC_Compliance.fsmFlags & LMIC_COMPLIANCE_FSM_CONFIRM),
            sendUplinkCompleteCb, NULL) == 0) {
        // queued successfully
        LMIC_COMPLIANCE_PRINTF(
                "lmic_compliance.acSendUplink: queued uplink message(%u, %p)\n",
                (unsigned) downlink & 0xFFFF,
                LMIC.client.txMessageCb
                );
    } else {
        // failed to queue; just skip this cycle.
        sendUplinkCompleteCb(NULL, false);
    }
}

static void sendUplinkCompleteCb(void *pUserData, int fSuccess) {
    LMIC_Compliance.eventflags |= LMIC_COMPLIANCE_EVENT_UPLINK_COMPLETE;
    LMIC_COMPLIANCE_PRINTF("%s(%s)\n", __func__, txSuccessToString(fSuccess));
    fsmEvalDeferred();
}

static void acSendUplinkBuffer(void) {
    // send uplink data.
    if (LMIC_sendWithCallback(
            LORAWAN_PORT_COMPLIANCE,
            LMIC_Compliance.uplinkMessage, LMIC_Compliance.uplinkSize,
            /* confirmed? */ (LMIC_Compliance.fsmFlags & LMIC_COMPLIANCE_FSM_CONFIRM) != 0,
            sendUplinkCompleteCb,
            NULL) == 0
        ) {
        LMIC_COMPLIANCE_PRINTF("%s: queued %u bytes\n", __func__, LMIC_Compliance.uplinkSize);
    } else {
        LMIC_COMPLIANCE_PRINTF("%s: uplink %u bytes failed\n", __func__, LMIC_Compliance.uplinkSize);
    }
}

static const char *txSuccessToString(int fSuccess) {
    return fSuccess ? "ok" : "failed";
}

static void acDoJoin(void) {
    LMIC_COMPLIANCE_PRINTF("acDoJoin\n");

    LMIC_Compliance.eventflags &= ~LMIC_COMPLIANCE_EVENT_JOINED;

    LMIC_unjoin();
    LMIC_Compliance.downlinkCount = 0;
    LMIC_startJoining();
}
