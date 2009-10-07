/*
   -	nnetcdf.h --
   -		This file declares functions that read
   -		NetCDF files.  See nnetcdf (3).
   -
   .	Copyright (c) 2009 Gordon D. Carrie
   .	All rights reserved.
   .
   .	Please send feedback to dev0@trekix.net
 */

#ifndef NNCDF_H_
#define NNCDF_H_

#include <setjmp.h>
#include <netcdf.h>

#define NNCDF_ERROR 1

int NNC_Open(char *, jmp_buf);
size_t NNC_Inq_Dim(int , char *, jmp_buf);
char *NNC_Get_Var_Text(int , char *, char *, jmp_buf);
char *NNC_Get_String(int , char *, jmp_buf);
unsigned char *NNC_Get_Var_UChar(int , char *, unsigned char *, jmp_buf);
int *NNC_Get_Var_Int(int , char *, int *, jmp_buf);
float *NNC_Get_Var_Float(int , char *, float *, jmp_buf);
double *NNC_Get_Var_Double(int , char *, double *, jmp_buf);
char *NNC_Get_Att_String(int , char *, char *, jmp_buf);
int NNC_Get_Att_Int(int , char *, char *, jmp_buf);
float NNC_Get_Att_Float(int , char *, char *, jmp_buf);

#endif
