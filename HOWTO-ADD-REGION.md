# Adding a new region to Arduino LMIC

This variant of the Arduino LMIC code supports adding additional regions beyond the eu868 and us915 bands supported by the original IBM LMIC 1.6 code.

This document outlines how to add a new region.

<!--
  This TOC uses the VS Code markdown TOC extension AlanWalk.markdown-toc.
  We strongly recommend updating using VS Code, the markdown-toc extension and the
  bierner.markdown-preview-github-styles extension. Note that if you are using
  VS Code 1.29 and Markdown TOC 1.5.6, https://github.com/AlanWalk/markdown-toc/issues/65
  applies -- you must change your line-ending to some non-auto value in Settings>
  Text Editor>Files.  `\n` works for me.
-->
<!-- markdownlint-disable MD033 MD004 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Planning](#planning)
	- [Determine the region/region category](#determine-the-regionregion-category)
	- [Check whether the region is already listed in `lmic_config_preconditions.h`](#check-whether-the-region-is-already-listed-in-lmic_config_preconditionsh)
- [Make the appropriate changes in `lmic_config_preconditions.h`](#make-the-appropriate-changes-in-lmic_config_preconditionsh)
- [Document your region in `README.md`](#document-your-region-in-readmemd)
- [Add the definitions for your region in `lorabase.h`](#add-the-definitions-for-your-region-in-lorabaseh)
- [Edit `lmic_bandplan.h`](#edit-lmic_bandplanh)
- [Create <code>lmic_<em>newregion</em>.c</code>](#create-codelmic_emnewregionemccode)
- [General Discussion](#general-discussion)
- [Adding the region to the Arduino_LoRaWAN library](#adding-the-region-to-the-arduino_lorawan-library)

<!-- /TOC -->
<!-- markdownlint-restore -->
<!-- Due to a bug in Markdown TOC, the table is formatted incorrectly if tab indentation is set other than 4. Due to another bug, this comment must be *after* the TOC entry. -->

## Planning

### Determine the region/region category

Compare the target region (in the LoRaWAN regional specification) to the EU868 and US915 regions. There are three possibilities.

1. The region is like the EU region. There are a limited number of channels (up to 8), and only a small number of channels are used for OTAA join operations. The response masks refer to individual channels, and the JOIN-response can send frequencies of specific channels to be added.

2. The region is like the US region. There are many channels (the US has 64) with fixed frequencies, and the channel masks refer to subsets of the fixed channels.

3. The region is not really like either the EU or US. At the moment, it seems that CN470-510MHz (section 2.6 of LoRaWAN Regional Parameters spec V1.0.2rB) falls into this category.

Band plans in categories (1) and (2) are easily supported. Band plans in category (3) are not supported by the current code.

### Check whether the region is already listed in `lmic_config_preconditions.h`

Check `src/lmic/lmic_config_preconditions.h` and scan the `LMIC_REGION_...` definitions. The numeric values are assigned based on the subchapter in section 2 of the LoRaWAN 1.0.2 Regional Parameters document. If your symbol is already there, then the first part of adaptation has already been done. There will already be a corresponding `CFG_...` symbol. But if your region isn't supported, you'll need to add it here.

- `LMIC_REGION_myregion` must be a distinct integer, and must be less than 32 (so as to fit into a bitmask)

## Make the appropriate changes in `lmic_config_preconditions.h`

- `LMIC_REGION_SUPPORTED` is a bit mask of all regions supported by the code. Your new region must appear in this list.
- `CFG_LMIC_REGION_MASK` is a bit mask that, when expanded, returns a bitmask for each defined `CFG_...` variable. You must add your `CFG_myregion` symbol to this list.
- `CFG_region` evaluates to the `LMIC_REGION_...` value for the selected region (as long as only one region is selected). The header files check for this, so you don't have to.
- `CFG_LMIC_EU_like_MASK` is a bitmask of regions that are EU-like, and `CFG_LMIC_US_like_MASK` is a bitmask of regions that are US-like. Add your region to the appropriate one of these two variables.

## Document your region in `README.md`

You'll see where the regions are listed. Add yours.

## Add the definitions for your region in `lorabase.h`

- If your region is EU like, copy the EU block. Document any duty-cycle limitations.
- if your region is US like, copy the US block.
- As appropriate, copy `lorabase_eu868.h` or `lorabase_us915.h` to make your own <code>lorabase_<em>newregion</em>.h</code>.  Fill in the symbols.

At time of writing, you need to duplicate some code to copy some settings from `lorabase_eu868.h` or `lorabase_us915.h` to the new file; and you need to put some region-specific knowledge into the `lorabase.h` header file. The long-term direction is to put all the regional knowledge into the region-specific header, and then the central code will just copy. The architectural impulse is that we'll want to be able to reuse the regional header files in other contexts. On the other hand, because it's error prone, we don't want to `#include` files that aren't being used; otherwise you could accidentally use EU parameters in US code, etc.

- Now's a good time to test-compile and clean out errors introduced. Make sure you set the region to your new target region. You'll have problems compiling, but they should look like this:

    ```console
    lmic.c:29: In file included from

    lmic_bandplan.h: 52:3: error: #error "LMICbandplan_maxFrameLen() not defined by bandplan"
       # error "LMICbandplan_maxFrameLen() not defined by bandplan"

    lmic_bandplan.h: 56:3: error: #error "pow2dBm() not defined by bandplan"
       # error "pow2dBm() not defined by bandplan"
    ```

- If using an MCCI BSP, you might want to edit your local copy of `boards.txt` to add the new region.

- If using an MCCI BSP, you should definitely edit the template files to add the new region to the list.

- Modify the `.travis.yml` file to test the new region.

## Edit `lmic_bandplan.h`

The next step is to add the region-specific interfaces for your region.

Do this by editing `lmic_bandplan.h` and adding the appropriate call to a (new) region-specific file `lmic_bandplan_myregion.h`, where "myregion" is the abbreviation for your region.

Then, if your region is eu868-like, copy `lmic_bandplan_eu868.h` to create your new region-specific header file; otherwise copy `lmic_bandplan_us915.h`.

Edit the file.

Try to compile again; you should now get link errors related to your new band-plan, like this:

```console
c:\tmp\buildfolder\libraries\arduino-lmic\lmic\lmic.c.o: In function `lowerDR':

C:\Users\tmm\Documents\Arduino\libraries\arduino-lmic\src\lmic/lorabase.h:667: undefined reference to `constant_table__DR2RPS_CRC'
```

## Create <code>lmic_<em>newregion</em>.c</code>

Once again, you will start by copying either `lmic_eu868.c` or `lmic_us915.c` to create your new file. Then touch it up as necessary.

## General Discussion

- You'll find it easier to do the test compiles using the example scripts in this directory, rather than trying to get all the Catena framework going too. On the other hand, working with the Catena framework will expose more problems.

- Don't forget to check and update the examples.

- You will also need to update the `boards.template` file for MCCI BSPs, in order to get the region to show up in the Arduino IDE menu.

- You will need to update the [`arduino-lorawan`](https://github.com/mcci-catena/arduino-lorawan) library to include support for the new region. (See [below](#adding-the-region-to-the-arduino_lorawan-library) for instructions.)

- Please increase the version of `arduino-lmic` (symbol `ARDUINO_LMIC_VERSION` in `src/lmic/lmic.h`), and change `arduino-lorawan`'s `Arduino_LoRaWAN_lmic.h` to check for at least that newer version.

- Please also increase the version of the `arduino-lorawan` library (symbol `ARDUINO_LORAWAN_VERSION`).

## Adding the region to the Arduino_LoRaWAN library

In `Arduino_LoRaWAN_ttn.h`:

- Add a new class with name `Arduino_LoRaWAN_ttn_myregion`, copied either from the `Arduino_LoRaWAN_ttn_eu868` class or the `Arduino_LoRaWAN_ttn_us915` class.
- Extend the list of `#if defined(CFG_eu868)` etc. to define `Arduino_LoRaWAN_REGION_TAG` to the suffix of your new class if `CFG_myregion` is defined.

Then copy and edit either `ttn_eu868_netbegin.cpp`/`ttn_eu868_netjoin.cpp` or `ttn_us915_netbegin.cpp`/`ttn_us915_netjoin.cpp` to make your own file(s) for the key functions.
