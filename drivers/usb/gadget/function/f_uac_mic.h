/*
 * f_uac_mic.h -- Minimal UAC1 microphone-only USB function (no ALSA)
 */

#ifndef _F_UAC_MIC_H_
#define _F_UAC_MIC_H_

int uac_mic_bind_config(struct usb_configuration *c);

#endif /* _F_UAC_MIC_H_ */
