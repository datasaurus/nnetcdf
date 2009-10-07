/*
   -	nnetcdf.c --
   -		This file defines functions that read
   -		NetCDF files.  See nnetcdf (3).
   -
   .	Copyright (c) 2009 Gordon D. Carrie
   .	All rights reserved.
   .
   .	Please send feedback to dev0@trekix.net
 */

#include <string.h>
#include "alloc.h"
#include "err_msg.h"
#include "nnetcdf.h"

/* Open a NetCDF file. See nnetcdf (3). */
int NNC_Open(char *file_nm, jmp_buf error_env)
{
    int status;
    int ncid;

    if ((status = nc_open(file_nm, 0, &ncid)) != 0) {
	Err_Append("Could not open ");
	Err_Append(file_nm ? file_nm : "(NULL)");
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return ncid;
}

/* Return the size of a NetCDF dimension.  See nnetcdf (3). */
size_t NNC_Inq_Dim(int ncid, char *name, jmp_buf error_env)
{
    int dimid;
    int status;
    size_t len;

    if ((status = nc_inq_dimid(ncid, name, &dimid)) != 0) {
	Err_Append("Could not find dimension named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_inq_dim(ncid, dimid, NULL, &len)) != 0) {
	Err_Append("Could not retrieve size of ");
	Err_Append(name);
	Err_Append("dimension.  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return len;
}

/* Return a string from a NetCDF file. See nnetcdf (3). */
char * NNC_Get_String(int ncid, char *name, jmp_buf error_env)
{
    char *val;		/* Return value */
    int varid;		/* Variable identifier */
    int dimid;		/* Dimension of variable */
    size_t len;		/* Dimension size */
    char *c, *ce;	/* Loop parameters */
    int status;		/* NetCDF function return value */

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_inq_vardimid(ncid, varid, &dimid)) != 0) {
	Err_Append("Could not get dimension for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_inq_dimlen(ncid, dimid, &len)) != 0) {
	Err_Append("Could not get dimension length for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !(val = MALLOC(len)) ) {
	Err_Append("Allocation failed for ");
	Err_Append(name);
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    for (c = val, ce = c + len; c < ce; c++) {
	*c = ' ';
    }
    if ((status = nc_get_var_text(ncid, varid, val)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    *(val + len) = '\0';

    return val;
}

/* Retrieve a character variable from a NetCDF file. See nnetcdf (3). */
char *NNC_Get_Var_Text(int ncid, char *name, char *cPtr, jmp_buf error_env)
{
    int varid;		/* Variable identifier */
    int status;

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !cPtr ) {
	int ndims;
	static int *dimidPtr;
	int *d, *de;
	size_t sz;

	if ((status = nc_inq_varndims(ncid, varid, &ndims)) != 0) {
	    Err_Append("Could not get dimension count for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ( !(dimidPtr = REALLOC(dimidPtr, ndims * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ((status = nc_inq_vardimid(ncid, varid, dimidPtr)) != 0) {
	    Err_Append("Could not get dimensions for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	for (sz = 1, d = dimidPtr, de = d + ndims; d < de; d++) {
	    size_t l;

	    if ((status = nc_inq_dimlen(ncid, *d, &l)) != 0) {
		Err_Append("Could not get dimension size for ");
		Err_Append(name);
		Err_Append(".  NetCDF error message is:\n");
		Err_Append(nc_strerror(status));
		Err_Append(".\n");
		longjmp(error_env, NNCDF_ERROR);
	    }
	    sz *= l;
	}
	if ( !(cPtr = MALLOC(sz)) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
    }
    if ((status = nc_get_var_text(ncid, varid, cPtr)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return cPtr;
}

/* Retrieve an unsigned char variable from a NetCDF file. See nnetcdf (3). */
unsigned char * NNC_Get_Var_UChar(int ncid, char *name, unsigned char *uPtr,
	jmp_buf error_env)
{
    int varid;		/* Variable identifier */
    int status;

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !uPtr ) {
	int ndims;
	static int *dimidPtr;
	int *d, *de;
	size_t sz;

	if ((status = nc_inq_varndims(ncid, varid, &ndims)) != 0) {
	    Err_Append("Could not get dimension count for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ( !(dimidPtr = REALLOC(dimidPtr, ndims * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ((status = nc_inq_vardimid(ncid, varid, dimidPtr)) != 0) {
	    Err_Append("Could not get dimensions for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	for (sz = 1, d = dimidPtr, de = d + ndims; d < de; d++) {
	    size_t l;

	    if ((status = nc_inq_dimlen(ncid, *d, &l)) != 0) {
		Err_Append("Could not get dimension size for ");
		Err_Append(name);
		Err_Append(".  NetCDF error message is:\n");
		Err_Append(nc_strerror(status));
		Err_Append(".\n");
		longjmp(error_env, NNCDF_ERROR);
	    }
	    sz *= l;
	}
	if ( !(uPtr = MALLOC(sz * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
    }
    if ((status = nc_get_var_uchar(ncid, varid, uPtr)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return uPtr;
}

/* Retrieve an integer variable from a NetCDF file. See nnetcdf (3). */
int * NNC_Get_Var_Int(int ncid, char *name, int *iPtr, jmp_buf error_env)
{
    int varid;		/* Variable identifier */
    int status;

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !iPtr ) {
	int ndims;
	static int *dimidPtr;
	int *d, *de;
	size_t sz;

	if ((status = nc_inq_varndims(ncid, varid, &ndims)) != 0) {
	    Err_Append("Could not get dimension count for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ( !(dimidPtr = REALLOC(dimidPtr, ndims * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ((status = nc_inq_vardimid(ncid, varid, dimidPtr)) != 0) {
	    Err_Append("Could not get dimensions for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	for (sz = 1, d = dimidPtr, de = d + ndims; d < de; d++) {
	    size_t l;

	    if ((status = nc_inq_dimlen(ncid, *d, &l)) != 0) {
		Err_Append("Could not get dimension size for ");
		Err_Append(name);
		Err_Append(".  NetCDF error message is:\n");
		Err_Append(nc_strerror(status));
		Err_Append(".\n");
		longjmp(error_env, NNCDF_ERROR);
	    }
	    sz *= l;
	}
	if ( !(iPtr = MALLOC(sz * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
    }
    if ((status = nc_get_var_int(ncid, varid, iPtr)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return iPtr;
}

/* Retrieve a float variable from a NetCDF file. See nnetcdf (3). */
float * NNC_Get_Var_Float(int ncid, char *name, float *fPtr, jmp_buf error_env)
{
    int varid;		/* Variable identifier */
    int status;

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !fPtr ) {
	int ndims;
	static int *dimidPtr;
	int *d, *de;
	size_t sz;

	if ((status = nc_inq_varndims(ncid, varid, &ndims)) != 0) {
	    Err_Append("Could not get dimension count for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ( !(dimidPtr = REALLOC(dimidPtr, ndims * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ((status = nc_inq_vardimid(ncid, varid, dimidPtr)) != 0) {
	    Err_Append("Could not get dimensions for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	for (sz = 1, d = dimidPtr, de = d + ndims; d < de; d++) {
	    size_t l;

	    if ((status = nc_inq_dimlen(ncid, *d, &l)) != 0) {
		Err_Append("Could not get dimension size for ");
		Err_Append(name);
		Err_Append(".  NetCDF error message is:\n");
		Err_Append(nc_strerror(status));
		Err_Append(".\n");
		longjmp(error_env, NNCDF_ERROR);
	    }
	    sz *= l;
	}
	if ( !(fPtr = MALLOC(sz * sizeof(float))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
    }
    if ((status = nc_get_var_float(ncid, varid, fPtr)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return fPtr;
}

/* Retrieve a double variable from a NetCDF file. See nnetcdf (3). */
double * NNC_Get_Var_Double(int ncid, char *name, double *dPtr, jmp_buf error_env)
{
    int varid;		/* Variable identifier */
    int status;

    if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !dPtr ) {
	int ndims;
	static int *dimidPtr;
	int *d, *de;
	size_t sz;

	if ((status = nc_inq_varndims(ncid, varid, &ndims)) != 0) {
	    Err_Append("Could not get dimension count for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ( !(dimidPtr = REALLOC(dimidPtr, ndims * sizeof(int))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	if ((status = nc_inq_vardimid(ncid, varid, dimidPtr)) != 0) {
	    Err_Append("Could not get dimensions for ");
	    Err_Append(name);
	    Err_Append(".  NetCDF error message is:\n");
	    Err_Append(nc_strerror(status));
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
	for (sz = 1, d = dimidPtr, de = d + ndims; d < de; d++) {
	    size_t l;

	    if ((status = nc_inq_dimlen(ncid, *d, &l)) != 0) {
		Err_Append("Could not get dimension size for ");
		Err_Append(name);
		Err_Append(".  NetCDF error message is:\n");
		Err_Append(nc_strerror(status));
		Err_Append(".\n");
		longjmp(error_env, NNCDF_ERROR);
	    }
	    sz *= l;
	}
	if ( !(dPtr = MALLOC(sz * sizeof(double))) ) {
	    Err_Append("Could not allocate dimension array for ");
	    Err_Append(name);
	    Err_Append(".\n");
	    longjmp(error_env, NNCDF_ERROR);
	}
    }
    if ((status = nc_get_var_double(ncid, varid, dPtr)) != 0) {
	Err_Append("Could not get value for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return dPtr;
}

/* Get a string attribute associated with a NetCDF variable. See nnetcdf (3). */
char * NNC_Get_Att_String(int ncid, char *name, char *att, jmp_buf error_env)
{
    int varid;
    int status;
    size_t len;
    char *val;

    if (strcmp(name, "NC_GLOBAL") == 0) {
	varid = NC_GLOBAL;
	name = "global attribute";
    } else if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_inq_attlen(ncid, varid, att, &len)) != 0) {
	Err_Append("Could not get string length for ");
	Err_Append(att);
	Err_Append(" of ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ( !(val = MALLOC(len + 1)) ) {
	Err_Append("Allocation failed for ");
	Err_Append(name);
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_get_att_text(ncid, varid, att, val)) != 0) {
	Err_Append("Could not get ");
	Err_Append(att);
	Err_Append("attribute for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    val[len] = '\0';
    return val;
}

/* Get a integer attribute associated with a NetCDF variable. See nnetcdf (3). */
int NNC_Get_Att_Int(int ncid, char *name, char *att, jmp_buf error_env)
{
    int varid;
    int status;
    int i;

    if (strcmp(name, "NC_GLOBAL") == 0) {
	varid = NC_GLOBAL;
	name = "global attribute";
    } else if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_get_att_int(ncid, varid, att, &i)) != 0) {
	Err_Append("Could not get ");
	Err_Append(att);
	Err_Append("attribute for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return i;
}

/* Get a float attribute associated with a NetCDF variable. See nnetcdf (3). */
float NNC_Get_Att_Float(int ncid, char *name, char *att, jmp_buf error_env)
{
    int varid;
    int status;
    float v;

    if (strcmp(name, "NC_GLOBAL") == 0) {
	varid = NC_GLOBAL;
	name = "global attribute";
    } else if ((status = nc_inq_varid(ncid, name, &varid)) != 0) {
	Err_Append("No variable named ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    if ((status = nc_get_att_float(ncid, varid, att, &v)) != 0) {
	Err_Append("Could not get ");
	Err_Append(att);
	Err_Append("attribute for ");
	Err_Append(name);
	Err_Append(".  NetCDF error message is:\n");
	Err_Append(nc_strerror(status));
	Err_Append(".\n");
	longjmp(error_env, NNCDF_ERROR);
    }
    return v;
}
