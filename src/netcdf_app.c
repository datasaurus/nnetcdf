/*
   -	nnetcdf_app.c --
   -		Command line access to NetCDF files.
   -
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
#include <netcdf.h>
#include "hash.h"
#include "alloc.h"

/* Callbacks for the subcommands.  */ 
typedef int (callback)(int , char **);
static callback headers_cb;
static callback data_cb;

/*
   Subcommand names and associated callbacks. Empty command names and NULL
   callbacks are place holders to ensure perfect hashing when fetching a
   callback.  Parser does not search buckets. Convenience application
   pr_hash_cmd helps make this table.
 */

#define N_HASH_CMD 7
static char *cmd1v[N_HASH_CMD] = {
    "data", "", "", "", "", "headers", "", 
};
static callback *cb1v[N_HASH_CMD] = {
    data_cb, NULL, NULL, NULL, NULL, headers_cb, NULL, 
};

/* Usage: nnetcdf command [args ...] */

int main(int argc, char *argv[])
{
    char *argv0 = argv[0];
    char *argv1;
    int n;

    if ( argc < 2 ) {
	fprintf(stderr, "%s command [args ...]\n", argv0);
	exit(EXIT_FAILURE);
    }
    argv1 = argv[1];
    n = Hash(argv1, N_HASH_CMD);
    if ( strcmp(argv1, cmd1v[n]) != 0 ) {
	fprintf(stderr, "%s: unknown command %s. "
		"Subcommand must be one of ", argv0, argv1);
	for (n = 0; n < N_HASH_CMD; n++) {
	    if ( strlen(cmd1v[n]) > 0 ) {
		fprintf(stderr, " %s", cmd1v[n]);
	    }
	}
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
    }
    if ( !cb1v[n](argc, argv) ) {
	fprintf(stderr, "%s: %s failed.\n", argv0, argv1);
    }

    return 0;
}

/* nnetcdf headers file */
static int headers_cb(int argc, char *argv[])
{
    char *argv0, *argv1;		/* argv[0], argv[1] */
    char *nc_fl_nm;			/* Path to NetCDF file */
    int nc_id;				/* NetCDF file identifier */
    int status;				/* Return code from NetCDF function
					   call */
    char name[NC_MAX_NAME];		/* Dimension or variable name */
    int num_dims, num_vars;		/* Number of dimensions, variables */
    int *dim_ids = NULL;		/* Dimension identifiers */
    size_t len;				/* Dimension length */
    nc_type xtype;			/* Data type */
    int d, v;				/* Dimension, variable index */

    argv0 = argv[0];
    argv1 = argv[1];
    if ( argc != 3 ) {
	fprintf(stderr, "Usage: %s %s file\n", argv0, argv1);
	return 0;
    }
    nc_fl_nm = argv[2];
    if ( (status = nc_open(nc_fl_nm, 0, &nc_id)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: failed to open %s.\n%s\n",
		argv0, argv1, nc_fl_nm, nc_strerror(status));
	goto error;
    }
    if ( (status = nc_inq_ndims(nc_id, &num_dims)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not get number of dimensions.\n"
		"%s\n", argv0, argv1, nc_strerror(status));
	goto error;
    }
    for (d = 0; d < num_dims; d++) {
	if ( (status = nc_inq_dim(nc_id, d, name, &len)) != NC_NOERR ) {
	    fprintf(stderr, "%s %s: could not get information for dimension"
		    " %d.\n%s\n", argv0, argv1, d, nc_strerror(status));
	    goto error;
	}
	printf("dim %s %zi\n", name, len);
    }
    if ( (status = nc_inq_nvars(nc_id, &num_vars)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not get number of variables.\n"
		"%s\n", argv0, argv1, nc_strerror(status));
	goto error;
    }
    if ( !(dim_ids = CALLOC(num_dims, sizeof(int))) ) {
	fprintf(stderr, "%s %s: could not allocate array of %u dimension "
		"identifiers.\n", argv0, argv1, num_dims);
	goto error;
    }
    for (v = 0; v < num_vars; v++) {
	status = nc_inq_var(nc_id, v, name, &xtype, &num_dims, dim_ids, NULL);
	if ( status != NC_NOERR ) {
	    fprintf(stderr, "%s %s: could not get information for variable"
		    " %d.\n%s\n", argv0, argv1, v, nc_strerror(status));
	    goto error;
	}
	printf("var %s", name);
	switch (xtype) {
	    case NC_BYTE:   printf(" NC_BYTE");   break;
	    case NC_CHAR:   printf(" NC_CHAR");   break;
	    case NC_SHORT:  printf(" NC_SHORT");  break;
	    case NC_INT:    printf(" NC_INT");    break;
	    case NC_FLOAT:  printf(" NC_FLOAT");  break;
	    case NC_DOUBLE: printf(" NC_DOUBLE"); break;
	    case NC_UBYTE:  printf(" NC_UBYTE");  break;
	    case NC_USHORT: printf(" NC_USHORT"); break;
	    case NC_UINT:   printf(" NC_UINT");   break;
	    default:        fprintf(stderr, "\n%s %s: unknown type for "
				    "variable %s\n", argv0, argv1,
				    name);        goto error;
	}
	for (d = 0; d < num_dims; d++) {
	    status = nc_inq_dimname(nc_id, dim_ids[d], name);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "\n%s %s: could not get name for dimension %d "
			"of %s.\n%s\n", argv0, argv1, d, name,
			nc_strerror(status));
		goto error;
	    }
	    printf(" %s", name);
	}
	printf("\n");
    }
    FREE(dim_ids);
    nc_close(nc_id);
    return 1;

error:
    FREE(dim_ids);
    nc_close(nc_id);
    return 0;
}

/* nnetcdf data var_name index [index ...] file */
static int data_cb(int argc, char *argv[])
{
    char *argv0, *argv1;		/* argv[0], argv[1] */
    char *nc_fl_nm;			/* Path to NetCDF file */
    int nc_id;				/* NetCDF file identifier */
    char *var_nm;			/* Variable name, from command line */
    int var_id;				/* NetCDF identifier for variable */
    nc_type xtype;			/* Type of var */
    int num_idx;			/* Number of indeces specified on
					   command line */
    int status;				/* Return code from NetCDF function
					   call */
    size_t *start = NULL;		/* Where to start slice in NetCDF file.
					   See netcdf (3). */
    int num_dims;			/* Number of dimensions of variable */
    int *dim_ids = NULL;		/* Dimension identifiers */
    size_t *count = NULL;		/* Count of values along each dimension
					   for which values will be printed.
					   See netcdf (3). */
    int a, d;				/* Argument index, dimension index */
    void *dat = NULL;			/* Data values to fetch and print */
    size_t num_elem;			/* Number of data values */
    int llen;				/* Number of elements in each line of
					   output */

    argv0 = argv[0];
    argv1 = argv[1];
    if ( argc < 4 ) {
	fprintf(stderr, "Usage: %s %s var_name [index ...] file\n",
		argv0, argv1);
	return 0;
    }
    var_nm = argv[2];
    nc_fl_nm = argv[argc - 1];
    num_idx = argc - 4;

    /* Open data file and get information about the variable */
    if ( (status = nc_open(nc_fl_nm, 0, &nc_id)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: failed to open %s.\n%s\n",
		argv0, argv1, nc_fl_nm, nc_strerror(status));
	goto error;
    }
    if ( (status = nc_inq_varid(nc_id, var_nm, &var_id)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not find variable named %s.\n%s\n",
		argv0, argv1, var_nm, nc_strerror(status));
	goto error;
    }
    if ( (nc_inq_vartype(nc_id, var_id, &xtype)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not determine type for %s.\n"
		"%s\n", argv0, argv1, var_nm, nc_strerror(status));
	goto error;
    }
    if ( (status = nc_inq_varndims(nc_id, var_id, &num_dims)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not get number of dimensions for %s.\n"
		"%s\n", argv0, argv1, var_nm, nc_strerror(status));
	goto error;
    }
    if ( num_idx > num_dims ) {
	fprintf(stderr, "%s %s: number of indeces exceeds number of "
		"dimensions.\n", argv0, argv1);
	goto error;
    }
    if ( !(dim_ids = CALLOC(num_dims, sizeof(int))) ) {
	fprintf(stderr, "%s %s: could not allocate array of %u dimension "
		"identifiers.\n", argv0, argv1, num_dims);
	goto error;
    }
    if ( !(start = CALLOC(num_dims, sizeof(size_t))) ) {
	fprintf(stderr, "%s %s: could not allocate array of %u indeces.\n",
		argv0, argv1, num_dims);
	goto error;
    }
    if ( !(count = CALLOC(num_dims, sizeof(size_t))) ) {
	fprintf(stderr, "%s %s: could not allocate array of %u dimension "
		"sizes.\n", argv0, argv1, num_dims);
	goto error;
    }
    if ( (status = nc_inq_vardimid(nc_id, var_id, dim_ids)) != NC_NOERR ) {
	fprintf(stderr, "%s %s: could not get dimension identifiers for %s.\n"
		"%s\n", argv0, argv1, var_nm, nc_strerror(status));
	goto error;
    }

    /*
       Initialize count array with sizes of each dimension. Thus, default
       is to print all values for each dimension.
     */

    for (d = 0; d < num_dims; d++) {
	if ( (status = nc_inq_dimlen(nc_id, dim_ids[d], count + d))
		!= NC_NOERR ) {
	    fprintf(stderr, "%s %s: could not get length for dimension %d.\n"
		    "%s\n", argv0, argv1, d, nc_strerror(status));
	    goto error;
	}
    }

    /*
       Put indeces from command line into start. Set corresponding element
       of count to 1, indicating that the only element of this dimension to be
       printed is the one the index specifies, instead of all members.
     */

    for (a = 3, d = 0; a < argc - 1; a++, d++) {
	if ( sscanf(argv[a], "%zd", start + d) != 1 ) {
	    fprintf(stderr, "%s %s: expected integer for index %d, got %s\n",
		    argv0, argv1, a - 3, argv[a]);
	    goto error;
	}
	count[d] = 1;
    }

    /* Fetch and print array */
    for (num_elem = 1, d = 0; d < num_dims; d++) {
	num_elem *= count[d];
    }
    llen = count[num_dims - 1];
    switch (xtype) {
	case NC_BYTE:
	case NC_CHAR:
	    if ( !(dat = CALLOC(num_elem, 1)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_text(nc_id, var_id, start, count, (char *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((char *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_SHORT:
	    if ( !(dat = CALLOC(num_elem, 2)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_short(nc_id, var_id, start, count,
		    (short *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((short *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_INT:
	    if ( !(dat = CALLOC(num_elem, 4)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_int(nc_id, var_id, start, count, (int *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((int *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_FLOAT:
	    if ( !(dat = CALLOC(num_elem, 4)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_float(nc_id, var_id, start, count,
		    (float *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%g%s",
			((float *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_DOUBLE:
	    if ( !(dat = CALLOC(num_elem, 8)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_double(nc_id, var_id, start, count,
		    (double *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%g%s",
			((double *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_UBYTE:
	    if ( !(dat = CALLOC(num_elem, 1)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_ubyte(nc_id, var_id, start, count,
		    (unsigned char *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((unsigned char *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_USHORT:
	    if ( !(dat = CALLOC(num_elem, 2)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_ushort(nc_id, var_id, start, count,
		    (unsigned short *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((unsigned short *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	case NC_UINT:
	    if ( !(dat = CALLOC(num_elem, 4)) ) {
		fprintf(stderr, "%s %s: could not allocate data array "
			"with %zd elements.\n", argv0, argv1, num_elem);
	    }
	    status = nc_get_vara_uint(nc_id, var_id, start, count,
		    (unsigned int *)dat);
	    if ( status != NC_NOERR ) {
		fprintf(stderr, "%s %s: could not read %s.\n%s\n",
			argv0, argv1, var_nm, nc_strerror(status));
		goto error;
	    }
	    for (d = 0; d < num_elem; d++) {
		printf("%d%s",
			((unsigned int *)dat)[d],
			((d + 1) % llen) == 0 ? "\n" : " ");
	    }
	    break;
	default:
	    fprintf(stderr, "%s %s: cannot read type of %s\n",
		    argv0, argv1, var_nm);
	    goto error;
    }

    FREE(start);
    FREE(dim_ids);
    FREE(count);
    FREE(dat);
    nc_close(nc_id);
    return 1;

error:
    FREE(start);
    FREE(dim_ids);
    FREE(count);
    FREE(dat);
    nc_close(nc_id);
    return 0;
}
