/* 2012 Andrew D'Angelo <excel@pharcyde.org> */
/* version .2 :: now with GAME FACE support */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bmpfile.h> //hopefully remove dependence on libbmp
#include <errno.h>   //and switch to raw bitmap output eventually
#include <sys/stat.h>

//Makefile: (requires libbmp)
//gbcdump:
//	gcc -Wall -lm gbcdump.c /usr/local/lib/libbmp.a -o gbcdump

//According to Mr. Baffy: Game face is at 0x11fc

//current process: read each hexblock and convert to bitmap string then store in array, write pixels later
//extremely ineficcient. new process (borrowed from Baffy):
//method read takes an offset and bmp object and writes to it
//replace writing logic with bmp_write from lib
//loop 30 times, increasing the counter by 0x1000 and write the game face afterwards
//todo: picture preview strip? pic offset + 0x0e00

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

void substr(char *string, int start, int end, char *frame) {
	//Due to function modifying a passed pointer, it is void instead of char*
	if (end>strlen(string)) { //0 based, but add 1 to end for the \0
		end=strlen(string);
	}
	strncpy(frame, string+start, end-start);
}

void usage(char* name) {
	fprintf(stderr, "Usage: %s [OPTION] battery.sav\n\n  -q, quiet mode\n  -w, write including blanks\n  -d, dry run (don't write anything)\n  -n [FILENAME], output in custom folder\n  --help, display this text\n\nReport bugs to Andrew D'Angelo <excel@pharcyde.org>\n", name);
	exit(1);
}

void write_bmp(int number, char *foldername, char *pixel, rgb_pixel_t licorice, rgb_pixel_t nickel, 
				rgb_pixel_t silver, rgb_pixel_t snow){
	bmpfile_t *bmp;
	char finalpath[32];
	int j, k;
	
	if ((bmp = bmp_create(128,112,8)) == NULL) {
		fprintf(stderr, "error! bitmap fatal");
	}
	for (j=0;j<128;j++) {
		for (k=0;k<112;k++) {
			switch (pixel[(k*128)+j]) {
				case 'B':
					bmp_set_pixel(bmp,j,k,licorice);
					break;
				case 'D':
					bmp_set_pixel(bmp,j,k,nickel);
					break;
				case 'L':
					bmp_set_pixel(bmp,j,k,silver);
					break;
				case 'W':
					bmp_set_pixel(bmp,j,k,snow);
					break;
			}
		}
	}
	sprintf(finalpath,"%s/%s%i.bmp",foldername,foldername,number);
	bmp_save(bmp, finalpath);
	bmp_destroy(bmp); //according to bmpfile.c, this free(bmp)
}

int unweave_and_untile(char *buffer, char *pixel) { //auto-dedent this shit ASAP
//ideally we'd be passing merely a pointer to the array to this function, but it's not working out
	char byte_one[9];
	byte_one[8] = '\0';
	char byte_two[9];
	byte_two[8] = '\0';
	char unwoven_twobyte[17];
	unwoven_twobyte[16] = '\0';
	int tile_col = 0, tile_row = 0, row = 0;
	int offset = 0;
	int i, j = 0;
	int IS_BLANK = 1;

for (i=0;i<3584;i+=2) { //loop through each 2 bind
				strncpy(byte_one,bytetobin(buffer[i]),8);
				strncpy(byte_two,bytetobin(buffer[i+1]),8);
				for (j=0;j<8;j++) {
					unwoven_twobyte[j*2] = byte_one[j];
					unwoven_twobyte[j*2+1] = byte_two[j];
				}
				if (IS_BLANK) {
					if (strncmp(unwoven_twobyte,"0000000000000000",17) != 0) {
						IS_BLANK = 0; //different than the python release, we skip
					} //pictures instead of stopping the dump
				}
				offset = (tile_col*8)+(row*128)+(tile_row*128*8); //let me tell you
				for (j=0;j<8;j++) { //huge pain in the ass this was, this whole offset business
					if (unwoven_twobyte[j*2] == '0') {
						if (unwoven_twobyte[j*2+1] == '0') { 
							//*(pixel+offset+j) = 'W';
							pixel[offset+j] = 'W';
						} else { 
							//*(pixel+offset+j) = 'D';
							pixel[offset+j] = 'D';
						}
					} else {
						if (unwoven_twobyte[j*2+1] == '0') {
							//*(pixel+offset+j) = 'L';
							pixel[offset+j] = 'L';
						} else { 
							//*(pixel+offset+j) = 'B';
							pixel[offset+j] = 'B';
						}
					}
				}
				row++;
				if (row == 8) {
					row = 0;
					tile_col++;
				} //basically instead of using a grid to find the right tile, we simply
				if (tile_col == 16) { //use something more akin to wordwrap... but for 
					tile_col = 0; //array elements (these values spacing it correctly)
					tile_row++;
				}
			}
			
	return IS_BLANK;
}

int main(int argc, char *argv[]) {

	int i;
	int LOUD = 1;
	int SKIP_BLANK = 1;
	int DRY_RUN = 0;
	int MAX_LEN = 32;
	char filename[MAX_LEN+2], foldername[MAX_LEN+2];
	filename[0] = '\0';
	foldername[0] = '\0';

	//OS X tiger color names <3
	rgb_pixel_t snow = {255, 255, 255, 0};
	rgb_pixel_t silver = {170, 170, 170, 0};
	rgb_pixel_t nickel = {85, 85, 85, 0};
	rgb_pixel_t licorice = {0, 0, 0, 0};

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-q") == 0) {
			LOUD = 0;
		} else if (strncmp(argv[i], "-w", MAX_LEN) == 0) {
			SKIP_BLANK = 0;
		} else if (strncmp(argv[i], "-d", MAX_LEN) == 0) {
			DRY_RUN = 1;
		} else if (strncmp(argv[i], "--help", MAX_LEN) == 0) {
			usage(argv[0]);
		} else if (strncmp(argv[i], "-n", MAX_LEN) == 0) {
			strncpy(foldername, argv[i+1], MAX_LEN+1);
			foldername[MAX_LEN+1] = '\0';
			i++;
		} else {
			strncpy(filename, argv[i], MAX_LEN+1);
			filename[MAX_LEN+1] = '\0';
		}
	}
	
	//kitten cam: https://new.livestream.com/accounts/398160/events/1594566/player_pop_up
	if (strlen(foldername) == 0) {
		if (chrloc(filename, '.') != -1) {
			char *frame = (char *)malloc(MAX_LEN*sizeof(char));
			frame[MAX_LEN] = '\0';
			substr(filename, 0, chrloc(filename, '.'), (char *)frame);
			strncpy(foldername, frame, MAX_LEN);
			free(frame);
		} else {
			strncpy(foldername, filename, MAX_LEN+1);
			foldername[MAX_LEN+1] = '\0';
		}
	}
	
	FILE *fileptr;
	fileptr = fopen(filename, "rb");
	if (fileptr) {
		//Init vars
		int loc;
		int skip_me[30] = {};
		char *buffer;
		
		buffer = (char *)malloc(3585*sizeof(char));
		if (buffer == NULL) {
			if (LOUD) { fprintf(stderr, "error! out of memory\n"); }
			return 1;
		}
		
		//set up folders n such
		if (strlen(filename)>0) {
		
			//Verify save
			for (loc=12209;loc<130993;loc+=4096) {
				fseek(fileptr,loc,SEEK_SET);
				fread(buffer,5,1,fileptr);
				
				if (strncmp(buffer, "Magic", 5) != 0){
					if (LOUD) { fprintf(stderr, "error! magic check failed\n"); }
					return 1;
				}
			}
			if (LOUD) { printf("Save is OK. "); }
			
			if (LOUD) { 
				if (DRY_RUN) { printf("Reading in file %s and not dumping\n\n", filename); }
				else { printf("Reading in file %s and dumping to %s/\n\n", filename, foldername); }
			}
		} else {
			if (LOUD) { fprintf(stderr, "error! no valid filename provided\n");
			usage(argv[0]); }
			else { return 1; }
		}
		
		//Read photos
		int NUMBER_OF_PHOTOS = 0;
		int CURRENT_PICTURE;
		char **picture_array; //double pointers -- how awful
		picture_array = malloc(30*sizeof(char *)); //mallocing the array was
		if (picture_array == NULL) { //a lot more painful, but better (heap)
			if (LOUD) { fprintf(stderr, "error! out of memory\n"); }
			return 1;
		}
		char *game_face = (char *)malloc((3584*4+1)*sizeof(char));
		
		for (i=0;i<30;i++) {
			picture_array[i] = (char *)malloc((3584*4+1)*sizeof(char));
			if (picture_array[i] == NULL) {
				if (LOUD) { fprintf(stderr, "error! out of memory\n"); }
				return 1;
			}
		}
		
		int IS_BLANK;
		//char *pixel;
		
		if (LOUD) { printf("Reading photos...\n"); }
			
		fseek(fileptr,4604,SEEK_SET); //get your game_face on
		fread(buffer,3584,1,fileptr);
		buffer[3584] = '\0';
		game_face[3584*4] = '\0';
		IS_BLANK = unweave_and_untile(buffer, game_face);
		if (IS_BLANK) {
			if (LOUD) { printf("No game face!\n"); }
		} else {
			if (LOUD) { printf("Game face is OK...\n"); }
		}
		
		for (loc=8192;loc<8192+(4096*30);loc+=4096) {
			CURRENT_PICTURE = (int)((loc-8192)/4096); //we are going for a more array
			fseek(fileptr,loc,SEEK_SET); //oriented solution and so CURR_P is zero-base
			fread(buffer,3584,1,fileptr);
			buffer[3584] = '\0';
			picture_array[CURRENT_PICTURE][3584*4] = '\0'; //must init :-)
			//pixel = picture_array[CURRENT_PICTURE];
			//Now we have a buffer of binary data

			//each 1 bind > 4 hex > 16 bits > unweave > each 2 bits > color > array
			//holy crap, forgot all about tiles. ok
			IS_BLANK = unweave_and_untile(buffer, picture_array[CURRENT_PICTURE]);

			//By now we have a fully unwoven, untiled and "colored" picture array
			if (LOUD) {
				printf("%2i ",CURRENT_PICTURE+1);
				if (!IS_BLANK || !SKIP_BLANK) { NUMBER_OF_PHOTOS++; }
				if (IS_BLANK) {
					printf("is blank.");
					if (SKIP_BLANK) { skip_me[CURRENT_PICTURE] = 1; }
				} else {
					printf("is OK.");
				}
				if ((CURRENT_PICTURE+1) % 5 == 0) {
					printf("\n");	//Maybe dynamic spacing?
				} else {
					printf("\t");
				}
			}
		}

	
		if (LOUD) { printf("\nWriting %i photos... ", NUMBER_OF_PHOTOS); }
		fflush(stdout); //so we can output before processing
		
		if (1) { //remove this if block when you can autodedent
			//Write photos
			if (!DRY_RUN) { if (mkdir(foldername, 0000711) == -1) {
				char modfoldername[32];
				strncpy(modfoldername, foldername, 32);
				
				if (strncmp(strerror(errno),"File exists",12) == 0) {
					i = 0;
					while ((mkdir(modfoldername, 0000711) == -1) && (i<256)) { //lets be reasonable here
						if (strncmp(strerror(errno),"File exists",12) != 0) {
							sprintf(foldername,"%s%i",foldername,i);
							printf("%s",modfoldername);
							break;
						}
						i++;
						sprintf(modfoldername,"%s%i",foldername,i);
					}
				} else if (strncmp(strerror(errno),"Permission denied",17) == 0) {
					if (LOUD) { fprintf(stderr, "error! permission denied\n"); }
					return 1;
				} else {
					if (LOUD) { fprintf(stderr, "error! unhandled: %s\n",strerror(errno)); }
					return 1;
				}
			} }
			
			for (i=0;i<NUMBER_OF_PHOTOS+1;i++) {
				if (!skip_me[i] && !DRY_RUN) { //error checking: write permissions?
					//do it
					//>>LOLNOPE
					write_bmp(i, foldername, picture_array[i], licorice, nickel, silver, snow);
				}
			}
			if (LOUD) { printf("Writing the game face... "); }
			write_bmp(666, foldername, game_face, licorice, nickel, silver, snow);
			if (LOUD) {
				if (!DRY_RUN) {
					printf("OK\n");
				} else {
					printf("Dry run\n");
				}
			}
		}
		
		for (i=0;i<30;i++) { free(picture_array[i]); }
		free(picture_array); //time for cleanup
		free(game_face);
		free(buffer);
		free(fileptr);
		
	} else {
		if (LOUD) { fprintf(stderr, "error! cannot open file %s\n", filename); }
		return 1;
	}

	return 0;
}
