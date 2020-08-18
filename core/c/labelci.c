/**
 * @file   labelci.c
 * @author Pierre Soille
 * @date   
 * 
 * @details see also \cite soille2008pami
 */





#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "miallib.h"
#include "pqueue.h"
#include "fifo.h"
#include "op.h"


#include "uc_def.h"
#define CC_LBL_TYPE UINT32
#define t_CC_LBL_TYPE t_UINT32
#define BORDER_VAL        0x80000000
#define FIRST_LBL         0x00000200 /* 512 */
#define LBL_BIT           0x7FFFFE00
#define BORDER_OR_LBL_BIT 0x8FFFFE00
#define R_BIT             0x000000FF
IMAGE *uc_labelci(IMAGE *im, IMAGE *imse, int ox, int oy, int oz, int rl)
{
  /* First 2006-07-31: after much reading and thinking */
  PIX_TYPE *p, valcrt;
  IMAGE *imlbl;
  CC_LBL_TYPE *plbl, lbl=FIRST_LBL;
  unsigned long int npix, i, j, ofs, ofsq, ofsk;
  int rk, rcrt=0, rlcrt, prio;
  long int  k, *shft;
  
  FIFO4 *q;
  int n, nx, ny, nz;
  int box[BOXELEM];
  
  PQDATUM apqd[1];
  struct node *pqd;
  struct pqueue *pq;
  
  nx = GetImNx(im);
  ny = GetImNy(im);
  nz = GetImNz(im);
  npix=nx*ny*nz;

  imlbl= (IMAGE *)create_image(t_CC_LBL_TYPE, nx, ny, nz);
  if (imlbl == NULL){
    (void)sprintf(buf,"_labelcc(): not enough memory!\n"); errputstr(buf);
    return NULL;
  }
  pq = pqinit(NULL, 10000);  /* priority queue */
  if (pq == NULL){
    free_image(imlbl);
    return NULL;
  }
  q = create_fifo4(500); 
  if (q == NULL){
    free_image(imlbl);
    free_pq(pq);
    return NULL;
  }

  n = objectpix(imse);
  if (n==ERROR)
    return NULL;
  shft = (long int *)calloc(n, sizeof(long int));
  if (shft == NULL){
    free_image(imlbl);
    free_pq(pq);
    free_fifo4(q);
    return NULL;
  }

  /*  Take SE  into account  */
  box[0] = GetImNx(imse);
  box[1] = GetImNy(imse);
  box[2] = GetImNz(imse);
  box[3] = ox;
  box[4] = oy;
  box[5] = oz;
  set_shift_and_box((UCHAR *)GetImPtr(imse), box, GetImNx(im), GetImNy(im), shft);
  
  if (u32_framebox(imlbl,box,BORDER_VAL)==ERROR){
    free_image(imlbl);
    free_pq(pq);
    free_fifo4(q);
    free((char*)shft);
    return NULL;
  }
  i32_arithcst(imlbl,R_BIT+1,OR_op);

  /* Here we go */
  p   = (PIX_TYPE *)GetImPtr(im);
  plbl= (CC_LBL_TYPE *)GetImPtr(imlbl);
  for (i=0;i<npix;i++){
    if (plbl[i]<FIRST_LBL){ /* not yet labelled */
      rlcrt=rl;
      plbl[i]=lbl;
      /* init queue */
      for (k=0; k<n; k++){
	ofsk=i+shft[k];
	if (plbl[ofsk]&BORDER_OR_LBL_BIT){
	  if (plbl[ofsk]&BORDER_VAL){
	    continue;
	  }
	  if (rlcrt>=abs(p[i]-p[ofsk]))
	    rlcrt=abs(p[i]-p[ofsk])-1;
	  continue;
	}
	rk=abs(p[i]-p[ofsk]);
	if (rk>rlcrt)
	  continue;
	plbl[ofsk]=rk;
	pqd = (PQDATUM )malloc(sizeof(struct node));
	pqd->prio = rk;
	pqd->offset= (long int)ofsk;
	pqmininsert(pq, pqd);	
      }
      // printf("after init, lbl=%d, rlcrt=%d, rcrt=%d\n", (int)lbl, rlcrt, rcrt);

      fifo4_lookreset(q);
      while ( (ofsq=fifo4_look(q)) != 0){
	printf("!!!!!!!!!!!!!! stack not empty!!!\n");
      }
      /* here we go */
      if( pqpeek(pq, apqd) != NULL)
	rcrt=apqd[0]->prio;      
      while (pqpeek(pq, apqd) != NULL){
/* 	if (i>=33 && i<=40){ */
/* 	  printf("lbl=%d, i=%d\n", lbl, i); */
/*           printf("lbl=%d, rlcrt=%d, rcrt=%d\n", (int)lbl, rlcrt, rcrt); */

/* 	  i32_dumpxyz(imlbl,0,0,0,20,20); */
/* 	} */
    
	pqminremove(pq, apqd);
	ofs=apqd[0]->offset;
	prio=apqd[0]->prio;
        free((char*) *apqd);

	if (plbl[ofs]&LBL_BIT)
	    continue;

	if (rcrt>rlcrt){
	  fifo4_add(q, (long int)ofs);
	  goto myreset;
	}
	if (prio>rcrt){ /* nth 'layer' done */
	  /* first look for largest edge weight in CC (wmax) */
	  fifo4_lookreset(q);
	  while ( (ofsq=fifo4_look(q)) != 0){
	    valcrt=p[ofsq];
	    for (k=0; k<n; k++){
	      ofsk=ofsq+shft[k];
	      // printf("rcrt=%d plbl[%d]=%d \n", rcrt, ofsk,plbl[ofsk]);
	      if ( (plbl[ofsk]<=rcrt) || (plbl[ofsk]==lbl) ){ /* pixel in current CC */
		if (abs(valcrt-p[ofsk])>rl){ /* ci<1 */
	          fifo4_add(q, (long int)ofs);
		  goto myreset;
		}
	      }
	    }
	  }
	  
	  /* ci=1 set current r-CC to lbl and proceed */
	  while ( (ofsq=fifo4_remove(q)) != 0)
	    plbl[ofsq]=lbl;
	  rcrt=prio;
	  if (plbl[ofs]&LBL_BIT)
	    continue;
	}

	fifo4_add(q, (long int)ofs);

	if (rcrt>rlcrt){ /* reset */
	myreset:
  
	  for (j=1; j<pq->size; j++){
	    if (pq->d[j] != NULL){
	      ofsq=pq->d[j]->offset;
	      if(!(plbl[ofsq]&LBL_BIT)){
		plbl[ofsq]=R_BIT+1;
	      }
	      free((char*) (pq->d[j]) );
	    }
	  }
	  pq->size=1;

	  while ( (ofsq=fifo4_remove(q)) != 0){
	    if (plbl[ofsq]&LBL_BIT){
	      printf("SHOULD NEVER HAPPEN!\n");
	      continue;
	    }
	    plbl[ofsq]=R_BIT+1;  // rcrt;
	  }
	  break;
	}  // end reset

	for (k=0; k<n; k++){
	  ofsk=ofs+shft[k];
	  if (plbl[ofsk]&BORDER_OR_LBL_BIT){
	    if (plbl[ofsk]&BORDER_VAL){
	      continue;
	    }
	    if (plbl[ofsk]!=lbl){
	      if (rlcrt>=abs(p[ofs]-p[ofsk])){
		rlcrt=abs(p[ofs]-p[ofsk])-1;
		if (rcrt>rlcrt)
		  goto myreset;
	      }
	    }
	    continue;
	  }
	  rk=abs(p[ofs]-p[ofsk]);
	  if (rk>rlcrt)
	    continue;
	  if (rk<plbl[ofsk]){
            plbl[ofsk]=rk;
	    pqd = (PQDATUM )malloc(sizeof(struct node));
	    pqd->prio = rk;
	    pqd->offset= (long int)ofsk;
	    pqmininsert(pq, pqd);
	  }
	}
      }
      fifo4_lookreset(q);
      while ( (ofsq=fifo4_look(q)) != 0){
	valcrt=p[ofsq];
	for (k=0; k<n; k++){
	  ofsk=ofsq+shft[k];
	  if ( (plbl[ofsk]<=rcrt) || (plbl[ofsk]==lbl) ){
	    if (abs(valcrt-p[ofsk])>rl){ /* ci<1 */
	      goto myreset2;
	    }
	  }
	}
      }
      while ( (ofsq=fifo4_remove(q)) != 0){
	plbl[ofsq]=lbl;
      }
myreset2:
      while ( (ofsq=fifo4_remove(q)) != 0){
	plbl[ofsq]=R_BIT+1;
      }
      lbl++;
    }
  }
  free_pq(pq);
  free_fifo4(q);
  free((char*)shft);
  for (i=0;i<npix;i++) /* set first label to 1 */
    plbl[i]-=511;
  u32_framebox(imlbl,box,0);
  return(imlbl);
}
#undef CC_LBL_TYPE
#undef t_CC_LBL_TYPE
#undef BORDER_VAL
#include "uc_undef.h"

#include "us_def.h"
#define CC_LBL_TYPE UINT32
#define t_CC_LBL_TYPE t_UINT32
#define BORDER_VAL        0x80000000
#define FIRST_LBL         0x00000200 /* 512 */
#define LBL_BIT           0x7FFFFE00
#define BORDER_OR_LBL_BIT 0x8FFFFE00
#define R_BIT             0x000000FF
IMAGE *us_labelci(IMAGE *im, IMAGE *imse, int ox, int oy, int oz, int rl)
{
  /* First 2006-07-31: after much reading and thinking */
  PIX_TYPE *p, valcrt;
  IMAGE *imlbl;
  CC_LBL_TYPE *plbl, lbl=FIRST_LBL;
  unsigned long int npix, i, j, ofs, ofsq, ofsk;
  int rk, rcrt=0, rlcrt, prio;
  long int  k, *shft;
  
  FIFO4 *q;
  int n, nx, ny, nz;
  int box[BOXELEM];
  
  PQDATUM apqd[1];
  struct node *pqd;
  struct pqueue *pq;
  
  nx = GetImNx(im);
  ny = GetImNy(im);
  nz = GetImNz(im);
  npix=nx*ny*nz;

  imlbl= (IMAGE *)create_image(t_CC_LBL_TYPE, nx, ny, nz);
  if (imlbl == NULL){
    (void)sprintf(buf,"_labelcc(): not enough memory!\n"); errputstr(buf);
    return NULL;
  }
  pq = pqinit(NULL, 10000);  /* priority queue */
  if (pq == NULL){
    free_image(imlbl);
    return NULL;
  }
  q = create_fifo4(500); 
  if (q == NULL){
    free_image(imlbl);
    free_pq(pq);
    return NULL;
  }

  n = objectpix(imse);
  if (n==ERROR)
    return NULL;
  shft = (long int *)calloc(n, sizeof(long int));
  if (shft == NULL){
    free_image(imlbl);
    free_pq(pq);
    free_fifo4(q);
    return NULL;
  }

  /*  Take SE  into account  */
  box[0] = GetImNx(imse);
  box[1] = GetImNy(imse);
  box[2] = GetImNz(imse);
  box[3] = ox;
  box[4] = oy;
  box[5] = oz;
  set_shift_and_box((UCHAR *)GetImPtr(imse), box, GetImNx(im), GetImNy(im), shft);
  
  if (u32_framebox(imlbl,box,BORDER_VAL)==ERROR){
    free_image(imlbl);
    free_pq(pq);
    free_fifo4(q);
    free((char*)shft);
    return NULL;
  }
  i32_arithcst(imlbl,R_BIT+1,OR_op);

  /* Here we go */
  p   = (PIX_TYPE *)GetImPtr(im);
  plbl= (CC_LBL_TYPE *)GetImPtr(imlbl);
  for (i=0;i<npix;i++){
    if (plbl[i]<FIRST_LBL){ /* not yet labelled */
      rlcrt=rl;
      plbl[i]=lbl;
      /* init queue */
      for (k=0; k<n; k++){
	ofsk=i+shft[k];
	if (plbl[ofsk]&BORDER_OR_LBL_BIT){
	  if (plbl[ofsk]&BORDER_VAL){
	    continue;
	  }
	  if (rlcrt>=abs(p[i]-p[ofsk]))
	    rlcrt=abs(p[i]-p[ofsk])-1;
	  continue;
	}
	rk=abs(p[i]-p[ofsk]);
	if (rk>rlcrt)
	  continue;
	plbl[ofsk]=rk;
	pqd = (PQDATUM )malloc(sizeof(struct node));
	pqd->prio = rk;
	pqd->offset= (long int)ofsk;
	pqmininsert(pq, pqd);	
      }
      // printf("after init, lbl=%d, rlcrt=%d, rcrt=%d\n", (int)lbl, rlcrt, rcrt);

      fifo4_lookreset(q);
      while ( (ofsq=fifo4_look(q)) != 0){
	printf("!!!!!!!!!!!!!! stack not empty!!!\n");
      }
      /* here we go */
      if( pqpeek(pq, apqd) != NULL)
	rcrt=apqd[0]->prio;      
      while (pqpeek(pq, apqd) != NULL){
/* 	if (i>=33 && i<=40){ */
/* 	  printf("lbl=%d, i=%d\n", lbl, i); */
/*           printf("lbl=%d, rlcrt=%d, rcrt=%d\n", (int)lbl, rlcrt, rcrt); */

/* 	  i32_dumpxyz(imlbl,0,0,0,20,20); */
/* 	} */
    
	pqminremove(pq, apqd);
	ofs=apqd[0]->offset;
	prio=apqd[0]->prio;
        free((char*) *apqd);

	if (plbl[ofs]&LBL_BIT)
	    continue;

	if (rcrt>rlcrt){
	  fifo4_add(q, (long int)ofs);
	  goto myreset;
	}
	if (prio>rcrt){ /* nth 'layer' done */
	  /* first look for largest edge weight in CC (wmax) */
	  fifo4_lookreset(q);
	  while ( (ofsq=fifo4_look(q)) != 0){
	    valcrt=p[ofsq];
	    for (k=0; k<n; k++){
	      ofsk=ofsq+shft[k];
	      // printf("rcrt=%d plbl[%d]=%d \n", rcrt, ofsk,plbl[ofsk]);
	      if ( (plbl[ofsk]<=rcrt) || (plbl[ofsk]==lbl) ){ /* pixel in current CC */
		if (abs(valcrt-p[ofsk])>rl){ /* ci<1 */
	          fifo4_add(q, (long int)ofs);
		  goto myreset;
		}
	      }
	    }
	  }
	  
	  /* ci=1 set current r-CC to lbl and proceed */
	  while ( (ofsq=fifo4_remove(q)) != 0)
	    plbl[ofsq]=lbl;
	  rcrt=prio;
	  if (plbl[ofs]&LBL_BIT)
	    continue;
	}

	fifo4_add(q, (long int)ofs);

	if (rcrt>rlcrt){ /* reset */
	myreset:
  
	  for (j=1; j<pq->size; j++){
	    if (pq->d[j] != NULL){
	      ofsq=pq->d[j]->offset;
	      if(!(plbl[ofsq]&LBL_BIT)){
		plbl[ofsq]=R_BIT+1;
	      }
	      free((char*) (pq->d[j]) );
	    }
	  }
	  pq->size=1;

	  while ( (ofsq=fifo4_remove(q)) != 0){
	    if (plbl[ofsq]&LBL_BIT){
	      printf("SHOULD NEVER HAPPEN!\n");
	      continue;
	    }
	    plbl[ofsq]=R_BIT+1;  // rcrt;
	  }
	  break;
	}  // end reset

	for (k=0; k<n; k++){
	  ofsk=ofs+shft[k];
	  if (plbl[ofsk]&BORDER_OR_LBL_BIT){
	    if (plbl[ofsk]&BORDER_VAL){
	      continue;
	    }
	    if (plbl[ofsk]!=lbl){
	      if (rlcrt>=abs(p[ofs]-p[ofsk])){
		rlcrt=abs(p[ofs]-p[ofsk])-1;
		if (rcrt>rlcrt)
		  goto myreset;
	      }
	    }
	    continue;
	  }
	  rk=abs(p[ofs]-p[ofsk]);
	  if (rk>rlcrt)
	    continue;
	  if (rk<plbl[ofsk]){
            plbl[ofsk]=rk;
	    pqd = (PQDATUM )malloc(sizeof(struct node));
	    pqd->prio = rk;
	    pqd->offset= (long int)ofsk;
	    pqmininsert(pq, pqd);
	  }
	}
      }
      fifo4_lookreset(q);
      while ( (ofsq=fifo4_look(q)) != 0){
	valcrt=p[ofsq];
	for (k=0; k<n; k++){
	  ofsk=ofsq+shft[k];
	  if ( (plbl[ofsk]<=rcrt) || (plbl[ofsk]==lbl) ){
	    if (abs(valcrt-p[ofsk])>rl){ /* ci<1 */
	      goto myreset2;
	    }
	  }
	}
      }
      while ( (ofsq=fifo4_remove(q)) != 0){
	plbl[ofsq]=lbl;
      }
myreset2:
      while ( (ofsq=fifo4_remove(q)) != 0){
	plbl[ofsq]=R_BIT+1;
      }
      lbl++;
    }
  }
  free_pq(pq);
  free_fifo4(q);
  free((char*)shft);
  for (i=0;i<npix;i++) /* set first label to 1 */
    plbl[i]-=511;
  u32_framebox(imlbl,box,0);
  return(imlbl);
}
#undef CC_LBL_TYPE
#undef t_CC_LBL_TYPE
#undef BORDER_VAL
#include "us_undef.h"

IMAGE *labelci(IMAGE *im, IMAGE *imse, int ox, int oy, int oz, int rl)
{
  switch (GetImDataType(im)){

  case t_UCHAR:
    return(uc_labelci(im,imse,ox,oy,oz,rl));
    break;
    
  case t_USHORT:
    return(us_labelci(im,imse,ox,oy,oz,rl));
    break;
    
  default:
    (void)sprintf(buf,"labelci(): invalid pixel type\n"); errputstr(buf);
    return(NULL);
  }
  return(NULL);
}
