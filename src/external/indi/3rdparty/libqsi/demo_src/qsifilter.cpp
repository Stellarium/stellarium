/******************************************************************************
NAME
 QSIFILTER 

DESCRIPTION
 Utility program to control/name filters

COPYRIGHT (C)
	Quantum Scientific Imaging, Inc.
 	as a contribution from Andreas Mueller

 ******************************************************************************/

#include "qsiapi.h"
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <cstdlib>

void	usage(const char *progname) {
	std::cout << "usage:" <<std::endl;
	std::cout << progname << " [ options ] help" << std::endl;
	std::cout << progname << " [ options ] list" << std::endl;
	std::cout << progname << " [ options ] position { <pos> | <name> }" << std::endl;
	std::cout << progname << " [ options ] name <pos> <name>" << std::endl;
	std::cout << progname << " [ options ] offset <pos> <offset>" << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << " -s <serial>     serial number of camera to connect to";
	std::cout << std::endl;
	std::cout << " -h, -?          display this help message and exit";
	std::cout << std::endl;
}

QSICamera cam;

int main(int argc, char** argv)
{
	int filters = 0;

	char	*serial = NULL;

	bool hasFilters;

	int	c;
	while (EOF != (c = getopt(argc, argv, "s:h?"))) 
		switch (c) {
		case 's':
			serial = optarg;
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			break;
		}

	// next argument is the command
	if (argc <= optind) {
		std::cerr << "command missing" << std::endl;
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	std::string	command(argv[optind++]);

	// handle the help command
	if (command == "help") {
		usage(argv[0]);
		return EXIT_SUCCESS;
	}

	// now start talking to the 
	cam.put_UseStructuredExceptions(true);
	try
	{
		//Discover the connected cameras
		int iNumFound;
		std::string camSerial[QSICamera::MAXCAMERAS];
		std::string camDesc[QSICamera::MAXCAMERAS];

		cam.get_AvailableCameras(camSerial, camDesc, iNumFound);

		if (iNumFound == 0) {
			std::cerr << "no camera found" << std::endl;
			return EXIT_FAILURE;
		}

		if ((iNumFound > 1) && (serial == NULL)) {
			std::cerr << "multiple cameras, must specify serial:";
			std::cerr << std::endl;
			for (int i = 0; i < iNumFound; i++)
			{
				std::cerr << camSerial[i] << ":" << camDesc[i] << "\n";
			}
			return EXIT_FAILURE;
		}

		if (serial) {
			cam.put_SelectCamera(std::string(serial));
		} 
		
		// connect to the camera
		cam.put_Connected(true);

		cam.get_HasFilterWheel(&hasFilters);
	
		if (!hasFilters)
		{
			std::cerr << "camera has no filters" << std::endl;
			return EXIT_FAILURE;
		}

		// How many filter positions?
		cam.get_FilterCount(filters);
	} 
	catch (std::runtime_error &err)
	{
		std::string text = err.what();
		std::cout << text << "\n";
		std::string last("");
		cam.get_LastError(last);
		std::cout << last << "\n";
		std::cout << "exiting with errors\n";
		exit(1);
	}

	// get a list of names for the filters
	try {
		cam.get_FilterCount(filters);
		// Allocate arrays for filter names and offsets
		std::string	*names = new std::string[filters];
		long	*offsets = new long[filters];
		// Get the names and filter offsets
		// values are stored in the .QSIConfig file in
		// the user's home directory
		cam.get_Names(names);
		cam.get_FocusOffset(offsets);

		// current filter position
		short	currentpos;
		cam.get_Position(&currentpos);

		// list the current filter status
		if (command == "list") {
			for (int i = 0; i < filters; i++) {
				std::cout << "[" << (i + 1) << "]: ";
				std::cout << "\"" << names[i] << "\" ";
				std::cout << offsets[i];
				std::cout << std::endl;
			}
			return EXIT_SUCCESS;
		}

		// position command
		if (command == "position") {
			if (argc > optind) {
				std::string	pos(argv[optind]);
				short	position = atoi(pos.c_str());
				// try position by name
				if (position == 0) {
					for (short i = 0; i < filters; i++) {
						if (names[i] == pos) {
							position = i;
						}
					}
				} else {
					position = position - 1;
				}
				cam.put_Position(position);
				std::cout << "new position: " << position;
				std::cout << std::endl;
			} else {
				std::cout << "current position: ";
				std::cout << currentpos << std::endl;
			}
			return EXIT_SUCCESS;
		}

		// name a filter
		if (command == "name") {
			if (argc <= optind + 1) {
				std::cerr << "not enough arguments" << std::endl;
				return EXIT_FAILURE;
			}
			int	pos = atoi(argv[optind++]);
			if ((pos < 1) || (pos > filters)) {
				std::cerr << "not a valid filter position";
				std::cerr << std::endl;
				return EXIT_FAILURE;
			}
			names[pos - 1] = argv[optind];
			cam.put_Names(names);
			return EXIT_SUCCESS;
		}

		// set the focus offset for a filter
		if (command == "offset") {
			if (argc <= optind + 1) {
				std::cerr << "not enough arguments" << std::endl;
				return EXIT_FAILURE;
			}
			int	pos = atoi(argv[optind++]);
			if ((pos < 1) || (pos > filters)) {
				std::cerr << "not a valid filter position";
				std::cerr << std::endl;
				return EXIT_FAILURE;
			}
			offsets[pos - 1] = atoi(argv[optind]);
			cam.put_FocusOffset(offsets);
			return EXIT_SUCCESS;
		}

		std::cerr << "command '" << command << "' unknown" << std::endl;
		return EXIT_FAILURE;
	}
	catch (std::runtime_error &err)
	{
		std::string text = err.what();
		std::cout << text << "\n";
		std::string last("");
		cam.get_LastError(last);
		std::cout << last << "\n";
		std::cout << "exiting with errors\n";
		exit(1);
	}
}
