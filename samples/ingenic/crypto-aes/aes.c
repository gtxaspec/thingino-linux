#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <errno.h>
#include "aes.h"

//#define AF_ALG 38
#define SOL_ALG 279

void aes_crypt(unsigned char *input, int inlen, unsigned char *out,
        const unsigned char *key, unsigned char *oiv, int op, int mode, int key_len)
{
    int opfd;
    int tfmfd;
    unsigned int cbuflen;

    struct sockaddr_alg sa ={};
    sa.salg_family = AF_ALG;
    memcpy(sa.salg_type, "skcipher", sizeof("skcipher"));

    if(mode == CBC)
        memcpy(sa.salg_name, "cbc(aes)", sizeof("cbc(aes)"));
    else
        memcpy(sa.salg_name, "ecb(aes)", sizeof("ecb(aes)"));

    struct msghdr msg = {};
    struct cmsghdr *cmsg;

    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(20)] = {};

    struct af_alg_iv *iv;
    struct iovec iov;
    tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (tfmfd < 0) {
        fprintf(stderr, "sockfd: %s\n", strerror(errno));

        return;
    }
    if(bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa))) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        close(tfmfd);

        return;
    }
    setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY, key, key_len / 8);
    opfd = accept(tfmfd, NULL, 0);

    msg.msg_control = cbuf;

    if(mode == CBC)
        msg.msg_controllen = sizeof(cbuf);
    else
        msg.msg_controllen = sizeof(cbuf) - CMSG_SPACE(20);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type = ALG_SET_OP;
    cmsg->cmsg_len = CMSG_LEN(4);
    if(op)
    *(__u32 *)CMSG_DATA(cmsg) = ALG_OP_ENCRYPT;
    else
    *(__u32 *)CMSG_DATA(cmsg) = ALG_OP_DECRYPT;

	if(mode == CBC) {
		cmsg = CMSG_NXTHDR(&msg, cmsg);
		cmsg->cmsg_level = SOL_ALG;
		cmsg->cmsg_type = ALG_SET_IV;
		cmsg->cmsg_len = CMSG_LEN(20);
		iv = (void *)CMSG_DATA(cmsg);
		memcpy(iv->iv, oiv, 16);
		iv->ivlen = 16;
	}

	iov.iov_base = input;
	iov.iov_len = inlen;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	int send_len = 0;
	int send_total = 0;
	int read_len = 0;
	int read_total = 0;

	while(1){
		send_len =  sendmsg(opfd, &msg, 0);
		printf("send len = %d\n", send_len);
		read_len =  read(opfd, out, send_len);
		printf("read len = %d\n", read_len);
		if(read_len <= 0)
			break;
		send_total += send_len;
		printf("send total = %d\n", send_total);
		if(send_total < inlen){
			msg.msg_controllen = 0;
			iov.iov_base = input + send_total;
			iov.iov_len = inlen - send_total;
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			out = out + send_total;
			continue;
		}
		if(send_total >= inlen)
			break;
	}

	close(opfd);
	close(tfmfd);
}
