/*
    Scan and print all the PC/SC devices available
    Copyright (C) 2001-2018  Ludovic Rousseau <ludovic.rousseau@free.fr>

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
#include <signal.h>
typedef void (*sighandler_t)(int);
#include <sysexits.h>
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

typedef enum
{
	True = TRUE, False = FALSE
} Boolean;

#define TIMEOUT 1000	/* 1 second timeout */


/* command used to parse (on screen) the ATR */
#define ATR_PARSER "ATR_analysis"

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
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
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
	printf("%s usage:\n\n\t%s [ -h | -V | -n | -r | -s] \n\n", pname, pname);
	printf("\t\t  -h : this help\n");
	printf("\t\t  -V : print version number\n");
	printf("\t\t  -n : no ATR analysis\n");
	printf("\t\t  -r : only lists readers\n");
	printf("\t\t  -s : stress mode\n");
	printf("\t\t  -q : quiet mode\n");
	printf("\t\t  -v : verbose mode (default)\n");
	printf("\n");
}

const char *blue = "";
const char *red = "";
const char *magenta = "";
const char *color_end = "";

const char *cub2 = "";
const char *cub3 = "";
const char *cpl = "";

static Boolean is_member(const char *  item, const char * list[])
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
	const char *color_terms[] = { "linux", "xterm", "xterm-color", "xterm-256color",
		"Eterm", "rxvt", "rxvt-unicode", "cygwin", 0};
	const char *no_ansi_cursor_terms[] = {"dumb", "emacs", 0};
	const char *term = getenv("TERM");
	if (term == 0)
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
	}
	else
	{
		cub2 = "\033[2D";
		cub3 = "\033[3D";
		cpl = "\033[";
	}
}
/* There should be no \033 beyond this line! */

Boolean spinning_interrupted = False;
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
	printf("%s %c ", cub3, c);
	fflush(stdout);
}

static void spin_suspend(void)
{
	printf("%s %s", cub2, cub2);
	fflush(stdout);
	if (spinning_interrupted)
	{
		printf("\n");
		exit(EX_OK);
	}
}

static sighandler_t old_interrupt_signal_handler;
#define EX_USER_INTERRUPT (1)

static void user_interrupt_signal_handler(int signal)
{
	if (spinning_interrupted)
	{
		/*
		 * If the user interrupts twice, before the program exits,
		 * then we call the old interrupt signal handler,
		 * or by default we exit.
		 */

		if (old_interrupt_signal_handler == SIG_IGN)
		{
			return;
		}

		if (old_interrupt_signal_handler == SIG_DFL)
		{
			exit(EX_USER_INTERRUPT);
		}

		old_interrupt_signal_handler(signal);
	}
	else
	{
		spinning_interrupted = True;
	}
}

static void initialize_signal_handlers(void)
{
	old_interrupt_signal_handler = signal(SIGINT, user_interrupt_signal_handler);
}


typedef struct
{
	const char *pname;
	Boolean analyse_atr;
	Boolean stress_card;
	Boolean print_version;
	Boolean verbose;
	Boolean only_list_readers;
} options_t;

static options_t Options;

static void initialize_options(options_t *options, const char *pname)
{
	options->pname = pname;
#ifdef WIN32
	options->analyse_atr = False;
#else
	options->analyse_atr = True;
#endif
	options->stress_card = False;
	options->print_version = False;
	options->verbose = True;
	options->only_list_readers = False;
}

#ifdef WIN32
#define OPTIONS "Vhvrsq"
#else
#define OPTIONS "Vhnvrsq"
#endif

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
				options->analyse_atr = False;
				break;

			case 'V':
				options->print_version = True;
				break;

			case 'v':
				options->verbose = True;
				break;

			case 'r':
				options->only_list_readers = True;
				break;

			case 's':
				options->stress_card = True;
				break;

			case 'q':
				options->verbose = False;
				break;

			case 'h':
				usage(pname);
				exit(EX_OK);

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

static LONG stress(SCARDCONTEXT hContext, const char *readerName)
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

static void print_version(void)
{
	printf("PC/SC device scanner\n");
	printf("V %s (c) 2001-2018, Ludovic Rousseau <ludovic.rousseau@free.fr>\n",
			PACKAGE_VERSION);
}

int main(int argc, char *argv[])
{
	int current_reader;
#ifdef __APPLE__
	uint32_t rv;
#else
	LONG rv;
#endif
	SCARDCONTEXT hContext;
	SCARD_READERSTATE *rgReaderStates_t = NULL;
	SCARD_READERSTATE rgReaderStates[1];
	DWORD dwReaders = 0, dwReadersOld;
	LPSTR mszReaders = NULL;
	char *ptr = NULL;
	const char **readers = NULL;
	int nbReaders, i;
	char atr[MAX_ATR_SIZE*3+1];	/* ATR in ASCII */
	char atr_command[sizeof(atr)+sizeof(ATR_PARSER)+2+1];
	int pnp = TRUE;

	initialize_terminal();
	if (0 != parse_options(argc, argv, &Options))
	{
		exit(EX_USAGE);
	}
	if (Options.print_version)
	{
		print_version();
		exit(EX_OK);
	}

#ifdef WIN32
	if (Options.verbose)
	{
		printf("%sPress shift key to quit%s\n", magenta, color_end);
	}
#else
	initialize_signal_handlers();
#endif

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	test_rv("SCardEstablishContext", rv, hContext);

	rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
	rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

	rv = SCardGetStatusChange(hContext, 0, rgReaderStates, 1);
	if (rgReaderStates[0].dwEventState & SCARD_STATE_UNKNOWN)
	{
		if (Options.verbose)
		{
			printf("%sPlug'n play reader name not supported. Using polling every %d ms.%s\n", magenta, TIMEOUT, color_end);
		}
		pnp = FALSE;
	}
	else
	{
		if (Options.verbose)
		{
			printf("%sUsing reader plug'n play mechanism%s\n", magenta, color_end);
		}
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
		if (Options.verbose)
		{
			printf("%sWaiting for the first reader...%s   ", red, color_end);
			fflush(stdout);
		}

		if (pnp)
		{
			rgReaderStates[0].szReader = "\\\\?PnP?\\Notification";
			rgReaderStates[0].dwCurrentState = SCARD_STATE_UNAWARE;

			spin_start();
			do
			{
				rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates, 1);
				spin_update();
#ifdef WIN32
				if (GetKeyState(VK_SHIFT) & 0x80)
					spinning_interrupted = TRUE;
#endif
			}
			while ((SCARD_E_TIMEOUT == rv) && !spinning_interrupted);
			spin_suspend();

			test_rv("SCardGetStatusChange", rv, hContext);
		}
		else
		{
			rv = SCARD_S_SUCCESS;
			spin_start();
			while ((SCARD_S_SUCCESS == rv) && (dwReaders == dwReadersOld) && !spinning_interrupted)
			{
				rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
				if (SCARD_E_NO_READERS_AVAILABLE == rv)
					rv = SCARD_S_SUCCESS;
				sleep(1);
				spin_update();
			}
			spin_suspend();
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

	print_readers(readers, nbReaders);
	if (Options.only_list_readers)
	{
		exit(EX_OK);
	}

	/* allocate the ReaderStates table */
	rgReaderStates_t = calloc(nbReaders+1, sizeof(* rgReaderStates_t));
	if (NULL == rgReaderStates_t)
	{
		fprintf(stderr, "%s: Not enough memory for readers states\n", Options.pname);
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
	if (pnp)
	{
		rgReaderStates_t[nbReaders].szReader = "\\\\?PnP?\\Notification";
		rgReaderStates_t[nbReaders].dwCurrentState = SCARD_STATE_UNAWARE;
		nbReaders++;
	}

#ifdef WIN32
	int oldNbReaders;
	int oldNbReaders_init = FALSE;
#endif
	spin_start();

	/* Wait endlessly for all events in the list of readers
	 * We only stop in case of an error
	 */
	rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);
	while (((rv == SCARD_S_SUCCESS) || (rv == SCARD_E_TIMEOUT)) && !spinning_interrupted)
	{
		time_t t;

		if (pnp)
		{
			/* check if the number of readers has changed */
#ifdef WIN32
			LONG newNbReaders = rgReaderStates_t[nbReaders-1].dwEventState >> 16;
			if (! oldNbReaders_init)
			{
				oldNbReaders = newNbReaders;
				oldNbReaders_init = TRUE;
			}
			if (newNbReaders != oldNbReaders)
#else
			if (rgReaderStates_t[nbReaders-1].dwEventState &
					SCARD_STATE_CHANGED)
#endif
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
#if defined(__APPLE__) || defined(WIN32)
			if (rgReaderStates_t[current_reader].dwCurrentState ==
				rgReaderStates_t[current_reader].dwEventState)
				continue;
#endif

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

			/* Event number */
			printf("  Event number: %s%ld%s\n", magenta,
				rgReaderStates_t[current_reader].dwEventState >> 16,
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
				} while (SCARD_S_SUCCESS == rv && ! spinning_interrupted);

				rgReaderStates_t[current_reader].dwCurrentState = SCARD_STATE_UNAWARE;
			}
		} /* for */

		rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t,
			nbReaders);

		spin_update();

#ifdef WIN32
	if (GetKeyState(VK_SHIFT) & 0x80)
		spinning_interrupted = TRUE;
#endif
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

	return EX_OK;
}
