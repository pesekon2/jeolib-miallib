# functions are destructive or non-destructive
# destructive does not always exists (@function in  lisp)
# non-destructive always exists

from . import miallib as _miallib

#import pointop_base

# we want pointop_base appear at the same level as the additional functions provided in this file.
from .pointop_base import *

from . import imem_base as _imem_base

def arithNew(i0, i1, op, inplace=0):
    """Arithmetic operations between two input images.  By default the operation is performed in place so that the first image holds the result of the opration once completed.

    :param i0: first image
    :param i1: second image
    :param op: integer for operation type
    :param inplace: 0 for in place (default), otherwise
    :returns: result of the operation (i.e., i0 if in place, a new image otherwise
    :rtype: an image

    """
    if (inplace==0):
        arith(i0, i1, op)
        return i0
    i2=_imem_base.copy_image(i0)
    arith(i2, i1, op)
    return i2
    

def d_arith(i0, i1, op):
    arith(i0, i1, op)
    return i0
    
def nd_arith(i0, i1, op):
    i2=_imem_base.copy_image(i0)
    arith(i2, i1, op)
    return i2

def d_imsqrt(i0):
    if imsqrt(i0)==_miallib.NO_ERROR:
        return i0
    return _miallib.ERROR

def nd_imsqrt(i0):
    i1=_imem_base.copy_image(i0)
    if imsqrt(i1)==_miallib.NO_ERROR:
        return i1
    return _miallib.ERROR
    
def d_thresh(i0,low,high,bg,fg):    
    r=thresh(i0, low, high, bg, fg)
    if r==_miallib.NO_ERROR:
        return i0
    else:
        return 'd_thresh(): invalid data type'

def nd_thresh(i0,low,high,bg,fg):
    i1=_imem_base.copy_image(i0)
    return d_thresh(i1,low,high,bg,fg)

def d_blank(i0,val):
    r=blank(i0, val)
    if r==_miallib.NO_ERROR:
        return i0
    else:
        return 'd_blank(): invalid data type'
                    
def nd_blank(i0,val):
    i1=_imem_base.copy_image(i0)
    return d_blank(i1,val)

def d_setlevel(i0,low,high,val):
    r= setlevel(i0, low, high, val)
    if  r==_miallib.NO_ERROR:
        return i0
    else:
        return 'd_setlevel(): invalid data type'

def nd_setlevel(i0,low,high,val):
    i1=_imem_base.copy_image(i0)
    return d_setlevel(i1,low,val)


# def setgtval(gt, val, type):
#     if type==_miallib.t_UCHAR:
#         gt.us_val=val
#     elif type==_miallib.t_USHORT:
#         gt.us_val=val
#     elif type==_miallib.t_INT32:
#         gt.i32_val=val
#     elif type==_miallib.t_UINT32:
#         gt.u32_val=val
#     elif type==_miallib.t_INT64:
#         gt.i64_val=val
#     elif type==_miallib.t_UINT64:
#         gt.u64_val=val
#     elif type==_miallib.t_FLOAT:
#         gt.f_val=val
#     elif type==_miallib.t_DOUBLE:
#         gt.d_val=val
#     else:
#         print('setgtval: invalid type provided:', type)
