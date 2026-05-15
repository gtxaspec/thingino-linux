#ifndef __AES_H__
#define __AES_H__


#define CBC	0
#define ECB	1

void aes_crypt(unsigned char *input, int inlen, unsigned char *out,
        const unsigned char *key, unsigned char *oiv, int op, int mode, int key_len);

#endif  // __AES_H__
