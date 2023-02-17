#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

static void resetHunk(HUNK *hp) {
	hp->type = HUNK_NO_TYPE;
	hp->serial = 0;
	hp->old_start = 0;
	hp->old_end = 0;
	hp->new_start = 0;
	hp->new_end = 0;
}

/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

static int serial = 0;
// static int ch;
static int old_range = 0;//old_end - old_start
static int new_range = 0;//new_end - new_start
static int hunkCompleted = 1;// 1 if hunk completed 0 if not
static char *delArr = hunk_deletions_buffer;
// *(hunk_deletions_buffer + 510) = (unsigned char) 0x0;
// *(hunk_deletions_buffer + 511) = (unsigned char) 0x0;
// *(hunk_deletions_buffer + 509) = (unsigned char) 0x0;
static char *addArr = hunk_additions_buffer;
// *(hunk_additions_buffer + 510) = (unsigned char) 0x0;
// *(hunk_additions_buffer + 511) = (unsigned char) 0x0;
// *(hunk_additions_buffer + 509) = (unsigned char) 0x0;

int hunk_next(HUNK *hp, FILE *in) {
    // TO BE IMPLEMENTED
	int num;
	int ch;
	while((ch = fgetc(in)) != EOF) {
		//prinbt out while file
		// if(ch == '\n') {
		// 	printf("\nnew line\n");
		// } else {
		// 	printf("%c", ch);
		// }
		//Read line
		// if(ch == '<' || ch == '>' || ch == '-') {
		// 	debug("Go to next line");
		// 	while(ch != '\n') {
		// 		ch = fgetc(in);
		// 	}
		// 	return hunk_next(hp, in);
		// }
		//Read hunk
		// 0-9[, 0-9] [a,c,d] 0-9[, 0-9]

		//check if first ch is a num
		if(ch < '0' || ch > '9') {
			error("Hunk doesn't start with a number 0 - 9");
			resetHunk(hp);
			return ERR;
		}
		// while(ch < '0' || ch > '9') {
		// 	error("Hunk doesn't start with a number 0 - 9");
		// 	resetHunk(hp);
		// 	return ERR;
		// }
		num = 0;
		//get old_start
		while(ch >= '0' && ch <= '9') {
			num = num * 10 + (ch - '0');
			ch = fgetc(in);
		}
		hp->old_start = num;
		if(ch != ',' && ch != 'a' && ch != 'c' && ch != 'd') {
			error("Not ',' or a,d,c");
			resetHunk(hp);
			return ERR;
		}
		//ch has to be ",", a c or d or else error would've been thrown before
		//get old_end
		if(ch == ',') {
			ch = fgetc(in);
			if(ch < '0' || ch > '9') {
				error("Number doesn't start with a number 0 - 9");
				resetHunk(hp);
				return ERR;
			}
			num = 0;
			while(ch >= '0' && ch <= '9') {
				num = num * 10 + (ch - '0');
				ch = fgetc(in);
			}
			if(ch != 'a' && ch != 'c' && ch != 'd') {
				error("Not a,d or c");
				resetHunk(hp);
				return ERR;
			} else {
				hp->old_end = num;
			}
		} else {
			hp->old_end = hp->old_start;
		}



		//old version of checking old_start and end
		// if(ch >= '0' && ch <= '9') {
		// 	// check first two num
		// 	// num = num * 10 + ch - '0';
		// 	// ch = fgetc(in);
		// 	hp->old_start = ch - '0';
		// 	debug("old_start = %c", ch);
		// 	ch = fgetc(in);
		// 	if(ch == ',') {
		// 		ch = fgetc(in);
		// 		if(ch >= '0' && ch <= '9') {
		// 			hp->old_end = ch - '0';
		// 			debug("old_end = %c", ch);
		// 		} else {
		// 			error("Second character not a number");
		// 			resetHunk(hp);
		// 			return ERR;
		// 		}
		// 	} else if(ch == 'a' || ch == 'c' || ch == 'd') {
		// 		hp->old_end = hp->old_start;
		// 		//add hunk type back to stream
		// 		// if(ch == 'a') {
		// 		// 	hp->type = HUNK_APPEND_TYPE;
		// 		// }
		// 		// if(ch == 'c') {
		// 		// 	hp->type = HUNK_CHANGE_TYPE;
		// 		// }
		// 		// if(ch == 'd') {
		// 		// 	hp->type = HUNK_DELETE_TYPE;
		// 		// }
		// 		ungetc(ch, in);
		// 	} else {
		// 		error("Not ',' or a,d,c");
		// 		resetHunk(hp);
		// 		return ERR;
		// 	}
		// } else {
		// 	error("Hunk doesn't start with a number 0 - 9");
		// 	resetHunk(hp);
		// 	return ERR;
		// }

		//check hunk type
		// if(ch == 'a') {
		// 	if(hp->old_start != hp->old_end) {
		// 		error("Append, old_start != old_end");
		// 		return ERR;
		// 	}
		// 	hp->type = HUNK_APPEND_TYPE;
		// }
		// if(ch == 'c') {
		// 	hp->type = HUNK_CHANGE_TYPE;
		// }
		// if(ch == 'd') {
		// 	hp->type = HUNK_DELETE_TYPE;
		// }

		switch(ch) {
			case 'a':
				if(hp->old_start != hp->old_end) {
					error("Append, old_start != old_end");
					return ERR;
				}
				hp->type = HUNK_APPEND_TYPE;
				break;
			case 'c':
				hp->type = HUNK_CHANGE_TYPE;
				break;
			case 'd':
				hp->type = HUNK_DELETE_TYPE;
				break;
		}




		//check new_start and new_end
		ch = fgetc(in);
		//get new_start
		if(ch < '0' && ch > '9') {
			error("Number doesn't start with a number 0 - 9");
			resetHunk(hp);
			return ERR;
		}
		num = 0;
		while(ch >= '0' && ch <= '9') {
			num = num * 10 + (ch - '0');
			ch = fgetc(in);
		}
		hp->new_start = num;
		if(ch != ',' && ch != '\n') {
			error("Not ',' or new line");
			resetHunk(hp);
			return ERR;
		}
		//ch has to be "," or "\n"
		//get new_end
		if(ch == ',') {
			ch = fgetc(in);
			if(ch < '0' || ch > '9') {
				error("Number doesn't start with a number 0 - 9");
				resetHunk(hp);
				return ERR;
			}
			num = 0;
			while(ch >= '0' && ch <= '9') {
				num = num * 10 + (ch - '0');
				ch = fgetc(in);
			}
			if(ch != '\n') {
				error("Not new line");
				resetHunk(hp);
				return ERR;
			} else {
				hp->new_end = num;
			}
		} else {
			hp->new_end = hp->new_start;
		}



		//old version of checking new_start and end
		// ch = fgetc(in);

		// if(ch >= '0' && ch <= '9') {
		// 	hp->new_start = ch - '0';
		// 	debug("new_start = %c", ch);
		// 	ch = fgetc(in);
		// 	if(ch == ',') {
		// 		ch = fgetc(in);
		// 		if(ch >= '0' && ch <= '9') {
		// 			hp->new_end = ch - '0';
		// 			debug("new_end = %c", ch);
		// 		} else {
		// 			error("Second character not a number");
		// 			resetHunk(hp);
		// 			return ERR;
		// 		}
		// 	} else if(ch == '\n') {
		// 		hp->new_end = hp->new_start;
		// 		ungetc(ch, in);
		// 	} else {
		// 		error("Not ',' or a,d,c");
		// 		resetHunk(hp);
		// 		return ERR;
		// 	}
		// } else {
		// 	error("Hunk doesn't start with a number 0 - 9");
		// 	resetHunk(hp);
		// 	return ERR;
		// }


		//check for delete if new_start == new_end
		if(hp->type == 2 && hp->new_start != hp->new_end) {
			error("Delete, new_start != new_end");
			resetHunk(hp);
			return ERR;
		}

		//check if hunk line ends as \n
		if(ch != '\n') {
			error("Did not end in new line");
			resetHunk(hp);
			ungetc(ch, in);
			return ERR;
		}

		//check if numbers make sense
		if(hp->old_start > hp->old_end || hp->new_start > hp->new_end) {
			// hp->old_start == 0|| hp->old_end == 0 || hp->new_start == 0 || hp->new_end == 0) {
			debug("%d", hp->old_start);
			debug("%d", hp->old_end);
			debug("%d", hp->new_start);
			debug("%d", hp->new_end);
			error("Line range error");
			resetHunk(hp);
			return ERR;
		}

		//print msg
		// char t;
		// if(hp->type == 1) {
		// 	t = 'a';
		// }
		// if(hp->type == 2) {
		// 	t = 'd';
		// }
		// if(hp->type == 3) {
		// 	t = 'c';
		// }
		// //cal # of lines modified for old and new file after hunk_next is success
		// success("Serial: %d\n%d,%d\n%c\n%d,%d", hp->serial, hp->old_start, hp->old_end, t, hp->new_start, hp->new_end);
		hp->serial = ++serial;
		old_range = hp->old_end - hp->old_start + 1;
		new_range = hp->new_end - hp->new_start + 1;
		hunkCompleted = 0;
		if(hp->type == HUNK_APPEND_TYPE) {
			addArr = hunk_additions_buffer;
			*addArr = (unsigned char) 0x0;
			*(addArr + 1) = (unsigned char) 0x0;
		} else if(hp->type == HUNK_DELETE_TYPE) {
			delArr = hunk_deletions_buffer;
			*delArr = (unsigned char) 0x0;
			*(delArr + 1) = (unsigned char) 0x0;
		} else {
			addArr = hunk_additions_buffer;
			*addArr = (unsigned char) 0x0;
			*(addArr + 1) = (unsigned char) 0x0;
			delArr = hunk_deletions_buffer;
			*delArr = (unsigned char) 0x0;
			*(delArr + 1) = (unsigned char) 0x0;
		}
		*(hunk_deletions_buffer + 509) = (unsigned char) 0x0;
		*(hunk_additions_buffer + 509) = (unsigned char) 0x0;
		return 0;
	}
	resetHunk(hp);
	return EOF;
    // abort();
}


/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

static int length = 0; //length of current line
static int lineCounter = 0;//count num of lines currently counted for one section
static int addSecForC = 0;// 0 if on delete sec for c or 1 if on add sec
static char *byte1;
static char *byte2;

//implement buffer max--------------------------------------------------------------------------

int hunk_getc(HUNK *hp, FILE *in) {
    // TO BE IMPLEMENTED
	// if(err == 1) {
	// 	error("continue to return ERR");
	// 	return ERR;
	// }
	if(hunkCompleted == 1) {
		error("Hunk completed. Go to next hunk");
		return ERR;
	}
	//get character
	int ch = fgetc(in);
	//hunk a
	if(hp->type == HUNK_APPEND_TYPE || (hp->type == HUNK_CHANGE_TYPE && addSecForC == 1)) {
		//case: beginning of a line "> "
		if(length == 0) {
			// ch = fgetc(in);
			// debug("%c", ch);
			//end of section
			if((ch >= '0' && ch <= '9') || ch == EOF) {
				if(lineCounter == new_range) {
					success("EOS for append");
					if(hp->type == HUNK_CHANGE_TYPE) {
						addSecForC = 0;
					}
					ungetc(ch, in);
					length = 0;
					hunkCompleted = 1;
					// debug("%c", *(hunk_additions_buffer + 508));
					// debug("%c", *(hunk_additions_buffer + 509));
					// debug("%c", *(hunk_additions_buffer + 510));
					// debug("%c", *(hunk_additions_buffer + 511));
					return EOS;
				} else {
					error("line count and new line range not match");


					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
			} else if(ch != '>') {
				error("Does not began with >");


				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;


				return ERR;
			} else {
				//initialize buffer
				if(*(hunk_additions_buffer + 509) == 0) {
					byte1 = addArr;
					byte2 = ++addArr;
				} 
				// else {
				// 	debug("%c", *(hunk_additions_buffer + 508));
				// 	debug("%c", *(hunk_additions_buffer + 509));
				// 	debug("%c", *(hunk_additions_buffer + 510));
				// 	debug("%c", *(hunk_additions_buffer + 511));
				// }
				// *byte1 = (unsigned char) 0x0;
				// *byte2 = (unsigned char) 0x0;
				// byte1 = addArr;
				// *byte1 = (unsigned char) 0x04;
				// debug("0x%x", *byte1);	
				// byte2 = ++addArr;
				// *byte2 = (unsigned char) 0xf2;
				// debug("0x%x", (unsigned char) *byte2);
				//get first character
				length++;
				ch = fgetc(in);
				lineCounter++;
				//check if num of current lines > max line for new
				if(lineCounter > new_range) {
					error("Maximum line exceed");
					

					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
				//there is a space
				if(ch == ' ') {
					ch = fgetc(in);
				} else {
					error("No space after >");

					

					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
				if(*(hunk_additions_buffer + 509) == 0) {
					*(++addArr) = ch;
					if(*(hunk_additions_buffer + 509) != 0) {
						*byte1 = ((unsigned char) length) & 0xff;
						*byte2 = ((unsigned char) length) >> 8;
					}
				}
				return ch;
			}
		}

		//the end of a line
		if(ch == '\n') {
			length++;
			//store length
			// *(++addArr) = ch;
			if(*(hunk_additions_buffer + 509) == 0) {
				*(++addArr) = ch;
			
				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;
				// debug("%x", *byte1);
				// debug("%x", *byte2);
				// addArr = byte2 + 1;
				// for(int i = 0; i < *byte1; i++) {
				// 	if(*(addArr + i) == '\n') {
				// 		debug("%d", *(addArr + i));
				// 	} else {
				// 		debug("%c", *(addArr + i));
				// 	}
				// }
				//check if next line is start of a hunk or ---
				addArr++;
				*addArr = (unsigned char) 0x0;
				*(addArr + 1) = (unsigned char) 0x0;
			}
			length = 0;
			//returning NL
			return ch;
			// ch = fgetc(in);
			// debug("%c", ch);
			// if((ch >= '0' && ch <= '9') || ch == '-') {
			// 	if(lineCounter == new_range) {
			// 		success("EOS for append");
			// 		ungetc(ch, in);
			// 		length = 0;
			// 		return EOS;
			// 	} else {
			// 		error("line count and new line range not match");
			// 		return ERR;
			// 	}
			// } else if(ch == '>'){
			// 	length = 0;
			// 	ungetc(ch, in);
			// }
		}
		//if line doesn't end with new line
		if(ch == EOF) {
			error("Doesn't end with new line");

				

			*byte1 = ((unsigned char) length) & 0xff;
			*byte2 = ((unsigned char) length) >> 8;


			return ERR;
		}
		//return every character in the middle
		if(*(hunk_additions_buffer + 509) == 0) {
			*(++addArr) = ch;
			if(*(hunk_additions_buffer + 509) != 0) {
				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;
			}
		}
		length++;
		return ch;
	}
	if(hp->type == HUNK_DELETE_TYPE || (hp->type == HUNK_CHANGE_TYPE && addSecForC == 0)) {
		if(length == 0) {
			// ch = fgetc(in);
			// debug("%c", ch);
			//end of section
			if((ch >= '0' && ch <= '9') || ch == '-' || ch == EOF) {
				if(lineCounter == old_range) {
					success("EOS for append");
					if(ch == '-') {
						//get all the ---
						if(hp->type != HUNK_CHANGE_TYPE) {
							error("type not c but have -");
							

						*byte1 = ((unsigned char) length) & 0xff;
						*byte2 = ((unsigned char) length) >> 8;


							return ERR;
						}
						int dashCount = 0;
						while(dashCount < 4 && ch == '-') {
							dashCount++;
							ch = fgetc(in);
						}
						//ch should be \n
						if(dashCount != 3 || ch != '\n') {
							error("Middle section not '---NL'");
							
							*byte1 = ((unsigned char) length) & 0xff;
							*byte2 = ((unsigned char) length) >> 8;


							return ERR;
						}
						//check if next line starts with >
						ch = fgetc(in);
						if(ch != '>') {
							error("Missing append section");
							
							*byte1 = ((unsigned char) length) & 0xff;
							*byte2 = ((unsigned char) length) >> 8;


							return ERR;
						} else {
							ungetc(ch, in);
							addSecForC = 1;
							return EOS;
						}
					} else {
						ungetc(ch, in);
						length = 0;
						hunkCompleted = 1;
						return EOS;
					}
				} else {
					error("line count and new line range not match");

				
					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
			} else if(ch != '<') {
				error("Does not began with <");
				
				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;


				return ERR;
			} else {
				//set up length in buffer
				if(*(hunk_deletions_buffer + 509) == 0) {
					byte1 = delArr;
					byte2 = ++delArr;
				}
				// byte1 = delArr;
				// byte2 = ++delArr;
				// *byte1 = (unsigned char) 0x0;
				// *byte2 = (unsigned char) 0x0;
				//get first character
				length++;
				ch = fgetc(in);
				lineCounter++;
				//check if num of current lines > max line for new
				if(lineCounter > old_range) {
					error("Maximum line exceed");
					
					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
				//there is a space
				if(ch == ' ') {
					ch = fgetc(in);
				} else {
					error("No space after <");
					
					*byte1 = ((unsigned char) length) & 0xff;
					*byte2 = ((unsigned char) length) >> 8;


					return ERR;
				}
				if(*(hunk_deletions_buffer + 509) == 0) {
					*(++delArr) = ch;
					if(*(hunk_deletions_buffer + 509) != 0) {
						*byte1 = ((unsigned char) length) & 0xff;
						*byte2 = ((unsigned char) length) >> 8;
					}
				}
				// *(++delArr) = ch;
				return ch;
			}
		}

		//the end of a line
		if(ch == '\n') {
			length++;
			if(*(hunk_deletions_buffer + 509) == 0) {
				*(++delArr) = ch;
			
			//store \n-------------------------------------------------------------------
				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;
				// debug("%x", *byte1);
				// debug("%x", *byte2);
				// delArr = byte2 + 1;
				// for(int i = 0; i < *byte1; i++) {
				// 	if(*(delArr + i) == '\n') {
				// 		debug("%d", *(delArr + i));
				// 	} else {
				// 		debug("%c", *(delArr + i));
				// 	}
				// }
				//check if next line is start of a hunk or ---
				delArr++;
				*delArr = (unsigned char) 0x0;
				*(delArr + 1) = (unsigned char) 0x0;
			}
			length = 0;
			return ch;
		}
		if(ch == EOF) {
			error("Doesn't end with new line");
			
			*byte1 = ((unsigned char) length) & 0xff;
			*byte2 = ((unsigned char) length) >> 8;


			return ERR;
		}
		//return every character in the middle
		length++;
		if(*(hunk_deletions_buffer + 509) == 0) {
			*(++delArr) = ch;
			if(*(hunk_deletions_buffer + 509) != 0) {
				*byte1 = ((unsigned char) length) & 0xff;
				*byte2 = ((unsigned char) length) >> 8;
			}
		}
		return ch;
	}
	return ERR;
    // abort();
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    // TO BE IMPLEMENTED
	if(hp->type == HUNK_APPEND_TYPE) {
		fprintf(out, "%d%c",hp->old_start, 'a');
		if(hp->new_start == hp->new_end) {
			fprintf(out, "%d\n",hp->new_start);
		} else {
			fprintf(out, "%d,%d\n",hp->new_start, hp->new_end);
		}

		byte1 = hunk_additions_buffer;
		byte2 = byte1 + 1;
		while((*byte2 << 8) + *byte1 != 0) {
			char *letterPtr = byte2;
			int lineLen = (*byte2 << 8) + *byte1;
			// debug("%c",*letterPtr);
			// debug("%p", hunk_additions_buffer+509);
			if(letterPtr > hunk_additions_buffer+509) {
				fputc(*(hunk_additions_buffer+509), out);
			} else if(lineLen > 1){
				fprintf(out, "> ");
				debug("%d", lineLen);
			}
			while(lineLen != 0) {
				fputc(*(++letterPtr), out);
				lineLen--;
			}
			byte1 = letterPtr + 1;
			byte2 = byte1 + 1;
		}
		if(*(hunk_additions_buffer + 509) != 0) {
			fprintf(out, "...\n");
		}
	}
	if(hp->type == HUNK_DELETE_TYPE || hp->type == HUNK_CHANGE_TYPE) {
	// && hunkCompleted == 1) {
		if(hp->old_start == hp->old_end) {
			fprintf(out, "%d", hp->old_start);
		} else {
			fprintf(out, "%d,%d",hp->old_start, hp->old_end);
		}
		if(hp->type == HUNK_DELETE_TYPE) {
			fprintf(out, "d%d\n",hp->new_start);
		} else {
			fputc('c', out);
			if(hp->new_start == hp->new_end) {
				fprintf(out, "%d\n",hp->new_start);
			} else {
				fprintf(out, "%d,%d\n",hp->new_start, hp->new_end);
			}
		}
		byte1 = hunk_deletions_buffer;
		byte2 = byte1 + 1;
		while((*byte2 << 8) + *byte1 != 0) {
			char *letterPtr = byte2;
			int lineLen = (*byte2 << 8) + *byte1;
			// fprintf(out, "< ");
			if(letterPtr > hunk_deletions_buffer+509) {
				fputc(*(hunk_deletions_buffer+509), out);
			} else if(lineLen > 1){
				fprintf(out, "< ");
			}
			while(lineLen != 0) {
				fputc(*(++letterPtr), out);
				lineLen--;
			}
			byte1 = letterPtr + 1;
			byte2 = byte1 + 1;
		}
		if(*(hunk_deletions_buffer + 509) != 0) {
			fprintf(out, "...\n");
		}
		if(hp->type == HUNK_CHANGE_TYPE) {
			fprintf(out, "---\n");
			byte1 = hunk_additions_buffer;
			byte2 = byte1 + 1;
			while((*byte2 << 8) + *byte1 != 0) {
				char *letterPtr = byte2;
				int lineLen = (*byte2 << 8) + *byte1;
				if(letterPtr > hunk_additions_buffer+509) {
					fputc(*(hunk_additions_buffer+509), out);
				} else if(lineLen > 1){
					fprintf(out, "> ");
				}
				while(lineLen != 0) {
					fputc(*(++letterPtr), out);
					lineLen--;
				}
				byte1 = letterPtr + 1;
				byte2 = byte1 + 1;
			}
			if(*(hunk_additions_buffer + 509) != 0) {
				fprintf(out, "...\n");
			}
		}
	}
    // abort();
}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    // TO BE IMPLEMENTED
	HUNK hp;
	*(hunk_deletions_buffer + 510) = (unsigned char) 0x0;
	*(hunk_deletions_buffer + 511) = (unsigned char) 0x0;
	*(hunk_deletions_buffer + 509) = (unsigned char) 0x0;
	*(hunk_additions_buffer + 510) = (unsigned char) 0x0;
	*(hunk_additions_buffer + 511) = (unsigned char) 0x0;
	*(hunk_additions_buffer + 509) = (unsigned char) 0x0;
	int hunkNextStatus;
	int lineInPtr = 0;
	int lineOutPtr = 0;
	int inChar;
	int diffChar;
	int linesInOut = 0;
	while((hunkNextStatus = hunk_next(&hp, diff)) == 0) {
		if(hp.type == HUNK_APPEND_TYPE) {
			if(lineOutPtr == 0) {
				lineInPtr++;
				lineOutPtr++;
			}
			debug("In: %d, Out: %d", lineInPtr, lineOutPtr);
			// debug("%d and %d", lineInPtr, hp.old_end);
			while(lineInPtr <= hp.old_end) {
				if((inChar = fgetc(in)) != '\n'){
					// printf("%c", inChar);
					if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
						fputc(inChar, out);
					}
				} else {
					if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
						fputc(inChar, out);
					}
					lineInPtr++;
					lineOutPtr++;
					linesInOut++;
				}
			}
			debug("In: %d, Out: %d", lineInPtr, lineOutPtr);
			// debug("%d and %d", lineOutPtr, hp.new_start);
			if(lineOutPtr == hp.new_start) {
				diffChar = hunk_getc(&hp, diff);
				while(diffChar != EOS && diffChar != ERR) {
					if(diffChar == '\n') {
						lineOutPtr++;
						linesInOut++;
					}
					// if(lineOutPtr > hp.new_end + 1) {
					// 	error("Line in out > hp.new_end + 1");
					// 	return -1;
					// }
					// fputc(diffChar, out);
					if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
						// fputc(inChar, out);
						fputc(diffChar, out);
					}
					diffChar = hunk_getc(&hp, diff);
				}
				if(diffChar == EOS) {
					lineCounter = 0;
				}
				if(linesInOut != hp.new_end || diffChar == ERR) {
					error("lineoutPtr != hp.new_end + 1 or diffChar == ERR");
					if((global_options & QUIET_OPTION) != QUIET_OPTION) {
						fprintf(stderr, "LinesInOut != hp.new_end or diffChar == ERR\n");
						hunk_show(&hp, stderr);
						fprintf(stderr, "...\n");
					}
					return -1;
				}
				success("Hunk add complete");
			} else {
				error("Line in out != hp.new_start");
				if((global_options & QUIET_OPTION) != QUIET_OPTION) {
					fprintf(stderr, "Line in out != hp.new_start\n");
					hunk_show(&hp, stderr);
					fprintf(stderr, "...\n");
				}
				return -1;
			}
		}
		if(hp.type == HUNK_DELETE_TYPE || hp.type == HUNK_CHANGE_TYPE) {
			if(lineInPtr == 0) {
				lineInPtr++;
				// lineOutPtr++;
				// if(hp.type == HUNK_CHANGE_TYPE) {
				// 	lineOutPtr++;
				// }
			}
			//lineInPtr should be on the line to be deleted
			debug("%d and %d", linesInOut, hp.new_start);
			while(lineInPtr < hp.old_start) {
				if((inChar = fgetc(in)) != '\n'){
					if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
						fputc(inChar, out);
					}
				} else {
					if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
						fputc(inChar, out);
					}
					lineInPtr++;
					lineOutPtr++;
				}
			}
			//lineOutPtr should be on the line before the first deleted line
			debug("%d and %d", linesInOut, hp.new_start);
			// if((hp.type == HUNK_CHANGE_TYPE && lineOutPtr == hp.old_start - 1) ||
			// (hp.type == HUNK_DELETE_TYPE && lineOutPtr == hp.old_start - 1)) {
			if(linesInOut == hp.new_start - 1 || linesInOut == 0) {
				diffChar = hunk_getc(&hp, diff);
				inChar = fgetc(in);
				while(diffChar != EOS && diffChar != ERR) {
					if(diffChar == '\n') {
						lineInPtr++;
					}
					if(diffChar == inChar) {
						diffChar = hunk_getc(&hp, diff);
						if(diffChar != EOS && diffChar != ERR) {
							inChar = fgetc(in);
						}
						// debug("%c and %c", diffChar, inChar);
					} else {
						error("Delete line don't match");
						if((global_options & QUIET_OPTION) != QUIET_OPTION) {
							fprintf(stderr, "Delete line don't match\n");
							hunk_show(&hp, stderr);
							fprintf(stderr, "...\n");
						}
						return -1;
					}
				}
				// debug("%c", inChar);
				// ungetc(inChar, in);
				if(diffChar == EOS) {
					lineCounter = 0;
				}
				if(lineInPtr != hp.old_end + 1 || diffChar == ERR) {
					error("LineInPtr != hp.old_end or diffChar == ERR");
					if((global_options & QUIET_OPTION) != QUIET_OPTION) {
						fprintf(stderr, "LineInPtr != hp.old_end or diffChar == ERR\n");
						hunk_show(&hp, stderr);
						fprintf(stderr, "...\n");
					}
					return -1;
				}
				success("Hunk delete complete");
				debug("In: %d, Out: %d", lineInPtr, lineOutPtr);
				if(hp.type == HUNK_CHANGE_TYPE) {
					if(lineOutPtr == 0) {
						// lineInPtr++;
						lineOutPtr++;
					}
					// debug("%d and %d", lineInPtr, hp.old_end);
					debug("In: %d, Out: %d", lineInPtr, lineOutPtr);
					while(lineInPtr <= hp.old_end) {
						if((inChar = fgetc(in)) != '\n'){
							if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
								fputc(inChar, out);
							}
						} else {
							if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
								fputc(inChar, out);
							}
							lineInPtr++;
							lineOutPtr++;
							linesInOut++;
						}
					}
					// debug("%d and %d", lineOutPtr, hp.new_start);
					if(lineOutPtr == hp.new_start) {
						diffChar = hunk_getc(&hp, diff);
						while(diffChar != EOS && diffChar != ERR) {
							if(diffChar == '\n') {
								lineOutPtr++;
								linesInOut++;
							}
							// if(lineOutPtr > hp.new_end + 1) {
							// 	error("Line in out > hp.new_end + 1");
							// 	return -1;
							// }
							if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
								// fputc(inChar, out);
								fputc(diffChar, out);
							}
							diffChar = hunk_getc(&hp, diff);
						}
						if(diffChar == EOS) {
							lineCounter = 0;
						}
						if(linesInOut != hp.new_end || diffChar == ERR) {
							error("lineoutPtr != hp.new_end + 1 or diffChar == ERR");
							if((global_options & QUIET_OPTION) != QUIET_OPTION) {
								fprintf(stderr, "LinesInOut != hp.new_end or diffChar == ERR\n");
								hunk_show(&hp, stderr);
								fprintf(stderr, "...\n");
							}
							return -1;
						}
						debug("In: %d, Out: %d", lineInPtr, lineOutPtr);
						success("Hunk add complete");
					} else {
						error("Line in out != hp.new_start");
						if((global_options & QUIET_OPTION) != QUIET_OPTION) {
							fprintf(stderr, "Line in out != hp.new_start\n");
							hunk_show(&hp, stderr);
							fprintf(stderr, "...\n");
						}
						return -1;
					}
				}
			} else {
				error("LinesInOut != hp.new_start - 1 or linesInOut != 0");
				if((global_options & QUIET_OPTION) != QUIET_OPTION) {
					fprintf(stderr, "LinesInOut != hp.new_start - 1 or linesInOut != 0\n");
					hunk_show(&hp, stderr);
					fprintf(stderr, "...\n");
				}
				return -1;
			}
		}
	}

	// 	int ch = hunk_getc(&hp, diff);
	// 	while(ch != EOS && ch != ERR) {
	// 		// debug("%c", ch);
	// 		// if(ch == '\n') {
	// 		// 	printf("\n");
	// 		// 	fflush(stdout);
	// 		// } else {
	// 		// 	printf("%c", ch);
	// 		// 	fflush(stdout);
	// 		// }
	// 		ch = hunk_getc(&hp, diff);
	// 	}
	// 	if(ch == EOS) {
	// 		lineCounter = 0;
	// 		if(hp.type != HUNK_CHANGE_TYPE) {
	// 			hunk_show(&hp, stderr);
	// 		}
	// 		// fflush(stderr);
	// 	}
	// 	if(hp.type == HUNK_CHANGE_TYPE) {
	// 		int ch = hunk_getc(&hp, diff);
	// 		while(ch != EOS && ch != ERR) {
	// 			// debug("%c", ch);
	// 			// if(ch == '\n') {
	// 			// 	printf("\n");
	// 			// 	fflush(stdout);
	// 			// } else {
	// 			// 	printf("%c", ch);
	// 			// 	fflush(stdout);
	// 			// }
	// 			ch = hunk_getc(&hp, diff);
	// 		}
	// 		if(ch == EOS) {
	// 			lineCounter = 0;
	// 			hunk_show(&hp, stderr);
	// 		}
	// 	}
	// }
	if(hunkNextStatus == ERR) {
		return -1;
	}
	while((inChar = fgetc(in)) != EOF) {
		if((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
			fputc(inChar, out);
		}
	}
	//change this later
	return 0;
    // abort();
}
