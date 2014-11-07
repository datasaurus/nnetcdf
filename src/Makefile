# Copyright (c) 2013, Gordon D. Carrie. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Where to install things. Make sure ${PREFIX}/bin is in PATH.
# PREFIX = /usr
PREFIX = ${HOME}/local

# Usual places to put things under $PREFIX
BINDIR = ${PREFIX}/bin
MANDIR = ${PREFIX}/share/man
BIN_EXECS = netcdf_app nc_cmp
SCRIPT_EXECS = 
EXECS = ${BIN_EXECS} ${SCRIPT_EXECS}

CC = gcc -std=c99 

# NETCDF_INCLUDES = -I /opt/local/include -I${PREFIX}/include
INCLUDES = ${NETCDF_INCLUDES}
CFLAGS = -O -Wall -Wmissing-prototypes ${INCLUDES}

# EFENCE_LIBS = -L/usr/local/lib -lefence
NETCDF_LIBS = -lnetcdf
LIBS = ${NETCDF_LIBS} ${EFENCE_LIBS} -lm

RM = rm -fr
CP = cp -f

.SUFFIXES : .o

.c.o :
	${CC} ${CFLAGS} -c $<

all : ${EXECS}

install : ${EXECS}
	mkdir -p ${BINDIR}
	${CP} ${EXECS} ${BINDIR}
	mkdir -p ${MANDIR}/man1
	${CP} ../man/man1/*.1 ${MANDIR}/man1
	mkdir -p ${MANDIR}/man3
	${CP} ../man/man3/*.3 ${MANDIR}/man3

NNETCDF_OBJ = netcdf_app.o hash.o strlcpy.o alloc.o
netcdf_app : ${NNETCDF_OBJ}
	${CC} ${CFLAGS} -o netcdf_app ${NNETCDF_OBJ} ${LIBS}

nc_cmp : nc_cmp.o alloc.o nnetcdf.o
	${CC} ${CFLAGS} -o nc_cmp nc_cmp.o nnetcdf.o alloc.o ${LIBS}

CMD_HASH_SRC = prhash_cmd.c hash.c strlcpy.c alloc.c
prhash_cmd : ${CMD_HASH_SRC}
	${CC} ${CFLAGS} -o prhash_cmd ${CMD_HASH_SRC}

netcdf_app.o : netcdf_app.c

nnetcdf.o : nnetcdf.c nnetcdf.h

hash.o : hash.c hash.h

alloc.o : alloc.c alloc.h

strlcpy.o : strlcpy.c strlcpy.h

clean :
	${RM} ${BIN_EXECS} prhash_cmd *.o *.core* *.dSYM
