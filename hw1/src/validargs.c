#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    // // TO BE IMPLEMENTED.
	if(argc < 2) {
		global_options = 0x0;
		error("Less than 2 args");
		return -1;
	}
	for(int i = 1; i < argc; i++) {
		argv++;
		// printf("%s\n", argv);
		// printf("count: %i\n", argc);
		// printf("%c\n", **argv);
		// printf("%c", 'h');
		char* head = *argv;
		if(**argv == '-') {
			char letter = *(*argv + 1);
			// printf("%c\n", letter);
			char nullTerm = *(*argv + 2);
			// printf("hi: %c\n", '\0');
			if(nullTerm != '\0') {
				global_options = 0x0;
				// printf("not null terminated");
				debug("Unknown flag");
				return -1;
			}
			// if(letter == 'h' || letter == 'n' || letter == 'q') {
			if(letter == 'h') {
				if(i == 1) {
					global_options = HELP_OPTION;
					success("flag -h active");
					return 0;
				} else {
					global_options = 0x0;
					error("-h not last argument");
					return -1;
				}
			}
			// if(letter != 'n' && letter != 'q' && i != argc - 1) {
			// 	return -1;
			// }
			if(letter == 'n') {
				if(i == argc - 1) {
					global_options = 0x0;
					error("-n is last argv");
					return -1;
				} else {
					success("flag -n active");
					global_options = global_options | NO_PATCH_OPTION;
				}
			}
			if(letter == 'q') {
				if(i == argc - 1) {
					global_options = 0x0;
					error("-q is last argv");
					return -1;
				} else {
					success("flag -q active");
					global_options = global_options | QUIET_OPTION;
				}
				// } else {
				// 	return -1;
				// }
			}
			// return -1; //if none of those letters
		} else if(i == argc - 1) {
			diff_filename = head;
			success("diff_name set: %s", diff_filename);
			return 0;
		} else {
			global_options = 0x0;
			error("filename not last argv");
			return -1;
		}
	}
	return -1;
    // abort();
}
