char *get_caption(FILE *file, char *caption, int offset) {
	char buffer[2];
	int i;

	static const char lookup[115] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W',
		'X','Y','Z','_','\'',',','.',

		'A','A','A','A','E','E','E','E','I','I','O','O','U','U','N', 
		'-','&','!','?',' ',
		
		'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w',
		'x','y','z','*','~','#',' ',

		'a','a','a','a','e','e','e','e','i','i','o','o','u','u','n','c','B','H','S','N',

		'0','1','2','3','4','5','6','7','8','9','/',':','~','"','@'
	}; //There is no way I'm dealing with UTF-8 yet.
	//Somewhere along the way it ends up not printing anything past the second accented uppercase O

	fseek(file, 0L, SEEK_SET);

	for (i=0;i<28;i+=1) {
		fseek(file,offset+i,SEEK_SET);
		fread(buffer,1,1,file);
		
		if ((85 > (int)buffer[0]) || ((int)buffer[0] > 201)) {
			caption[i] = '\0'; //Broken caption alert!
			break; //Stop right there, criminal scum
			//5 years for what you've done, the rest because you hex-edited
		}

		caption[i] = lookup[(int)(buffer[0]-86)]; //GBC encodes 'A' as 0x56, or 86, and so forth
	}

	caption[28] = '\0';

	return caption;
}