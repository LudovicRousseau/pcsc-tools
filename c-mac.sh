#!/bin/bash

# use ${varname:-word} to return word only if varname is not already defined
PCSC_CFLAGS=${PCSC_CFLAGS:--framework PCSC}
PCSC_LIBS=${PCSC_LIBS:--framework PCSC}

./configure \
	PCSC_CFLAGS="$PCSC_CFLAGS" \
	PCSC_LIBS="$PCSC_LIBS" \
	"$@"
