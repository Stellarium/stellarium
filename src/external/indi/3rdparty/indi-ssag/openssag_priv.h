/*
 * File: openssag_priv.h
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 * Migration to libusb 1.0 by Peter Polakovic
 *
 */

#ifndef __OPENSSAG_PRIV_H_
#define __OPENSSAG_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void IDMessage(const char *dev, const char *fmt, ...);
#ifdef __cplusplus
}
#endif

#define DBG(...) IDMessage("SSAG CCD", __VA_ARGS__)

extern libusb_context *ctx;

#endif /* __OPENSSAG_PRIV_H_ */
