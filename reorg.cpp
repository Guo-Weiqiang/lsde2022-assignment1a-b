#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <map>
#include <vector>

#include "utils.h"

using namespace std;

unsigned long person_num = 0;

// the database
Person *person_map;
unsigned int *knows_map;
unsigned short *interest_map;

unsigned long *person_id_col;
unsigned short *birthday_col;
unsigned short *location_col;
unsigned long *knows_first_col; 
unsigned short *knows_n_col;
unsigned long *interests_first_col;
unsigned short *interest_n_col;



int birthday_comparator(const void *v1, const void *v2) {
    Person_Simp *r1 = (Person_Simp *) v1;
    Person_Simp *r2 = (Person_Simp *) v2;
    if (r1->birthday > r2->birthday)
        return 1;
    else if (r1->birthday < r2->birthday)
        return -1;
	else if (r1->person_id < r2->person_id)
        return -1;
    else if (r1->person_id > r2->person_id)
        return +1;
	return 0;
}

 



int main(int argc, char *argv[]) {
	unsigned long file_length;

	unsigned long interest_bytesize;
	unsigned long knows_bytesize;
	interest_map = (unsigned short *) mmapr(makepath(argv[1], "interest", "bin"), &interest_bytesize);
	knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_bytesize);
	person_map   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &file_length);
	person_num   = file_length / sizeof(Person);

	/* pass1:
	/* 1. only keep local friends
	/* 2. remove persons with 0 friends, and update person.bin, interest.bin */

	printf("starting to reduce the problem!\n");

	FILE* fd_knows2 = fopen(makepath(argv[1], "knows2", "bin"), (char*) "w");

	unsigned int *knows_n_array = (unsigned int *) malloc(person_num * sizeof(unsigned int));
	for(unsigned long i=0; i < person_num; i++) {
    	unsigned short knows_n = person_map[i].knows_n;
		
		for(long j = 0; j < person_map[i].knows_n; j++) {
			unsigned long f = knows_map[person_map[i].knows_first + j]; // person in my knows-list
			if (person_map[f].location != person_map[i].location) {
				knows_n -= 1;
				continue;
			}

			fwrite(&f, sizeof(int), 1, fd_knows2); 
		}

		knows_n_array[i] = knows_n;
	}

	fclose(fd_knows2);

	printf("person number is %ld\n", person_num);
	// update knows_n, knows_first in person_map;  remove persons with zero friends left-over;
	unsigned long knows2_bytesize;
	unsigned int *knows2_map = (unsigned int *) mmapr(makepath(argv[1], "knows2","bin"), &knows2_bytesize);

	
	unsigned long *k2k = (unsigned long *) malloc(person_num * sizeof(long));

	unsigned long remove_count = 0;
	for (unsigned long i = 0; i < person_num; i++) {
		
		if (knows_n_array[i] == 0) {
			remove_count += 1;
			k2k[i] = 0;  // note heer
		}
		else {
			k2k[i] = i - remove_count;
		}
	}
	printf("remove count is %ld\n", remove_count);



	FILE* fd_knows3 = fopen(makepath(argv[1], "knows3", "bin"), (char*) "w");
	unsigned long knows_num = knows2_bytesize / sizeof(unsigned int);

	for (unsigned long i = 0; i < knows_num; i++) {

		fwrite(&k2k[knows2_map[i]], sizeof(unsigned int), 1, fd_knows3); 
	}
	fclose(fd_knows3);
	munmap(knows2_map, knows2_bytesize);


	FILE* fd_person2 = fopen(makepath(argv[1], "person2", "bin"), (char*) "w");
	FILE* fd_interest2 = fopen(makepath(argv[1], "interest2", "bin"), (char*) "w");


	unsigned long count = 0;
	unsigned long interest_shift = 0;
	unsigned int local_index = 0;
	for (unsigned int i = 0; i < person_num; i++) {
		if (knows_n_array[i] == 0) {
			interest_shift += person_map[i].interest_n;
			continue;
		}
		Person_Simp * person_tmp = (Person_Simp *) malloc(sizeof(Person_Simp));
		person_tmp->person_id = person_map[i].person_id;
		person_tmp->birthday = person_map[i].birthday;
		person_tmp->knows_first = count;
		person_tmp->knows_n = knows_n_array[i];
		
		count += knows_n_array[i];

		unsigned long tmp = person_map[i].interests_first - interest_shift;
		person_tmp->interests_first = tmp;
		person_tmp->interest_n = person_map[i].interest_n;

		person_tmp->index = local_index;
		local_index++;

		fwrite(person_tmp, sizeof(Person_Simp), 1, fd_person2); // person tmp is a point, so we dont need to use &person_tmp!!!!


		unsigned long j = person_map[i].interests_first;
        unsigned long range = person_map[i].interests_first + person_map[i].interest_n;

        for (j; j < range; j++) {
			unsigned short interest = interest_map[j];
			fwrite(&interest, sizeof(unsigned short), 1, fd_interest2);
        }

	}

	munmap(interest_map, interest_bytesize);
	munmap(knows_map, knows_bytesize);
	free(knows_n_array);
	fclose(fd_person2);
	fclose(fd_interest2);

	printf("Reduce the problem successfully!\n");
	

	unsigned long person2_bytesize;
	Person_Simp *person2_map   = (Person_Simp *) mmapw(makepath(argv[1], "person2", "bin"), &person2_bytesize);
	person_num = person2_bytesize / sizeof(Person_Simp);

	/* pass2:
	/* partition birthday, but cannot fix the bug, alas:( */

	// qsort(person2_map, person_num, sizeof(Person_Simp), &birthday_comparator);

	// unsigned long knows3_bytesize;
	// unsigned int *knows3_map = (unsigned int *) mmapr(makepath(argv[1], "knows3","bin"), &knows3_bytesize);
	// knows_num = knows3_bytesize / sizeof(unsigned int);
	

	// FILE* fd_knows4 = fopen(makepath(argv[1], "knows4", "bin"), (char*) "w");

	// unsigned int *index_map = (unsigned int *) malloc(person_num * sizeof(unsigned int));

	// for (unsigned long i = 0; i < person_num; i++) {
	// 	// unsigned int tmp = person2_map[i].index;
	// 	unsigned int tmp = p_array[i].index;
	// 	index_map[tmp] = i ;
	// }

	// for (unsigned long i = 0; i < knows_num; i++) {
	// 	fwrite(&index_map[knows3_map[i]], sizeof(unsigned int), 1, fd_knows4);
	// }

	// fclose(fd_knows4);

	/* pass3:
	/* output column storaged files */

	printf("starting to store data in columnar storage format!\n");

	FILE* fd_person_id_col = fopen(makepath(argv[1], "person_id_col", "bin"), (char*) "w");
	FILE* fd_birthday_col = fopen(makepath(argv[1], "birthday_col", "bin"), (char*) "w");
	FILE* fd_knows_first_col = fopen(makepath(argv[1], "knows_first_col", "bin"), (char*) "w");
	FILE* fd_knows_n_col = fopen(makepath(argv[1], "knows_n_col", "bin"), (char*) "w");
	FILE* fd_interests_first_col = fopen(makepath(argv[1], "interests_first_col", "bin"), (char*) "w");
	FILE* fd_interest_n_col = fopen(makepath(argv[1], "interest_n_col", "bin"), (char*) "w");

	for (unsigned int person_offset = 0; person_offset < person_num; person_offset++) {
		fwrite(&person2_map[person_offset].person_id, sizeof(unsigned long), 1, fd_person_id_col);		
		fwrite(&person2_map[person_offset].birthday, sizeof(unsigned short), 1, fd_birthday_col);
		fwrite(&person2_map[person_offset].knows_first, sizeof(unsigned long), 1, fd_knows_first_col);
		fwrite(&person2_map[person_offset].knows_n, sizeof(unsigned short), 1, fd_knows_n_col);
		fwrite(&person2_map[person_offset].interests_first, sizeof(unsigned long), 1, fd_interests_first_col);		
		fwrite(&person2_map[person_offset].interest_n, sizeof(unsigned short), 1, fd_interest_n_col);
	}



	fclose(fd_person_id_col);
	fclose(fd_birthday_col);
	fclose(fd_knows_first_col);
	fclose(fd_knows_n_col);
	fclose(fd_interests_first_col);
	fclose(fd_interest_n_col);
	
	printf("Output file successfully!\n");

	/* pass 4: 
	/* remove non-mutual local friends */

	printf("starting to remove non-mutual friend!\n");

	unsigned long knows4_bytesize;
	unsigned int *knows4_map = (unsigned int *) mmapr(makepath(argv[1], "knows3","bin"), &knows4_bytesize);
	knows_num = knows4_bytesize / sizeof(unsigned int);
	printf("knows4 num is %ld\n", knows4_bytesize / sizeof(unsigned int));
	unsigned long *knows_first_col_map = (unsigned long *) mmapr(makepath(argv[1], "knows_first_col","bin"), &knows4_bytesize); 
	unsigned short *knows_n_col_map = (unsigned short *) mmapr(makepath(argv[1], "knows_n_col","bin"), &knows4_bytesize);
	person_num = knows4_bytesize / sizeof(unsigned short);

	FILE* fd_knows5 = fopen(makepath(argv[1], "knows5", "bin"), (char*) "w");
	

	unsigned int *knows5_n_array = (unsigned int *) malloc(person_num * sizeof(unsigned int));
	for(unsigned long i=0; i < person_num; i++) {
    	unsigned short knows_n = knows_n_col_map[i];
		
		for(long j = 0; j < knows_n_col_map[i]; j++) {
			unsigned long f = knows4_map[knows_first_col_map[i] + j]; // person in my knows-list
			int flag = 0;
			for (long k = 0; k < knows_n_col_map[f]; k++) {
				unsigned long f_friend = knows4_map[knows_first_col_map[f] + k];
				if (f_friend == i) {
					flag = 1;
					fwrite(&f, sizeof(int), 1, fd_knows5); 
					break;
				}
			}
			if (flag == 0) knows_n -= 1;	
		}

		knows5_n_array[i] = knows_n;
	}
	
	fd_knows_first_col = fopen(makepath(argv[1], "knows_first_col", "bin"), (char*) "w");
	fd_knows_n_col = fopen(makepath(argv[1], "knows_n_col", "bin"), (char*) "w");
	count = 0;
	for (unsigned int i = 0; i < person_num; i++) {
		fwrite(&count, sizeof(unsigned long), 1, fd_knows_first_col);
		fwrite(&knows5_n_array[i], sizeof(unsigned short), 1, fd_knows_n_col);
		count += knows5_n_array[i];
	}

	fclose(fd_knows_first_col);
	fclose(fd_knows_n_col);
	fclose(fd_knows5);

	printf("Remove non-mutual friend successfully!\n");


	/* pass 5:
	/* build inverted index */
	printf("starting to build inverted index!\n");
	unsigned long interest2_bytesize;
	unsigned short *interest2_map = (unsigned short *) mmapr(makepath(argv[1], "interest2", "bin"), &interest2_bytesize);
	unsigned long interest_num = interest2_bytesize / sizeof(unsigned short);


	map<unsigned short, vector<unsigned int>> Dict;
	for (unsigned int i = 0; i < person_num; i++) {
		unsigned long start = person2_map[i].interests_first;
		unsigned short end  = person2_map[i].interest_n;

		for (unsigned long j = start; j < start + end; j++) {
			Dict[interest2_map[j]].push_back(i);
		} 
	}


	FILE *fd_postings = fopen(makepath(argv[1], "postings", "bin"), (char*) "w");
	FILE *fd_dict = fopen(makepath(argv[1], "dict", "bin"), (char*) "w");

	unsigned long ap_off_count = 0;
	unsigned int ap_num_count = 0;
	map<unsigned short, vector<unsigned int>>::iterator iter1;
	for (iter1 = Dict.begin();iter1 != Dict.end();iter1++) {
		unsigned short key_val = iter1->first;
		vector<unsigned int> val = iter1->second;

		ap_num_count = val.size();
		for (int i = 0; i < ap_num_count; i++) {
			fwrite(&val[i], sizeof(unsigned int), 1, fd_postings);
		}

		dictionary *dict = (dictionary *) malloc(sizeof(dictionary));
		dict->art = key_val;
		dict->ap_offset = ap_off_count;
		dict->ap_num = ap_num_count;
		fwrite(dict, sizeof(dictionary), 1, fd_dict);

		ap_off_count += ap_num_count;
		free(dict);
	}

	printf("Build inverted index successfully!\n");

	return 0;
}

