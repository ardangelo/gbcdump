#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bmpfile.h>
#include "gbcdump.h"

#define FNLEN (32)

/*	Variables	*/
rgb_pixel_t colors[] = { /*	snow, nickel, silver, and licorice	*/
	{255, 255, 255, 0},	{85, 85, 85, 0},
	{170, 170, 170, 0},	{0, 0, 0, 0}
};
unsigned int i, j, k, tile;

/*	Functions	*/
int main(int argc, char *argv[]) {
	FILE *fileptr;
	bmpfile_t *bmp;
	char filename[FNLEN];
	char hex_buffer[2];
	char bin1_buffer[9]; //change into bitpacked array of size 2
	char bin2_buffer[9];
	bin1_buffer[8] = '\0';
	bin2_buffer[8] = '\0';

	for (i=1;i<argc;i++) { //i is arg iterator
		if ((fileptr = fopen(argv[i],"rb"))) {
			printf("File loaded: %s\n",argv[i]);
			strncpy(filename,argv[i],FNLEN);
			break;
		}
		fprintf(stderr,"Error loading file: %s\n",argv[i]);
		fclose(fileptr);
	}

	if (!fileptr)
		return 1;

	if (validate_file(fileptr)) {
		fprintf(stderr,"File failed MAGIC check: %s\n",filename);
		fclose(fileptr);
		return 1;
	}
	
	printf("File passed MAGIC check: %s\n",filename);
	bmp = bmp_create(128,112,8);

	for (i=0x2000;i<0x2000+(0x1000*1);i+=0x1000) { //i is offset
		fseek(fileptr,i,SEEK_SET);
		for (tile=0;tile<224;tile++) {
			for (j=0;j<8;j++) { //j is for each row of pixels
				for (k=0;k<4;k++) { //k is for each 2 pixels in row
					fread(hex_buffer,1,1,fileptr);
					fread(hex_buffer+1,1,1,fileptr);
					bytetobin(hex_buffer,bin1_buffer);
					bytetobin(hex_buffer+1,bin2_buffer);
					printf("%s %s\n",bin1_buffer,bin2_buffer);

					//fseek(fileptr,1,SEEK_CUR); //set up for next read
				}
			}
		}
	}

	bmp_save(bmp,"test.bmp");

	bmp_destroy(bmp);
	fclose(fileptr);

	return 0;
}

/*	Aux. Functions	*/
int validate_file(FILE *file) {
	char buffer[6];
	
	fseek(file, 0L, SEEK_END);
	if ((int)ftell(file) != 131072) {
		return 1;
	}
	fseek(file, 0L, SEEK_SET);

	for (i=0x2FB1;i<0x1FFB1;i+=0x1000) {
		fseek(file,i,SEEK_SET);
		fread(buffer,5,1,file);
		
		if (strncmp(buffer, "Magic", 5) != 0){
			return 1;
		}
	}

	return 0;
}

const char *bytetobin(char *data, char *dest) { //from EvilTeach
	*dest = '\0';

	int z;
	for (z = 128; z > 0; z >>= 1) {
		strcat(dest, ((*data & z) == z) ? "1" : "0");
	}

	return dest;
}

int bit_int(const char a, const char b) {
	switch (a) {
		case '0': switch (b) { case '0': return 0; case '1': return 1; }
		case '1': switch (b) { case '0': return 2; case '1': return 3; }
	}

	return -1;
}
