#ifndef __VIENNA_RNA_PACKAGE_MEA_H__
#define __VIENNA_RNA_PACKAGE_MEA_H__

#include "data_structures.h"

/**
*** \brief Computes a MEA (maximum expected accuracy) structure.
***
*** Computes a MEA (maximum expected accuracy) structure, such that expected
*** accuracy \f[ A(S) = \sum_{(i,j) \in S} 2 \gamma p_{ij} + \sum_{i \notin S} p^u_i} \f]
*** is maximised. Higher values of \f$\gamma\f$ result in more base pairs of lower
*** probability and thus higher sensitivity. Low values of gamm result in structures
*** containing only highly likely pairs (high specificity).
*** The code of the MEA function also demonstrates the use of sparse dynamic
*** programming scheme to reduce the time and memory complexity of folding.
**/
float MEA(plist *p, char *structure, double gamma);

#endif
