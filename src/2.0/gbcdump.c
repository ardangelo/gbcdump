/* Game Boy Camera image dumper */
/* 2013 Andrew D'Angelo <excel@pharcyde.org> */
/* version 2.0 :: it's da bomb */

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
unsigned int i, j, k, z;

/*	Functions	*/
int main(int argc, char *argv[]) {
	FILE *fileptr;
	bmpfile_t *bmp;
	char filename[FNLEN], path[FNLEN];
	char hex_buffer[2];
	unsigned int tile;

	if (!(argc-1)){
		fprintf(stderr,"Usage: %s battery.sav\n\nWritten by Andrew D'Angelo <excel@pharcyde.org>\n    http://lickitung.it.cx/\n",argv[0]);
		return 1;
	}

	for (i=1;i<argc;i++) { //i is arg iterator
		if ((fileptr = fopen(argv[i], "rb"))) {
			printf("File loaded: %s\n",argv[i]);
			strncpy(filename, argv[i], FNLEN);
			break;
		}
		fprintf(stderr, "Error loading file: %s\n", argv[i]);
		fclose(fileptr);
	}
	if (!fileptr)
		return 1;
	if (validate_file(fileptr)) {
		fprintf(stderr, "File failed MAGIC check: %s\n", filename);
		fclose(fileptr);
		return 1;
	}
	printf("File passed MAGIC check: %s\n", filename);
	bmp = bmp_create(128, 112, 8);
	for (i=0; i<30; i++) { //i is offset
		fseek(fileptr, 0x2000+(i*0x1000), SEEK_SET);
		printf("%02i. ", i + 1);
		for (tile=0; tile<224; tile++) { //tile or tile_col / tile_row?
			for (j=0; j<8; j++) { //j is for each row of pixels
				z = 128;
				fread(hex_buffer, 2, 1, fileptr);
				for (k=0; k<8; k++) { //k is for each pixel in row
					bmp_set_pixel(bmp, ((tile % 16) * 8) + k, (((tile - (tile % 16)) % 15) * 8) + j, colors[((*hex_buffer & z) == z) * 2 + ((*(hex_buffer + 1) & z) == z)]);
					z>>=1;
				}
			}
		}
		printf("Dumped. ");
		sprintf(path, "%s-%02i.bmp", filename, i + 1);
		bmp_save(bmp, path);
		printf("Written.\n");
	}
	bmp_destroy(bmp);
	fclose(fileptr);

	return 0;
}

/*	Aux. Functions	*/
int validate_file(FILE *file) {
	char buffer[6];
	
	fseek(file, 0L, SEEK_END);
	if ((int)ftell(file) != 131072)
		return 1;
	fseek(file, 0L, SEEK_SET);
	for (i=0x2FB1; i<0x1FFB1; i+=0x1000) {
		fseek(file, i, SEEK_SET);
		fread(buffer, 5, 1, file);
		if (strncmp(buffer, "Magic", 5) != 0)
			return 1;
	}

	return 0;
}
