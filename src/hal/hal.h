/*******************************************************************************
 * Copyright (c) 2015-2016 Matthijs Kooijman
 * Copyright (c) 2016-2018 MCCI Corporation
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/
#ifndef _hal_hal_h_
#define _hal_hal_h_

#include "arduino_lmic_hal_configuration.h"

// for compatbility reasons, we need to disclose the configuration
// structure as global type lmic_pinmap.
using lmic_pinmap = Arduino_LMIC::HalPinmap_t;

// similarly, we need to disclose NUM_DIO and LMIC_UNUSED_PIN
static const int NUM_DIO = lmic_pinmap::NUM_DIO;

// Use this for any unused pins.
const u1_t LMIC_UNUSED_PIN = lmic_pinmap::UNUSED_PIN;

// Declared here, to be defined and initialized by the application.
// Use os_init_ex() if you want not to use a const table, or if
// you need to define a derived type (so you can override methods).
extern const lmic_pinmap lmic_pins;

#endif // _hal_hal_h_
