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

/** \file regress.c
 * Provides routines for FT-regression
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "array.h"
#include "ft.h"
/* #include "regress.h" */
#include "lib_linalg.h"


/** \struct RegressALS
 *  \brief Alternating least squares regression
 *  \var RegressALS::dim
 *  dimension of approximation
 *  \var RegressALS::nparams
 *  number of params in each core
 *  \var RegressALS::N
 *  number of data points
 *  \var RegressALS::y
 *  evaluations
 *  \var RegressALS::x
 *  input locations
 *  \var RegressALS::core
 *  core over which to optimize
 *  \var RegressALS::pre_eval
 *  evaluations prior to dimension *core*
 * \var RegressALS::post_eval
 *  evaluations post dimension *core*
 * \var RegressALS:grad_space
 *  spare space for gradient evaluations
 * \var RegressALS::grad_core_space
 *  space for evaluation of the gradient of the core with respect to every param
 * \var RegressALS::fparam_space
 *  space for evaluation of gradient of params for a given function in a core
 * \var RegressALS::ft
 *  function_train
 * \var RegressALS::ft_param
 *  flattened array of function train parameters
 */
struct RegressALS
{
    size_t dim;
    size_t * nparams;
    
    size_t N; 
    const double * y;
    const double * x;
    
    size_t core;
    double * prev_eval;
    double * post_eval;
    double * curr_eval;

    double * grad_space;
    double * grad_core_space;
    double * fparam_space;

    struct FunctionTrain * ft;
    double * ft_param;
};

/***********************************************************//**
    Allocate ALS regression
***************************************************************/
struct RegressALS * regress_als_alloc(size_t dim)
{
    struct RegressALS * als = malloc(sizeof(struct RegressALS));
    if (als == NULL){
        fprintf(stderr, "Failure to allocate RegressALS\n");
        return NULL;
    }
    
    als->dim = dim;
    als->nparams = calloc_size_t(dim);
    
    als->y = NULL;
    als->x = NULL;

    als->prev_eval = NULL;
    als->post_eval = NULL;
    als->curr_eval = NULL;

    als->grad_space      = NULL;
    als->grad_core_space = NULL;
    als->fparam_space    = NULL;

    als->ft = NULL;
    als->ft_param = NULL;
    
    return als;
}

/***********************************************************//**
    Free ALS regression options
***************************************************************/
void regress_als_free(struct RegressALS * als)
{
    if (als != NULL){

        free(als->nparams);  als->nparams    = NULL;
        
        free(als->prev_eval); als->prev_eval = NULL;
        free(als->post_eval); als->post_eval = NULL;
        free(als->curr_eval); als->curr_eval = NULL;

        free(als->grad_space);      als->grad_space      = NULL;
        free(als->grad_core_space); als->grad_core_space = NULL;
        free(als->fparam_space);    als->fparam_space    = NULL;

        function_train_free(als->ft); als->ft = NULL;
        free(als->ft_param);          als->ft_param = NULL;
        free(als); als = NULL;
    }
}

/***********************************************************//**
    Add data to ALS
***************************************************************/
void regress_als_add_data(struct RegressALS * als, size_t N, const double * x, const double * y)
{
    assert (als != NULL);
    als->N = N;
    als->x = x;
    als->y = y;
}


/***********************************************************//**
    Prepare memmory
***************************************************************/
void regress_als_prep_memory(struct RegressALS * als, struct FunctionTrain * ft)
{
    assert (als != NULL);
    assert (ft != NULL);
    if (als->dim != ft->dim){
        fprintf(stderr, "ALS Regression dimension is not the same as FT dimension\n");
        assert (als->dim == ft->dim);
    }
    
    size_t maxrank = function_train_get_maxrank(ft);

    size_t maxparamfunc = 0;
    size_t nparamfunc = 0;
    size_t max_core_params = 0;
    size_t ntotparams = 0;
    for (size_t ii = 0; ii < ft->dim; ii++){
        nparamfunc = 0;
        als->nparams[ii] = function_train_core_get_nparams(ft,ii,&nparamfunc);
        if (nparamfunc > maxparamfunc){
            maxparamfunc = nparamfunc;
        }
        if (als->nparams[ii] > max_core_params){
            max_core_params = als->nparams[ii];
        }
        ntotparams += als->nparams[ii];
    }

    als->prev_eval = calloc_double(maxrank);
    als->post_eval = calloc_double(maxrank);
    als->curr_eval = calloc_double(maxrank*maxrank);
    /* max_core_params = 1000; */
    /* printf("grad_core_space allocated = %zu\n",max_core_params*maxrank*maxrank); */
    /* printf("max paramfunc = %zu\n",max_core_params); */
    als->grad_space      = calloc_double(max_core_params);
    als->grad_core_space = calloc_double(max_core_params * maxrank * maxrank);
    als->fparam_space    = calloc_double(maxparamfunc);

    als->ft = function_train_copy(ft);
    als->ft_param = calloc_double(ntotparams);

    size_t running = 0, incr;
    for (size_t ii = 0; ii < ft->dim; ii++){
        incr = function_train_core_get_params(ft,ii,als->ft_param + running);
        running += incr;
    }
}

/********************************************************//**
    Set which core we are regressing on
************************************************************/
void regress_als_set_core(struct RegressALS * als, size_t core)
{
    assert (als != NULL);
    assert (core < als->dim);
    als->core = core;
}

/********************************************************//**
    LS regression objective function
************************************************************/
double regress_core_LS(size_t nparam, const double * param, double * grad, void * arg)
{

    struct RegressALS * als = arg;
    size_t d      = als->dim;
    size_t core   = als->core;
    assert( nparam == als->nparams[core]);

    /* printf("N=%zu, d= %zu\n",als->N,d); */
    /* printf("data = \n"); */
    /* dprint2d_col(d,als->N,als->x); */
    /* printf("update core params\n"); */
    function_train_core_update_params(als->ft,core,nparam,param);
    /* printf("updated core params\n"); */

    if (grad != NULL){
        for (size_t ii = 0; ii < nparam; ii++){
            grad[ii] = 0.0;
        }
    }
    double out=0.0, eval,resid;
    if (grad != NULL){
        for (size_t ii = 0; ii < als->N; ii++){
            eval = function_train_core_param_grad_eval(als->ft, als->x+ii*d,
                                                       core, nparam,
                                                       als->grad_core_space,
                                                       als->fparam_space,
                                                       als->grad_space,
                                                       als->prev_eval,
                                                       als->curr_eval,
                                                       als->post_eval);

            resid = als->y[ii]-eval;
            out += 0.5*resid*resid;
            cblas_daxpy(nparam,-resid,als->grad_space,1,grad,1);
        }
    }
    else{
        for (size_t ii = 0; ii < als->N; ii++){
            /* dprint(d,als->x+ii*d); */
            eval = function_train_eval(als->ft, als->x+ii*d);
            resid = als->y[ii]-eval;
            out += 0.5*resid*resid;
        }
    }

    return out;
}


/********************************************************//**
    Optimize over a particular core
************************************************************/
int regress_als_run_core(struct RegressALS * als, struct c3Opt * optimizer, double *val)
{
    assert (als != NULL);
    assert (als->ft != NULL);

    size_t core = als->core;
    size_t nparambefore = 0;
    for (size_t ii = 0; ii < core; ii++){
        nparambefore += als->nparams[ii];
    }
    int info = c3opt_minimize(optimizer,als->ft_param + nparambefore,val);
    if (info < 0){
        fprintf(stderr, "Minimizing core %zu resulted in nonconvergence with output %d\n",core,info);
    }
    
    function_train_core_update_params(als->ft,core,als->nparams[core],als->ft_param + nparambefore);
    return info;
}


/********************************************************//**
    Advance to the right
************************************************************/
void regress_als_step_right(struct RegressALS * als)
{

    assert (als != NULL);
    assert (als->ft != NULL);
    if (als->core == als->dim-1){
        fprintf(stderr, "Cannot step right in ALS regression, already on last dimension!!\n");
        exit(1);
    }
    als->core++;
}
