CC = gcc

# DEBUG=-g
# DEBUG=-g
# DEBUG=
# PROFILE_FLAG=-pg
# PROFILE_FLAG=
# OPTIMIZE_FLAG=
# OPTIMIZE_FLAG=-O3
# OPTIMIZE_FLAG=-O3 -pedantic -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security
# OPTIMIZE_FLAG=-O3 -pedantic
OPTIONS=-Wall -Wextra -Werror

all:	chompdrv

chompdrv: chompdrv.o
	gcc chompdrv.o -o chompdrv -lusb-1.0 -lstdc++

chompdrv.o: chompdrv.c
	gcc $(OPTIONS) ${DEBUG} ${PROFILE_FLAG} ${OPTIMIZE_FLAG} -pthread -Wall -c chompdrv.c -I/usr/include/libusb-1.0 -lusb-1.0 -lstdc++
