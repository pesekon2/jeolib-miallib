#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mialib.h"


/** \addtogroup group_stat
 *  @{
 */

/* from Numerical Recipes in C */
void indexx(int n, double arrin[], int indx[])
{
  int l,j,ir,indxt,i;
  double q;

  for (j=1;j<=n;j++) indx[j]=j;
  if (n == 1) return;
  l=(n >> 1) + 1;
  ir=n;
  for (;;){
    if (l > 1)
      q=arrin[(indxt=indx[--l])];
    else{
      q=arrin[(indxt=indx[ir])];
      indx[ir]=indx[1];
      if (--ir == 1){
	indx[1]=indxt;
	return;
      }
    }
    i=l;
    j=l << 1;
    while (j <= ir){
      if (j < ir && arrin[indx[j]] < arrin[indx[j+1]]) j++;
      if (q < arrin[indx[j]]){
	indx[i]=indx[j];
	j += (i=j);
      }
      else j=ir+1;
    }
    indx[i]=indxt;
  }
}

/* from Numerical Recipes in C */
void f_indexx(int n, float arrin[], int indx[])
{
  int l,j,ir,indxt,i;
  float q;

  for (j=1;j<=n;j++) indx[j]=j; 
  if (n == 1) return;
  l=(n >> 1) + 1;
  ir=n;
  for (;;){
    if (l > 1)
      q=arrin[(indxt=indx[--l])];
    else{
      q=arrin[(indxt=indx[ir])];
      indx[ir]=indx[1];
      if (--ir == 1){
	indx[1]=indxt;
	return;
      }
    }
    i=l;
    j=l << 1;
    while (j <= ir){
      if (j < ir && arrin[indx[j]] < arrin[indx[j+1]]) j++;
      if (q < arrin[indx[j]]){
	indx[i]=indx[j];
	j += (i=j);
      }
      else j=ir+1;
    }
    indx[i]=indxt;
  }
}

/**
  @param i0 an image pointer
  @return a 1-D image (INT32 type) with the first pixel giving the offset to the smallest value of im, the second pixel the second smallest value, etc.  The sorting is based on the indexx() function of the numerical recipes in C \cite press-etal92.
*/
IMAGE *sortindex(IMAGE *i0)
{
  IMAGE *itmp, *imidx;
  int flag=0;

  if (GetImDataType(i0) != t_FLOAT){
    itmp=to_float(i0);
    flag=1;
    if (itmp==NULL)
      return NULL;
  }
  else
    itmp=i0;

  imidx=(IMAGE *)create_image(t_INT32, GetImNPix(i0), 1 , 1);
  if (imidx==NULL){
    (void)sprintf(buf,"sortindex(): insufficient memory\n"); errputstr(buf);
    if (flag)
      free_image(itmp);
    return NULL;
  }   

  f_indexx(GetImNPix(i0), ((MIAFLOAT *)GetImPtr(itmp))-1, ((int *)GetImPtr(imidx))-1);
  if (flag)
    free_image(itmp);
  return imidx;
}

/*@}*/
