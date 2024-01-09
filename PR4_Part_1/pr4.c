#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include "uthash.h"


struct ngram
{
    char gram[11];
    int counter; 
    UT_hash_handle hh;
};

struct ngram *table = NULL;

//Comparison function for HASH_SORT

int sort_count(struct ngram *ng1, struct ngram *ng2)
{
    if (ng1->counter<ng2->counter) return 1;
    if (ng1->counter==ng2->counter) return strcmp(ng1->gram, ng2->gram);
        else return -1;
}


// frees the hash table
int free_table()
{   
    struct ngram *p;
    
    for(p = table; p != NULL; p = p->hh.next)
    {
        free(p);
    }

    return 1;
} 


int ngram_func(int argc, char **argv)
{
    char *filename = argv[1];  //pointer containing filename
    int xnum = atoi(argv[2]);  //int containing ngram number 
    char currentline[4096];    //allocate space for line ##would need to use safer getline() if public software##
    int linelength;            //stores length oc currentline
    char *result;              //pointer to fgets return
    char *curngram = malloc(xnum+1);
    struct ngram *ng_ptr, *find_ptr;
    struct ngram temps;  //temp struct as malloc only needed if new hash needs to be added
    
    //opens file for reading and handles error
    FILE *infile;  
    //FILE *outfile;
    infile = fopen(filename, "r");
    if (infile == NULL)
    {
        printf("Unable to open file %s.\n", filename);
        exit(1);
    }
    

    

    //loop to analyze ngrams by line 
    while (1)
    {
        result = fgets(currentline, (sizeof(currentline)+1), infile); //breaks at EOF
        if (result == NULL)
        {
            break;
        } 
        
        
        linelength = strlen(currentline) - 1;
        
        int i;
        
        
        for (i=0; i <= linelength-xnum; i++)
        {
            
            strncpy(temps.gram, currentline+i, xnum);
            temps.gram[xnum] = '\0';
            find_ptr = NULL;

            
            HASH_FIND_STR(table, temps.gram, find_ptr);
            if (find_ptr != NULL)
            {
                find_ptr->counter += 1; //increment counter if "collision" would occur
            }
            else 
            {
                ng_ptr = (struct ngram *)malloc(sizeof (struct ngram));
                ng_ptr->counter = 1; //init counter to 0
                strncpy(ng_ptr->gram, currentline+i, xnum);
                ng_ptr->gram[xnum] = '\0';
                HASH_ADD_STR(table, gram, ng_ptr); //add new ngram to hash table
                
                
                //verify add worked, not needed for performance but kept for potential debug
                /*
                HASH_FIND_STR(table, ng_ptr->gram, find_ptr);
                if (find_ptr == NULL) 
                {
                    printf("add failed ngram=%s", ng_ptr->gram);
                }
                */
                   
            }
            
        }
        
    } 
    

    
    HASH_SORT(table, sort_count);
    
    struct ngram *p;
    int pcount = 0;
    for(p = table; p != NULL; p = p->hh.next)
    {
        if (pcount<10)
        {
            printf("%s: %d\n", p->gram, p->counter);
            pcount++;
        }
        
    }
    
    
    
    fclose(infile); //close input file
    free(curngram);
    return 1;
}






int main(int argc, char **argv)
{

    if (argc != 3)
    {
        printf("usage: ./pr4 [file] [n]\n");
        exit(1);
    }

    ngram_func(argc, argv);
    free_table();

    return 0;

}
