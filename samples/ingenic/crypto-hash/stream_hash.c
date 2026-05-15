#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <linux/socket.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

//#include <openssl/sha.h>

#ifndef SOL_ALG
#define SOL_ALG 279
#endif

#define MAX_SINGLE_TRANS_SIZE	(64*1024) //单次send最大传输文件大小

unsigned int get_file_size(FILE *file)
{
	int current_position = ftell(file);

	fseek(file, 0, SEEK_END);

	int file_size = ftell(file);

	fseek(file, current_position, SEEK_SET);

	return file_size;
}

int main(int argc, char *argv[])
{
	int opfd;
	int tfmfd;
	int inlen = 0;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "hash",
		.salg_name = "sha256" };
	int i = 0;
	int send_count = 0;
	unsigned char buf[32];
	int ret;
	unsigned int file_size;
	char salg_name_buffer[8];

	FILE *srcfp = fopen(argv[1], "r");
	if(srcfp == NULL) {
		printf("open input file error !!!!\n");
		return -1;
	}

	strncpy(sa.salg_name, argv[2], sizeof(salg_name_buffer));
	printf("algname = %s\n", sa.salg_name);
	file_size = get_file_size(srcfp);
	printf("file size = %d\n", file_size);
	void *input;
	//(unsigned int *)posix_memalign(&input, 4096, file_size);
	input = malloc(file_size);
	if(input == NULL) {
		printf("alloc error !!!!\n");
		return -1;
	}
	memset(input, 0, file_size);

	//fread(input, file_size, 1, srcfp);
	//printf("mode = %s\n", *mode);
	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (tfmfd < 0) {
		perror("socket");
		free(input);
		return -1;
	}

	ret = bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		perror("bind");
		close(tfmfd);
		free(input);
		return -1;
	}

	opfd = accept(tfmfd, NULL, 0);
	if (ret < 0) {
		perror("bind");
		close(tfmfd);
		free(input);
		return -1;
	}

	if(file_size > MAX_SINGLE_TRANS_SIZE){
		printf("file size  = %d\n", file_size);
		printf("max single size  = %d\n", MAX_SINGLE_TRANS_SIZE);
		inlen = MAX_SINGLE_TRANS_SIZE;
	}else{
		inlen = file_size;
	}
	printf("first send inlen = %d\n", inlen);

	int last_data = 0;
	send_count = (file_size)/MAX_SINGLE_TRANS_SIZE;
	printf("send count = %d\n", send_count);
	if(send_count > 0){
		last_data = file_size%inlen;
	}else{
		last_data = file_size;
	}
	for(i = 0; i < send_count; i++){
		fseek(srcfp, MAX_SINGLE_TRANS_SIZE*i, SEEK_SET);
		fread(input, MAX_SINGLE_TRANS_SIZE, 1, srcfp);
		inlen = MAX_SINGLE_TRANS_SIZE;
		printf("inlen%d = %d\n", i, inlen);
		ret = send(opfd, input, inlen, MSG_MORE);
	}

	printf("last_data = %d\n", last_data);//如果文件是非4K对齐，last_data是保持4K对齐后剩下的数据长度，需要单独做一次send
	fread(input, last_data, 1, srcfp);
	ret = send(opfd, input, last_data, MSG_MORE);
	// computing expected
	unsigned char expected_digest[64];
	/*SHA256(init_input, total, expected_digest);*/


	printf("result   ");
	if(!strcmp(sa.salg_name, "sha256")){
		ret = read(opfd, buf, 32);
		for (i = 0; i < 32; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else if(!strcmp(sa.salg_name, "sha512")){
		ret = read(opfd, buf, 64);
		for (i = 0; i < 64; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else if(!strcmp(sa.salg_name, "sha1")){
		ret = read(opfd, buf, 20);
		for (i = 0; i < 20; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else if(!strcmp(sa.salg_name, "md5")){
		ret = read(opfd, buf, 16);
		for (i = 0; i < 16; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else if(!strcmp(sa.salg_name, "sha224")){
		ret = read(opfd, buf, 28);
		for (i = 0; i < 28; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else if(!strcmp(sa.salg_name, "sha384")){
		ret = read(opfd, buf, 48);
		for (i = 0; i < 48; i++) {
			printf("%02x", (unsigned char)buf[i]);
		}
	}else{
		printf("please input right digest type!!\neg:./sample_k_hash input_file sha256");
	}
	printf("\n");

	close(opfd);
	close(tfmfd);
	free(input);



	return 0;
}
