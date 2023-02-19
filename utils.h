#define REPORTING_N 1000000
#define LINEBUFLEN 1024

// typedef unsigned long byteoffset;
typedef unsigned int  entrycount;


typedef struct {
    unsigned long  person_id;
    unsigned short birthday;
    unsigned short location;
    unsigned long  knows_first;
    unsigned short knows_n;
    unsigned long  interests_first;
    unsigned short interest_n;
} Person;

typedef struct {
    unsigned long  person_id;
    unsigned short birthday;
    unsigned long  knows_first;
    unsigned short knows_n;
    unsigned long  interests_first;
    unsigned short interest_n;
    unsigned int index;
} Person_Simp;

typedef struct {
	unsigned short art;  // artists
	unsigned long ap_offset;  // artist starting offset in posting table
	unsigned int ap_num;  // the number of person who like artist
} dictionary;

typedef struct {
    unsigned long person_index;  // person_index in personX.bin
    signed char score;  
} index_score;

unsigned int birthday2days(unsigned short birthday) {
	unsigned int month = birthday / 100;
	unsigned int day = birthday % 100;
	// printf("month is %d, day is %d\n", month, day);

	unsigned int days[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	unsigned int sum = 0;

	for (unsigned char i = 0; i < month - 1; i++) {
		sum += days[i];
	}
	sum += day;
	return sum;

}

void parse_csv(char* fname, void (*line_handler)(unsigned char nfields, char** fieldvals)) {
    long nlines = 0;

    FILE* stream = fopen(fname, "r");
    if (stream == NULL) {
        fprintf(stderr, "Can't read file at %s\n", fname);
        exit(-1);
    }
    char line[LINEBUFLEN];
    char* tokens[10];
    unsigned int col, idx;
    tokens[0] = line;

    while (fgets(line, LINEBUFLEN, stream)) {
        col = 0;
        // parse the csv line into array of strings
        for (idx=0; idx<LINEBUFLEN; idx++) { 
            if (line[idx] == '|' || line[idx] == '\n') {
                line[idx] = '\0';
                col++;
                tokens[col] = &line[idx+1];
            } // lookahead to find end of line
            if (line[idx+1] == '\0') {
                break;
            }
        }
        (*line_handler)(col, tokens);
        nlines++;
        if (nlines % REPORTING_N == 0) {
            printf("%s: read %lu lines\n", fname, nlines);
        }
    }
    fclose(stream);
}


FILE* open_binout(char* filename) {
    FILE* outfile;
    outfile = fopen(filename, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "Could not open %s for writing\n", filename);
        exit(-1);
    }
    return outfile;
}

/* 
    convert birthday from date to four-digit integer with month and day 
    easier comparisions and less storage space
*/
unsigned short birthday_to_short(char* date) {
    unsigned short bdaysht;
    char dmbuf[3];
    dmbuf[2] = '\0';
    dmbuf[0] = *(date + 5);
    dmbuf[1] = *(date + 6);
    bdaysht = atoi(dmbuf) * 100;
    dmbuf[0] = *(date + 8);
    dmbuf[1] = *(date + 9);
    bdaysht += atoi(dmbuf);
    return bdaysht;
}

void* mmapr(char* filename, unsigned long *filelen) {
    int fd;
    struct stat sbuf;
    void *mapaddr;

    if ((fd = open(filename, O_RDONLY)) == -1) {
        fprintf(stderr, "failed to open %s\n", filename);
        exit(1);
    }

    if (stat(filename, &sbuf) == -1) {
        fprintf(stderr, "failed to stat %s\n", filename);
        exit(1);
    }
    
    // mapaddr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    mapaddr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapaddr == MAP_FAILED) {
        fprintf(stderr, "failed to mmap %s\n", filename);
        exit(1);
    }
    *filelen = sbuf.st_size;


    return mapaddr;
}

// writable mmap
void* mmapw(char* filename, unsigned long *filelen) {
    int fd;
    struct stat sbuf;
    void *mapaddr;

    if ((fd = open(filename, O_RDONLY)) == -1) {
        fprintf(stderr, "failed to open %s\n", filename);
        exit(1);
    }

    if (stat(filename, &sbuf) == -1) {
        fprintf(stderr, "failed to stat %s\n", filename);
        exit(1);
    }
    
    // mapaddr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    mapaddr = mmap(0, sbuf.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mapaddr == MAP_FAILED) {
        fprintf(stderr, "failed to mmap %s\n", filename);
        exit(1);
    }
    *filelen = sbuf.st_size;


    return mapaddr;
}

char* makepath(char* dir, const char* file, const char* ext) {
    char* out = (char*) malloc(1024), *sep = (char*) "";
        if (strlen(dir) && dir[strlen(dir)-1] != '/') sep = (char*) "/";
    sprintf(out, "%s%s%s.%s", dir, sep, file, ext);
    return out;
}
