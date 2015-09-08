// Copyright (c) 2014-2015, Massachusetts Institute of Technology
//
// This file is part of the Compressed Continuous Computation (C3) toolbox
// Author: Alex A. Gorodetsky 
// Contact: goroda@mit.edu

// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice, 
//    this list of conditions and the following disclaimer in the documentation 
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its contributors 
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//Code

/** \file monitoring.c
 * Provides routines for monitoring functions and storing/recalling their evaluations
*/

#include <stdlib.h>
#include <string.h>

#include "stringmanip.h"
#include "monitoring.h"
#include "array.h"

/***********************************************************//**
    Initialize a function monitor of an n dimensional function

    \param f [in] - function to wrap
    \param args [in] - function arguments
    \param dim [in] - dimension of function
    \param tsize [in] - initial table size for storing evaluations

    \return fm [out] - function monitor
***************************************************************/
struct FunctionMonitor *
function_monitor_initnd( double (*f)(double *, void *), void * args, size_t dim,
                        size_t tsize)
{
    struct FunctionMonitor * fm = NULL;
    if (NULL == ( fm = malloc(sizeof(struct FunctionMonitor)))){
        fprintf(stderr, "failed to allocate function monitor\n");
        exit(1);
    }
    fm->ftype = 0;
    fm->dim = dim;
    fm->args = args;
    fm->f.fnd = f;
    fm->evals = create_hashtable_cp(tsize);
    return fm;
}

/***********************************************************//**
    Free memory allocated to function monitor

    \param fm [inout] - function monitor
***************************************************************/
void function_monitor_free( struct FunctionMonitor * fm){
    
    free_hashtable_cp(fm->evals); 
    fm->evals = NULL;
    free(fm); fm = NULL;
}

/***********************************************************//**
    Evaluate a function using the function monitor to recall/store evaluations

    \param x [in] - location at which to evaluate
    \param args [in] - void pointer to function monitor

    \return val - evaluation of a function
***************************************************************/
double function_monitor_eval(double * x, void * args)
{
    //printf("In eval function monitor \n");   
    struct FunctionMonitor * fm = args;
    
    //printf("serial;ize it \n");
    //printf("x = ");
    //printf("fm->dim=%zu\n",fm->dim);
    //dprint(fm->dim,x);
    //printf("ok!\n");
    char * ser = serialize_darray_to_text(fm->dim,x);

    char * sval = lookup_key(fm->evals,ser);
    //printf("sval = %s\n",sval);
    double val;
    if (sval != NULL){
        val = deserialize_double_from_text(sval);
        //memmove(&val, sval, sizeof(double));
        //printf("here! val=%3.5f\n",val);
    }
    else{
        val = fm->f.fnd(x,fm->args);
        //printf("val=%G\n",val);
        sval = serialize_double_to_text(val);
        //printf("sval = %s\n",sval);
        struct Cpair * cp = cpair_create(ser,sval);
        add_cpair(fm->evals, cp);
        cpair_free(cp);
    }
    //printf("done with eval function monitor \n");
    free(ser); ser = NULL;
    free(sval); sval = NULL;
    return val;
}       

/***********************************************************//**
    Print the function evaluations to a file

    \param fm [in] - function monitor to print
    \param fp [in] - file pointer

    \note 
        last column is evaluation
***************************************************************/
void function_monitor_print_to_file(struct FunctionMonitor * fm, FILE * fp)
{
    size_t ii, jj;
    char * serx;
    char * sery;
    double * x = NULL;
    size_t dim;
    double val;

    for (ii = 0; ii < fm->dim; ii++){
        fprintf(fp, "x%zu ",ii);
    }
    fprintf(fp, "f\n");

    //printf("printing\n");
    for (ii = 0; ii < fm->evals->size; ii++){
        //printf("ii=%zu/%zu\n",ii,fm->evals->size);
        struct PairList * cur = fm->evals->table[ii];
        while (cur != NULL){
            serx = cur->data->a;
            sery = cur->data->b;
            val = deserialize_double_from_text(sery);
            x = deserialize_darray_from_text(serx,&dim);
            for (jj = 0; jj < dim; jj++){
                fprintf(fp,"%3.15f ",x[jj]);
            }
            fprintf(fp,"%3.15f\n",val);
            free(x);
            x = NULL;
            cur = cur->next;
        }
    }
}


/////////////////////////////////////
void PushVal(struct storevals ** head, size_t N, double * xIn, double fIn)
{
    struct storevals * newNode = malloc(sizeof(struct storevals));
    newNode->dim = N;
    newNode->x = calloc_double(N);
    memmove(newNode->x, xIn, N * sizeof(double));
    newNode->f = fIn;
    newNode->next = *head;
    *head = newNode;
}

void PrintVals2d(FILE * f, struct storevals * head){

    struct storevals * current = head;
    fprintf(f, "x1 x2  f \n");
    while (current != NULL){
        fprintf(f,"%3.2f %3.2f %3.2f \n",
            current->x[0], current->x[1], current->f);
        current = current->next;
    }
}

void DeleteStoredVals(struct storevals ** head){
    // This is a bit inefficient and doesnt free the last node
    struct storevals * current = *head;
    struct storevals * next;
    while (current != NULL){
        next = current->next;
        free(current->x);
        free(current);
        current = next;
    }
    *head = NULL;
}

/***********************************************************//**
    Create a Cpair from two char * 's 

    \param a [in] - first char *
    \param b [in] - second char *

    \return pair - combined chars
***************************************************************/
struct Cpair * cpair_create(char * a, char * b)
{
    struct Cpair * pair = NULL;
    if (NULL == (pair = malloc(sizeof(struct Cpair)))){
        fprintf(stderr, "failed to allocate memory for cpair.\n");
        exit(1);
    }
    size_t N1 = strlen(a);
    size_t N2 = strlen(b);

    pair->a = malloc((N1+1)*sizeof(char));
    pair->b = malloc((N2+1)*sizeof(char));

    strncpy(pair->a,a,N1);
    strncpy(pair->b,b,N2);
    pair->a[N1] = '\0';
    pair->b[N2] = '\0';

    return pair;
}

/***********************************************************//**
    Free memory allocated to Cpair

    \param pair [inout] 
***************************************************************/
void cpair_free(struct Cpair * pair){
    free(pair->a); pair->a = NULL;
    free(pair->b); pair->b = NULL;
    free(pair); pair=NULL;
}

/***********************************************************//**
    Copy a Cpair

    \param pair [in] - Cpair to copy
    \return out - copied pair
***************************************************************/
struct Cpair * copy_cpair(struct Cpair * pair)
{
    struct Cpair * out = cpair_create(pair->a, pair->b);
    return out;
}

void print_cpair(struct Cpair * pair){
    printf("( %s , %s ) \n", pair->a, pair->b);
}

/***********************************************************//**
    Push a Cpair onto a list

    \param l [inout] - Cpair list
    \param data [in] - Cpair to push
***************************************************************/
void pair_push(struct PairList ** l, struct Cpair * data)
{
    struct PairList * newNode = malloc(sizeof(struct PairList));

    newNode->data = copy_cpair(data);
    newNode->next = *l;
    *l = newNode;
}

void print_pair_list(struct PairList * pl){
    
    struct PairList * pt = pl;
    while (pt != NULL){
        print_cpair(pt->data);
        pt = pt->next;
    }
}

/***********************************************************//**
    Check if two cpairs are equal

    \param a [in] - first cpair
    \param b [in] - second cpair

    \return out - 1 if equal 0 if not
***************************************************************/
int cpair_isequal(struct Cpair * a, struct Cpair * b){
    int out = 0;
    if ( strcmp(a->a,b->a) == 0){
        if (strcmp(a->b, b->b) == 0){
            out = 1;
        }
    }
    return out;
}

/***********************************************************//**
    Delete a pair list

    \param head - pointer to first element of the list
***************************************************************/
void pair_list_delete(struct PairList ** head){
    struct PairList * current = *head;
    struct PairList * next;
    while (current != NULL){
        next = current->next;
        cpair_free(current->data);current->data = NULL;
        free(current); current = NULL;
        current = next;
    }
    *head = NULL;
}

/***********************************************************//**
    Get the length of a pair list

    \param head - pointer to first element of the list

    \return len - length of the list
***************************************************************/
size_t pair_list_len(struct PairList * head)
{

    size_t len = 0;
    struct PairList * current = head;
    while (current != NULL){
        len++;
        current = current->next;
    }
    return len;
}

/***********************************************************//**
    Get the index of a pair in a list

    \param head [in] - pointer to first element of the list
    \param pair [in] - pair to get the index of

    \return len - length of the list
    
    \note
        returns 0 if doesnt exist otherwise returns location+1
***************************************************************/
size_t pair_list_index(struct PairList * head, struct Cpair * pair)
{
    // returns first occurence
    struct PairList * cur = head;
    size_t ind = 0;
    size_t count = 0;
    int same = 0;
    while (cur != NULL){
        same = cpair_isequal(cur->data,pair);
        count++;
        if (same == 1){
            break;
        }
        cur = cur->next;
    }
    if (same == 1){
        ind = count;
    }
    return ind;
}

/***********************************************************//**
    Allocate memory for a new hashtable of cpairs

    \param size [in] - size table
    
    \return new_table 
***************************************************************/
struct HashtableCpair * 
create_hashtable_cp(size_t size)
{

    struct HashtableCpair * new_table = NULL;
    if (size < 1) return NULL;
    
    if (NULL == (new_table = malloc(sizeof(struct HashtableCpair)))){
        fprintf(stderr, "failed to allocate memory for hashtable.\n");
        exit(1);
    }
    if (NULL == (new_table->table = malloc(size * sizeof(struct PairList *)))){
        fprintf(stderr, "failed to allocate memory for hashtable.\n");
        exit(1);
    }

    new_table->size = size;
    size_t ii;
    for (ii = 0; ii < size; ii++){
        new_table->table[ii] = NULL;
    }

    return new_table;
}

/***********************************************************//**
    Lookup a key in the hashtable

    \param ht [in] - hashtable
    \param key [in] - key to lookup
    
    \return out - either NULL or the second element in the pair stored under the key 
***************************************************************/
char * lookup_key(struct HashtableCpair * ht , char * key)
{
    struct PairList * pl = NULL;
    size_t val = hashsimple(ht->size,key);
    
    char * out = NULL;
    size_t N;
    for (pl = ht->table[val]; pl != NULL; pl = pl->next){
        if (strcmp(key,pl->data->a) == 0){
            N = strlen(pl->data->b);
            out = malloc((N+1)*sizeof(char));
            memmove(out,pl->data->b,N*sizeof(char));
            out[N]='\0';
            return out;
        }
    }
    return out;
}

/***********************************************************//**
    Add a Cpair to the table

    \param ht [in] - hashtable
    \param cp [in] - cpair to insert
    
    \return
        0 if good, 1 if some err, 2 if exists
***************************************************************/
int add_cpair(struct HashtableCpair * ht, struct Cpair * cp){

    char * curr_pl = NULL;

    size_t val = hashsimple(ht->size,cp->a);
    
    curr_pl = lookup_key(ht,cp->a);
    if (curr_pl != NULL) {
        free(curr_pl); curr_pl = NULL;
        return 2;
    }
    
    pair_push(&(ht->table[val]), cp);
    free(curr_pl);

    return 0;
}

/***********************************************************//**
    Free memory allocated to the hashtable

    \param ht [inout] - hashtable
***************************************************************/
void free_hashtable_cp(struct HashtableCpair * ht){

    if (ht != NULL){
        size_t ii;
        for (ii = 0; ii < ht->size; ii++){
            pair_list_delete(&(ht->table[ii]));
        }
        free(ht->table); ht->table = NULL;
        free(ht); ht = NULL;
    }
}

/***********************************************************//**
    Get number of elements stored in the hashtable

    \param ht [inout] - hashtable

    \return N - number of elements in the hashtable
***************************************************************/
size_t nstored_hashtable_cp(struct HashtableCpair * ht)
{
    
    size_t N = 0;
    size_t ii;
    for (ii = 0; ii < ht->size; ii++){
        N += pair_list_len(ht->table[ii]);
    }
    return N;
}

/***********************************************************//**
    Simple hash function

    \param size [in] - hashtable size
    \param str [in] - string to hash
    

    \return hashval

    \note
         http://www.sparknotes.com/cs/searching/hashtables/section3/page/2/
***************************************************************/
size_t hashsimple(size_t size, char * str)
{   
    size_t hashval = 0;
    
    for (; *str != '\0'; str++){
        hashval = (size_t) *str + (hashval << 5) - hashval;
    }
    
    return hashval % size;
}