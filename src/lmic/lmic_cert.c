/*

Module:  lmic_cert.c

Function:
        Implementation of the certification engine.

Copyright notice and license info:
        See LICENSE file accompanying this project.

Author:
        Terry Moore, MCCI Corporation	March 2019

Description:
        See function descriptions.

*/

#include "lmic.h"
#include "lmic_cert.h"
#include "lorawan_spec_cert.h"
#include <stdbool.h>
#include <string.h>

#if defined(LMIC_PRINTF_TO)
# include <stdio.h>
# define LMIC_CERT_PRINTF(f, ...) printf(f, ## __VA_ARGS__)
#else
# define LMIC_CERT_PRINTF(f, ...) do { ; } while (0)
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
static void acScheduleNextUplink(void);
static void acSendUplink(void);
static void evActivate(void);
static void evDeactivate(void);
static void evJoinCommand(void);
static void evMessage(const uint8_t *pMessage, size_t nMessage);
static void fsmEval(void);
static lmic_cert_fsmstate_t fsmDispatch(lmic_cert_fsmstate_t, bool);
static bool isActivateMessage(const uint8_t *pMessage, size_t nMessage);
static void sendEchoResponse(const uint8_t *pMessage, size_t nMessage);
static lmic_txmessage_cb_t sendUplinkCompleteCb;
static osjobcbfn_t timeToSendTestMessageCb;
static lmic_event_cb_t lmicEventCb;

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

lmic_cert_t LMIC_Cert;

/*

Name:   LMIC_certRxMessage()

Function:
        Add certification-awareness to LMIC applications by filtering messages.

Definition:
        lmic_cert_rx_action_t LMIC_certRxMessage(
                                u1_t        port,
                                const u1_t *pMessage,
                                size_t      nMessage
                                );

Description:
        Clients who want to handle the LoRaWAN certification protocol on
        port 224 should call this routine each time a downlink message is
        received. This function will update the internal certification state,
        and return an appropriate action to the user.

        If the result is `LMIC_CERT_RX_ACTION_PROCESS`, then the client should
        process the message as usual. Otherwise, the client should discard the
        message. The other values further allow the client to track entry into,
        and exit from, certification state.  `LMIC_CERT_RX_ACTION_START` signals
        entry into certification state; `LMIC_CERT_RX_ACTION_END` signals exit
        from certification state; and `LMIC_CERT_RX_ACTION_IGNORE` indicates
        a mesage that should be discarded while already in certification
        state.

Returns:
        See description.

*/

lmic_cert_rx_action_t
LMIC_certRxMessage(
    uint8_t port,
    const uint8_t *pMessage,
    size_t nMessage
) {
    lmic_cert_state_t const certState = LMIC_Cert.state;

    // filter normal messages.
    if (port != LORAWAN_PORT_CERT) {
        return lmic_cert_state_IsActive(certState) ? LMIC_CERT_RX_ACTION_PROCESS
                                                   : LMIC_CERT_RX_ACTION_IGNORE
                                                   ;
    }

    // it's a message to port 224.
    // if we're not active, ignore everything but activation messages
    if (certState == LMIC_CERT_STATE_IDLE) {
        if (isActivateMessage(pMessage, nMessage)) {
            evActivate();
        } // else ignore.
    } else {
        evMessage(pMessage, nMessage);
    }
    if (lmic_cert_state_IsActive(certState) == lmic_cert_state_IsActive(LMIC_Cert.state))
        return LMIC_CERT_RX_ACTION_IGNORE;
    else if (! lmic_cert_state_IsActive(certState))
        return LMIC_CERT_RX_ACTION_START;
    else
        return LMIC_CERT_RX_ACTION_END;
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
    const uint8_t body[LORAWAN_CERT_CMD_ACTIVATE_LEN] = {
        LORAWAN_CERT_CMD_ACTIVATE,
        LORAWAN_CERT_CMD_ACTIVATE_MAGIC,
        LORAWAN_CERT_CMD_ACTIVATE_MAGIC,
        LORAWAN_CERT_CMD_ACTIVATE_MAGIC,
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
    LMIC_Cert.eventflags |= LMIC_CERT_EVENT_ACTIVATE;
    fsmEval();
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
    LMIC_Cert.eventflags |= LMIC_CERT_EVENT_DEACTIVATE;
    fsmEval();
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
        case LORAWAN_CERT_CMD_DEACTIVATE: {
            evDeactivate();
            break;
        }
        case LORAWAN_CERT_CMD_ACTIVATE: {
            if (isActivateMessage(pMessage, nMessage))
                evActivate();
            break;
        }
        case LORAWAN_CERT_CMD_SET_CONFIRM: {
            LMIC_Cert.fsmFlags |= LMIC_CERT_FSM_CONFIRM;
            break;
        }
        case LORAWAN_CERT_CMD_SET_UNCONFIRM: {
            LMIC_Cert.fsmFlags &= ~LMIC_CERT_FSM_CONFIRM;
            break;
        }
        case LORAWAN_CERT_CMD_ECHO: {
            sendEchoResponse(pMessage, nMessage);
            break;
            }
        case LORAWAN_CERT_CMD_LINK: {
            // not clear what theis request does.
            break;
        }
        case LORAWAN_CERT_CMD_JOIN: {
            evJoinCommand();
            break;
        }
        default:
            break;
    }
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
    LMIC_Cert.eventflags |= LMIC_CERT_EVENT_JOIN_CMD;
    fsmEval();
}

/*

Name:   sendEchoResponse()

Function:
        Format and transmit the response to an echo downlink (aka echo request).

Definition:
        void sendEchoResponse(
                const uint8_t *pMessage,
                size_t nMessage
                );

Description:
        The echo response is formatted and transmitted. Since we just received
        a downlink, it's always safe to do this.

Returns:
        No explicit result.

*/

static lmic_txmessage_cb_t sendEchoResponseCb;

static void sendEchoResponse(
    const uint8_t *pMessage,
    size_t nMessage
) {
    uint8_t response[LORAWAN_CERT_CMD_ECHO_LEN_MAX];
    uint8_t *pResponse;

    if (nMessage > sizeof(response))
        return;

    // create the echo message.
    pResponse = response;

    // copy the command byte unchanged.
    *pResponse++ = *pMessage++;
    --nMessage;

    // each byte in the body has to be incremented by one.
    for (; nMessage > 0; --nMessage) {
        *pResponse++ = (uint8_t)(*pMessage++ + 1);
    }

    // send the uplink data. Two undocumented things here:
    // first, that the response goes to the cert port;
    // second, that the confirmation bit must track the
    // confirmed state.
    if (LMIC_sendWithCallback(
            LORAWAN_PORT_CERT,
            pResponse, pResponse - response,
            /* confirmed? */ LMIC_Cert.fsmFlags & LMIC_CERT_FSM_CONFIRM,
            sendEchoResponseCb,
            NULL) == 0
        ) {
        LMIC_Cert.fsmFlags |= LMIC_CERT_FSM_UPLINK_BUSY;
    }
}

static const char *txSuccessToString(int fSuccess) {
    return fSuccess ? "ok" : "failed";
}

void sendEchoResponseCb(void *pUserData, int fSuccess) {
    LMIC_API_PARAMETER(pUserData);
    LMIC_API_PARAMETER(fSuccess);

    LMIC_CERT_PRINTF("%s: tx complete(%s)\n", __func__, txSuccessToString(fSuccess));
    LMIC_Cert.fsmFlags &= ~LMIC_CERT_FSM_UPLINK_BUSY;
    fsmEval();
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

static const char * lmic_cert_fsmstate_Getname(lmic_cert_fsmstate_t state) {
    const char * const names[] = { LMIC_CERT_FSMSTATE__NAMES };

    if ((unsigned) state > sizeof(names)/sizeof(names[0]))
        return "<<unknown>>";
    else
        return names[state];
}

static void fsmEval(void) {
    bool fNewState;

    // check for reentry.
    do {
        lmic_cert_fsmflags_t const fsmFlags = LMIC_Cert.fsmFlags;

        if (fsmFlags & LMIC_CERT_FSM_ACTIVE) {
            LMIC_Cert.fsmFlags = fsmFlags | LMIC_CERT_FSM_REENTERED;
            return;
        }

        // record that we're active
        LMIC_Cert.fsmFlags = fsmFlags | LMIC_CERT_FSM_ACTIVE;
    } while (0);

    // evaluate and change state
    fNewState = false;
    for (;;) {
        lmic_cert_fsmstate_t const oldState = LMIC_Cert.fsmState;
        lmic_cert_fsmstate_t newState;

        newState = fsmDispatch(oldState, fNewState);

        if (newState == LMIC_CERT_FSMSTATE_NOCHANGE) {
            lmic_cert_fsmflags_t const fsmFlags = LMIC_Cert.fsmFlags;

            if ((fsmFlags & LMIC_CERT_FSM_REENTERED) == 0) {
                // not reentered, no change: get out.
                LMIC_Cert.fsmFlags = fsmFlags & ~LMIC_CERT_FSM_ACTIVE;
                return;
            } else {
                // reentered. reset reentered flag and keep going.
                LMIC_Cert.fsmFlags = fsmFlags & ~LMIC_CERT_FSM_REENTERED;
                fNewState = false;
            }
        } else {
            // state change!
            LMIC_CERT_PRINTF("%s: change state %s(%u) => %s(%u)\n",
                __func__,
                lmic_cert_fsmstate_Getname(oldState), (unsigned) oldState,
                lmic_cert_fsmstate_Getname(newState), (unsigned) newState
                );
            fNewState = true;
            LMIC_Cert.fsmState = newState;
        }
    }
}

/*

Name:   fsmDispatch()

Function:
        Dispatch to the appropriate event handler.

Definition:
        lmic_cert_fsmstate_t fsmDispatch(
                                lmic_cert_fsmstate_t state,
                                bool fEntry
                                );

Description:
        This function is called by the evalutator as needed. `state`
        is set to the current state of the FSM, and `fEntry` is
        true if and only if this state has just been entered via a
        transition arrow. (Might be a transition to self.)

Returns:
        This function returns LMIC_CERT_FSMSTATE_NOCHANGE if
        the FSM is to remain in this state until an event occurs.
        Otherwise it returns the new state.

*/

static lmic_cert_fsmstate_t
fsmDispatch(
    lmic_cert_fsmstate_t state,
    bool fEntry
) {
    lmic_cert_fsmstate_t newState;

    // currently, this is a stub.
    newState = LMIC_CERT_FSMSTATE_NOCHANGE;

    switch (state) {
        case LMIC_CERT_FSMSTATE_INITIAL: {
            newState = LMIC_CERT_FSMSTATE_INACTIVE;
            LMIC_Cert.saveEvent.pEventCb = LMIC.client.eventCb;
            LMIC_Cert.saveEvent.pUserData = LMIC.client.eventUserData;
            break;
        }

        case LMIC_CERT_FSMSTATE_INACTIVE: {
            LMIC_Cert.eventflags &= ~LMIC_CERT_EVENT_DEACTIVATE;

            if (fEntry) {
                acExitActiveMode();
            }

            if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_ACTIVATE) {
                newState = LMIC_CERT_FSMSTATE_ACTIVE;
            }
            break;
        }

        case LMIC_CERT_FSMSTATE_ACTIVE: {
            LMIC_Cert.eventflags &= ~LMIC_CERT_EVENT_ACTIVATE;

            if (fEntry) {
                acEnterActiveMode();
            }
            newState = LMIC_CERT_FSMSTATE_TESTMODE;
            break;
        }

        case LMIC_CERT_FSMSTATE_TESTMODE: {
            if (fEntry) {
                acEnterTestMode();
            }

            if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_DEACTIVATE) {
                newState = LMIC_CERT_FSMSTATE_INACTIVE;
            } else if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_JOIN_CMD) {
                newState = LMIC_CERT_FSMSTATE_JOINING;
            } else if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_SEND_UPLINK) {
                acSendUplink();
            } else if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_UPLINK_COMPLETE) {
                acScheduleNextUplink();
            }
            break;
        }

        case LMIC_CERT_FSMSTATE_JOINING: {
            LMIC_Cert.eventflags &= ~LMIC_CERT_EVENT_JOIN_CMD;

            if (fEntry)
                acDoJoin();

            if (LMIC_Cert.eventflags & LMIC_CERT_EVENT_JOINED) {
                newState = LMIC_CERT_FSMSTATE_TESTMODE;
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
    LMIC_Cert.state = LMIC_CERT_STATE_ACTIVE;

    LMIC_registerEventCb(lmicEventCb, NULL);
}

static void acEnterTestMode(void) {
    LMIC_Cert.eventflags &= ~(LMIC_CERT_EVENT_SEND_UPLINK | LMIC_CERT_EVENT_UPLINK_COMPLETE);

    // schedule an uplink.
    os_setTimedCallback(
        &LMIC_Cert.uplinkJob,
        os_getTime() + ms2osticks(10),
        timeToSendTestMessageCb
        );
}

static void lmicEventCb(
    void *pUserData,
    ev_t ev
) {
    LMIC_API_PARAMETER(pUserData);

    // pass to user handler
    if (LMIC_Cert.saveEvent.pEventCb) {
        LMIC_Cert.saveEvent.pEventCb(
            LMIC_Cert.saveEvent.pUserData, ev
            );
    }

    // if it's a EV_JOINED, we should tell the FSM.
    if (ev == EV_JOINED) {
        LMIC_Cert.eventflags |= LMIC_CERT_EVENT_JOINED;
        fsmEval();
    }
}

static void timeToSendTestMessageCb(osjob_t *j) {
    LMIC_Cert.eventflags |= LMIC_CERT_EVENT_SEND_UPLINK;
    fsmEval();
}


static void acExitActiveMode(void) {
    LMIC_Cert.state = LMIC_CERT_STATE_IDLE;
    os_clearCallback(&LMIC_Cert.uplinkJob);
    LMIC_clrTxData();
    LMIC_registerEventCb(LMIC_Cert.saveEvent.pEventCb, LMIC_Cert.saveEvent.pUserData);
}


static void acSendUplink(void) {
    uint8_t payload[2];
    uint32_t const downlink = LMIC.seqnoDn;

    // ignore event if uplink is busy.
    if (LMIC_Cert.fsmFlags & LMIC_CERT_FSM_UPLINK_BUSY)
        return;

    // build the uplink message
    payload[0] = (uint8_t) (downlink >> 8);
    payload[1] = (uint8_t) downlink;

    // reset the flags
    LMIC_Cert.eventflags &= ~(LMIC_CERT_EVENT_SEND_UPLINK | LMIC_CERT_EVENT_UPLINK_COMPLETE);

    // don't try to send if busy; might be sending echo message.
    if ((LMIC.opmode & OP_TXRXPEND) == 0 &&
        LMIC_sendWithCallback(
            LORAWAN_PORT_CERT,
            payload, sizeof(payload),
            /* confirmed? */
            !! (LMIC_Cert.fsmFlags & LMIC_CERT_FSM_CONFIRM),
            sendUplinkCompleteCb, NULL) == 0) {
        // queued successfully
        LMIC_CERT_PRINTF(
                "lmic_cert.acSendUplink: queued uplink message(%u, %p)\n",
                (unsigned) downlink & 0xFFFF,
                LMIC.client.txMessageCb
                );
    } else {
        // failed to queue; just skip this cycle.
        sendUplinkCompleteCb(NULL, false);
    }
}

static void sendUplinkCompleteCb(void *pUserData, int fSuccess) {
    LMIC_Cert.eventflags |= LMIC_CERT_EVENT_UPLINK_COMPLETE;
    LMIC_CERT_PRINTF("%s(%s)\n", __func__, txSuccessToString(fSuccess));
    fsmEval();
}

static void acScheduleNextUplink(void) {
    LMIC_Cert.eventflags &= ~(LMIC_CERT_EVENT_SEND_UPLINK | LMIC_CERT_EVENT_UPLINK_COMPLETE);

    // schedule an uplink.
    os_setTimedCallback(
        &LMIC_Cert.uplinkJob,
        os_getTime() + sec2osticks(5),
        timeToSendTestMessageCb
        );
}

static void acDoJoin(void) {
    LMIC_CERT_PRINTF("acDoJoin\n");

    LMIC_Cert.eventflags &= ~(LMIC_CERT_EVENT_JOIN_CMD | LMIC_CERT_EVENT_JOINED);

    LMIC_unjoin();
    LMIC_startJoining();
}
