/** \file data_structures.c **/

/*
                  Data structure creation/destruction

                  This file contains everything which is necessary to
                  obtain and destroy datastructures used in the folding
                  recurrences throughout the VienneRNA paclage

                  c Ronny Lorenx

                  Vienna RNA package
*/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "ViennaRNA/utils.h"
#include "ViennaRNA/structure_utils.h"
#include "ViennaRNA/energy_par.h"
#include "ViennaRNA/data_structures.h"
#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/params.h"
#include "ViennaRNA/gquad.h"
#include "ViennaRNA/aln_util.h"
#include "ViennaRNA/ribo.h"
#include "ViennaRNA/constraints.h"
#include "ViennaRNA/part_func.h"
#include "ViennaRNA/cofold.h"
#include "ViennaRNA/mm.h"

#ifdef _OPENMP
#include <omp.h>
#endif

/*
#################################
# PRIVATE MACROS                #
#################################
*/

/* the definitions below indicate which arrays should be allocated upon retrieval of a matrices data structure */
#define ALLOC_NOTHING     0
#define ALLOC_F           1
#define ALLOC_F5          2
#define ALLOC_F3          4
#define ALLOC_FC          8
#define ALLOC_C           16
#define ALLOC_FML         32
#define ALLOC_PROBS       256
#define ALLOC_AUX         512

#define ALLOC_CIRC        1024
#define ALLOC_HYBRID      2048
#define ALLOC_UNIQ        4096


#define ALLOC_MFE_DEFAULT         (ALLOC_F5 | ALLOC_C | ALLOC_FML)

#define ALLOC_PF_WO_PROBS         (ALLOC_F | ALLOC_C | ALLOC_FML)
#define ALLOC_PF_DEFAULT          (ALLOC_PF_WO_PROBS | ALLOC_PROBS | ALLOC_AUX)

/*
#################################
# GLOBAL VARIABLES              #
#################################
*/

/*
#################################
# PRIVATE VARIABLES             #
#################################
*/

/*
#################################
# PRIVATE FUNCTION DECLARATIONS #
#################################
*/
PRIVATE void            add_pf_matrices( vrna_fold_compound *vc, vrna_mx_t type, unsigned int alloc_vector);
PRIVATE void            add_mfe_matrices(vrna_fold_compound *vc, vrna_mx_t type, unsigned int alloc_vector);
PRIVATE void            set_fold_compound(vrna_fold_compound *vc, vrna_md_t *md_p, vrna_mx_t mx_type, unsigned int mx_alloc_vector, unsigned int options);
PRIVATE void            make_pscores(vrna_fold_compound *vc);
PRIVATE vrna_mx_mfe_t   *get_mfe_matrices_alloc(unsigned int n, vrna_mx_t type, unsigned int alloc_vector);
PRIVATE vrna_mx_pf_t    *get_pf_matrices_alloc(unsigned int n, vrna_mx_t type, unsigned int alloc_vector);
PRIVATE void            rescale_params(vrna_fold_compound *vc);
PRIVATE unsigned int    get_mx_alloc_vector(vrna_md_t *md_p, unsigned int options);
PRIVATE void            add_params(vrna_fold_compound *vc, vrna_md_t *md_p, unsigned int options);


/*
#################################
# BEGIN OF FUNCTION DEFINITIONS #
#################################
*/

PUBLIC void
vrna_free_mfe_matrices(vrna_fold_compound *vc){

  unsigned int  i, j, ij;
  int           cnt1;

  if(vc){
    vrna_mx_mfe_t *self = vc->matrices;
    if(self){
      switch(self->type){
        case VRNA_MX_DEFAULT:   free(self->f5);
                                free(self->f3);
                                free(self->fc);
                                free(self->c);
                                free(self->fML);
                                free(self->fM1);
                                free(self->fM2);
                                free(self->ggg);
                                break;

        case VRNA_MX_2DFOLD:    /* This will be some fun... */
#ifdef COUNT_STATES
                                if(self->N_F5 != NULL){
                                  for(i = 1; i <= vc->length; i++){
                                    if(!self->N_F5[i]) continue;
                                    for(cnt1 = self->k_min_F5[i]; cnt1 <= vars->k_max_F5[i]; cnt1++)
                                      if(vars->l_min_F5[i][cnt1] < INF){
                                        vars->N_F5[i][cnt1] += vars->l_min_F5[i][cnt1]/2;
                                        free(vars->N_F5[i][cnt1]);
                                      }
                                    if(vars->k_min_F5[i] < INF){
                                      vars->N_F5[i] += vars->k_min_F5[i];
                                      free(vars->N_F5[i]);
                                    }
                                  }
                                  free(vars->N_F5);
                                }
#endif

                                if(self->E_F5 != NULL){
                                  for(i = 1; i <= vc->length; i++){
                                    if(!self->E_F5[i]) continue;
                                    for(cnt1 = self->k_min_F5[i]; cnt1 <= self->k_max_F5[i]; cnt1++)
                                      if(self->l_min_F5[i][cnt1] < INF){
                                        self->E_F5[i][cnt1] += self->l_min_F5[i][cnt1]/2;
                                        free(self->E_F5[i][cnt1]);
                                      }
                                    if(self->k_min_F5[i] < INF){
                                      self->E_F5[i] += self->k_min_F5[i];
                                      free(self->E_F5[i]);
                                      self->l_min_F5[i] += self->k_min_F5[i];
                                      self->l_max_F5[i] += self->k_min_F5[i];
                                      free(self->l_min_F5[i]);
                                      free(self->l_max_F5[i]);
                                    }
                                  }
                                  free(self->E_F5);
                                  free(self->l_min_F5);
                                  free(self->l_max_F5);
                                  free(self->k_min_F5);
                                  free(self->k_max_F5);
                                }

                                if(self->E_F3 != NULL){
                                  for(i = 1; i <= vc->length; i++){
                                    if(!self->E_F3[i]) continue;
                                    for(cnt1 = self->k_min_F3[i]; cnt1 <= self->k_max_F3[i]; cnt1++)
                                      if(self->l_min_F3[i][cnt1] < INF){
                                        self->E_F3[i][cnt1] += self->l_min_F3[i][cnt1]/2;
                                        free(self->E_F3[i][cnt1]);
                                      }
                                    if(self->k_min_F3[i] < INF){
                                      self->E_F3[i] += self->k_min_F3[i];
                                      free(self->E_F3[i]);
                                      self->l_min_F3[i] += self->k_min_F3[i];
                                      self->l_max_F3[i] += self->k_min_F3[i];
                                      free(self->l_min_F3[i]);
                                      free(self->l_max_F3[i]);
                                    }
                                  }
                                  free(self->E_F3);
                                  free(self->l_min_F3);
                                  free(self->l_max_F3);
                                  free(self->k_min_F3);
                                  free(self->k_max_F3);
                                }

#ifdef COUNT_STATES
                                if(self->N_C != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->N_C[ij]) continue;
                                      for(cnt1 = self->k_min_C[ij]; cnt1 <= self->k_max_C[ij]; cnt1++)
                                        if(self->l_min_C[ij][cnt1] < INF){
                                          self->N_C[ij][cnt1] += self->l_min_C[ij][cnt1]/2;
                                          free(self->N_C[ij][cnt1]);
                                        }
                                      if(self->k_min_C[ij] < INF){
                                        self->N_C[ij] += self->k_min_C[ij];
                                        free(self->N_C[ij]);
                                      }
                                    }
                                  }
                                  free(self->N_C);
                                }
#endif

                                if(self->E_C != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->E_C[ij]) continue;
                                      for(cnt1 = self->k_min_C[ij]; cnt1 <= self->k_max_C[ij]; cnt1++)
                                        if(self->l_min_C[ij][cnt1] < INF){
                                          self->E_C[ij][cnt1] += self->l_min_C[ij][cnt1]/2;
                                          free(self->E_C[ij][cnt1]);
                                        }
                                      if(self->k_min_C[ij] < INF){
                                        self->E_C[ij] += self->k_min_C[ij];
                                        free(self->E_C[ij]);
                                        self->l_min_C[ij] += self->k_min_C[ij];
                                        self->l_max_C[ij] += self->k_min_C[ij];
                                        free(self->l_min_C[ij]);
                                        free(self->l_max_C[ij]);
                                      }
                                    }
                                  }
                                  free(self->E_C);
                                  free(self->l_min_C);
                                  free(self->l_max_C);
                                  free(self->k_min_C);
                                  free(self->k_max_C);
                                }

#ifdef COUNT_STATES
                                if(self->N_M != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->N_M[ij]) continue;
                                      for(cnt1 = self->k_min_M[ij]; cnt1 <= self->k_max_M[ij]; cnt1++)
                                        if(self->l_min_M[ij][cnt1] < INF){
                                          self->N_M[ij][cnt1] += self->l_min_M[ij][cnt1]/2;
                                          free(self->N_M[ij][cnt1]);
                                        }
                                      if(self->k_min_M[ij] < INF){
                                        self->N_M[ij] += self->k_min_M[ij];
                                        free(self->N_M[ij]);
                                      }
                                    }
                                  }
                                  free(self->N_M);
                                }
#endif

                                if(self->E_M != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->E_M[ij]) continue;
                                      for(cnt1 = self->k_min_M[ij]; cnt1 <= self->k_max_M[ij]; cnt1++)
                                        if(self->l_min_M[ij][cnt1] < INF){
                                          self->E_M[ij][cnt1] += self->l_min_M[ij][cnt1]/2;
                                          free(self->E_M[ij][cnt1]);
                                        }
                                      if(self->k_min_M[ij] < INF){
                                        self->E_M[ij] += self->k_min_M[ij];
                                        free(self->E_M[ij]);
                                        self->l_min_M[ij] += self->k_min_M[ij];
                                        self->l_max_M[ij] += self->k_min_M[ij];
                                        free(self->l_min_M[ij]);
                                        free(self->l_max_M[ij]);
                                      }
                                    }
                                  }
                                  free(self->E_M);
                                  free(self->l_min_M);
                                  free(self->l_max_M);
                                  free(self->k_min_M);
                                  free(self->k_max_M);
                                }

#ifdef COUNT_STATES
                                if(self->N_M1 != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->N_M1[ij]) continue;
                                      for(cnt1 = self->k_min_M1[ij]; cnt1 <= self->k_max_M1[ij]; cnt1++)
                                        if(self->l_min_M1[ij][cnt1] < INF){
                                          self->N_M1[ij][cnt1] += self->l_min_M1[ij][cnt1]/2;
                                          free(self->N_M1[ij][cnt1]);
                                        }
                                      if(self->k_min_M1[ij] < INF){
                                        self->N_M1[ij] += self->k_min_M1[ij];
                                        free(self->N_M1[ij]);
                                      }
                                    }
                                  }
                                  free(self->N_M1);
                                }
#endif

                                if(self->E_M1 != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->E_M1[ij]) continue;
                                      for(cnt1 = self->k_min_M1[ij]; cnt1 <= self->k_max_M1[ij]; cnt1++)
                                        if(self->l_min_M1[ij][cnt1] < INF){
                                          self->E_M1[ij][cnt1] += self->l_min_M1[ij][cnt1]/2;
                                          free(self->E_M1[ij][cnt1]);
                                        }
                                      if(self->k_min_M1[ij] < INF){
                                        self->E_M1[ij] += self->k_min_M1[ij];
                                        free(self->E_M1[ij]);
                                        self->l_min_M1[ij] += self->k_min_M1[ij];
                                        self->l_max_M1[ij] += self->k_min_M1[ij];
                                        free(self->l_min_M1[ij]);
                                        free(self->l_max_M1[ij]);
                                      }
                                    }
                                  }
                                  free(self->E_M1);
                                  free(self->l_min_M1);
                                  free(self->l_max_M1);
                                  free(self->k_min_M1);
                                  free(self->k_max_M1);
                                }

                                if(self->E_M2 != NULL){
                                  for(i = 1; i < vc->length-TURN-1; i++){
                                    if(!self->E_M2[i]) continue;
                                    for(cnt1 = self->k_min_M2[i]; cnt1 <= self->k_max_M2[i]; cnt1++)
                                      if(self->l_min_M2[i][cnt1] < INF){
                                        self->E_M2[i][cnt1] += self->l_min_M2[i][cnt1]/2;
                                        free(self->E_M2[i][cnt1]);
                                      }
                                    if(self->k_min_M2[i] < INF){
                                      self->E_M2[i] += self->k_min_M2[i];
                                      free(self->E_M2[i]);
                                      self->l_min_M2[i] += self->k_min_M2[i];
                                      self->l_max_M2[i] += self->k_min_M2[i];
                                      free(self->l_min_M2[i]);
                                      free(self->l_max_M2[i]);
                                    }
                                  }
                                  free(self->E_M2);
                                  free(self->l_min_M2);
                                  free(self->l_max_M2);
                                  free(self->k_min_M2);
                                  free(self->k_max_M2);
                                }

                                if(self->E_Fc != NULL){
                                  for(cnt1 = self->k_min_Fc; cnt1 <= self->k_max_Fc; cnt1++)
                                    if(self->l_min_Fc[cnt1] < INF){
                                      self->E_Fc[cnt1] += self->l_min_Fc[cnt1]/2;
                                      free(self->E_Fc[cnt1]);
                                    }
                                  if(self->k_min_Fc < INF){
                                    self->E_Fc += self->k_min_Fc;
                                    free(self->E_Fc);
                                    self->l_min_Fc += self->k_min_Fc;
                                    self->l_max_Fc += self->k_min_Fc;
                                    free(self->l_min_Fc);
                                    free(self->l_max_Fc);
                                  }
                                }

                                if(self->E_FcI != NULL){
                                  for(cnt1 = self->k_min_FcI; cnt1 <= self->k_max_FcI; cnt1++)
                                    if(self->l_min_FcI[cnt1] < INF){
                                      self->E_FcI[cnt1] += self->l_min_FcI[cnt1]/2;
                                      free(self->E_FcI[cnt1]);
                                    }
                                  if(self->k_min_FcI < INF){
                                    self->E_FcI += self->k_min_FcI;
                                    free(self->E_FcI);
                                    self->l_min_FcI += self->k_min_FcI;
                                    self->l_max_FcI += self->k_min_FcI;
                                    free(self->l_min_FcI);
                                    free(self->l_max_FcI);
                                  }
                                }

                                if(self->E_FcH != NULL){
                                  for(cnt1 = self->k_min_FcH; cnt1 <= self->k_max_FcH; cnt1++)
                                    if(self->l_min_FcH[cnt1] < INF){
                                      self->E_FcH[cnt1] += self->l_min_FcH[cnt1]/2;
                                      free(self->E_FcH[cnt1]);
                                    }
                                  if(self->k_min_FcH < INF){
                                    self->E_FcH += self->k_min_FcH;
                                    free(self->E_FcH);
                                    self->l_min_FcH += self->k_min_FcH;
                                    self->l_max_FcH += self->k_min_FcH;
                                    free(self->l_min_FcH);
                                    free(self->l_max_FcH);
                                  }
                                }

                                if(self->E_FcM != NULL){
                                  for(cnt1 = self->k_min_FcM; cnt1 <= self->k_max_FcM; cnt1++)
                                    if(self->l_min_FcM[cnt1] < INF){
                                      self->E_FcM[cnt1] += self->l_min_FcM[cnt1]/2;
                                      free(self->E_FcM[cnt1]);
                                    }
                                  if(self->k_min_FcM < INF){
                                    self->E_FcM += self->k_min_FcM;
                                    free(self->E_FcM);
                                    self->l_min_FcM += self->k_min_FcM;
                                    self->l_max_FcM += self->k_min_FcM;
                                    free(self->l_min_FcM);
                                    free(self->l_max_FcM);
                                  }
                                }

                                free(self->E_F5_rem);
                                free(self->E_F3_rem);
                                free(self->E_C_rem);
                                free(self->E_M_rem);
                                free(self->E_M1_rem);
                                free(self->E_M2_rem);

                                break;

        default:                /* do nothing */
                                break;
      }
      free(self);
      vc->matrices = NULL;
    }
  }
}

PUBLIC void
vrna_free_pf_matrices(vrna_fold_compound *vc){

  unsigned int  i, j, ij;
  int           cnt1;

  if(vc){
    vrna_mx_pf_t  *self = vc->exp_matrices;
    if(self){
      switch(self->type){
        case VRNA_MX_DEFAULT:   free(self->q);
                                free(self->qb);
                                free(self->qm);
                                free(self->qm1);
                                free(self->qm2);
                                free(self->probs);
                                free(self->G);
                                free(self->q1k);
                                free(self->qln);
                                break;

        case VRNA_MX_2DFOLD:    /* This will be some fun... */
                                if(self->Q != NULL){
                                  for(i = 1; i <= vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->Q[ij]) continue;
                                      for(cnt1 = self->k_min_Q[ij]; cnt1 <= self->k_max_Q[ij]; cnt1++)
                                        if(self->l_min_Q[ij][cnt1] < INF){
                                          self->Q[ij][cnt1] += self->l_min_Q[ij][cnt1]/2;
                                          free(self->Q[ij][cnt1]);
                                        }
                                      if(self->k_min_Q[ij] < INF){
                                        self->Q[ij] += self->k_min_Q[ij];
                                        free(self->Q[ij]);
                                        self->l_min_Q[ij] += self->k_min_Q[ij];
                                        self->l_max_Q[ij] += self->k_min_Q[ij];
                                        free(self->l_min_Q[ij]);
                                        free(self->l_max_Q[ij]);
                                      }
                                    }
                                  }
                                  free(self->Q);
                                  free(self->l_min_Q);
                                  free(self->l_max_Q);
                                  free(self->k_min_Q);
                                  free(self->k_max_Q);
                                }

                                if(self->Q_B != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->Q_B[ij]) continue;
                                      for(cnt1 = self->k_min_Q_B[ij]; cnt1 <= self->k_max_Q_B[ij]; cnt1++)
                                        if(self->l_min_Q_B[ij][cnt1] < INF){
                                          self->Q_B[ij][cnt1] += self->l_min_Q_B[ij][cnt1]/2;
                                          free(self->Q_B[ij][cnt1]);
                                        }
                                      if(self->k_min_Q_B[ij] < INF){
                                        self->Q_B[ij] += self->k_min_Q_B[ij];
                                        free(self->Q_B[ij]);
                                        self->l_min_Q_B[ij] += self->k_min_Q_B[ij];
                                        self->l_max_Q_B[ij] += self->k_min_Q_B[ij];
                                        free(self->l_min_Q_B[ij]);
                                        free(self->l_max_Q_B[ij]);
                                      }
                                    }
                                  }
                                  free(self->Q_B);
                                  free(self->l_min_Q_B);
                                  free(self->l_max_Q_B);
                                  free(self->k_min_Q_B);
                                  free(self->k_max_Q_B);
                                }

                                if(self->Q_M != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->iindx[i] - j;
                                      if(!self->Q_M[ij]) continue;
                                      for(cnt1 = self->k_min_Q_M[ij]; cnt1 <= self->k_max_Q_M[ij]; cnt1++)
                                        if(self->l_min_Q_M[ij][cnt1] < INF){
                                          self->Q_M[ij][cnt1] += self->l_min_Q_M[ij][cnt1]/2;
                                          free(self->Q_M[ij][cnt1]);
                                        }
                                      if(self->k_min_Q_M[ij] < INF){
                                        self->Q_M[ij] += self->k_min_Q_M[ij];
                                        free(self->Q_M[ij]);
                                        self->l_min_Q_M[ij] += self->k_min_Q_M[ij];
                                        self->l_max_Q_M[ij] += self->k_min_Q_M[ij];
                                        free(self->l_min_Q_M[ij]);
                                        free(self->l_max_Q_M[ij]);
                                      }
                                    }
                                  }
                                  free(self->Q_M);
                                  free(self->l_min_Q_M);
                                  free(self->l_max_Q_M);
                                  free(self->k_min_Q_M);
                                  free(self->k_max_Q_M);
                                }

                                if(self->Q_M1 != NULL){
                                  for(i = 1; i < vc->length; i++){
                                    for(j = i; j <= vc->length; j++){
                                      ij = vc->jindx[j] + i;
                                      if(!self->Q_M1[ij]) continue;
                                      for(cnt1 = self->k_min_Q_M1[ij]; cnt1 <= self->k_max_Q_M1[ij]; cnt1++)
                                        if(self->l_min_Q_M1[ij][cnt1] < INF){
                                          self->Q_M1[ij][cnt1] += self->l_min_Q_M1[ij][cnt1]/2;
                                          free(self->Q_M1[ij][cnt1]);
                                        }
                                      if(self->k_min_Q_M1[ij] < INF){
                                        self->Q_M1[ij] += self->k_min_Q_M1[ij];
                                        free(self->Q_M1[ij]);
                                        self->l_min_Q_M1[ij] += self->k_min_Q_M1[ij];
                                        self->l_max_Q_M1[ij] += self->k_min_Q_M1[ij];
                                        free(self->l_min_Q_M1[ij]);
                                        free(self->l_max_Q_M1[ij]);
                                      }
                                    }
                                  }
                                  free(self->Q_M1);
                                  free(self->l_min_Q_M1);
                                  free(self->l_max_Q_M1);
                                  free(self->k_min_Q_M1);
                                  free(self->k_max_Q_M1);
                                }

                                if(self->Q_M2 != NULL){
                                  for(i = 1; i < vc->length-TURN-1; i++){
                                    if(!self->Q_M2[i]) continue;
                                    for(cnt1 = self->k_min_Q_M2[i]; cnt1 <= self->k_max_Q_M2[i]; cnt1++)
                                      if(self->l_min_Q_M2[i][cnt1] < INF){
                                        self->Q_M2[i][cnt1] += self->l_min_Q_M2[i][cnt1]/2;
                                        free(self->Q_M2[i][cnt1]);
                                      }
                                    if(self->k_min_Q_M2[i] < INF){
                                      self->Q_M2[i] += self->k_min_Q_M2[i];
                                      free(self->Q_M2[i]);
                                      self->l_min_Q_M2[i] += self->k_min_Q_M2[i];
                                      self->l_max_Q_M2[i] += self->k_min_Q_M2[i];
                                      free(self->l_min_Q_M2[i]);
                                      free(self->l_max_Q_M2[i]);
                                    }
                                  }
                                  free(self->Q_M2);
                                  free(self->l_min_Q_M2);
                                  free(self->l_max_Q_M2);
                                  free(self->k_min_Q_M2);
                                  free(self->k_max_Q_M2);
                                }

                                if(self->Q_c != NULL){
                                  for(cnt1 = self->k_min_Q_c; cnt1 <= self->k_max_Q_c; cnt1++)
                                    if(self->l_min_Q_c[cnt1] < INF){
                                      self->Q_c[cnt1] += self->l_min_Q_c[cnt1]/2;
                                      free(self->Q_c[cnt1]);
                                    }
                                  if(self->k_min_Q_c < INF){
                                    self->Q_c += self->k_min_Q_c;
                                    free(self->Q_c);
                                    self->l_min_Q_c += self->k_min_Q_c;
                                    self->l_max_Q_c += self->k_min_Q_c;
                                    free(self->l_min_Q_c);
                                    free(self->l_max_Q_c);
                                  }
                                }

                                if(self->Q_cI != NULL){
                                  for(cnt1 = self->k_min_Q_cI; cnt1 <= self->k_max_Q_cI; cnt1++)
                                    if(self->l_min_Q_cI[cnt1] < INF){
                                      self->Q_cI[cnt1] += self->l_min_Q_cI[cnt1]/2;
                                      free(self->Q_cI[cnt1]);
                                    }
                                  if(self->k_min_Q_cI < INF){
                                    self->Q_cI += self->k_min_Q_cI;
                                    free(self->Q_cI);
                                    self->l_min_Q_cI += self->k_min_Q_cI;
                                    self->l_max_Q_cI += self->k_min_Q_cI;
                                    free(self->l_min_Q_cI);
                                    free(self->l_max_Q_cI);
                                  }
                                }

                                if(self->Q_cH != NULL){
                                  for(cnt1 = self->k_min_Q_cH; cnt1 <= self->k_max_Q_cH; cnt1++)
                                    if(self->l_min_Q_cH[cnt1] < INF){
                                      self->Q_cH[cnt1] += self->l_min_Q_cH[cnt1]/2;
                                      free(self->Q_cH[cnt1]);
                                    }
                                  if(self->k_min_Q_cH < INF){
                                    self->Q_cH += self->k_min_Q_cH;
                                    free(self->Q_cH);
                                    self->l_min_Q_cH += self->k_min_Q_cH;
                                    self->l_max_Q_cH += self->k_min_Q_cH;
                                    free(self->l_min_Q_cH);
                                    free(self->l_max_Q_cH);
                                  }
                                }

                                if(self->Q_cM != NULL){
                                  for(cnt1 = self->k_min_Q_cM; cnt1 <= self->k_max_Q_cM; cnt1++)
                                    if(self->l_min_Q_cM[cnt1] < INF){
                                      self->Q_cM[cnt1] += self->l_min_Q_cM[cnt1]/2;
                                      free(self->Q_cM[cnt1]);
                                    }
                                  if(self->k_min_Q_cM < INF){
                                    self->Q_cM += self->k_min_Q_cM;
                                    free(self->Q_cM);
                                    self->l_min_Q_cM += self->k_min_Q_cM;
                                    self->l_max_Q_cM += self->k_min_Q_cM;
                                    free(self->l_min_Q_cM);
                                    free(self->l_max_Q_cM);
                                  }
                                }

                                free(self->Q_rem);
                                free(self->Q_B_rem);
                                free(self->Q_M_rem);
                                free(self->Q_M1_rem);
                                free(self->Q_M2_rem);

                                break;

        default:                /* do nothing */
                                break;
      }

      free(self->expMLbase);
      free(self->scale);

      free(self);
      vc->exp_matrices = NULL;
    }
  }
}

PUBLIC  void
vrna_free_fold_compound(vrna_fold_compound *vc){

  int s;

  if(vc){

    /* first destroy common attributes */
    vrna_free_mfe_matrices(vc);
    vrna_free_pf_matrices(vc);
    free(vc->iindx);
    free(vc->jindx);
    free(vc->params);
    free(vc->exp_params);
    vrna_hc_free(vc->hc);

    /* now distinguish the vc type */
    switch(vc->type){
      case VRNA_VC_TYPE_SINGLE:     free(vc->sequence);
                                    free(vc->sequence_encoding);
                                    free(vc->sequence_encoding2);
                                    free(vc->ptype);
                                    free(vc->ptype_pf_compat);
                                    vrna_sc_free(vc->sc);
                                    break;
      case VRNA_VC_TYPE_ALIGNMENT:  for(s=0;s<vc->n_seq;s++){
                                      free(vc->sequences[s]);
                                      free(vc->S[s]);
                                      free(vc->S5[s]);
                                      free(vc->S3[s]);
                                      free(vc->Ss[s]);
                                      free(vc->a2s[s]);
                                    }
                                    free(vc->sequences);
                                    free(vc->cons_seq);
                                    free(vc->S_cons);
                                    free(vc->S);
                                    free(vc->S5);
                                    free(vc->S3);
                                    free(vc->Ss);
                                    free(vc->a2s);
                                    free(vc->pscore);
                                    if(vc->scs){
                                      for(s=0;s<vc->n_seq;s++)
                                        vrna_sc_free(vc->scs[s]);
                                      free(vc->scs);
                                    }
                                    break;
      default:                      /* do nothing */
                                    break;
    }

    /* free Distance Class Partitioning stuff (should be NULL if not used */
    free(vc->reference_pt1);
    free(vc->reference_pt2);
    free(vc->referenceBPs1);
    free(vc->referenceBPs2);
    free(vc->bpdist);
    free(vc->mm1);
    free(vc->mm2);
    
    free(vc);
  }
}


PUBLIC vrna_fold_compound*
vrna_get_fold_compound( const char *sequence,
                        vrna_md_t *md_p,
                        unsigned int options){

  int length;
  unsigned int mx_alloc_vector;
  vrna_fold_compound *vc;
  vrna_md_t           md;

  if(sequence == NULL) return NULL;

  /* sanity check */
  length = strlen(sequence);
  if(length == 0)
    vrna_message_error("vrna_get_fold_compound: sequence length must be greater 0");

  vc              = (vrna_fold_compound *)vrna_alloc(sizeof(vrna_fold_compound));
  vc->type        = VRNA_VC_TYPE_SINGLE;
  vc->length      = length;
  vc->sequence    = strdup(sequence);

  /* get a copy of the model details */
  if(md_p)
    md = *md_p;
  else /* this fallback relies on global parameters and thus is not threadsafe */
    vrna_md_set_globals(&md);

  mx_alloc_vector = get_mx_alloc_vector(&md, options);

  set_fold_compound(vc, &md, VRNA_MX_DEFAULT, mx_alloc_vector, options);

  return vc;
}

PUBLIC vrna_fold_compound*
vrna_get_fold_compound_ali( const char **sequences,
                            vrna_md_t *md_p,
                            unsigned int options){

  int s, n_seq, length;
  unsigned int mx_alloc_vector;
  vrna_fold_compound *vc;
  vrna_md_t           md;
  
  if(sequences == NULL) return NULL;

  for(s=0;sequences[s];s++); /* count the sequences */

  n_seq = s;

  length = strlen(sequences[0]);
  /* sanity check */
  if(length == 0)
    vrna_message_error("vrna_get_fold_compound_ali: sequence length must be greater 0");
  for(s = 0; s < n_seq; s++)
    if(strlen(sequences[s]) != length)
      vrna_message_error("vrna_get_fold_compound_ali: uneqal sequence lengths in alignment");

  vc            = (vrna_fold_compound *)vrna_alloc(sizeof(vrna_fold_compound));
  vc->type      = VRNA_VC_TYPE_ALIGNMENT;

  vc->n_seq     = n_seq;
  vc->length    = length;
  vc->sequences = (char **)vrna_alloc(sizeof(char *) * (vc->n_seq + 1));
  for(s = 0; sequences[s]; s++)
    vc->sequences[s] = strdup(sequences[s]);

  /* get a copy of the model details */
  if(md_p)
    md = *md_p;
  else /* this fallback relies on global parameters and thus is not threadsafe */
    vrna_md_set_globals(&md);

  mx_alloc_vector = get_mx_alloc_vector(&md, options);

  set_fold_compound(vc, &md, VRNA_MX_DEFAULT, mx_alloc_vector, options);

  return vc;
}

PUBLIC vrna_fold_compound*
vrna_get_fold_compound_2D(const char *sequence,
                          const char *s1,
                          const char *s2,
                          vrna_md_t *md_p,
                          unsigned int options){

  int                 length, l, turn;
  unsigned int        mx_alloc_vector;
  vrna_fold_compound  *vc;
  vrna_md_t           md;


  if(sequence == NULL) return NULL;

  /* sanity check */
  length = strlen(sequence);
  if(length == 0)
    vrna_message_error("vrna_get_fold_compound_2D: sequence length must be greater 0");

  l = strlen(s1);
  if(l != length)
    vrna_message_error("vrna_get_fold_compound_2D: sequence and s1 differ in length");

  l = strlen(s2);
  if(l != length)
    vrna_message_error("vrna_get_fold_compound_2D: sequence and s2 differ in length");

  vc              = (vrna_fold_compound *)vrna_alloc(sizeof(vrna_fold_compound));
  vc->type        = VRNA_VC_TYPE_SINGLE;
  vc->length      = length;
  vc->sequence    = strdup(sequence);
  mx_alloc_vector = ALLOC_NOTHING;

  /* get a copy of the model details */
  if(md_p)
    md = *md_p;
  else /* this fallback relies on global parameters and thus is not threadsafe */
    vrna_md_set_globals(&md);

  /* always make uniq ML decomposition ! */
  md.uniq_ML = 1;

  mx_alloc_vector = get_mx_alloc_vector(&md, options);

  set_fold_compound(vc, &md, VRNA_MX_2DFOLD, mx_alloc_vector, options);

  /* set all fields that are unique to Distance class partitioning... */
  turn  = vc->params->model_details.min_loop_size;
  vc->reference_pt1 = vrna_pt_get(s1);
  vc->reference_pt2 = vrna_pt_get(s2);
  vc->referenceBPs1 = vrna_refBPcnt_matrix(vc->reference_pt1, turn);
  vc->referenceBPs2 = vrna_refBPcnt_matrix(vc->reference_pt2, turn);
  vc->bpdist        = vrna_refBPdist_matrix(vc->reference_pt1, vc->reference_pt2, turn);
  /* compute maximum matching with reference structure 1 disallowed */
  vc->mm1           = maximumMatchingConstraint(vc->sequence, vc->reference_pt1);
  /* compute maximum matching with reference structure 2 disallowed */
  vc->mm2           = maximumMatchingConstraint(vc->sequence, vc->reference_pt2);

  vc->maxD1         = vc->mm1[vc->iindx[1]-length] + vc->referenceBPs1[vc->iindx[1]-length];
  vc->maxD2         = vc->mm2[vc->iindx[1]-length] + vc->referenceBPs2[vc->iindx[1]-length];

  return vc;
}

PUBLIC void
vrna_params_update( vrna_fold_compound *vc,
                    vrna_param_t *parameters){

  if(vc){
    if(vc->params)
      free(vc->params);
    if(parameters){
      vc->params = vrna_params_copy(parameters);
    } else {
      switch(vc->type){
        case VRNA_VC_TYPE_SINGLE:     /* fall through */

        case VRNA_VC_TYPE_ALIGNMENT:  vc->params = vrna_params_get(NULL);
                                      break;

        default:                      break;
      }
    }
  }
}

PUBLIC void
vrna_exp_params_update( vrna_fold_compound *vc,
                        vrna_exp_param_t *params){

  if(vc){
    if(vc->exp_params)
      free(vc->exp_params);
    if(params){
      vc->exp_params = vrna_exp_params_copy(params);
    } else {
      switch(vc->type){
        case VRNA_VC_TYPE_SINGLE:     vc->exp_params = vrna_exp_params_get(NULL);
                                      if(vc->cutpoint > 0)
                                        vc->exp_params->model_details.min_loop_size = 0;
                                      break;

        case VRNA_VC_TYPE_ALIGNMENT:  vc->exp_params = vrna_exp_params_ali_get(vc->n_seq, NULL);
                                      break;

        default:                      break;
      }
    }
    /* fill additional helper arrays for scaling etc. */
    vrna_exp_params_rescale(vc, NULL);
  }
}

PUBLIC void
vrna_exp_params_rescale(vrna_fold_compound *vc,
                        double *mfe){

  if(vc){
    vrna_exp_param_t *pf = vc->exp_params;
    if(pf){
      double kT = pf->kT;

      if(vc->type == VRNA_VC_TYPE_ALIGNMENT)
        kT /= vc->n_seq;

      vrna_md_t *md = &(pf->model_details);
      if(mfe){
        kT /= 1000.;
        pf->pf_scale = exp(-(md->sfact * *mfe)/ kT / vc->length);
      } else if(pf->pf_scale < 1.){  /* mean energy for random sequences: 184.3*length cal */
        pf->pf_scale = exp(-(-185+(pf->temperature-37.)*7.27)/kT);
        if(pf->pf_scale < 1.)
          pf->pf_scale = 1.;
      }
      rescale_params(vc);
    }
  }
}


/*
#####################################
# BEGIN OF STATIC HELPER FUNCTIONS  #
#####################################
*/
PRIVATE unsigned int
get_mx_alloc_vector(vrna_md_t *md_p, unsigned int options){
  unsigned int  v;

  v = ALLOC_NOTHING;

  /* default MFE matrices ? */
  if(options & VRNA_OPTION_MFE)
    v |= ALLOC_MFE_DEFAULT;

  /* default PF matrices ? */
  if(options & VRNA_OPTION_PF)
    v |= (md_p->compute_bpp) ? ALLOC_PF_DEFAULT : ALLOC_PF_WO_PROBS;

  if(options & VRNA_OPTION_HYBRID)
    v |= ALLOC_HYBRID;

  /* matrices for circular folding ? */
  if(md_p->circ){
    md_p->uniq_ML = 1; /* we need unique ML arrays for circular folding */
    v |= ALLOC_CIRC;
  }

  /* unique ML decomposition ? */
  if(md_p->uniq_ML)
    v |= ALLOC_UNIQ;

  return v;
}


PRIVATE void
rescale_params( vrna_fold_compound *vc){

  int           i;
  vrna_exp_param_t  *pf = vc->exp_params;
  vrna_mx_pf_t      *m  = vc->exp_matrices;

  m->scale[0] = 1.;
  m->scale[1] = 1./pf->pf_scale;
  m->expMLbase[0] = 1;
  m->expMLbase[1] = pf->expMLbase / pf->pf_scale;
  for (i=2; i<=vc->length; i++) {
    m->scale[i] = m->scale[i/2]*m->scale[i-(i/2)];
    m->expMLbase[i] = pow(pf->expMLbase, (double)i) * m->scale[i];
  }
}

PRIVATE void
add_params( vrna_fold_compound *vc,
            vrna_md_t *md_p,
            unsigned int options){

  if(options & VRNA_OPTION_MFE)
    vc->params = vrna_params_get(md_p);

  if(options & VRNA_OPTION_PF){
    vc->exp_params  = (vc->type == VRNA_VC_TYPE_SINGLE) ? \
                        vrna_exp_params_get(md_p) : \
                        vrna_exp_params_ali_get(vc->n_seq, md_p);
  }

}

PRIVATE void
set_fold_compound(vrna_fold_compound *vc,
                  vrna_md_t *md_p,
                  vrna_mx_t mx_type,
                  unsigned int mx_alloc_vector,
                  unsigned int options){


  char *sequence, **sequences;
  unsigned int        length, s;
  int                 cp;                     /* cut point for cofold */
  char                *seq, *seq2;

  sequence          = NULL;
  sequences         = NULL;
  cp                = -1;

  /* some default init values */
  vc->params        = NULL;
  vc->exp_params    = NULL;
  vc->matrices      = NULL;
  vc->exp_matrices  = NULL;
  vc->hc            = NULL;

  switch(vc->type){
    case VRNA_VC_TYPE_SINGLE:     sequence  = vc->sequence;

                                  seq2 = strdup(sequence);
                                  seq = vrna_cut_point_remove(seq2, &cp); /*  splice out the '&' if concatenated sequences and
                                                                        reset cp... this should also be safe for
                                                                        single sequences */
                                  vc->cutpoint            = cp;

                                  if((cp > 0) && (md_p->min_loop_size == TURN))
                                    md_p->min_loop_size = 0;  /* is it safe to set this here? */

                                  free(vc->sequence);
                                  vc->sequence            = seq;
                                  vc->length              = length = strlen(seq);
                                  vc->sequence_encoding   = vrna_seq_encode(seq, md_p);
                                  vc->sequence_encoding2  = vrna_seq_encode_simple(seq, md_p);
                                  if(!(options & VRNA_OPTION_EVAL_ONLY)){
                                    vc->ptype               = vrna_get_ptypes(vc->sequence_encoding2, md_p);
                                    /* backward compatibility ptypes */
                                    vc->ptype_pf_compat     = ((options & VRNA_OPTION_PF) || (mx_type == VRNA_MX_2DFOLD)) ? get_ptypes(vc->sequence_encoding2, md_p, 1) : NULL;
                                  } else {
                                    vc->ptype           = NULL;
                                    vc->ptype_pf_compat = NULL;
                                  }
                                  vc->sc                  = NULL;
                                  free(seq2);
                                  break;

    case VRNA_VC_TYPE_ALIGNMENT:  sequences     = vc->sequences;

                                  vc->length    = length = vc->length;

                                  vc->cons_seq  = consensus((const char **)sequences);
                                  vc->S_cons    = vrna_seq_encode_simple(vc->cons_seq, md_p);

                                  vc->pscore    = (int *) vrna_alloc(sizeof(int)*((length*(length+1))/2+2));

                                  oldAliEn = vc->oldAliEn  = md_p->oldAliEn;

                                  vc->S   = (short **)          vrna_alloc((vc->n_seq+1) * sizeof(short *));
                                  vc->S5  = (short **)          vrna_alloc((vc->n_seq+1) * sizeof(short *));
                                  vc->S3  = (short **)          vrna_alloc((vc->n_seq+1) * sizeof(short *));
                                  vc->a2s = (unsigned short **) vrna_alloc((vc->n_seq+1) * sizeof(unsigned short *));
                                  vc->Ss  = (char **)           vrna_alloc((vc->n_seq+1) * sizeof(char *));

                                  for (s = 0; s < vc->n_seq; s++) {
                                    vrna_ali_encode(vc->sequences[s],
                                                    &(vc->S[s]),
                                                    &(vc->S5[s]),
                                                    &(vc->S3[s]),
                                                    &(vc->Ss[s]),
                                                    &(vc->a2s[s]),
                                                    md_p);
                                  }
                                  vc->S5[vc->n_seq]  = NULL;
                                  vc->S3[vc->n_seq]  = NULL;
                                  vc->a2s[vc->n_seq] = NULL;
                                  vc->Ss[vc->n_seq]  = NULL;
                                  vc->S[vc->n_seq]   = NULL;

                                  vc->scs       = NULL;
                                  break;

    default:                      /* do nothing ? */
                                  break;
  }

  vc->iindx         = vrna_get_iindx(vc->length);
  vc->jindx         = vrna_get_indx(vc->length);

  /* now come the energy parameters */
  add_params(vc, md_p, options);

  if(!(options & VRNA_OPTION_EVAL_ONLY)){ /* allocate memory for DP matrices */

    if(options & VRNA_OPTION_MFE)
      add_mfe_matrices(vc, mx_type, mx_alloc_vector);

    if(options & VRNA_OPTION_PF)
      add_pf_matrices(vc, mx_type, mx_alloc_vector);
  }

  if(vc->type == VRNA_VC_TYPE_ALIGNMENT)
    make_pscores(vc);

  if(!(options & VRNA_OPTION_EVAL_ONLY))
    vrna_hc_init(vc); /* add default hard constraints */

}

PRIVATE void
add_pf_matrices(vrna_fold_compound *vc,
                vrna_mx_t type,
                unsigned int alloc_vector){

  if(vc){
    vc->exp_matrices  = get_pf_matrices_alloc(vc->length, type, alloc_vector);
    if(vc->exp_params->model_details.gquad){
      switch(vc->type){
        case VRNA_VC_TYPE_SINGLE:   vc->exp_matrices->G = get_gquad_pf_matrix(vc->sequence_encoding2, vc->exp_matrices->scale, vc->exp_params);
                                    break;
        default:                    /* do nothing */
                                    break;
      }
    }
    vrna_exp_params_rescale(vc, NULL);
  }
}

PRIVATE void
add_mfe_matrices( vrna_fold_compound *vc,
                  vrna_mx_t type,
                  unsigned int alloc_vector){

  if(vc){
    vc->matrices = get_mfe_matrices_alloc(vc->length, type, alloc_vector);

    if(vc->params->model_details.gquad){
      switch(vc->type){
        case VRNA_VC_TYPE_SINGLE:     vc->matrices->ggg = get_gquad_matrix(vc->sequence_encoding2, vc->params);
                                      break;
        case VRNA_VC_TYPE_ALIGNMENT:  vc->matrices->ggg = get_gquad_ali_matrix(vc->S_cons, vc->S, vc->n_seq,  vc->params);
                                      break;
        default:                      /* do nothing */
                                      break;
      }
    }
  }
}



PRIVATE vrna_mx_mfe_t  *
get_mfe_matrices_alloc( unsigned int n,
                        vrna_mx_t type,
                        unsigned int alloc_vector){

  unsigned int  i, size, lin_size;
  vrna_mx_mfe_t *vars;

  if(n >= (unsigned int)sqrt((double)INT_MAX))
    vrna_message_error("get_mfe_matrices_alloc@data_structures.c: sequence length exceeds addressable range");

  size          = ((n + 1) * (n + 2)) / 2;
  lin_size      = n + 2;
  vars          = (vrna_mx_mfe_t *)vrna_alloc(sizeof(vrna_mx_mfe_t));
  vars->length  = n;
  vars->type    = type;

  switch(type){
    case VRNA_MX_DEFAULT:   if(alloc_vector & ALLOC_F5)
                              vars->f5  = (int *) vrna_alloc(sizeof(int) * lin_size);
                            else
                              vars->f5  = NULL;

                            if(alloc_vector & ALLOC_F3)
                              vars->f3  = (int *) vrna_alloc(sizeof(int) * lin_size);
                            else
                              vars->f3  = NULL;

                            if(alloc_vector & ALLOC_HYBRID)
                              vars->fc  = (int *) vrna_alloc(sizeof(int) * lin_size);
                            else
                              vars->fc  = NULL;

                            if(alloc_vector & ALLOC_C)
                              vars->c   = (int *) vrna_alloc(sizeof(int) * size);
                            else
                              vars->c   = NULL;

                            if(alloc_vector & ALLOC_FML)
                              vars->fML = (int *) vrna_alloc(sizeof(int) * size);
                            else
                              vars->fML = NULL;

                            if(alloc_vector & ALLOC_UNIQ)
                              vars->fM1 = (int *) vrna_alloc(sizeof(int) * size);
                            else
                              vars->fM1 = NULL;

                            if(alloc_vector & ALLOC_CIRC)
                              vars->fM2 = (int *) vrna_alloc(sizeof(int) * lin_size);
                            else
                              vars->fM2 = NULL;

                            /* setting exterior loop energies for circular case to INF is always safe */
                            vars->FcH = vars->FcI = vars->FcM = vars->Fc = INF;

                            vars->ggg = NULL;

                            break;

    case VRNA_MX_2DFOLD:    if(alloc_vector & ALLOC_F5){
                              vars->E_F5      = (int ***) vrna_alloc(sizeof(int **) * lin_size);
                              vars->l_min_F5  = (int **)  vrna_alloc(sizeof(int *)  * lin_size);
                              vars->l_max_F5  = (int **)  vrna_alloc(sizeof(int *)  * lin_size);
                              vars->k_min_F5  = (int *)   vrna_alloc(sizeof(int)    * lin_size);
                              vars->k_max_F5  = (int *)   vrna_alloc(sizeof(int)    * lin_size);
                              vars->E_F5_rem  = (int *)   vrna_alloc(sizeof(int)    * lin_size);
                              for(i = 0; i <= n; i++)
                                vars->E_F5_rem[i] = INF;
                            } else {
                              vars->E_F5      = NULL;
                              vars->l_min_F5  = NULL;
                              vars->l_max_F5  = NULL;
                              vars->k_min_F5  = NULL;
                              vars->k_max_F5  = NULL;
                              vars->E_F5_rem  = NULL;
                            }

                            if(alloc_vector & ALLOC_F3){
                              vars->E_F3      = (int ***) vrna_alloc(sizeof(int **)  * lin_size);
                              vars->l_min_F3  = (int **)  vrna_alloc(sizeof(int *)   * lin_size);
                              vars->l_max_F3  = (int **)  vrna_alloc(sizeof(int *)   * lin_size);
                              vars->k_min_F3  = (int *)   vrna_alloc(sizeof(int)     * lin_size);
                              vars->k_max_F3  = (int *)   vrna_alloc(sizeof(int)     * lin_size);
                              vars->E_F3_rem  = (int *)   vrna_alloc(sizeof(int)    * lin_size);
                              for(i = 0; i <= n; i++)
                                vars->E_F3_rem[i] = INF;
                            } else {
                              vars->E_F3      = NULL;
                              vars->l_min_F3  = NULL;
                              vars->l_max_F3  = NULL;
                              vars->k_min_F3  = NULL;
                              vars->k_max_F3  = NULL;
                              vars->E_F3_rem  = NULL;
                            }

                            if(alloc_vector & ALLOC_C){
                              vars->E_C     = (int ***) vrna_alloc(sizeof(int **) * size);
                              vars->l_min_C = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->l_max_C = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->k_min_C = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->k_max_C = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->E_C_rem = (int *)   vrna_alloc(sizeof(int)    * size);
                              for(i = 0; i < size; i++)
                                vars->E_C_rem[i] = INF;
                            } else {
                              vars->E_C     = NULL;
                              vars->l_min_C = NULL;
                              vars->l_max_C = NULL;
                              vars->k_min_C = NULL;
                              vars->k_max_C = NULL;
                              vars->E_C_rem = NULL;
                            }

                            if(alloc_vector & ALLOC_FML){
                              vars->E_M     = (int ***) vrna_alloc(sizeof(int **) * size);
                              vars->l_min_M = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->l_max_M = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->k_min_M = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->k_max_M = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->E_M_rem = (int *)   vrna_alloc(sizeof(int)    * size);
                              for(i = 0; i < size; i++)
                                vars->E_M_rem[i] = INF;
                            } else {
                              vars->E_M     = NULL;
                              vars->l_min_M = NULL;
                              vars->l_max_M = NULL;
                              vars->k_min_M = NULL;
                              vars->k_max_M = NULL;
                              vars->E_M_rem = NULL;
                            }

                            if(alloc_vector & ALLOC_UNIQ){
                              vars->E_M1      = (int ***) vrna_alloc(sizeof(int **) * size);
                              vars->l_min_M1  = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->l_max_M1  = (int **)  vrna_alloc(sizeof(int *)  * size);
                              vars->k_min_M1  = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->k_max_M1  = (int *)   vrna_alloc(sizeof(int)    * size);
                              vars->E_M1_rem  = (int *)   vrna_alloc(sizeof(int)    * size);
                              for(i = 0; i < size; i++)
                                vars->E_M1_rem[i] = INF;
                            } else {
                              vars->E_M1      = NULL;
                              vars->l_min_M1  = NULL;
                              vars->l_max_M1  = NULL;
                              vars->k_min_M1  = NULL;
                              vars->k_max_M1  = NULL;
                              vars->E_M1_rem  = NULL;
                            }

                            if(alloc_vector & ALLOC_CIRC){
                              vars->E_M2      = (int ***) vrna_alloc(sizeof(int **)  * lin_size);
                              vars->l_min_M2  = (int **)  vrna_alloc(sizeof(int *)   * lin_size);
                              vars->l_max_M2  = (int **)  vrna_alloc(sizeof(int *)   * lin_size);
                              vars->k_min_M2  = (int *)   vrna_alloc(sizeof(int)     * lin_size);
                              vars->k_max_M2  = (int *)   vrna_alloc(sizeof(int)     * lin_size);
                              vars->E_M2_rem  = (int *)   vrna_alloc(sizeof(int)     * lin_size);
                              for(i = 0; i <= n; i++)
                                vars->E_M2_rem[i] = INF;
                            } else {
                              vars->E_M2      = NULL;
                              vars->l_min_M2  = NULL;
                              vars->l_max_M2  = NULL;
                              vars->k_min_M2  = NULL;
                              vars->k_max_M2  = NULL;
                              vars->E_M2_rem  = NULL;
                            }

                            /* setting exterior loop energies for circular case to INF is always safe */
                            vars->E_Fc      = NULL;
                            vars->E_FcH     = NULL;
                            vars->E_FcI     = NULL;
                            vars->E_FcM     = NULL;
                            vars->E_Fc_rem  = INF;
                            vars->E_FcH_rem = INF;
                            vars->E_FcI_rem = INF;
                            vars->E_FcM_rem = INF;

#ifdef COUNT_STATES
                            vars->N_C   = (unsigned long ***) vrna_alloc(sizeof(unsigned long **)  * size);
                            vars->N_F5  = (unsigned long ***) vrna_alloc(sizeof(unsigned long **)  * lin_size);
                            vars->N_M   = (unsigned long ***) vrna_alloc(sizeof(unsigned long **)  * size);
                            vars->N_M1  = (unsigned long ***) vrna_alloc(sizeof(unsigned long **)  * size);
#endif

                            break;
    default:                /* do nothing */
                            break;
  }

  return vars;
}



PRIVATE vrna_mx_pf_t  *
get_pf_matrices_alloc(unsigned int n,
                      vrna_mx_t type,
                      unsigned int alloc_vector){

  unsigned int  i, size, lin_size;
  vrna_mx_pf_t  *vars;

  if(n >= (unsigned int)sqrt((double)INT_MAX))
    vrna_message_error("get_pf_matrices_alloc@data_structures.c: sequence length exceeds addressable range");

  size          = ((n + 1) * (n + 2)) / 2;
  lin_size      = n + 2;
  vars          = (vrna_mx_pf_t *)vrna_alloc(sizeof(vrna_mx_pf_t));
  vars->length  = n;
  vars->type    = type;


  switch(type){
    case VRNA_MX_DEFAULT:   if(alloc_vector & ALLOC_F)
                              vars->q = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * size);
                            else
                              vars->q = NULL;

                            if(alloc_vector & ALLOC_C)
                              vars->qb  = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * size);
                            else
                              vars->qb = NULL;

                            if(alloc_vector & ALLOC_FML)
                              vars->qm    = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * size);
                            else
                              vars->qm = NULL;

                            if(alloc_vector & ALLOC_UNIQ)
                              vars->qm1   = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * size);
                            else
                              vars->qm1 = NULL;

                            if(alloc_vector & ALLOC_CIRC)
                              vars->qm2   = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * lin_size);
                            else
                              vars->qm2 = NULL;

                            if(alloc_vector & ALLOC_PROBS)
                              vars->probs = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * size);
                            else
                              vars->probs = NULL;

                            if(alloc_vector & ALLOC_AUX){
                              vars->q1k   = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * lin_size);
                              vars->qln   = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * lin_size);
                            } else {
                              vars->q1k = NULL;
                              vars->qln = NULL;
                            }

                            break;

    case VRNA_MX_2DFOLD:    if(alloc_vector & ALLOC_F){
                              vars->Q       = (FLT_OR_DBL ***)vrna_alloc(sizeof(FLT_OR_DBL **)  * size);
                              vars->l_min_Q = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->l_max_Q = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->k_min_Q = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->k_max_Q = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->Q_rem   = (FLT_OR_DBL *)  vrna_alloc(sizeof(FLT_OR_DBL)     * size);
                            } else {
                              vars->Q       = NULL;
                              vars->l_min_Q = NULL;
                              vars->l_max_Q = NULL;
                              vars->k_min_Q = NULL;
                              vars->k_max_Q = NULL;
                              vars->Q_rem   = NULL;
                            }

                            if(alloc_vector & ALLOC_C){
                              vars->Q_B       = (FLT_OR_DBL ***)vrna_alloc(sizeof(FLT_OR_DBL **)  * size);
                              vars->l_min_Q_B = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->l_max_Q_B = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->k_min_Q_B = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->k_max_Q_B = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->Q_B_rem   = (FLT_OR_DBL *)  vrna_alloc(sizeof(FLT_OR_DBL)     * size);
                            } else {
                              vars->Q_B       = NULL;
                              vars->l_min_Q_B = NULL;
                              vars->l_max_Q_B = NULL;
                              vars->k_min_Q_B = NULL;
                              vars->k_max_Q_B = NULL;
                              vars->Q_B_rem   = NULL;
                            }

                            if(alloc_vector & ALLOC_FML){
                              vars->Q_M       = (FLT_OR_DBL ***)vrna_alloc(sizeof(FLT_OR_DBL **)  * size);
                              vars->l_min_Q_M = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->l_max_Q_M = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->k_min_Q_M = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->k_max_Q_M = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->Q_M_rem   = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL)      * size);
                            } else {
                              vars->Q_M       = NULL;
                              vars->l_min_Q_M = NULL;
                              vars->l_max_Q_M = NULL;
                              vars->k_min_Q_M = NULL;
                              vars->k_max_Q_M = NULL;
                              vars->Q_M_rem   = NULL;
                            }

                            if(alloc_vector & ALLOC_UNIQ){
                              vars->Q_M1        = (FLT_OR_DBL ***)vrna_alloc(sizeof(FLT_OR_DBL **)  * size);
                              vars->l_min_Q_M1  = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->l_max_Q_M1  = (int **)        vrna_alloc(sizeof(int *)          * size);
                              vars->k_min_Q_M1  = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->k_max_Q_M1  = (int *)         vrna_alloc(sizeof(int)            * size);
                              vars->Q_M1_rem    = (FLT_OR_DBL *)  vrna_alloc(sizeof(FLT_OR_DBL)     * size);
                            } else {
                              vars->Q_M1        = NULL;
                              vars->l_min_Q_M1  = NULL;
                              vars->l_max_Q_M1  = NULL;
                              vars->k_min_Q_M1  = NULL;
                              vars->k_max_Q_M1  = NULL;
                              vars->Q_M1_rem    = NULL;
                            }

                            if(alloc_vector & ALLOC_CIRC){
                              vars->Q_M2        = (FLT_OR_DBL ***)vrna_alloc(sizeof(FLT_OR_DBL **)  * lin_size);
                              vars->l_min_Q_M2  = (int **)        vrna_alloc(sizeof(int *)          * lin_size);
                              vars->l_max_Q_M2  = (int **)        vrna_alloc(sizeof(int *)          * lin_size);
                              vars->k_min_Q_M2  = (int *)         vrna_alloc(sizeof(int)            * lin_size);
                              vars->k_max_Q_M2  = (int *)         vrna_alloc(sizeof(int)            * lin_size);
                              vars->Q_M2_rem    = (FLT_OR_DBL *)  vrna_alloc(sizeof(FLT_OR_DBL)     * lin_size);
                            }
                            else{
                              vars->Q_M2_rem    = NULL;
                              vars->Q_M2        = NULL;
                              vars->l_min_Q_M2  = NULL;
                              vars->l_max_Q_M2  = NULL;
                              vars->k_min_Q_M2  = NULL;
                              vars->k_max_Q_M2  = NULL;
                            }

                            vars->Q_c       = NULL;
                            vars->Q_cH      = NULL;
                            vars->Q_cI      = NULL;
                            vars->Q_cM      = NULL;
                            vars->Q_c_rem   = 0.;
                            vars->Q_cH_rem  = 0.;
                            vars->Q_cI_rem  = 0.;
                            vars->Q_cM_rem  = 0.;

                            break;

    default:                /* do nothing */
                            break;
  }

  /*
      always alloc the helper arrays for unpaired nucleotides in multi-
      branch loops and scaling
  */
  vars->scale     = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * lin_size);
  vars->expMLbase = (FLT_OR_DBL *) vrna_alloc(sizeof(FLT_OR_DBL) * lin_size);

  return vars;
}

PRIVATE void
make_pscores(vrna_fold_compound *vc){

  /* calculate co-variance bonus for each pair depending on  */
  /* compensatory/consistent mutations and incompatible seqs */
  /* should be 0 for conserved pairs, >0 for good pairs      */

#define NONE -10000 /* score for forbidden pairs */

  char *structure = NULL;
  int i,j,k,l,s, max_span;
  float **dm;
  int olddm[7][7]={{0,0,0,0,0,0,0}, /* hamming distance between pairs */
                  {0,0,2,2,1,2,2} /* CG */,
                  {0,2,0,1,2,2,2} /* GC */,
                  {0,2,1,0,2,1,2} /* GU */,
                  {0,1,2,2,0,2,1} /* UG */,
                  {0,2,2,1,2,0,2} /* AU */,
                  {0,2,2,2,1,2,0} /* UA */};

  short           **S         = vc->S;
  char            **AS        = vc->sequences;
  int             n_seq       = vc->n_seq;
  vrna_md_t       *md         = (vc->params) ? &(vc->params->model_details) : &(vc->exp_params->model_details);
  int             *pscore     = vc->pscore;     /* precomputed array of pair types */             
  int             *indx       = vc->jindx;                                             
  int             n           = vc->length;                                            

  if (md->ribo) {
    if (RibosumFile !=NULL) dm=readribosum(RibosumFile);
    else dm=get_ribosum((const char **)AS, n_seq, n);
  }
  else { /*use usual matrix*/
    dm=(float **)vrna_alloc(7*sizeof(float*));
    for (i=0; i<7;i++) {
      dm[i]=(float *)vrna_alloc(7*sizeof(float));
      for (j=0; j<7; j++)
        dm[i][j] = (float) olddm[i][j];
    }
  }

  max_span = md->max_bp_span;
  if((max_span < TURN+2) || (max_span > n))
    max_span = n;
  for (i=1; i<n; i++) {
    for (j=i+1; (j<i+TURN+1) && (j<=n); j++)
      pscore[indx[j]+i] = NONE;
    for (j=i+TURN+1; j<=n; j++) {
      int pfreq[8]={0,0,0,0,0,0,0,0};
      double score;
      for (s=0; s<n_seq; s++) {
        int type;
        if (S[s][i]==0 && S[s][j]==0) type = 7; /* gap-gap  */
        else {
          if ((AS[s][i] == '~')||(AS[s][j] == '~')) type = 7;
          else type = md->pair[S[s][i]][S[s][j]];
        }
        pfreq[type]++;
      }
      if (pfreq[0]*2+pfreq[7]>n_seq) { pscore[indx[j]+i] = NONE; continue;}
      for (k=1,score=0; k<=6; k++) /* ignore pairtype 7 (gap-gap) */
        for (l=k; l<=6; l++)
          score += pfreq[k]*pfreq[l]*dm[k][l];
      /* counter examples score -1, gap-gap scores -0.25   */
      pscore[indx[j]+i] = md->cv_fact *
        ((UNIT*score)/n_seq - md->nc_fact*UNIT*(pfreq[0] + pfreq[7]*0.25));

      if((j - i + 1) > max_span){
        pscore[indx[j]+i] = NONE;
      }
    }
  }

  if (md->noLP) /* remove unwanted pairs */
    for (k=1; k<n-TURN-1; k++)
      for (l=1; l<=2; l++) {
        int type,ntype=0,otype=0;
        i=k; j = i+TURN+l;
        type = pscore[indx[j]+i];
        while ((i>=1)&&(j<=n)) {
          if ((i>1)&&(j<n)) ntype = pscore[indx[j+1]+i-1];
          if ((otype<md->cv_fact*MINPSCORE)&&(ntype<md->cv_fact*MINPSCORE))  /* too many counterexamples */
            pscore[indx[j]+i] = NONE; /* i.j can only form isolated pairs */
          otype =  type;
          type  = ntype;
          i--; j++;
        }
      }


  if (fold_constrained&&(structure!=NULL)) {
    int psij, hx, hx2, *stack, *stack2;
    stack = (int *) vrna_alloc(sizeof(int)*(n+1));
    stack2 = (int *) vrna_alloc(sizeof(int)*(n+1));

    for(hx=hx2=0, j=1; j<=n; j++) {
      switch (structure[j-1]) {
      case 'x': /* can't pair */
        for (l=1; l<j-TURN; l++) pscore[indx[j]+l] = NONE;
        for (l=j+TURN+1; l<=n; l++) pscore[indx[l]+j] = NONE;
        break;
      case '(':
        stack[hx++]=j;
        /* fallthrough */
      case '[':
        stack2[hx2++]=j;
        /* fallthrough */
      case '<': /* pairs upstream */
        for (l=1; l<j-TURN; l++) pscore[indx[j]+l] = NONE;
        break;
      case ']':
        if (hx2<=0) {
          fprintf(stderr, "%s\n", structure);
          vrna_message_error("unbalanced brackets in constraints");
        }
        i = stack2[--hx2];
        pscore[indx[j]+i]=NONE;
        break;
      case ')':
        if (hx<=0) {
          fprintf(stderr, "%s\n", structure);
          vrna_message_error("unbalanced brackets in constraints");
        }
        i = stack[--hx];
        psij = pscore[indx[j]+i]; /* store for later */
        for (k=j; k<=n; k++)
          for (l=i; l<=j; l++)
            pscore[indx[k]+l] = NONE;
        for (l=i; l<=j; l++)
          for (k=1; k<=i; k++)
            pscore[indx[l]+k] = NONE;
        for (k=i+1; k<j; k++)
          pscore[indx[k]+i] = pscore[indx[j]+k] = NONE;
        pscore[indx[j]+i] = (psij>0) ? psij : 0;
        /* fallthrough */
      case '>': /* pairs downstream */
        for (l=j+TURN+1; l<=n; l++) pscore[indx[l]+j] = NONE;
        break;
      }
    }
    if (hx!=0) {
      fprintf(stderr, "%s\n", structure);
      vrna_message_error("unbalanced brackets in constraint string");
    }
    free(stack); free(stack2);
  }
  /*free dm */
  for (i=0; i<7;i++) {
    free(dm[i]);
  }
  free(dm);
}
