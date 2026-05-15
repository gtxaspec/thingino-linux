#ifndef _H265_ENTROPY_H_
#define _H265_ENTROPY_H_

#include "h265_bitstream.h"
#include "h265_slice.h"
#include "h265_core.h"

void defaultVPS(VPS *vps, struct h265_param *param);
void defaultSPS(SPS *sps, struct h265_param *param);
void defaultPPS(PPS *pps, struct h265_param *param);
void codeVPS(Bitstream *bs, const VPS *vps);
void codeSPS(Bitstream *bs, const SPS *sps, const ScalingList *scalingList, const ProfileTierLevel *ptl);
void codePPS(Bitstream *bs, const PPS *pps, bool filerAcross, int iPPSInitQpMinus26 );
void computeRPS(Slice *slice);
void getStreamHeaders(NALU *nalu, NALList *list, Bitstream *bs);
int getStream(NALU *nalu, NALList *list, Bitstream *bs, void *addr, int len);
int getStreamSegment(NALU *nalu, NALList *list, Bitstream *bs, void *addr, int len, int count, int is_final);

#endif //_H265_ENTROPY_H_
