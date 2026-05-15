#ifndef _H264_CABAC_H_
#define _H264_CABAC_H_

void i264e_cabac_init(void);
void i264e_cabac_context_init(void *state, int i_slice_type, int i_qp, int i_model);

#endif //_H264_CABAC_H_
