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

/* $Id: pcsc_scan.c,v 1.4 2001-11-08 08:34:04 rousseau Exp $ */

/*
 * $Log: not supported by cvs2svn $
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

#include <winscard.h>

int main(int argc, char *argv[])
{
	LONG rv;
	SCARDCONTEXT hContext;
	SCARD_READERSTATE_A rgReaderStates_t[PCSCLITE_MAX_CHANNELS], rgReaderStates;
	DWORD dwReaders;
	LPSTR mszReaders;
	char *ptr, *readers[PCSCLITE_MAX_CHANNELS];
	int nbReaders, current_reader, i;

	printf("PC/SC device scanner\n");
	printf("V " VERSION " (c) 2001, Ludovic Rousseau <ludovic.rousseau@free.fr>\n");
	printf("PCSC lite version: " PCSCLITE_VERSION_NUMBER "\n");

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardEstablishContext: Cannot Connect to Resource Manager %lX\n", rv);
		return 1;
	}

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

	/* set the initial states */
	for (i=0; i<nbReaders; i++)
	{
		rgReaderStates_t[i].szReader = readers[i];
		rgReaderStates_t[i].dwCurrentState = SCARD_STATE_EMPTY;
	}

	current_reader = 0;
	while (1)
	{
		time_t t;

		rgReaderStates = rgReaderStates_t[current_reader];
		
		rv = SCardGetStatusChange(hContext, 0, &rgReaderStates, 1);
		
		if (rgReaderStates.dwCurrentState != rgReaderStates.dwEventState)
		{
			rgReaderStates.dwCurrentState = rgReaderStates.dwEventState;
			rgReaderStates_t[current_reader] = rgReaderStates;

			if (rv == SCARD_E_TIMEOUT)
				continue;

			if (rv != SCARD_S_SUCCESS)
				printf("SCardGetStatusChange: %lX\n", rv);

			t = time(NULL);
			printf("\n%s Reader %d (%s)\n", ctime(&t), current_reader,
				rgReaderStates.szReader);

			printf("\tCard state: ");

			if (rgReaderStates.dwEventState & SCARD_STATE_IGNORE)
				printf("Ignore this reader, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_CHANGED)
				printf("State has changed, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_UNKNOWN)
				printf("Reader unknown, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_UNAVAILABLE)
				printf("Status unavailable, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_EMPTY)
				printf("Card removed, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_PRESENT)
				printf("Card inserted, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_ATRMATCH)
				printf("ATR matches card, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_EXCLUSIVE)
				printf("Exclusive Mode, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_INUSE)
				printf("Shared Mode, ");

			if (rgReaderStates.dwEventState & SCARD_STATE_MUTE)
				printf("Unresponsive card, ");

			printf("\n");

			if (rgReaderStates.cbAtr > 0)
			{
				printf("\tATR: ");
				for (i=0; i<rgReaderStates.cbAtr; i++)
					printf("%02X ", rgReaderStates.rgbAtr[i]);
				printf("\n");
			}
		}

		/* next reader */
		current_reader++;
		if (current_reader >= nbReaders)
			current_reader = 0;
	}

	rv = SCardReleaseContext(hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		printf("SCardReleaseContext: %lX\n", rv);
	}

	return 0;
} /* main */

