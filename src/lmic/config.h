#ifndef _lmic_config_h_
#define _lmic_config_h_

// In the original LMIC code, these config values were defined on the
// gcc commandline. Since Arduino does not allow easily modifying the
// compiler commandline, use this file instead.

#define CFG_eu868 1
//#define CFG_us915 1
#define CFG_sx1272_radio 1
//#define CFG_sx1276_radio 1

#endif // _lmic_config_h_
