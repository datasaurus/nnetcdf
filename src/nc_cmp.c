/*
   -	nc_cmp.c --
   -		Compare a field in two netcdf files
   -
   .	Usage:
   .		nc_cmp field_name file1 file2
   .
   .	Standard output will be descriptive information about the fields and
   .	their differences.
   .
   .	Copyright (c) 2013, Gordon D. Carrie. All rights reserved.
   .	
   .	Redistribution and use in source and binary forms, with or without
   .	modification, are permitted provided that the following conditions
   .	are met:
   .	
   .	    * Redistributions of source code must retain the above copyright
   .	    notice, this list of conditions and the following disclaimer.
   .
   .	    * Redistributions in binary form must reproduce the above copyright
   .	    notice, this list of conditions and the following disclaimer in the
   .	    documentation and/or other materials provided with the distribution.
   .	
   .	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   .	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   .	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   .	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   .	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   .	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   .	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   .	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   .	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   .	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   .	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   .
   .	Please send feedback to dev0@trekix.net
   .
   .	$Revision: 1.4 $ $Date: 2013/12/13 20:18:28 $
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <setjmp.h>
#include <unistd.h>
#include <netcdf.h>
#include "nnetcdf.h"
#include "hash.h"
#include "alloc.h"

/* Length of format specifier */
#define FMT_LEN 42

int main(int argc, char *argv[])
{
    char *argv0 = argv[0];
    char *var_nm;			/* Variable name, from command line */
    char *nc_fl_nm1, *nc_fl_nm2;	/* Path to NetCDF file */
    size_t l1, l2;			/* String lengths of nc_fl_nm1 and
					   nc_fl_nm2 */
    char fmt[FMT_LEN];			/* Format output string */
    double ign = INFINITY;		/* Ignore values with absolute value
					   greater than ign */
    int nc_id1, nc_id2;			/* NetCDF file identifier */
    int var_id;				/* NetCDF identifier for variable */
    nc_type xtype1, xtype2;		/* Type of var */
    char *xtype_s;			/* Type of var as a string */
    int num_dims;			/* Number of dimensions */
    int *dim_ids = NULL;		/* Dimension identifiers */
    size_t dimlen;			/* Length of a dimension */
    int status;				/* Return code from NetCDF function
					   call */
    jmp_buf err_env1, err_env2;		/* Jump buffer for nnetcdf calls */
    int c;				/* Option */
    int n;				/* Data element index */
    size_t num_elem1, num_elem2;	/* Number of data values */
    size_t n1, n2, nd;			/* Number of data values actually
					   used from file 1, file 2, and
					   in mean square difference
					   calculation */
    char *c1, *c2;			/* Data arrays */
    int *i1, *i2;
    float *f1, *f2;
    double *d1, *d2;
    unsigned char *uc1, *uc2;
    unsigned int *ui1, *ui2;
    double m1, rms1, m2, rms2;		/* Mean, root mean square */
    double msd;				/* Mean square difference */

    argv0 = argv[0];
    if ( argc < 4 ) {
	fprintf(stderr, "Usage: %s [options] field file1 file2\n", argv0);
	exit(EXIT_FAILURE);
    }
    while ((c = getopt(argc, argv, ":i:")) != -1) {
	switch(c) {
	    case 'i':
		if ( sscanf(optarg, "%lf", &ign) != 1 ) {
		    fprintf(stderr, "%s: expected a float for value to ignore, "
			    "got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		if ( ign <= 0.0 ) {
		    fprintf(stderr, "%s: value to ignore must be greater "
			    "than 0.0\n", argv0);
		    exit(EXIT_FAILURE);
		}
		printf("Ignoring values with absolute value >= %g\n", ign);
		break;
	    case ':':
		fprintf(stderr, "%s: -%c requires an argument\n",
			argv0, optopt);
		exit(EXIT_FAILURE);
		break;
	    case '?':
		fprintf(stderr, "%s: unknown option %c\n", argv0, c);
		exit(EXIT_FAILURE);
		break;
	}
    }
    var_nm = argv[argc - 3];
    nc_fl_nm1 = argv[argc - 2];
    nc_fl_nm2 = argv[argc - 1];

    /* Open first file and get variable information*/
    if ( (status = nc_open(nc_fl_nm1, 0, &nc_id1)) != NC_NOERR ) {
	fprintf(stderr, "%s: failed to open %s.\n%s\n",
		argv0, nc_fl_nm1, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_varid(nc_id1, var_nm, &var_id)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not find variable named %s in %s.\n%s\n",
		argv0, var_nm, nc_fl_nm1, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (nc_inq_vartype(nc_id1, var_id, &xtype1)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not determine type for %s in %s.\n"
		"%s\n", argv0, var_nm, nc_fl_nm1, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_ndims(nc_id1, &num_dims)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not get number of dimensions for %s.\n"
		"%s\n", argv0, nc_fl_nm1, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( !(dim_ids = CALLOC(num_dims, sizeof(int))) ) {
	fprintf(stderr, "%s: could not allocate array of %u dimension "
		"identifiers for %s.\n", argv0, num_dims, nc_fl_nm1);
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_vardimid(nc_id1, var_id, dim_ids)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not get dimension identifiers for %s "
		"in %s.\n%s\n", argv0, var_nm, nc_fl_nm1, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    for (num_elem1 = 0, n = 0; n < num_dims; n++) {
	status = nc_inq_dimlen(nc_id1, dim_ids[n], &dimlen);
	if ( status != NC_NOERR ) {
	    fprintf(stderr, "%s: could not get length for dimension %d "
		    "in %s.\n%s\n", argv0, n, nc_fl_nm1, nc_strerror(status));
	    exit(EXIT_FAILURE);
	}
	num_elem1 += dimlen;
    }

    /* Open second file and get variable information*/
    if ( (status = nc_open(nc_fl_nm2, 0, &nc_id2)) != NC_NOERR ) {
	fprintf(stderr, "%s: failed to open %s.\n%s\n",
		argv0, nc_fl_nm2, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_varid(nc_id2, var_nm, &var_id)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not find variable named %s in %s.\n%s\n",
		argv0, var_nm, nc_fl_nm2, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (nc_inq_vartype(nc_id2, var_id, &xtype2)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not determine type for %s in %s.\n"
		"%s\n", argv0, var_nm, nc_fl_nm2, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_ndims(nc_id2, &num_dims)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not get number of dimensions for %s.\n"
		"%s\n", argv0, nc_fl_nm2, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    if ( !(dim_ids = CALLOC(num_dims, sizeof(int))) ) {
	fprintf(stderr, "%s: could not allocate array of %u dimension "
		"identifiers for %s.\n", argv0, num_dims, nc_fl_nm2);
	exit(EXIT_FAILURE);
    }
    if ( (status = nc_inq_vardimid(nc_id2, var_id, dim_ids)) != NC_NOERR ) {
	fprintf(stderr, "%s: could not get dimension identifiers for %s "
		"in %s.\n%s\n", argv0, var_nm, nc_fl_nm2, nc_strerror(status));
	exit(EXIT_FAILURE);
    }
    for (num_elem2 = 0, n = 0; n < num_dims; n++) {
	status = nc_inq_dimlen(nc_id2, dim_ids[n], &dimlen);
	if ( status != NC_NOERR ) {
	    fprintf(stderr, "%s: could not get length for dimension %d "
		    "in %s.\n%s\n", argv0, n, nc_fl_nm2, nc_strerror(status));
	    exit(EXIT_FAILURE);
	}
	num_elem2 += dimlen;
    }

    /*
       Ensure variable name corresponds to variable of same type and size in
       both files
     */

    if ( xtype1 != xtype2 ) {
	fprintf(stderr, "%s not the same type in %s and %s\n",
		var_nm, nc_fl_nm1, nc_fl_nm2);
	exit(EXIT_FAILURE);
    }
    if ( num_elem1 != num_elem2 ) {
	fprintf(stderr, "%s: %s has different number of elements "
		"in %s and %s\n", argv0, var_nm, nc_fl_nm1, nc_fl_nm2);
	exit(EXIT_FAILURE);
    }

    /* Fetch arrays and compare */
    if ( setjmp(err_env1) == NNCDF_ERROR ) {
	fprintf(stderr, "%s: failed to retrieve %s from %s",
		argv0, var_nm, nc_fl_nm1);
	exit(EXIT_FAILURE);
    }
    if ( setjmp(err_env2) == NNCDF_ERROR ) {
	fprintf(stderr, "%s: failed to retrieve %s from %s",
		argv0, var_nm, nc_fl_nm2);
	exit(EXIT_FAILURE);
    }
    switch (xtype1) {
	case NC_BYTE:
	case NC_CHAR:
	    xtype_s = "byte";
	    c1 = NNC_Get_Var_Text(nc_id1, var_nm, NULL, err_env1);
	    c2 = NNC_Get_Var_Text(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(c1[n]) < ign ) {
		    m1 += c1[n];
		    rms1 += c1[n] * c1[n];
		    n1++;
		}
		if ( fabs(c2[n]) < ign ) {
		    m2 += c2[n];
		    rms2 += c2[n] * c2[n];
		    n2++;
		}
		if ( fabs(c1[n]) < ign &&  fabs(c2[n]) < ign ) {
		    msd += (c1[n] - c2[n]) * (c1[n] - c2[n]);
		    nd++;
		}
	    }
	case NC_INT:
	    xtype_s = "integer";
	    i1 = NNC_Get_Var_Int(nc_id1, var_nm, NULL, err_env1);
	    i2 = NNC_Get_Var_Int(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(i1[n]) < ign ) {
		    m1 += i1[n];
		    rms1 += i1[n] * i1[n];
		    n1++;
		}
		if ( fabs(i2[n]) < ign ) {
		    m2 += i2[n];
		    rms2 += i2[n] * i2[n];
		    n2++;
		}
		if ( fabs(i1[n]) < ign &&  fabs(i2[n]) < ign ) {
		    msd += (i1[n] - i2[n]) * (i1[n] - i2[n]);
		    nd++;
		}
	    }
	    break;
	case NC_FLOAT:
	    xtype_s = "float";
	    f1 = NNC_Get_Var_Float(nc_id1, var_nm, NULL, err_env1);
	    f2 = NNC_Get_Var_Float(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(f1[n]) < ign ) {
		    m1 += f1[n];
		    rms1 += f1[n] * f1[n];
		    n1++;
		}
		if ( fabs(f2[n]) < ign ) {
		    m2 += f2[n];
		    rms2 += f2[n] * f2[n];
		    n2++;
		}
		if ( fabs(f1[n]) < ign &&  fabs(f2[n]) < ign ) {
		    msd += (f1[n] - f2[n]) * (f1[n] - f2[n]);
		    nd++;
		}
	    }
	    break;
	case NC_DOUBLE:
	    xtype_s = "double";
	    d1 = NNC_Get_Var_Double(nc_id1, var_nm, NULL, err_env1);
	    d2 = NNC_Get_Var_Double(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(d1[n]) < ign ) {
		    m1 += d1[n];
		    rms1 += d1[n] * d1[n];
		    n1++;
		}
		if ( fabs(d2[n]) < ign ) {
		    m2 += d2[n];
		    rms2 += d2[n] * d2[n];
		    n2++;
		}
		if ( fabs(d1[n]) < ign &&  fabs(d2[n]) < ign ) {
		    msd += (d1[n] - d2[n]) * (d1[n] - d2[n]);
		    nd++;
		}
	    }
	    break;
	case NC_UBYTE:
	    xtype_s = "unsigned byte";
	    uc1 = NNC_Get_Var_UChar(nc_id1, var_nm, NULL, err_env1);
	    uc2 = NNC_Get_Var_UChar(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(uc1[n]) < ign ) {
		    m1 += uc1[n];
		    rms1 += uc1[n] * uc1[n];
		    n1++;
		}
		if ( fabs(uc2[n]) < ign ) {
		    m2 += uc2[n];
		    rms2 += uc2[n] * uc2[n];
		    n2++;
		}
		if ( fabs(uc1[n]) < ign &&  fabs(uc2[n]) < ign ) {
		    msd += (uc1[n] - uc2[n]) * (uc1[n] - uc2[n]);
		    nd++;
		}
	    }
	    break;
	case NC_UINT:
	    xtype_s = "unsigned integer";
	    ui1 = NNC_Get_Var_UInt(nc_id1, var_nm, NULL, err_env1);
	    ui2 = NNC_Get_Var_UInt(nc_id2, var_nm, NULL, err_env2);
	    for (m1 = m2 = rms1 = rms2 = msd = 0.0, n = n1 = n2 = nd = 0;
		    n < num_elem1;
		    n++) {
		if ( fabs(ui1[n]) < ign ) {
		    m1 += ui1[n];
		    rms1 += ui1[n] * ui1[n];
		    n1++;
		}
		if ( fabs(ui2[n]) < ign ) {
		    m2 += ui2[n];
		    rms2 += ui2[n] * ui2[n];
		    n2++;
		}
		if ( fabs(ui1[n]) < ign &&  fabs(ui2[n]) < ign ) {
		    msd += (ui1[n] - ui2[n]) * (ui1[n] - ui2[n]);
		    nd++;
		}
	    }
	    break;
	default:
	    fprintf(stderr, "%s: cannot read type of %s\n",
		    argv0, var_nm);
	    exit(EXIT_FAILURE);
    }
    m1 /= n1;
    rms1 = sqrt(rms1 / n1);
    m2 /= n2;
    rms2 = sqrt(rms2 / n2);
    msd /= nd;
    printf("Field %s. %zd %s elements\n", var_nm, num_elem1, xtype_s);
    l1 = strlen(nc_fl_nm1);
    l2 = strlen(nc_fl_nm2);
    if ( l2 > l1 ) {
	l1 = l2;
    }
    if ( snprintf(fmt, FMT_LEN, "File %%-%zds: mean = %%g mean square = %%g\n",
		l1) > FMT_LEN ) {
	fprintf(stderr, "Could not format output for file name "
		"with %zd characters.\n", l1);
	exit(EXIT_FAILURE);
    }
    printf(fmt, nc_fl_nm1, m1, rms1);
    printf(fmt, nc_fl_nm2, m2, rms2);
    printf("Mean square difference = %g\n", msd);

    exit(EXIT_SUCCESS);
}

