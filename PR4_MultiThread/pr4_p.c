#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include "uthash.h"
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pthread.h>
#include<sys/types.h>
#include<unistd.h>


//Global Variables

int NTHREADS;
char *FILEARRAY;
long long SIZE;
char *FILENAME;
int NGRAM_LEN;
char *MAPPED_FILE;
pthread_mutex_t MUTEX;


struct file_chunk
{
    long start;
    long end;
};
struct file_chunk file_chunks[512];


struct ngram
{
    char gram[12];
    int counter; 
    UT_hash_handle hh;
};

struct ngram *gl_table = NULL;  //Global hash table



//Comparison function for HASH_SORT

int sort_count(struct ngram *ng1, struct ngram *ng2)
{
    if (ng1->counter<ng2->counter) return 1;
    if (ng1->counter==ng2->counter) return strcmp(ng1->gram, ng2->gram);
        else return -1;
}


// frees the hash table

void free_table1(struct ngram *table)
{
    struct ngram *p, *tmp;
    HASH_ITER(hh, table, p, tmp)
    {
        HASH_DEL(table, p);
        free(p);
    }
    
}



void *ngram_p(void *param)
{  
    struct file_chunk *file_chunks;
    file_chunks = param;                //init parameters
    long start = file_chunks->start;    //pull start/end
    long end = file_chunks->end;
    int i, chunk_len;
    int ngram_pos;
    char curr_ngram[NGRAM_LEN+1];
    struct ngram *ngram_ptr;
    struct ngram *find_ptr;
    struct ngram *local_table = NULL;
    struct ngram *ngram_ptr2;
    struct ngram *find_ptr2;


    //Finding the ngrams
    chunk_len = end - start;
    if (chunk_len > 1)
    {
        
        
        chunk_len = chunk_len - (NGRAM_LEN-1);
        for (i=0;i<chunk_len;i++)
        {
            //must find next ngram
            ngram_pos = 0;
            while(ngram_pos<NGRAM_LEN)
            {
                curr_ngram[ngram_pos] = MAPPED_FILE[start + i + ngram_pos];
                if (curr_ngram[ngram_pos] == '\n')
                {
                    ngram_pos = 0;
                    break;
                }
                ngram_pos++;
            }
            
            if (ngram_pos != NGRAM_LEN)
            {
                continue;
            }
            curr_ngram[ngram_pos+1] = '\0';
            
            //Check for ngram in hash gl_table
            
            HASH_FIND_STR(local_table, curr_ngram, find_ptr);
            
            if (find_ptr == NULL) //If null, needs to be added
            {
                ngram_ptr = (struct ngram *)malloc(sizeof(struct ngram));
                ngram_ptr->counter = 1;
                strcpy (ngram_ptr->gram, curr_ngram);
                
                HASH_ADD_STR(local_table, gram, ngram_ptr);
                 
            }
            else //NGRAM already discovered, so increment
            {
                find_ptr->counter += 1;
            }

            
        
        }
        
    }

    
    HASH_SORT(local_table, sort_count);   //Sort the hash function
   
    

    //Add data to gl_hash
    struct ngram *p;
    pthread_mutex_lock(&MUTEX);
    for(p = local_table; p != NULL; p = p->hh.next)
    {
        HASH_FIND_STR(gl_table, p->gram, find_ptr2);
            
        if (find_ptr2 == NULL) //If null, needs to be added
        {
            ngram_ptr2 = (struct ngram *)malloc(sizeof(struct ngram));
            ngram_ptr2->counter = p->counter;
            strcpy (ngram_ptr2->gram, p->gram);
            
            HASH_ADD_STR(gl_table, gram, ngram_ptr2);
            
                
        }
        else //NGRAM already discovered, so add local counter to global
        {
            find_ptr2->counter += p->counter;
        }
    }

    pthread_mutex_unlock(&MUTEX);
    free_table1(local_table);
    
    return NULL;

}





int main(int argc, char **argv)
{
    
    int i, x, mutest;
    pthread_t *thread_array; //pointer to array thread
    FILENAME = argv[1];
    long app_chunk_size;
    long start_offset, end_offset;
    char *file_data;
    
    
    if (argc != 4)
    {
        printf("usage: ./pr4_p [file] [n] [t]\n");
        exit(1);
    }
    
    NTHREADS = strtol(argv[3], NULL, 10);
    if (NTHREADS < 1)
    {
        printf("Please enter a positive integer for the number of threads you want.\n");
        exit(1);
    }
    else if (NTHREADS > 512)
    {
        printf("512 is the max number of threads allowed.\n");
        exit(1);
    }
    
    NGRAM_LEN = atoi(argv[2]);

    struct stat fs;
    int fd = open(FILENAME, O_RDONLY); //open file for reading 
    fstat(fd, &fs); 
    SIZE = fs.st_size;
    


    //Chunk up file before creating threads
    
    MAPPED_FILE = mmap(0, SIZE, PROT_READ, MAP_PRIVATE, fd, 0);     //Map file
    if (MAPPED_FILE == NULL)                                        //handle error                     
    {
        printf("ERROR: MMAP failed.");
        exit(1);
    }
    close(fd);
    app_chunk_size = SIZE / NTHREADS;                               //Gets approximate chunk size
    
    //Find where chunk start/stops (looking for a \n)
    int init = 1;
    for (x=0; x<NTHREADS; x++)
    {
        //Creating first chunk from start of file
        if (init)
        {
            start_offset = 0;
            end_offset = app_chunk_size -1;
            init = 0;
        }
        //chunks follow after the previous chunk
        else
        {
            start_offset = end_offset + 1;
            end_offset = start_offset + app_chunk_size - 1;
        }

        //Chunks need to end on a \n 

        //check if chunk is at eof

        if (end_offset >= SIZE)
        {
            end_offset = SIZE - 1;
        }

        file_data = MAPPED_FILE + end_offset;

        //traverse to next \n
        for (i=0; i<512 ;i++)
        {
            if (file_data[i] == '\n')
            {
                end_offset += i;
                break;
            }
        }

        file_chunks[x].start = start_offset;
        file_chunks[x].end = end_offset;
        
    }


    //Create threads + mutex

   
   thread_array = malloc(NTHREADS * sizeof(pthread_t));
   if (thread_array == NULL)
   {
        printf("ERROR: malloc failed.");
        exit(1);
   } 
   

   mutest = pthread_mutex_init(&MUTEX, NULL);
   if (mutest != 0)
   {
        printf("ERROR: Mutex creation failed");
        exit(1);
   }
    
   for (i = 0; i < NTHREADS; i++)
   {
        pthread_create(&thread_array[i], NULL, ngram_p, &file_chunks[i]);
   }
    
   for (i = 0; i <NTHREADS; i++)
   {
        pthread_join(thread_array[i], NULL);
   }
   
   HASH_SORT(gl_table, sort_count);   //Sort the hash function
   
    
	int num_printed=0;
	struct ngram *p;
	for (p=gl_table; p != NULL; p = p->hh.next) {
		printf ("%s: %d\n", p->gram, p->counter);
		if (++num_printed >= 10)
			break;
	}

   
   mutest = pthread_mutex_destroy(&MUTEX);      
   if (mutest != 0)
   {
        printf("ERROR: Mutex destroy failed.");
        exit(1);
   }
   free(thread_array);
   
   if(munmap(MAPPED_FILE, SIZE) == -1)
   {
        printf("ERROR: MUNMAP failed.");
        exit(1);
   }
   
   free_table1(gl_table);


    return 0;


}