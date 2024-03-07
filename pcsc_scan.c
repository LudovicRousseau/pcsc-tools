/*
    Scan and print all the PC/SC devices available
    Copyright (C) 2001-2022  Ludovic Rousseau <ludovic.rousseau@free.fr>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_OK     0 /* successful termination */
#define EX_OSERR 71 /* system error (e.g., can't fork) */
#define EX_USAGE 64 /* command line usage error */
#endif
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif

#define TIMEOUT 3600*1000	/* 1 hour timeout */


#ifndef SCARD_E_NO_READERS_AVAILABLE
#define SCARD_E_NO_READERS_AVAILABLE 0x8010002E
#endif

#ifdef WIN32
const char *pcsc_stringify_error(DWORD rv)
{
	static char buffer[1024];

	if (! FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		rv,
		LANG_USER_DEFAULT,
		buffer,
		sizeof buffer,
		NULL))
	{
		/* if FormatMessage() failed */
		snprintf(buffer, sizeof(buffer), "ERROR: 0x%08X\n", rv);
	}

	return buffer;
}
#define MAX_ATR_SIZE 33
#endif

#define print_pcsc_error(fct, rv) \
	fprintf(stderr, "%s%s: %s%s\n", red, fct, pcsc_stringify_error(rv), color_end)

#define test_rv(fct, rv, hContext) \
do { \
	if (rv != SCARD_S_SUCCESS) \
	{ \
		print_pcsc_error(fct, rv); \
		(void)SCardReleaseContext(hContext); \
		return -1; \
	} \
} while(0)

static void usage(const char *pname)
{
	printf("%s usage:\n\n\t%s [ -h | -V | -n | -r | -c | -s | -t secs | -d | -p]\n\n", pname, pname);
	printf("\t\t  -h : this help\n");
	printf("\t\t  -V : print version number\n");
	printf("\t\t  -n : no ATR analysis\n");
	printf("\t\t  -r : only lists readers\n");
	printf("\t\t  -c : only lists cards\n");
	printf("\t\t  -s : stress mode\n");
	printf("\t\t  -q : quiet mode\n");
	printf("\t\t  -v : verbose mode (default)\n");
	printf("\t\t  -t secs : quit after secs seconds\n");
	printf("\t\t  -d : debug mode\n");
	printf("\t\t  -p : force use of PnP mechanism\n");
	printf("\n");
}

const char *blue = "";
const char *red = "";
const char *magenta = "";
const char *color_end = "";

const char *cub2 = "";
const char *cub3 = "";
const char *cpl = "";
const char *cnl = "";

time_t start_time;
_Atomic bool Interrupted = false;

typedef struct
{
	const char *pname;
	bool analyse_atr;
	bool stress_card;
	bool print_version;
	bool verbose;
	bool only_list_readers;
	bool only_list_cards;
	bool debug;
	bool pnp;
	long maxtime; // in seconds
} options_t;

static options_t Options;

SCARDCONTEXT hContext;

static bool is_member(const char *  item, const char * list[])
{
	int i = 0;
	while (list[i] && 0 != strcmp(item, list[i]))
	{
		i++;
	}
	return list[i] != 0;
}

static void initialize_terminal(void)
{
	const char *color_terms[] = { "linux", "xterm", "xterm-color",
		"xterm-256color", "Eterm", "rxvt", "rxvt-unicode", "cygwin",
		"wsvt25", 0};
	const char *no_ansi_cursor_terms[] = {"dumb", "emacs", 0};
	const char *term = getenv("TERM");

	/* do not use color if stdout is redirected */
	if (!isatty(fileno(stdout)) || (NULL == term))
	{
		term = "dumb";
	}
	if (is_member(term, color_terms))
	{
		blue = "\033[34m";
		red = "\033[31m";
		magenta = "\033[35m";
		color_end = "\033[0m";
	}
	if (is_member(term, no_ansi_cursor_terms))
	{
		cub2 = "\r"; /* use carriage return */
		cub3 = "\r";
		cpl = "\n"; /* can't do previous line,  let's go to the next line. */
		cnl = "\n";
	}
	else
	{
		cub2 = "\033[2D"; /* move back 2 characters */
		cub3 = "\033[3D"; /* move back 3 characters */
		cpl = "\033[";
	}
}
/* There should be no \033 beyond this line! */

static bool should_exit(void)
{
	if (Options.maxtime)
	{
		time_t t = time(NULL);
		if (t - start_time > Options.maxtime)
			return true;
	}

	if (Interrupted)
		return true;

	return false;
}

typedef enum
{
	SpinDisabled = -2,
	SpinStopped = -1,
	SpinRunning = 0
} SpinState_t;

_Atomic SpinState_t spin_state = SpinStopped;

static void spin_start(void)
{
	spin_state = Options.verbose ? SpinRunning : SpinDisabled;
	printf("%s", cnl);
}

static void spin_stop(void)
{
	spin_state = SpinStopped;

	/* clean previous output */
	printf("%s %s ", cub2, cub2);
	fflush(stdout);
}

static void *spin_update(void *p)
{
	char patterns[] = {'-', '\\', '|', '/'};

	/* 100 ms wait */
	struct timespec wait_time = {.tv_sec = 0, .tv_nsec = 1000*1000*100};

	(void)p;

again:
	/* wait until spinning starts */
	do
	{
		if (should_exit())
		{
			if (SpinDisabled == spin_state)
			{
				SCardCancel(hContext);
				pthread_exit(NULL);
			}
		}

		nanosleep(&wait_time, NULL);
	} while (spin_state < 0);

	do
	{
		char c = patterns[spin_state];

		if (should_exit())
		{
			SCardCancel(hContext);
			pthread_exit(NULL);
		}

		spin_state++;
		if (spin_state >= (int)sizeof patterns)
			spin_state = SpinRunning;
		printf("%s %c ", cub3, c);
		fflush(stdout);

		nanosleep(&wait_time, NULL);
	} while (spin_state >= SpinRunning);

	goto again;

	return NULL;
}

static void user_interrupt_signal_handler(int signal)
{
	(void)signal;
	Interrupted = true;
}

static void initialize_signal_handlers(void)
{
	signal(SIGINT, user_interrupt_signal_handler);
}


static void initialize_options(options_t *options, const char *pname)
{
	options->pname = pname;
#if defined(ATR_PARSER)
	options->analyse_atr = true;
#else
	options->analyse_atr = false;
#define ATR_PARSER ""
#endif
	options->stress_card = false;
	options->print_version = false;
	options->verbose = true;
	options->only_list_readers = false;
	options->only_list_cards = false;
	options->debug = false;
	options->pnp = false;
	options->maxtime = 0;
}

#define OPTIONS "Vhvrcst:qdpn"

static void print_version(void)
{
	printf("PC/SC device scanner\n");
	printf("V %s (c) 2001-2022, Ludovic Rousseau <ludovic.rousseau@free.fr>\n",
			PACKAGE_VERSION);
}

static int parse_options(int argc, char *argv[], options_t *options)
{
	const char *pname = argv[0];
	int opt;
	initialize_options(options, pname);
	while ((opt = getopt(argc, argv, OPTIONS)) != EOF)
	{
		switch (opt)
		{
			case 'n':
				options->analyse_atr = false;
				break;

			case 'V':
				print_version();
				exit(EX_OK);
				break;

			case 'v':
				options->verbose = true;
				break;

			case 'r':
				options->only_list_readers = true;
				options->verbose = false;
				break;

			case 'c':
				options->only_list_cards = true;
				options->verbose = false;
				options->analyse_atr = false;
				break;

			case 's':
				options->stress_card = true;
				break;

			case 't':
				options->maxtime = atol(optarg);
				break;

			case 'q':
				options->verbose = false;
				break;

			case 'p':
				options->pnp = true;
				break;

			case 'h':
				usage(pname);
				exit(EX_OK);

			case 'd':
				options->debug = true;
				break;

			default:
				usage(pname);
				exit(EX_USAGE);
		}
	}
	if (argc - optind != 0)
	{
		fprintf(stderr, "%s error: superfluous arguments: %s\n", pname, argv[optind]);
		usage(pname);
		exit(EX_USAGE);
	}
	return EX_OK;
}

static LONG stress(SCARDCONTEXT hContext2, const char *readerName)
{
	LONG rv, ret_rv = SCARD_S_SUCCESS;
	SCARDHANDLE hCard;
	DWORD dwActiveProtocol;
	const SCARD_IO_REQUEST *pioSendPci;

	if (Options.verbose)
	{
		printf("Stress card in reader: %s\n\n", readerName);
	}
	else
		printf("\n");
	rv = SCardConnect(hContext2, readerName, SCARD_SHARE_SHARED,
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
			fprintf(stderr, "Unknown protocol\n");
			return -1;
	}

	gettimeofday(&time_start, NULL);


#define COUNT 100
	size_t count;
	for (count=0; count<COUNT; count++)
	{
		printf("%sFAPDU n°: %ld\n", cpl, count);
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

		if (Interrupted)
			exit(EX_OSERR);
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
	if (0 == count)
		count = 1;
	printf("%f APDU/s\n", 1000000. / (delta / count));

	rv = SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
	if (rv != SCARD_S_SUCCESS)
	{
		print_pcsc_error("SCardDisconnect", rv);
		ret_rv = rv;
	}

	return ret_rv;
}

static void print_readers(const char **readers, int nbReaders)
{
	int i = 0;
	for (i = 0;i < nbReaders; i++)
	{
		printf("%s%d: %s%s\n", blue, i, readers[i], color_end);
	}
}

static void displayChangedStatus(SCARD_READERSTATE rgReaderStates[], int count)
{
	char state_bits[][sizeof "UNAVAILABLE"] = {
		"IGNORE",		/* 0x0001 */
		"CHANGED",		/* 0x0002 */
		"UNKNOWN",		/* 0x0004 */
		"UNAVAILABLE",	/* 0x0008 */
		"EMPTY",		/* 0x0010 */
		"PRESENT",		/* 0x0020 */
		"ATRMATCH",		/* 0x0040 */
		"EXCLUSIVE",	/* 0x0080 */
		"INUSE",		/* 0x0100 */
		"MUTE",			/* 0x0200 */
		"UNPOWERED"		/* 0x0400 */
	};
	printf("\n");
	for (int i=0; i<count; i++)
	{
		SCARD_READERSTATE r = rgReaderStates[i];
		printf("%d: %s, %d, 0x%04X -> 0x%04X",
			i,
			r.szReader,
			(int)(r.dwEventState >> 16),
			(int)(r.dwCurrentState & 0xFFFF),
			(int)(r.dwEventState & 0xFFFF));
		for (int b=0; b<11; b++)
		{
			int v = 1 << b;
			if ((r.dwEventState & v) && (r.dwCurrentState & v))
				printf(" =%s", state_bits[b]);
			if ((r.dwEventState & v) && !(r.dwCurrentState & v))
				printf(" %s+%s%s", blue, state_bits[b], color_end);
			if (!(r.dwEventState & v) && (r.dwCurrentState & v))
				printf(" %s-%s%s", red, state_bits[b], color_end);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	int current_reader;
#ifdef __APPLE__
	uint32_t rv;
#else
	LONG rv;
#endif
	SCARD_READERSTATE *rgReaderStates_t = NULL;
	SCARD_READERSTATE rgReaderStates[1] = { 0, };
	DWORD dwReaders = 0, dwReadersOld;
	LPSTR mszReaders = NULL;
	char *ptr = NULL;
	const char **readers = NULL;
	int nbReaders, i;
	char atr[MAX_ATR_SIZE*3+1];	/* ATR in ASCII */
	char atr_command[sizeof(atr)+sizeof(ATR_PARSER)+2+1];
	pthread_t spin_pthread;

	start_time = time(NULL);
	initialize_terminal();
	if (0 != parse_options(argc, argv, &Options))
	{
		exit(EX_USAGE);
	}
	if (Options.verbose)
	{
		print_version();
	}

	initialize_signal_handlers();

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	test_rv("SCardEstablishContext", rv, hContext);

	rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
	rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

	rv = SCardGetStatusChange(hContext, 0, rgReaderStates, 1);
	if (! Options.pnp && rgReaderStates[0].dwEventState & SCARD_STATE_UNKNOWN)
	{
		if (Options.verbose)
		{
			printf("%sPlug'n play reader name not supported. Using polling every %d ms.%s\n", magenta, TIMEOUT, color_end);
		}
	}
	else
	{
		Options.pnp = true;
		if (Options.verbose)
		{
			printf("%sUsing reader plug'n play mechanism%s\n", magenta, color_end);
		}
	}

	/* start spining thread */
	rv = pthread_create(&spin_pthread, NULL, spin_update, NULL);

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
	if (Options.verbose)
	{
		printf("%sScanning present readers...%s\n", red, color_end);
	}
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
		fprintf(stderr, "%s: malloc: not enough memory\n", Options.pname);
		exit(EX_OSERR);
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
		if (Options.only_list_cards || Options.only_list_readers)
		{
			printf("No reader found.\n");
			(void)SCardReleaseContext(hContext);
			return EX_OK;
		}

		if (Options.verbose)
		{
			printf("%sWaiting for the first reader...%s   ", red, color_end);
			fflush(stdout);
		}

		if (Options.pnp)
		{
			rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
			rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

			spin_start();
			do
			{
				rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates, 1);
			}
			while (SCARD_E_TIMEOUT == rv);
			spin_stop();

			test_rv("SCardGetStatusChange", rv, hContext);

			if (Options.debug)
				displayChangedStatus(rgReaderStates, 1);
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

				if (should_exit())
				{
					printf("\n");
					exit(EX_OK);
				}
			}
			spin_stop();
		}
		if (Options.verbose)
		{
			printf("found one\n");
		}
		goto get_readers;
	}
	else
		test_rv("SCardListReader", rv, hContext);

	/* allocate the readers table */
	readers = calloc(nbReaders+1, sizeof(char *));
	if (NULL == readers)
	{
		fprintf(stderr, "%s: Not enough memory for readers table\n", Options.pname);
		exit(EX_OSERR);
	}

	/* fill the readers table */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		readers[nbReaders] = ptr;
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (! Options.only_list_cards)
		print_readers(readers, nbReaders);
	if (Options.only_list_readers)
	{
		(void)SCardReleaseContext(hContext);
		exit(EX_OK);
	}

	/* allocate the ReaderStates table */
	rgReaderStates_t = calloc(nbReaders+1, sizeof(* rgReaderStates_t));
	if (NULL == rgReaderStates_t)
	{
		fprintf(stderr, "%s: Not enough memory for readers states\n", Options.pname);
		(void)SCardReleaseContext(hContext);
		exit(EX_OSERR);
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
	if (Options.pnp)
	{
		rgReaderStates_t[nbReaders].szReader = "\\\\?PnP?\\Notification";
		rgReaderStates_t[nbReaders].dwCurrentState = SCARD_STATE_UNKNOWN;
		nbReaders++;
	}

#ifdef WIN32
	int oldNbReaders;
	int oldNbReaders_init = false;
#endif
	spin_start();

	/* Wait endlessly for all events in the list of readers
	 * We only stop in case of an error
	 */
	rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);

	spin_stop();

	if (Options.debug)
		displayChangedStatus(rgReaderStates_t, nbReaders);
	while ((rv == SCARD_S_SUCCESS) || (rv == SCARD_E_TIMEOUT))
	{
		time_t t;

		if (Options.pnp)
		{
			/* check if the number of readers has changed */
#ifdef WIN32
			LONG newNbReaders = rgReaderStates_t[nbReaders-1].dwEventState >> 16;
			if (! oldNbReaders_init)
			{
				oldNbReaders = newNbReaders;
				oldNbReaders_init = true;
			}
			if (newNbReaders != oldNbReaders)
#else
			if (rgReaderStates_t[nbReaders-1].dwEventState &
					SCARD_STATE_CHANGED)
#endif
			{
				goto get_readers;
			}
		}
		else
		{
			/* A new reader appeared? */
			if ((SCardListReaders(hContext, NULL, NULL, &dwReaders)
				== SCARD_S_SUCCESS) && (dwReaders != dwReadersOld))
			{
				goto get_readers;
			}
		}

		if (rv != SCARD_E_TIMEOUT)
		{
			/* Timestamp the event as we get notified */
			t = time(NULL);
			printf("\n%s", ctime(&t));
		}

		/* Now we have an event, check all the readers in the list to see what
		 * happened */
		for (current_reader=0; current_reader < nbReaders; current_reader++)
		{
#if defined(__APPLE__) || defined(WIN32)
			if (rgReaderStates_t[current_reader].dwCurrentState ==
				rgReaderStates_t[current_reader].dwEventState)
				continue;
#endif

			/* The new current state is now the old event state */
			rgReaderStates_t[current_reader].dwCurrentState =
				rgReaderStates_t[current_reader].dwEventState;

			if (! (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_CHANGED))
				/* If nothing changed then skip to the next reader */
				continue;

			/* From here we know that the state for the current reader has
			 * changed because we did not pass through the continue statement
			 * above.
			 */

			/* Specify the current reader's number and name */
			printf(" Reader %d: %s%s%s\n", current_reader,
				magenta, rgReaderStates_t[current_reader].szReader,
				color_end);

			/* Event number */
			printf("  Event number: %s%d%s\n", magenta,
				(int)(rgReaderStates_t[current_reader].dwEventState >> 16),
				color_end);

			/* Dump the full current state */
			printf("  Card state: %s", red);

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_IGNORE)
				printf("Ignore this reader, ");

			if (rgReaderStates_t[current_reader].dwEventState &
				SCARD_STATE_UNKNOWN)
			{
				printf("Unknown\n");
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

				if (Options.analyse_atr)
				{
					printf("\n");

					sprintf(atr_command, ATR_PARSER " '%s'", atr);
					if (system(atr_command))
						perror(atr_command);

					printf("\n");
				}
			}

			LONG state = rgReaderStates_t[current_reader].dwEventState;
			if (state & SCARD_STATE_PRESENT
				&& !(state & SCARD_STATE_MUTE)
				&& Options.stress_card)
			{
				do
				{
					rv = stress(hContext, rgReaderStates_t[current_reader].szReader);
				} while (SCARD_S_SUCCESS == rv);

				rgReaderStates_t[current_reader].dwCurrentState = SCARD_STATE_UNAWARE;
			}
		} /* for */

		if (Options.only_list_cards)
			break;

		spin_start();

		rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t,
			nbReaders);

		spin_stop();

		if (Options.debug)
			displayChangedStatus(rgReaderStates_t, nbReaders);
	} /* while */

	/* A reader disappeared */
	if (SCARD_E_UNKNOWN_READER == rv)
		goto get_readers;

	/* If we get out the loop, GetStatusChange() was unsuccessful */
	if (rv != SCARD_E_CANCELLED)
		test_rv("SCardGetStatusChange", rv, hContext);
	else
	{
		printf("%s", cub3);
		fflush(stdout);
		pthread_join(spin_pthread, NULL);
	}

	/* We try to leave things as clean as possible */
	rv = SCardReleaseContext(hContext);
	test_rv("SCardReleaseContext", rv, hContext);

	/* free memory possibly allocated */
	if (NULL != readers)
		free(readers);
	if (NULL != rgReaderStates_t)
		free(rgReaderStates_t);

	return EX_OK;
}
