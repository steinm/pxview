#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libintl.h>
#include <sys/types.h>
#include <regex.h>
#include <paradox.h>
#include "config.h"
#define _(String) gettext(String)

void usage(char *progname) {
	printf(_("Version: %s %s http://sourceforge.net/projects/pxlib"), progname, VERSION);
	printf("\n");
	printf(_("Copyright: Copyright (C) 2003 Uwe Steinmann <uwe@steinmann.cx>"));
	printf("\n\n");
	printf(_("%s reads a paradox file and outputs information about the file\nor dumps the content in CSV format.\n\n"), progname);
	printf(_("Usage: %s [OPTIONS] FILE"), progname);
	printf("\n\n");
	printf(_("Options:"));
	printf("\n\n");
	printf(_("  -h, --help          this usage information."));
	printf("\n");
	printf(_("  -i, --info          show information about file."));
	printf("\n");
	printf(_("  -c, --csv           dump records in CSV format."));
	printf("\n");
	printf(_("  -s, --sql           dump records in SQL format."));
	printf("\n");
	printf(_("  -t, --shema         output schema of database."));
	printf("\n");
	printf(_("  --mode=MODE         set output mode (csv, sql, or schema)."));
	printf("\n");
	printf(_("  -o, --output=FILE   output data into file instead of stdout."));
	printf("\n");
	printf(_("  -b, --blobfile=FILE read blob data from file."));
	printf("\n");
	printf(_("  -p, --blobprefix=PREFIX prefix for all created files with blob data."));
	printf("\n");
	printf(_("  --separator=CHAR    character used to separate field values."));
	printf("\n");
	printf(_("  --enclosure=CHAR    character used to enclose field values."));
	printf("\n");
	printf(_("  --includeblobs      add blob fields in sql output."));
	printf("\n");
	printf(_("  --fields=REGEX      extended regular expression to select fields."));
	printf("\n");
	printf(_("  --tablename=NAME    overwrite name of database table."));
	printf("\n\n");
	printf(_("If you do not specify any of the options -i, -c, -s, or -t\nthen -i will be used."));
	printf("\n\n");
	printf(_("The options --separator and --enclosure will only affect csv output."));
	printf("\n\n");
	printf(_("The option --fields will only affect csv and sql output."));
	printf("\n\n");
	printf(_("If exporting csv format fields will be separated by tabulator\nand enclosed into \"."));
	printf("\n");
}

int main(int argc, char *argv[]) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	pxdoc_t *pxdoc;
	pxblob_t *pxblob;
	char *selectedfields;
	char *data, buffer[1000];
	int i, j, c; // general counters
	int first; // used to indicate if output has started or not
	int outputcsv = 0;
	int outputinfo = 0;
	int outputsql = 0;
	int outputschema = 0;
	int includeblobs = 0;
	char delimiter = '\t';
	char enclosure = '"';
	char *inputfile = NULL;
	char *blobfile = NULL;
	char *blobprefix = NULL;
	char *fieldregex = NULL;
	char *tablename = NULL;

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	setlocale (LC_NUMERIC, "C");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (GETTEXT_PACKAGE);
#endif

	while(1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"info", 0, 0, 'i'},
			{"csv", 0, 0, 'c'},
			{"sql", 0, 0, 's'},
			{"schema", 0, 0, 't'},
			{"blobfile", 1, 0, 'b'},
			{"blobprefix", 1, 0, 'p'},
			{"output", 1, 0, 'o'},
			{"help", 0, 0, 'h'},
			{"separator", 1, 0, 0},
			{"enclosure", 1, 0, 1},
			{"includeblobs", 0, 0, 2},
			{"fields", 1, 0, 'f'},
			{"tablename", 1, 0, 3},
			{"mode", 1, 0, 4},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "icstf:b:p:o:h",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 0:
				delimiter = optarg[0];
				break;
			case 1:
				enclosure = optarg[0];
				break;
			case 2:
				includeblobs = 1;;
				break;
			case 3:
				tablename = strdup(optarg);
				break;
			case 4:
				if(!strcmp(optarg, "csv")) {
					outputcsv = 1;
				} else if(!strcmp(optarg, "sql")) {
					outputsql = 1;
				} else if(!strcmp(optarg, "schema")) {
					outputschema = 1;
				}
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 't':
				outputschema = 1;
				break;
			case 'b':
				blobfile = strdup(optarg);
				break;
			case 'p':
				blobprefix = strdup(optarg);
				break;
			case 'f':
				fieldregex = strdup(optarg);
				break;
			case 'o':
				break;
			case 'i':
				outputinfo = 1;
				break;
			case 'c':
				outputcsv = 1;
				break;
			case 's':
				outputsql = 1;
				break;
		}
	}

	/* if none the output modes is selected then display info */
	if(outputinfo == 0 && outputcsv == 0 && outputschema == 0 && outputsql == 0)
		outputinfo = 1;

	if (optind < argc) {
		inputfile = strdup(argv[optind]);
	}

	if(!inputfile) {
		fprintf(stderr, _("You must at least specify an input file."));
		fprintf(stderr, "\n");
		fprintf(stderr, "\n");
		usage(argv[0]);
		exit(0);
	}

	pxdoc = PX_new();
	if(0 > PX_open_file(pxdoc, inputfile)) {
		fprintf(stderr, _("Could not open input file."));
		fprintf(stderr, "\n");
		exit(1);
	}
	pxh = pxdoc->px_head;

	/* Open the file containing the blobs if one is given */
	if(blobfile) {
		pxblob = PX_new_blob(pxdoc);
		if(0 > PX_open_blob_file(pxblob, blobfile)) {
			fprintf(stderr, _("Could not open blob file."));
			fprintf(stderr, "\n");
			PX_close(pxdoc);
			exit(1);
		}
	}

	if(outputinfo) {
		printf(_("File Version:        %1.1f\n"), (float) pxh->px_fileversion/10.0);
		printf(_("File Type:           "));
		switch(pxh->px_filetype) {
			case pxfFileTypIndexDB:
				printf(_("indexed .DB data file"));
				break;
			case pxfFileTypPrimIndex:
				printf(_("primary index .PX file"));
				break;
			case pxfFileTypNonIndexDB:
				printf(_("non-indexed .DB data file"));
				break;
			case pxfFileTypNonIncSecIndex:
				printf(_("non-incrementing secondary index .Xnn file"));
				break;
			case pxfFileTypSecIndex:
				printf(_("secondary index .Ynn file (inc or non-inc)"));
				break;
			case pxfFileTypIncSecIndex:
				printf(_("incrementing secondary index .Xnn file"));
				break;
			case pxfFileTypNonIncSecIndexG:
				printf(_("non-incrementing secondary index .XGn file"));
				break;
			case pxfFileTypSecIndexG:
				printf(_("secondary index .YGn file (inc or non inc)"));
				break;
			case pxfFileTypIncSecIndexG:
				printf(_("incrementing secondary index .XGn file"));
				break;
		}
		printf("\n");
		printf(_("Tablename:           %s\n"), pxh->px_tablename);
		printf(_("Num. of Records:     %d\n"), pxh->px_numrecords);
		printf(_("Num. of Fields:      %d\n"), pxh->px_numfields);
		printf(_("Header size:         %d (0x%X)\n"), pxh->px_headersize, pxh->px_headersize);
		printf(_("Max. Table size:     %d (0x%X)\n"), pxh->px_maxtablesize, pxh->px_maxtablesize*0x400);
		printf(_("Num. of Data Blocks: %d\n"), pxh->px_fileblocks);
		if((pxh->px_filetype == pxfFileTypNonIncSecIndex) ||
			 (pxh->px_filetype == pxfFileTypIncSecIndex))
			printf(_("Num. of Index Field: %d\n"), pxh->px_indexfieldnumber);
		printf(_("Num. of prim. Key fields: %d\n"), pxh->px_primarykeyfields);
		printf(_("Write protected:     %d\n"), pxh->px_writeprotected);
		printf(_("Code Page:           %d (0x%X)\n"), pxh->px_doscodepage, pxh->px_doscodepage);
		printf("\n");

		printf(_("Fieldname          | Type\n"));
		printf("------------------------------------\n");
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			printf("%18s | ", pxf->px_fname);
			switch(pxf->px_ftype) {
				case pxfAlpha:
					printf("char(%d)\n", pxf->px_flen);
					break;
				case pxfDate:
					printf("date(%d)\n", pxf->px_flen);
					break;
				case pxfShort:
					printf("int(%d)\n", pxf->px_flen);
					break;
				case pxfLong:
					printf("int(%d)\n", pxf->px_flen);
					break;
				case pxfCurrency:
					printf("currency(%d)\n", pxf->px_flen);
					break;
				case pxfNumber:
					printf("double(%d)\n", pxf->px_flen);
					break;
				case pxfLogical:
					printf("boolean(%d)\n", pxf->px_flen);
					break;
				case pxfMemoBLOb:
					printf("blob(%d)\n", pxf->px_flen);
					break;
				case pxfBLOb:
					printf("blob(%d)\n", pxf->px_flen);
					break;
				case pxfFmtMemoBLOb:
					printf("blob(%d)\n", pxf->px_flen);
					break;
				case pxfOLE:
					printf("ole(%d)\n", pxf->px_flen);
					break;
				case pxfGraphic:
					printf("graphic(%d)\n", pxf->px_flen);
					break;
				case pxfTime:
					printf("time(%d)\n", pxf->px_flen);
					break;
				case pxfTimestamp:
					printf("timestamp(%d)\n", pxf->px_flen);
					break;
				case pxfAutoInc:
					printf("autoinc(%d)\n", pxf->px_flen);
					break;
				case pxfBCD:
					printf("decimal(17,%d)\n", pxf->px_flen);
					break;
				case pxfBytes:
					printf("bytes(%d)\n", pxf->px_flen);
					break;
				default:
					printf("%c(%d)\n", pxf->px_ftype, pxf->px_flen);
			}
			pxf++;
		}
	}

	if(outputschema) {
		int sumlen = 0;
		if(tablename)
			printf("[%s]\n", tablename);
		else
			printf("[%s]\n", pxh->px_tablename);
		printf("Filetype=Delimited\n");
		printf("Delimiter=%c\n", enclosure);
		printf("Separator=%c\n", delimiter);
		printf("CharSet=ANSIINTL\n");
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			switch(pxf->px_ftype) {
				case pxfAlpha:
				case pxfDate:
				case pxfShort:
				case pxfLong:
				case pxfCurrency:
				case pxfNumber:
				case pxfLogical:
				case pxfTime:
				case pxfTimestamp:
				case pxfBCD:
				case pxfBytes:
					printf("Field%d=", i+1);
					printf("%s,", pxf->px_fname);
					break;
			}
			switch(pxf->px_ftype) {
				case pxfAlpha:
					printf("Char,%d,00,%d\n", pxf->px_flen, sumlen);
					sumlen += pxf->px_flen;
					break;
				case pxfDate:
					printf("ADate,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfShort:
					printf("Short Integer,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfLong:
					printf("Long Integer,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfCurrency:
					printf("Currency,20,02,%d\n", sumlen);
					sumlen += 20;
					break;
				case pxfNumber:
					printf("Float,20,02,%d\n", sumlen);
					sumlen += 20;
					break;
				case pxfLogical:
					printf("Boolean,%d,00,%d\n", pxf->px_flen, sumlen);
					sumlen += pxf->px_flen;
					break;
				case pxfMemoBLOb:
				case pxfBLOb:
				case pxfFmtMemoBLOb:
				case pxfOLE:
				case pxfGraphic:
				case pxfAutoInc:
					break;
				case pxfTime:
					printf("ATime,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfTimestamp:
					printf("ATimestamp,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfBCD:
					printf("Float,17,%d,%d\n", pxf->px_flen, sumlen);
					sumlen += 17;
					break;
				case pxfBytes:
					printf("Char,%d,00,%d\n", pxf->px_flen, sumlen);
					sumlen += pxf->px_flen;
					break;
				default:
					break;
			}
			pxf++;
		}
	}

	/* Check which fields shall be shown in sql or csv output */
	if(fieldregex) {
		regex_t preg;
		if(regcomp(&preg, fieldregex, REG_NOSUB|REG_EXTENDED)) {
			fprintf(stderr, _("Could not compile regular expression to select fields."));
			PX_close(pxdoc);
			exit;
		}
		/* allocate memory for selected field array */
		if((selectedfields = (char *) px_malloc(pxdoc, pxh->px_numfields, _("Could not allocate memory for array of selected fields."))) == NULL) {
			PX_close(pxdoc);
			exit;
		}
		memset(selectedfields, '\0', pxh->px_numfields);
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(0 == regexec(&preg, pxf->px_fname, 0, NULL, 0)) {
				selectedfields[i] = 1;
			}
			pxf++;
		}
	}

	/* Output data as comma separated values */
	if(outputcsv) {
		if((data = (char *) px_malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			if(selectedfields)
				px_free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		for(j=0; j<pxh->px_numrecords; j++) {
			int offset;
			if(PX_get_record(pxdoc, j, data)) {
				pxf = pxh->px_fields;
				offset = 0;
				first = 0;  // set to 1 when first field has been output
				for(i=0; i<pxh->px_numfields; i++) {
					if(fieldregex == NULL ||  selectedfields[i]) {
						if(first == 1)
							printf("%c", delimiter);
						switch(pxf->px_ftype) {
							case pxfAlpha:
								memcpy(buffer, &data[offset], pxf->px_flen);
								buffer[pxf->px_flen] = '\0';
								if(enclosure)
									printf("%c%s%c", enclosure, buffer, enclosure);
								else
									printf("%s", buffer);
								first = 1;
								break;
							case pxfDate:
								data[offset] ^= data[offset];
								printf("%d", *((int *)(&data[offset])));
								first = 1;
								break;
							case pxfShort:
								data[offset] ^= data[offset];
								printf("%d", *((short int *)(&data[offset])));
								first = 1;
								break;
							case pxfLong:
								data[offset] ^= data[offset];
								printf("%ld", *((long int *)(&data[offset])));
								first = 1;
								break;
							case pxfNumber:
								if(*((long *)(&data[offset])) & 0x80000000) {
									data[offset] &= 0x7f;
									printf("%f", *((double *)(&data[offset])));
								}
								first = 1;
								break;
							case pxfLogical:
								if(*((char *)(&data[offset])) & 0x80) {
									data[offset] &= 0x7f;
									if(data[offset])
										printf("1");
									else
										printf("0");
								}
								first = 1;
								break;
							case pxfGraphic:
							case pxfBLOb:
								if(pxblob) {
									char *blobdata;
									char filename[200];
									FILE *fp;
									size_t size, boffset, mod_nr;
									size = get_long(&data[offset+4]);
									boffset = get_long(&data[offset]) & 0xffffff00;
									mod_nr = get_short(&data[offset+8]);
									printf("offset=%ld ", boffset);
									printf("size=%ld ", size);
									printf("mod_nr=%d ", mod_nr);
									blobdata = PX_read_blobdata(pxblob, boffset, size);
									if(blobdata) {
										sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
										fp = fopen(filename, "w");
										fwrite(blobdata, size, 1, fp);
										fclose(fp);
									} else {
										fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
									}

								} else {
									printf("offset=%ld ", get_long(&data[offset]) & 0xffffff00);
									printf("size=%ld ", get_long(&data[offset+4]));
									printf("mod_nr=%d ", get_short(&data[offset+8]));
									hex_dump(&data[offset], pxf->px_flen);
								}
								first = 1;
								break;
							default:
								printf("");
						}
					}
					offset += pxf->px_flen;
					pxf++;
				}
				printf("\n");
			} else {
				fprintf(stderr, _("Couldn't get record\n"));
			}
		}
		px_free(pxdoc, data);
	}

	/* Output data as sql statements */
	if(outputsql) {
		/* Output table schema */
		if(tablename)
			printf("CREATE TABLE %s (\n", tablename);
		else
			printf("CREATE TABLE %s (\n", pxh->px_tablename);
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				if(first == 1)
					printf(",\n");
				switch(pxf->px_ftype) {
					case pxfAlpha:
					case pxfDate:
					case pxfShort:
					case pxfLong:
					case pxfCurrency:
					case pxfNumber:
					case pxfLogical:
					case pxfTime:
					case pxfTimestamp:
					case pxfBCD:
					case pxfBytes:
						printf("  %s ", pxf->px_fname);
						first = 1;
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs) {
							printf("  %s ", pxf->px_fname);
							first = 1;
						}
						break;
				}
				switch(pxf->px_ftype) {
					case pxfAlpha:
						printf("char(%d)", pxf->px_flen);
						break;
					case pxfDate:
						printf("date");
						break;
					case pxfShort:
						printf("smallint");
						break;
					case pxfLong:
						printf("integer");
						break;
					case pxfCurrency:
						printf("decimal(20,2)");
						break;
					case pxfNumber:
						printf("decimal(20,2)");
						break;
					case pxfLogical:
						printf("boolean");
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs)
							printf("oid");
						break;
					case pxfOLE:
					case pxfAutoInc:
						break;
					case pxfTime:
						printf("time");
						break;
					case pxfTimestamp:
						printf("timestamp");
						break;
					case pxfBCD:
						printf("decimal(17,%d)", pxf->px_flen);
						break;
					case pxfBytes:
						printf("char(%d)", pxf->px_flen);
						break;
					default:
						break;
				}
			}
			pxf++;
		}
		printf("\n);\n\n");

		if((data = (char *) px_malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			if(selectedfields)
				px_free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		if(tablename)
			printf("COPY %s (", tablename);
		else
			printf("COPY %s (", pxh->px_tablename);
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		/* output field name */
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				if(first == 1)
					printf(", ");
				switch(pxf->px_ftype) {
					case pxfAlpha:
					case pxfDate:
					case pxfShort:
					case pxfLong:
					case pxfNumber:
					case pxfLogical:
						printf("%s", pxf->px_fname);
						first = 1;
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs) {
							printf("%s", pxf->px_fname);
							first = 1;
						}
						break;
				}
			}
			pxf++;
		}
		printf(") FROM stdin;\n");
		for(j=0; j<pxh->px_numrecords; j++) {
			int offset;
			if(PX_get_record(pxdoc, j, data)) {
				first = 0;  // set to 1 when first field has been output
				offset = 0;
				pxf = pxh->px_fields;
				for(i=0; i<pxh->px_numfields; i++) {
					if(fieldregex == NULL ||  selectedfields[i]) {
						if(first == 1)
							printf("\t");
						switch(pxf->px_ftype) {
							case pxfAlpha:
								memcpy(buffer, &data[offset], pxf->px_flen);
								buffer[pxf->px_flen] = '\0';
								printf("%s", buffer);
								first = 1;
								break;
							case pxfDate:
								data[offset] ^= data[offset];
								printf("%d", *((int *)(&data[offset])));
								first = 1;
								break;
							case pxfShort:
								data[offset] ^= data[offset];
								printf("%d", *((short int *)(&data[offset])));
								first = 1;
								break;
							case pxfLong:
								// FIXME: distinguish between NULL and 0 as in pxfNumber
								data[offset] ^= data[offset];
								printf("%ld", *((long int *)(&data[offset])));
								first = 1;
								break;
							case pxfNumber:
								//hex_dump(&data[offset], pxf->px_flen);
								/* Paradox distinguishes a decimal 0.0 and not set value
								 * a decimal null is 8000000000000000 and a not set value
								 * are 8 null bytes.
								 */
								if(*((long *)(&data[offset])) & 0x80000000) {
									data[offset] &= 0x7f;
									printf("%.2f", *((double *)(&data[offset])));
								} else {
									printf("\\N");
								}
								first = 1;
								break;
							case pxfLogical:
								if(*((char *)(&data[offset])) & 0x80) {
									data[offset] &= 0x7f;
									if(data[offset])
										printf("TRUE");
									else
										printf("FALSE");
								} else {
									printf("\\N");
								}
								first = 1;
								break;
							case pxfMemoBLOb:
							case pxfBLOb:
							case pxfFmtMemoBLOb:
							case pxfGraphic:
								if(includeblobs) {
									printf("\\N");
									first = 1;
								}
								break;
							default:
								printf("");
						}
					}
					offset += pxf->px_flen;
					pxf++;
				}
				printf("\n");
			} else {
				fprintf(stderr, _("Couldn't get record\n"));
			}
		}
		printf("\\.\n");
		px_free(pxdoc, data);
	}

	if(selectedfields)
		px_free(pxdoc, selectedfields);

	PX_close(pxdoc);

	exit(0);
}
