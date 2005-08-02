/*
    Scan and print all the PC/SC devices available
    Copyright (C) 2001-2003  Ludovic Rousseau <ludovic.rousseau@free.fr>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/* $Id: pcsc_scan.c,v 1.17 2005-08-02 20:39:10 rousseau Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define TIMEOUT 1000	/* 1 second timeout */

/* command used to parse (on screen) the ATR */
#define ATR_PARSER "ATR_analysis"

void usage(void)
{
	printf("usage: pcsc_scan [-n] [-V] [-h]\n");
	printf("  -n : no ATR analysis\n");
	printf("  -V : print version number\n");
	printf("  -h : this help\n");
} /* usage */

int main(int argc, char *argv[])
{
	int current_reader;
	LONG rv;
	SCARDCONTEXT hContext;
	SCARD_READERSTATE_A *rgReaderStates_t = NULL;
	DWORD dwReaders, dwReadersOld;
	LPTSTR mszReaders = NULL;
	char *ptr, **readers = NULL;
	int nbReaders, i;
	char atr[MAX_ATR_SIZE*3+1];	/* ATR in ASCII */
	char atr_command[sizeof(atr)+sizeof(ATR_PARSER)+2+1];
	int opt;
	int analyse_atr = TRUE;

	printf("PC/SC device scanner\n");
	printf("V " VERSION " (c) 2001-2004, Ludovic Rousseau <ludovic.rousseau@free.fr>\n");
	printf("Compiled with PC/SC lite version: " PCSCLITE_VERSION_NUMBER "\n");

	while ((opt = getopt(argc, argv, "Vhn")) != EOF)
	{
		switch (opt)
		{
			case 'n':
				analyse_atr = FALSE;
				break;

			case 'V':
				/* the version number is printed by default */
				return 1;
				break;

			case 'h':
				default:
				usage();
				return 1;
				break;
		}
	}

	if (argc - optind != 0)
	{
		usage();
		return 1;
	}

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardEstablishContext: Cannot Connect to Resource Manager %lX\n", rv);
		return 1;
	}

get_readers:
	/* free memory possibly allocated in a previous loop */
	if (NULL != readers)
		free(readers);
	if (NULL != rgReaderStates_t)
		free(rgReaderStates_t);

	/* Retrieve the available readers list.
	 *
	 * 1. Call with a null buffer to get the number of bytes to allocate
	 * 2. malloc the necessary storage
	 * 3. call with the real allocated buffer
	 */
	printf("Scanning present readers\n");
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardListReader: %lX\n", rv);
	}
	dwReadersOld = dwReaders;

	/* if non NULL we came back so free first */
	if (mszReaders)
		free(mszReaders);

	mszReaders = malloc(sizeof(char)*dwReaders);
	if (mszReaders == NULL)
	{
		printf("malloc: not enough memory\n");
		return 1;
	}

	rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardListReader: %lX\n", rv);
	}

	/* Extract readers from the null separated string and get the total
	 * number of readers */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (nbReaders == 0)
	{
		printf("Waiting for the first reader...");
		fflush(stdout);
		while ((SCardListReaders(hContext, NULL, NULL, &dwReaders)
			== SCARD_S_SUCCESS) && (dwReaders == dwReadersOld))
			sleep(1);
		printf("found one\n");
		goto get_readers;
	}

	/* allocate the readers table */
	readers = calloc(nbReaders, sizeof(char *));
	if (NULL == readers)
	{
		printf("Not enough memory for readers table\n");
		return -1;
	}

	/* fill the readers table */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		printf("%d: %s\n", nbReaders, ptr);
		readers[nbReaders] = ptr;
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	/* allocate the ReaderStates table */
	rgReaderStates_t = calloc(nbReaders, sizeof(* rgReaderStates_t));
	if (NULL == rgReaderStates_t)
	{
		printf("Not enough memory for readers states\n");
		return -1;
	}

	/* Set the initial states to something we do not know
	 * The loop below will include this state to the dwCurrentState
	 */
	for (i=0; i<nbReaders; i++)
	{
		rgReaderStates_t[i].szReader = readers[i];
		rgReaderStates_t[i].dwCurrentState = SCARD_STATE_UNAWARE;
	}

	/* Wait endlessly for all events in the list of readers
	 * We only stop in case of an error
	 */
	rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);
	while ((rv == SCARD_S_SUCCESS) || (rv == SCARD_E_TIMEOUT))
	{
		/* A new reader appeared? */
		if ((SCardListReaders(hContext, NULL, NULL, &dwReaders)
			== SCARD_S_SUCCESS) && (dwReaders != dwReadersOld))
				goto get_readers;

		/* Now we have an event, check all the readers in the list to see what
		 * happened */
		for (current_reader=0; current_reader < nbReaders; current_reader++)
		{
			time_t t;

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_CHANGED)
			{
				/* If something has changed the new state is now the current
				 * state */
				rgReaderStates_t[current_reader].dwCurrentState =
					rgReaderStates_t[current_reader].dwEventState;
			}
			else
				/* If nothing changed then skip to the next reader */
				continue;

			/* From here we know that the state for the current reader has
			 * changed because we did not pass through the continue statement
			 * above.
			 */

			/* Timestamp the event as we get notified */
			t = time(NULL);

			/* Specify the current reader's number and name */
			printf("\n%s Reader %d (%s)\n", ctime(&t), current_reader,
				rgReaderStates_t[current_reader].szReader);

			/* Dump the full current state */
			printf("\tCard state: ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_IGNORE)
				printf("Ignore this reader, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_UNKNOWN)
			{
				printf("Reader unknown\n");
				goto get_readers;
			}

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_UNAVAILABLE)
				printf("Status unavailable, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_EMPTY)
				printf("Card removed, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_PRESENT)
				printf("Card inserted, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_ATRMATCH)
				printf("ATR matches card, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_EXCLUSIVE)
				printf("Exclusive Mode, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_INUSE)
				printf("Shared Mode, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_MUTE)
				printf("Unresponsive card, ");

			printf("\n");

			/* Also dump the ATR if available */
			if (rgReaderStates_t[current_reader].cbAtr > 0)
			{
				printf("\tATR: ");

				if (rgReaderStates_t[current_reader].cbAtr)
				{
					for (i=0; i<rgReaderStates_t[current_reader].cbAtr; i++)
						sprintf(&atr[i*3], "%02X ",
							rgReaderStates_t[current_reader].rgbAtr[i]);

					atr[i*3-1] = '\0';
				}
				else
					atr[0] = '\0';

				printf("%s\n\n", atr);

				if (analyse_atr)
				{
					sprintf(atr_command, ATR_PARSER " '%s'", atr);
					if (system(atr_command))
						perror(atr_command);
				}
			}

		} /* for */

		rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);
	} /* while */

	/* If we get out the loop, GetStatusChange() was unsuccessful */
	printf("SCardGetStatusChange: %lX\n", rv);
	
	/* We try to leave things as clean as possible */
	rv = SCardReleaseContext(hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardReleaseContext: %lX\n", rv);
	}

	/* free memory possibly allocated */
	if (NULL == readers)
		free(readers);
	if (NULL == rgReaderStates_t)
		free(rgReaderStates_t);

	return 0;
} /* main */

