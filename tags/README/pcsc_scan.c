/*
    Scan and print all the PC/SC devices available
    Copyright (C) 2001  Ludovic Rousseau <ludovic.rousseau@free.fr>

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

/* $Id: pcsc_scan.c,v 1.8 2002-06-14 07:55:37 rousseau Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.7  2002/05/15 11:37:49  lvictor
 * Readers states are initialized with SCARD_STATE_UNAWARE to make sure the
 * first query will update the state to something we know.
 * Modified the event loop to monitor all readers at once. SCARD_STATE_CHANGED
 * is used to detect which reader triggered the event.
 *
 * Revision 1.6  2002/05/14 16:03:44  lvictor
 * Added comments and modified the event listener to listen to all readers at
 * once... This demonstrates the possibility to use a list of readers to
 * listen events to...
 *
 * Revision 1.5  2001/11/08 08:46:49  rousseau
 * change PCSC to PC/SC
 *
 * Revision 1.4  2001/11/08 08:34:04  rousseau
 * set the wait time to 0 to get all the events
 *
 * Revision 1.3  2001/10/22 08:54:55  rousseau
 * go to the next reader _after_ printing information for the current oone
 *
 * Revision 1.2  2001/10/16 07:31:51  rousseau
 * commented number of allocated readers (wrong info?)
 *
 * Revision 1.1.1.1  2001/10/16 07:24:07  rousseau
 * Created directory structure
 *
 * Revision 1.3  2001/09/14 17:57:07  rousseau
 * debugged support of multi readers
 * added time information
 *
 * Revision 1.2  2001/09/13 21:03:01  rousseau
 * print each state change
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <winscard.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

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
	SCARD_READERSTATE_A rgReaderStates_t[PCSCLITE_MAX_CHANNELS];
	DWORD dwReaders;
	LPSTR mszReaders;
	char *ptr, *readers[PCSCLITE_MAX_CHANNELS];
	int nbReaders, i;
	char atr[MAX_ATR_SIZE*3+1];
	char atr_command[sizeof(atr)+sizeof(ATR_PARSER)+2+1];
	int opt;
	int analyse_atr = TRUE;

	printf("PC/SC device scanner\n");
	printf("V " VERSION " (c) 2001-2002, Ludovic Rousseau <ludovic.rousseau@free.fr>\n");
	printf("PC/SC lite version: " PCSCLITE_VERSION_NUMBER "\n");

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

	/* Retrieve the available readers list.
	 *
	 * 1. Call with a null buffer to get the number of bytes to allocate
	 * 2. malloc the necessary storage
	 * 3. call with the real allocated buffer
	 */
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardListReader: %lX\n", rv);
	}
	//printf("%ld allocated reader(s)\n", dwReaders);

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

	/* Extract readers from the null separated string and get thetotal
	 * number of readers
	 */
	nbReaders = 0;
	ptr = mszReaders;
	while ((*ptr != '\0') && (nbReaders < PCSCLITE_MAX_CHANNELS))
	{
		printf("%d: %s\n", nbReaders, ptr);
		readers[nbReaders] = ptr;
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (nbReaders == 0)
	{
		printf("No reader found\n");
		return 1;
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
	while ((rv = SCardGetStatusChange(hContext, INFINITE, rgReaderStates_t,
		nbReaders)) == SCARD_S_SUCCESS)
	{
		time_t t;

		// Now we have an event, check all the readers in the list to see what
		// happened
		for (current_reader=0; current_reader < nbReaders; current_reader++)
		{
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
				printf("Reader unknown, ");

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
	} /* while */

	/* If we get out the loop, GetStatusChange() was unsuccessful */
	printf("SCardGetStatusChange: %lX\n", rv);
	
	/* We try to leave things as clean as possible */
	rv = SCardReleaseContext(hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardReleaseContext: %lX\n", rv);
	}

	return 0;
} /* main */

