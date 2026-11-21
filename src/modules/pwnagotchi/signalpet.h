// ============================================================================
//  Signal Pet — a passive "signal-hunting" tamagotchi for nullcore.
//
//  Unlike Brucegotchi (which deauths Wi-Fi clients to farm handshakes), this
//  pet NEVER transmits or attacks. It only *listens*: it passively scans the
//  Wi-Fi, BLE and 433 MHz (CC1101) bands around it and changes mood based on
//  how much radio life it can sense. Quiet room -> sleepy/bored. Busy room ->
//  excited. A brand-new strong signal -> alert.
//
//  Safe and legal everywhere: receive-only, no packets are ever sent.
// ============================================================================
#ifndef __SIGNALPET_H__
#define __SIGNALPET_H__

#if !defined(LITE_VERSION)
void signalpet_start();
#endif

#endif
