/*
    Scan and print all the PC/SC devices available
    Copyright (C) 2001-2017  Ludovic Rousseau <ludovic.rousseau@free.fr>

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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define TIMEOUT 1000	/* 1 second timeout */

/* command used to parse (on screen) the ATR */
#define ATR_PARSER "ATR_analysis"

#ifndef SCARD_E_NO_READERS_AVAILABLE
#define SCARD_E_NO_READERS_AVAILABLE 0x8010002E
#endif

#define print_pcsc_error(fct, rv) \
	printf("%s%s: %s%s\n", red, fct, pcsc_stringify_error(rv), color_end)

#define test_rv(fct, rv, hContext) \
do { \
	if (rv != SCARD_S_SUCCESS) \
	{ \
		print_pcsc_error(fct, rv); \
		(void)SCardReleaseContext(hContext); \
		return -1; \
	} \
} while(0)

static void usage(void)
{
	printf("usage: pcsc_scan [-n] [-V] [-h]\n");
	printf("  -n : no ATR analysis\n");
	printf("  -V : print version number\n");
	printf("  -h : this help\n");
} /* usage */

const char *blue = "";
const char *red = "";
const char *magenta = "";
const char *color_end = "";

unsigned int spin_state = 0;
static void spin_start(void)
{
	spin_state = 0;
}

static void spin_update(void)
{
	char patterns[] = {'-', '\\', '|', '/'};
	char c = patterns[spin_state];
	spin_state++;
	if (spin_state >= sizeof patterns)
		spin_state = 0;
	printf("\33[3D %c ", c);
	fflush(stdout);
}

static void spin_suspend(void)
{
	printf("\33[2D \33[2D");
	fflush(stdout);
}

static LONG stress(LONG hContext, const char *readerName)
{
	LONG rv, ret_rv = SCARD_S_SUCCESS;
	SCARDHANDLE hCard;
	DWORD dwActiveProtocol;
	const SCARD_IO_REQUEST *pioSendPci;

	printf("Stress card\n\n");
	rv = SCardConnect(hContext, readerName, SCARD_SHARE_SHARED,
         SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
	if (rv != SCARD_S_SUCCESS)
	{
		print_pcsc_error("SCardConnect", rv);
		return rv;
	}

	/* Select Master File */
	BYTE pbSendBuffer[] = {0, 0xA4, 0, 0, 2, 0x3F, 0};
	BYTE pbRecvBuffer[256+2];
	DWORD dwSendLength, dwRecvLength;
	struct timeval time_start, time_end;

	switch (dwActiveProtocol)
	{
		case SCARD_PROTOCOL_T0:
			pioSendPci = SCARD_PCI_T0;
			break;
		case SCARD_PROTOCOL_T1:
			pioSendPci = SCARD_PCI_T1;
			break;
		case SCARD_PROTOCOL_RAW:
			pioSendPci = SCARD_PCI_RAW;
			break;
		default:
			printf("Unknown protocol\n");
			return -1;
	}

	gettimeofday(&time_start, NULL);

#define COUNT 100
	size_t count;
	for (count=0; count<COUNT; count++)
	{
		printf("\033[FAPDU n°: %ld\n", count);
		dwSendLength = sizeof(pbSendBuffer);
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbSendBuffer, dwSendLength,
			NULL, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			print_pcsc_error("SCardTransmit", rv);
			ret_rv = rv;
			break;
		}
	}

	gettimeofday(&time_end, NULL);
	struct timeval r;
	r.tv_sec = time_end.tv_sec - time_start.tv_sec;
	r.tv_usec = time_end.tv_usec - time_start.tv_usec;
	if (r.tv_usec < 0)
	{
		r.tv_sec--;
		r.tv_usec += 1000000;
	}
	long delta = r.tv_sec * 1000000 + r.tv_usec;
	printf("Total time: %ld µs\n", delta);
	printf("%f APDU/s\n", 1000000. / (delta / count));

	rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
	if (rv != SCARD_S_SUCCESS)
	{
		print_pcsc_error("SCardDisconnect", rv);
		ret_rv = rv;
	}

	return ret_rv;
}

int main(int argc, char *argv[])
{
	int current_reader;
	LONG rv;
	SCARDCONTEXT hContext;
	SCARD_READERSTATE *rgReaderStates_t = NULL;
	SCARD_READERSTATE rgReaderStates[1];
	DWORD dwReaders = 0, dwReadersOld;
	LPSTR mszReaders = NULL;
	char *ptr, **readers = NULL;
	int nbReaders, i;
	char atr[MAX_ATR_SIZE*3+1];	/* ATR in ASCII */
	char atr_command[sizeof(atr)+sizeof(ATR_PARSER)+2+1];
	int opt;
	int analyse_atr = TRUE;
	int stress_card = FALSE;
	int pnp = TRUE;

	printf("PC/SC device scanner\n");
	printf("V " PACKAGE_VERSION " (c) 2001-2017, Ludovic Rousseau <ludovic.rousseau@free.fr>\n");

	while ((opt = getopt(argc, argv, "Vhns")) != EOF)
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

			case 's':
				stress_card = TRUE;
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

	/* check if terminal supports color */
	{
		const char *terms[] = { "linux", "xterm", "xterm-color", "xterm-256color", "Eterm", "rxvt", "rxvt-unicode" };
		char *term;

		term = getenv("TERM");
		if (term)
		{
			size_t j;

			/* for each known color terminal */
			for (j = 0; j < sizeof(terms) / sizeof(terms[0]); j++)
			{
				/* we found a supported term? */
				if (0 == strcmp(terms[j], term))
				{
					blue = "\33[34m";
					red = "\33[31m";
					magenta = "\33[35m";
					color_end = "\33[0m";
					break;
				}
			}
		}
	}

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	test_rv("SCardEstablishContext", rv, hContext);

	rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
	rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

	rv = SCardGetStatusChange(hContext, 0, rgReaderStates, 1);
	if (rgReaderStates[0].dwEventState & SCARD_STATE_UNKNOWN)
	{
		printf("%sPlug'n play reader name not supported. Using polling every %d ms.%s\n", magenta, TIMEOUT, color_end);
		pnp = FALSE;
	}
	else
	{
		printf("%sUsing reader plug'n play mechanism%s\n", magenta, color_end);
	}

get_readers:
	/* free memory possibly allocated in a previous loop */
	if (NULL != readers)
	{
		free(readers);
		readers = NULL;
	}

	if (NULL != rgReaderStates_t)
	{
		free(rgReaderStates_t);
		rgReaderStates_t = NULL;
	}

	/* Retrieve the available readers list.
	 *
	 * 1. Call with a null buffer to get the number of bytes to allocate
	 * 2. malloc the necessary storage
	 * 3. call with the real allocated buffer
	 */
	printf("%sScanning present readers...%s\n", red, color_end);
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
	if (rv != SCARD_E_NO_READERS_AVAILABLE)
		test_rv("SCardListReaders", rv, hContext);

	dwReadersOld = dwReaders;

	/* if non NULL we came back so free first */
	if (mszReaders)
	{
		free(mszReaders);
		mszReaders = NULL;
	}

	mszReaders = malloc(sizeof(char)*dwReaders);
	if (mszReaders == NULL)
	{
		printf("malloc: not enough memory\n");
		return 1;
	}

	*mszReaders = '\0';
	rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);

	/* Extract readers from the null separated string and get the total
	 * number of readers */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (SCARD_E_NO_READERS_AVAILABLE == rv || 0 == nbReaders)
	{
		printf("%sWaiting for the first reader...%s   ", red, color_end);
		fflush(stdout);

		if (pnp)
		{
			rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
			rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

			spin_start();
			do
			{
				rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates, 1);
				spin_update();
			}
			while (SCARD_E_TIMEOUT == rv);
			spin_suspend();
			test_rv("SCardGetStatusChange", rv, hContext);
		}
		else
		{
			rv = SCARD_S_SUCCESS;
			spin_start();
			while ((SCARD_S_SUCCESS == rv) && (dwReaders == dwReadersOld))
			{
				rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
				if (SCARD_E_NO_READERS_AVAILABLE == rv)
					rv = SCARD_S_SUCCESS;
				sleep(1);
				spin_update();
			}
			spin_suspend();
		}
		printf("found one\n");
		goto get_readers;
	}
	else
		test_rv("SCardListReader", rv, hContext);

	/* allocate the readers table */
	readers = calloc(nbReaders+1, sizeof(char *));
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
		printf("%s%d: %s%s\n", blue, nbReaders, ptr, color_end);
		readers[nbReaders] = ptr;
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	/* allocate the ReaderStates table */
	rgReaderStates_t = calloc(nbReaders+1, sizeof(* rgReaderStates_t));
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
		rgReaderStates_t[i].cbAtr = sizeof rgReaderStates_t[i].rgbAtr;
	}

	/* If Plug and Play is supported by the PC/SC layer */
	if (pnp)
	{
		rgReaderStates_t[nbReaders].szReader = "\\\\?PnP?\\Notification";
		rgReaderStates_t[nbReaders].dwCurrentState = SCARD_STATE_UNAWARE;
		nbReaders++;
	}

	spin_start();

	/* Wait endlessly for all events in the list of readers
	 * We only stop in case of an error
	 */
	rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);
	while ((rv == SCARD_S_SUCCESS) || (rv == SCARD_E_TIMEOUT))
	{
		time_t t;

		if (pnp)
		{
			if (rgReaderStates_t[nbReaders-1].dwEventState &
					SCARD_STATE_CHANGED)
			{
				spin_suspend();
				goto get_readers;
			}
		}
		else
		{
			/* A new reader appeared? */
			if ((SCardListReaders(hContext, NULL, NULL, &dwReaders)
				== SCARD_S_SUCCESS) && (dwReaders != dwReadersOld))
			{
				spin_suspend();
				goto get_readers;
			}
		}

		if (rv != SCARD_E_TIMEOUT)
		{
			spin_suspend();
			/* Timestamp the event as we get notified */
			t = time(NULL);
			printf("\n%s", ctime(&t));
		}

		/* Now we have an event, check all the readers in the list to see what
		 * happened */
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

			spin_suspend();

			/* Specify the current reader's number and name */
			printf(" Reader %d: %s%s%s\n", current_reader,
				magenta, rgReaderStates_t[current_reader].szReader,
				color_end);

			/* Dump the full current state */
			printf("  Card state: %s", red);

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

			printf("%s\n", color_end);

			/* force display */
			fflush(stdout);

			/* Also dump the ATR if available */
			if (rgReaderStates_t[current_reader].cbAtr > 0)
			{
				printf("  ATR: ");

				if (rgReaderStates_t[current_reader].cbAtr)
				{
					unsigned int j;

					for (j=0; j<rgReaderStates_t[current_reader].cbAtr; j++)
						sprintf(&atr[j*3], "%02X ",
							rgReaderStates_t[current_reader].rgbAtr[j]);

					atr[j*3-1] = '\0';
				}
				else
					atr[0] = '\0';

				printf("%s%s%s\n", magenta, atr, color_end);

				/* force display */
				fflush(stdout);

				if (analyse_atr)
				{
					printf("\n");

					sprintf(atr_command, ATR_PARSER " '%s'", atr);
					if (system(atr_command))
						perror(atr_command);
				}
			}

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_PRESENT && stress_card)
			{
				stress(hContext, rgReaderStates_t[current_reader].szReader);
			}
		} /* for */

		rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t,
			nbReaders);

		spin_update();
	} /* while */

	spin_suspend();

	/* A reader disappeared */
	if (SCARD_E_UNKNOWN_READER == rv)
		goto get_readers;

	/* If we get out the loop, GetStatusChange() was unsuccessful */
	test_rv("SCardGetStatusChange", rv, hContext);

	/* We try to leave things as clean as possible */
	rv = SCardReleaseContext(hContext);
	test_rv("SCardReleaseContext", rv, hContext);

	/* free memory possibly allocated */
	if (NULL != readers)
		free(readers);
	if (NULL != rgReaderStates_t)
		free(rgReaderStates_t);

	return 0;
} /* main */

