/* Game Boy Camera image dumper released under GNU GPL v3.0 */
/* 2013 Andrew D'Angelo <excel@pharcyde.org> */
/* version 1.0 :: not as awful! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bmpfile.h> //hopefully remove dependence on libbmp
#include <errno.h>   //and switch to raw bitmap output eventually
#include <sys/stat.h>

#include "textcodes.c"

//Makefile: (requires libbmp)
//gbcdump-1.0:
//	gcc -Wall -lm gbcdump-1.0.c /usr/local/lib/libbmp.a -o gbcdump-1.0

//todo: picture preview strip? pic offset + 0x0e00

/*	GLOBALS	*/
char usage[] = "Usage: %s [OPTION] battery.sav\n\n  "
	"-f [FILENAME], output in custom folder\n  "
	"-n, don't use frames\n  "
	"-nc, don't dump captions\n  "
	"--help, display this text\n\nReport bugs to Andrew D'Angelo <excel@pharcyde.org>\n";
rgb_pixel_t snow = {255, 255, 255, 0};
rgb_pixel_t silver = {170, 170, 170, 0};
rgb_pixel_t nickel = {85, 85, 85, 0};
rgb_pixel_t licorice = {0, 0, 0, 0};
bmpfile_t *bmp;
#define FNLEN (32)

struct frame {
	char header[16][160];
	char left[112][16];
	char right[112][16];
	char footer[16][160];
} twenty; //switch twenty to a malloc

int i, j, k, l, loc;
int frame_number;
int captions;
char filename[FNLEN], foldername[FNLEN], finalpath[FNLEN];

/*	PROTOTYPES	*/
const char *bytetobin(int x);
int create_folder(char* foldername);
int chrloc(char *string, char chr);
int read_file(FILE *file, int offset, bmpfile_t *bmp);
int validate_file(FILE *file);
char *get_caption(FILE *file, char *caption, int offset); //textcodes.c
void write_frame(struct frame *twenty, bmpfile_t *bmp);

int main(int argc, char *argv[]) {
	#include "frames.c" //this is the top priority. stop doing this.

	filename[0] = '\0';
	frame_number = 20;
	captions = 1;

	//read args and set options
	if (argc == 1) {
		fprintf(stderr,usage,argv[0]);
		exit(1);
	}
	
	for (i=0;i<argc;i++) {
		if (strncmp(argv[i], "--help",7) == 0) {
			fprintf(stderr,usage,argv[0]);
			exit(1);
		} else if (strncmp(argv[i], "-n",3) == 0) {
			frame_number = 0;
		} else if (strncmp(argv[i], "-nc",3) == 0) {
			captions = 0;
		} else if (strncmp(argv[i], "-f",3) == 0) {
			strncpy(foldername, argv[i+1], FNLEN);
			foldername[FNLEN+1] = '\0';
			i++;
		} else {
			strncpy(filename, argv[i], FNLEN);
			filename[FNLEN+1] = '\0';
		}
	}
	if (chrloc(filename,'/') != -1) {
		fprintf(stderr,"Please move the battery file to the binary's folder, or vice-versa. Thanks!\n");
		exit(1);
	}
	if (strncmp(filename,"",1) == 0) {
		fprintf(stderr,usage,argv[0]);
	} else if (strncmp(foldername,"",1) == 0) {
		if (chrloc(filename,'.') != -1) {
			strncpy(foldername, filename, chrloc(filename,'.'));
		}
	}
	
	//make sure file exists
	FILE *fileptr;
	fileptr = fopen(filename, "rb");
	char caption[29];

	if (!fileptr) {
		fprintf(stderr,"error! file does not exist or permission denied!\n");
		exit(1);
	} else {
		//validate file with MAGIC
		if (validate_file(fileptr)) {
			fprintf(stderr,"error! file does not pass MAGIC check!\n");
			exit(1);
		} else {
			fprintf(stdout,"file is A-OK!\n");
		}
		//set up folders
		create_folder(foldername);
		fprintf(stdout,"dumping into folder %s\n", foldername);
		
		//loop 30 times, incrementing the HEX offset by 0x1000
		for (loc=0x2000;loc<0x2000+(0x1000*30);loc+=0x1000) {
			//create bitmap object
			bmp = bmp_create(128,112,8);
			if (frame_number) {
				bmp_destroy(bmp);
				bmp = bmp_create(160,144,8);
			}
			if (bmp == NULL) {
				fprintf(stderr, "error! bitmap fatal\n");
				exit(1);
			}

			//pass file pointer, bitmap object pointer to read method, get IS_BLANK back
			get_caption(fileptr,caption,(int)0x1000+((((loc-0x2000)/0x1000)+1)*0x1000)+0xf15);
			fprintf(stdout,"Photo %2i is ",(int)((loc-0x2000)/0x1000)+1);
			if (read_file(fileptr,loc,bmp)) {
				fprintf(stdout,"blank\n");
			} else {
				fprintf(stdout,"OK with frame %2i",frame_number);
				//now captions
				
				if (caption[0] != '\0') {
					if (captions) { fprintf(stdout," and caption: %s\n",caption); }
				} else {
					fprintf(stdout,"\n");
				}
				
				//deal with frames now
				if (frame_number) {
					//read frame_number from file
					write_frame(&twenty, bmp);
				}
				
				//write the pic if it's not blank with bmp_write from lib
				sprintf(finalpath,"%s/%s%i.bmp",foldername,foldername,(int)((loc-0x2000)/0x1000)+1);
				bmp_save(bmp, finalpath);
			}
			/*if (((int)((loc-0x2000)/0x1000)+1) % 5 == 0) {
				fprintf(stdout,"\n");
			} else {
				fprintf(stdout,"\t");
			}*/
			fflush(stdout); //so we can output before processing
			
			bmp_destroy(bmp);
		}
		//read the game face with read method
		frame_number = 0;
		bmp = bmp_create(128,112,8);
		if (bmp == NULL) {
			fprintf(stderr, "error! bitmap fatal\n");
			exit(1);
		}
		fprintf(stdout,"Game face is ");
		if (read_file(fileptr,0x11fc,bmp)) {
			fprintf(stdout,"blank\n");
		} else {
			fprintf(stdout,"OK\n");
			sprintf(finalpath,"%s/%sg.bmp",foldername,foldername);
			//write the game face
			bmp_save(bmp, finalpath);
		}
		
		bmp_destroy(bmp);
	}
	
	fclose(fileptr);
	
	return 0;
}

/*	FUNCTIONS	*/

const char *bytetobin(int x) { //accepts up to 255 (8 bit)
	static char b[9]; //From evilteach on StackExchange
	b[0] = '\0';

	int z;
	for (z = 128; z > 0; z >>= 1) {
		strcat(b, ((x & z) == z) ? "1" : "0");
	}

	return b;
}

int chrloc(char *string, char chr) {
	char *s;
	char chrs[2];
	chrs[0] = chr;
	chrs[1] = '\0';
	s = strstr(string, chrs);
	if (s != NULL) {
		unsigned int i;
		for (i=0;i<strlen(string);i++) {
			if (*(string+i) == chr) {
				return i;
			}
		}
	}
	return -1;
}

int create_folder(char* foldername) {
	char modfoldername[32];
	strncpy(modfoldername, foldername, 32);
	
	if (mkdir(foldername, 0000711) == -1) {
		if (strncmp(strerror(errno),"File exists",12) == 0) {
			i = 0;
			while ((mkdir(modfoldername, 0000711) == -1) && (i<256)) { //lets be reasonable here
				if (strncmp(strerror(errno),"File exists",12) != 0) {
					sprintf(foldername,"%s%i",foldername,i);
					break;
				}
				i++;
				sprintf(modfoldername,"%s%i",foldername,i);
			}
		} else if (strncmp(strerror(errno),"Permission denied",17) == 0) {
			fprintf(stderr, "error! permission denied\n");
			return 1;
		} else {
			fprintf(stderr, "error! unhandled: %s\n",strerror(errno));
			return 1;
		}
	}
	
	strncpy(foldername, modfoldername, 32);
	
	return 0;
}

int read_file(FILE *file, int offset, bmpfile_t *bmp) {
	int x, y, IS_BLANK = 1;
	char buffer[3585];
	char first_byte[9];
	char second_byte[9];
	
	fseek(file,offset,SEEK_SET);
	fread(buffer,3584,1,file);
	
	for (i=0;i<14;i++) {
		for (j=0;j<16;j++) {
			//loop through each byte in tile
			for (k=0;k<16;k+=2) {
				//set byte
				strncpy(first_byte,bytetobin((int)buffer[(16*j)+(256*i)+k]),8);
				strncpy(second_byte,bytetobin((int)buffer[(16*j)+(256*i)+k+1]),8);
				//check for content and set IS_BLANK accordingly
				if (IS_BLANK) {
					if (strncmp(first_byte,"00000000",8) && strncmp(second_byte,"00000000",8)) {
						IS_BLANK = 0;
					}
				}
				//set pixel of bmp based on parts of byte
				for (l=0;l<8;l++) {
					x = 8*j+l;
					y = 8*i+(int)(k/2);
					if (frame_number) {
						x+=16;
						y+=16;
					}
					switch (first_byte[l]) {
						case '0':
							switch (second_byte[l]) {
								case '0':
									bmp_set_pixel(bmp,x,y,snow);
									break;
								case '1':
									bmp_set_pixel(bmp,x,y,nickel);
									break;
							}
							break;
						case '1':
							switch (second_byte[l]) {
								case '0':
									bmp_set_pixel(bmp,x,y,silver);
									break;
								case '1':
									bmp_set_pixel(bmp,x,y,licorice);
									break;
							}
							break;
					}
				}
			}
		}
	}
	
	return IS_BLANK;
}

void write_frame(struct frame *twenty, bmpfile_t* bmp) {
	for (i=0;i<16;i++) {
		for (j=0;j<160;j++) {
			switch ((*twenty).header[i][j]) { //how strange that member takes precedence over derefrence
				case '0':	//if I didn't now this in advance I'd be tearing out my hair by now
					bmp_set_pixel(bmp,j,i,licorice);
					break;
				case '1':
					bmp_set_pixel(bmp,j,i,nickel);
					break;
				case '2':
					bmp_set_pixel(bmp,j,i,silver);
					break;
				case '3':
					bmp_set_pixel(bmp,j,i,snow);
					break;
			}
			switch ((*twenty).footer[i][j]) {
				case '0':
					bmp_set_pixel(bmp,j,i+128,licorice);
					break;
				case '1':
					bmp_set_pixel(bmp,j,i+128,nickel);
					break;
				case '2':
					bmp_set_pixel(bmp,j,i+128,silver);
					break;
				case '3':
					bmp_set_pixel(bmp,j,i+128,snow);
					break;
			}
		}
		for (j=0;j<112;j++) {
			switch ((*twenty).left[i][j]) {
				case '0':
					bmp_set_pixel(bmp,i,j+16,licorice);
					break;
				case '1':
					bmp_set_pixel(bmp,i,j+16,nickel);
					break;
				case '2':
					bmp_set_pixel(bmp,i,j+16,silver);
					break;
				case '3':
					bmp_set_pixel(bmp,i,j+16,snow);
					break;
			}
			switch ((*twenty).right[i][j]) {
				case '0':
					bmp_set_pixel(bmp,i+144,j+16,licorice);
					break;
				case '1':
					bmp_set_pixel(bmp,i+144,j+16,nickel);
					break;
				case '2':
					bmp_set_pixel(bmp,i+144,j+16,silver);
					break;
				case '3':
					bmp_set_pixel(bmp,i+144,j+16,snow);
					break;
			}
		}
	}
}

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
