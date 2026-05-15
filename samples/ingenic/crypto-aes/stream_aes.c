#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if_alg.h>
#include <sys/socket.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "aes.h"

#define DATA_LEN 64
#define SOL_ALG 279
#define KEY_LEN  256
#define MAX_SINGLE_TRANS_SIZE (64*1024)
#define DUMP

unsigned int swap_endian(unsigned int value) {
	return ((value & 0xFF000000) >> 24) |
		((value & 0x00FF0000) >> 8) |
		((value & 0x0000FF00) << 8) |
		((value & 0x000000FF) << 24);
}

#ifdef DUMP
int dump(unsigned int *in, int len, char *chars)
{
	int i;

	printf("%s\n", chars);
	for(i = 0; i < len; i++) {
		printf("0x%08x, ", in[i]);
		if((i + 1) % 4 == 0)
			printf("\n");
	}
	printf("\n");
	printf("\n");
}
#else
int dump(unsigned int *in, int len, char *chars)
{
}
#endif

int cmp_data(unsigned int *src, unsigned int *tar, unsigned int len)
{
	int i;
	for(i = 0; i < len; i++){
		if(src[i] != tar[i]) {
			printf("cmp data error !!i = %d\n", i);
			return -1;
		}
	}
	return 0;
}

int main(int argc, char const *argv[])
{
	unsigned int key[8] ={0};
	unsigned int ivec[4] = {0};
	int opfd;
	int tfmfd;
	unsigned int cbuflen;
    struct sockaddr_alg sa;
    struct af_alg_iv *iv;
    struct iovec iov;

	struct stat fstat;
	if(argc < 5) {
		printf("aes_test in_file out_file key_file op(en:1, de:0)\n");
		return -1;
	}

	int op = atoi(argv[4]);

	if(op != 0 && op != 1){
		printf("op value error\n");
		return -1;
	}

	stat(argv[1], &fstat);

	FILE *srcfp = fopen(argv[1], "r");
	FILE *keyfp = fopen(argv[3], "r");
	FILE *fp = fopen(argv[2], "w");

	unsigned int *input = (unsigned int *)malloc((fstat.st_size + 16) /16 *16);
	unsigned int *output = (unsigned int *)malloc((fstat.st_size + 16) /16 *16);

    fread(input, fstat.st_size, 1, srcfp);
	fread(key, KEY_LEN/8, 1, keyfp);
	fread(ivec, 16, 1, keyfp);

    aes_crypt((void *)input, fstat.st_size, (void *)output, (void *)key, (void *)ivec, op, CBC, KEY_LEN);

	fwrite(output, fstat.st_size, 1, fp);
	free(input);
	free(output);
	fclose(srcfp);
	fclose(keyfp);
	fclose(fp);


}
