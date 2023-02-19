#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>


#include "utils.h" // Person defined here, and the mmap calls to load files into virtual memory



// the database
unsigned int *knows_map;
bool * bitmap;

unsigned long *person_id_col;
unsigned short *birthday_col;
unsigned long *knows_first_col; 
unsigned short *knows_n_col;
unsigned int *birthday_tags;

dictionary *dict_map; 
unsigned int *posting; 

unsigned long person_num = 0;
unsigned long dict_num = 0;
unsigned long posting_num = 0;

FILE *outfile;

typedef struct {
    unsigned long person1_id;
    unsigned long person2_id;
    signed char score;
} Result;

int result_comparator(const void *v1, const void *v2) {
    Result *r1 = (Result *) v1;
    Result *r2 = (Result *) v2;
    if (r1->score > r2->score)
        return -1;
    else if (r1->score < r2->score)
        return +1;
    else if (r1->person1_id < r2->person1_id)
        return -1;
    else if (r1->person1_id > r2->person1_id)
        return +1;
     else if (r1->person2_id < r2->person2_id)
        return -1;
    else if (r1->person2_id > r2->person2_id)
        return +1;
    else
        return 0;
}

// sort index_score array 
int index_comparator(const void *v1, const void *v2) {
    index_score *r1 = (index_score *) v1;
    index_score *r2 = (index_score *) v2;

	// ascending
    if (r1->person_index > r2->person_index)
        return 1;
    else if (r1->person_index < r2->person_index)
        return -1;
    else
        return 0;
}

// int index_comparator(const void *v1, const void *v2) {
//     index_score *r1 = (index_score *) v1;
//     index_score *r2 = (index_score *) v2;
//     if (birthday_col[r1->person_index] > birthday_col[r2->person_index])
//         return 1;
//     else if (birthday_col[r1->person_index] < birthday_col[r2->person_index])
//         return -1;
// 	else if (r1->person_index > r2->person_index)
//         return 1;
//     else if (r1->person_index < r2->person_index)
//         return -1;
//     else
//         return 0;
// }

#define ARTIST_FAN -1



// perform union_sum operation to get person candidates and scores
index_score *union_sum(unsigned short artists[], unsigned short bdstart, unsigned short bdend){
	unsigned long length = 0;
	
	index_score * candidates = (index_score *) malloc(sizeof(index_score));

	// load person who is interested in artists A2 or A3 or A4
	int artist_count = 0;
	unsigned int count = 0;
	for (unsigned int i = 0; i < dict_num; i++) {
		if (dict_map[i].art == artists[0] || dict_map[i].art == artists[1] || dict_map[i].art == artists[2]) {
			length += dict_map[i].ap_num;
			candidates = (index_score *) realloc(candidates, length * sizeof(index_score));

			artist_count++;
			for (unsigned int j = dict_map[i].ap_offset; j < dict_map[i].ap_offset + dict_map[i].ap_num; j++) {
				candidates[count].person_index = posting[j];
				count++;
			}

		}
		if (artist_count == 3) break; // early out
	}

	posting_num	= length;  // record the number of qualified candidates

	// sort tmp according to person index
	qsort(candidates, length, sizeof(index_score), &index_comparator);

	// compute score of every person
	candidates[0].score = 0;
	for (unsigned int i = 1; i < length; i++) {
		if (candidates[i].person_index == candidates[i - 1].person_index) {
			candidates[i].score = 2;
			candidates[i - 1].score = 2;
			if (i > 1 && candidates[i].person_index == candidates[i - 2].person_index) {
				candidates[i].score = 3;
				candidates[i - 1].score = 3;
				candidates[i - 2].score = 3;
			}
		} 
		else candidates[i].score = 1;
	}
	return candidates;
}

void create_bitmap(unsigned short artist) {
	bitmap = (bool *) malloc(person_num * sizeof(bool));  // bitmap is a global variable, see the top
	for (unsigned int i = 0; i < person_num; i++) {
		bitmap[i] = false;
	}
	for (unsigned int i = 0; i < dict_num; i++) {
		if (dict_map[i].art == artist) {
			for (unsigned int j = dict_map[i].ap_offset; j < dict_map[i].ap_offset + dict_map[i].ap_num; j++) {
				bitmap[posting[j]] = true;
			}
			break;
		}
	}
}

void query(unsigned short qid, unsigned short artist, unsigned short artists[], unsigned short bdstart, unsigned short bdend) {
	unsigned int result_length = 0, result_maxsize = 15000;
	Result* results = (Result*) malloc(result_maxsize * sizeof(Result));

	printf("Running query %d\n", qid);

	index_score *candidates = union_sum(artists, bdstart, bdend);

	// create bitmap
	create_bitmap(artist);


	// query
	for (unsigned int i = 0; i < posting_num; i++) {
		unsigned int person1 = candidates[i].person_index;
		if (i > 1 && person1 == candidates[i - 1].person_index) continue;  // avoid computing duplicate results 
		
		if (bitmap[person1]) continue; // make sure person do not like A1

		if (birthday_col[person1] < bdstart || birthday_col[person1] > bdend) continue; 

		signed char score = candidates[i].score;
		unsigned long range = knows_first_col[person1] + knows_n_col[person1];
		for (unsigned long knows1 = knows_first_col[person1]; knows1 < range; knows1++) {
			unsigned int person2 = knows_map[knows1];

			if (bitmap[person2] != true) continue; // make sure person1's friend person2 likes A1
			
			results[result_length].person1_id = person1; 
			results[result_length].person2_id = person2; 
			results[result_length].score = score;
		
			if (++result_length >= result_maxsize) { // realloc result array if we run out of space
				results = (Result*) realloc(results, (result_maxsize*=2) * sizeof(Result));
			}
		}
	}


	// sort the results
	qsort(results, result_length, sizeof(Result), &result_comparator);

	// output the results to outfile
	for (unsigned int i = 0; i < result_length; i++) {
		unsigned int person1 = results[i].person1_id;
		unsigned int person2 = results[i].person2_id;
		
		fprintf(outfile, "%d|%d|%lu|%lu\n", qid, results[i].score, person_id_col[person1], person_id_col[person2]);
	}
		
	free(results);
	free(bitmap);
}


#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

void query_line_handler(unsigned char nfields, char** tokens) {
	unsigned short q_id, q_artist, q_bdaystart, q_bdayend;
	unsigned short q_artists[3];

	q_id         = atoi(tokens[QUERY_FIELD_QID]);
	q_artist     = atoi(tokens[QUERY_FIELD_A1]);
	q_artists[0] = atoi(tokens[QUERY_FIELD_A2]);
	q_artists[1] = atoi(tokens[QUERY_FIELD_A3]);
	q_artists[2] = atoi(tokens[QUERY_FIELD_A4]);
	q_bdaystart  = birthday_to_short(tokens[QUERY_FIELD_BS]);
	q_bdayend    = birthday_to_short(tokens[QUERY_FIELD_BE]);
	
	query(q_id, q_artist, q_artists, q_bdaystart, q_bdayend);
}

int main(int argc, char *argv[]) {
	unsigned long file_length;
	if (argc < 4) {
		fprintf(stderr, "Usage: [datadir] [query file] [results file]\n");
		exit(1);
	}

	// memory-map files created by loader 
	person_id_col = (unsigned long *) mmapr(makepath(argv[1], "person_id_col", "bin"), &file_length);
	birthday_col    = (unsigned short *)   mmapr(makepath(argv[1], "birthday_col",    "bin"), &file_length);
	knows_first_col   = (unsigned long *)         mmapr(makepath(argv[1], "knows_first_col",   "bin"), &file_length);
	knows_n_col = (unsigned short *) mmapr(makepath(argv[1], "knows_n_col", "bin"), &file_length);

	person_num   = file_length / sizeof(unsigned short);

	knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows5", "bin"), &file_length);
	
	unsigned long dict_bytesize;
	dict_map = (dictionary *) mmapr(makepath(argv[1], "dict", "bin"), &dict_bytesize);
	dict_num = dict_bytesize / sizeof(dictionary);

	posting = (unsigned int *) mmapr(makepath(argv[1], "postings", "bin"), &dict_bytesize);


	// unsigned long birthday_bytesize;
	// birthday_tags = (unsigned int *) mmapr(makepath(argv[1], "tags", "bin"), &birthday_bytesize);


  	outfile = fopen(argv[3], "w");  
  	if (outfile == NULL) {
  		fprintf(stderr, "Can't write to output file at %s\n", argv[3]);
		exit(-1);
  	}


  	// run through queries 
	parse_csv(argv[2], &query_line_handler);


	fclose(outfile);

	return 0;
}
