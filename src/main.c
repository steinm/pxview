#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libintl.h>
#include <sys/types.h>
#include <regex.h>
#include <libgen.h>
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
	printf(_("  -v, --verbose       be more verbose."));
	printf("\n");
	if(!strcmp(progname, "pxview")) {
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
	}
	printf(_("  -o, --output=FILE   output data into file instead of stdout."));
	printf("\n");
	printf(_("  -b, --blobfile=FILE read blob data from file."));
	printf("\n");
	printf(_("  -p, --blobprefix=PREFIX prefix for all created files with blob data."));
	printf("\n");
	printf(_("  -r, --recode=ENCODING sets the target encoding."));
	printf("\n");
	if(strcmp(progname, "px2sql")) {
		printf(_("  --separator=CHAR    character used to separate field values."));
		printf("\n");
		printf(_("  --enclosure=CHAR    character used to enclose field values."));
		printf("\n");
	}
	printf(_("  --includeblobs      add blob fields in sql output."));
	printf("\n");
	printf(_("  --fields=REGEX      extended regular expression to select fields."));
	printf("\n");
	if(strcmp(progname, "px2csv")) {
		printf(_("  --tablename=NAME    overwrite name of database table."));
		printf("\n");
		printf(_("  --deletetable       delete existing sql database table."));
		printf("\n");
	}
	printf("\n");
	if(!strcmp(progname, "pxview")) {
		printf(_("If you do not specify any of the options -i, -c, -s, or -t\nthen -i will be used."));
		printf("\n\n");
	}
	if(strcmp(progname, "px2sql")) {
		printf(_("The options --separator and --enclosure will only affect csv output."));
		printf("\n\n");
	}
	if(!strcmp(progname, "pxview")) {
		printf(_("The option --fields will only affect csv and sql output."));
		printf("\n\n");
	}
	if(strcmp(progname, "px2sql")) {
		printf(_("If exporting csv format fields will be separated by tabulator\nand enclosed into \"."));
		printf("\n\n");
	}
}

int main(int argc, char *argv[]) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	pxdoc_t *pxdoc = NULL;
	pxblob_t *pxblob = NULL;
	char *progname = NULL;
	char *selectedfields;
	char *data, *buffer = NULL;
	int i, j, c; // general counters
	int first; // used to indicate if output has started or not
	int outputcsv = 0;
	int outputinfo = 0;
	int outputsql = 0;
	int outputschema = 0;
	int outputdebug = 0;
	int includeblobs = 0;
	int deletetable = 0;
	int verbose = 0;
	char delimiter = '\t';
	char enclosure = '"';
	char *inputfile = NULL;
	char *outputfile = NULL;
	char *blobfile = NULL;
	char *blobprefix = NULL;
	char *fieldregex = NULL;
	char *tablename = NULL;
	char *targetencoding = NULL;
	FILE *outfp = NULL;

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	setlocale (LC_NUMERIC, "C");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (GETTEXT_PACKAGE);
#endif

	progname = basename(strdup(argv[0]));
	while(1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"info", 0, 0, 'i'},
			{"csv", 0, 0, 'c'},
			{"sql", 0, 0, 's'},
			{"schema", 0, 0, 't'},
			{"verbose", 0, 0, 'v'},
			{"blobfile", 1, 0, 'b'},
			{"blobprefix", 1, 0, 'p'},
			{"recode", 1, 0, 'r'},
			{"output", 1, 0, 'o'},
			{"help", 0, 0, 'h'},
			{"separator", 1, 0, 0},
			{"enclosure", 1, 0, 1},
			{"includeblobs", 0, 0, 2},
			{"fields", 1, 0, 'f'},
			{"tablename", 1, 0, 3},
			{"deletetable", 1, 0, 5},
			{"mode", 1, 0, 4},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "icsvtf:b:r:p:o:h",
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
				includeblobs = 1;
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
				} else if(!strcmp(optarg, "debug")) {
					outputdebug = 1;
				}
				break;
			case 5:
				deletetable = 1;
				break;
			case 'h':
				usage(progname);
				exit(0);
				break;
			case 'v':
				verbose = 1;
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
			case 'r':
				targetencoding = strdup(optarg);
				break;
			case 'f':
				fieldregex = strdup(optarg);
				break;
			case 'o':
				outputfile = strdup(optarg);
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

	if(!strcmp(progname, "px2sql")) {
		outputinfo = 0;
		outputcsv = 0;
		outputschema = 0;
		outputsql = 1;
	} else if(!strcmp(progname, "px2csv")) {
		outputinfo = 0;
		outputcsv = 1;
		outputschema = 0;
		outputsql = 0;
	}

	/* if none the output modes is selected then display info */
	if(outputinfo == 0 && outputcsv == 0 && outputschema == 0 && outputsql == 0 && outputdebug == 0)
		outputinfo = 1;

	if (optind < argc) {
		inputfile = strdup(argv[optind]);
	}

	if(!inputfile) {
		fprintf(stderr, _("You must at least specify an input file."));
		fprintf(stderr, "\n");
		fprintf(stderr, "\n");
		usage(progname);
		exit(0);
	}

	if(outputfile) {
		outfp = fopen(outputfile, "w");
		if(outfp == NULL) {
			fprintf(stderr, _("Could not open output file."));
			fprintf(stderr, "\n");
			exit(1);
		}
	} else {
		outfp = stdout;
	}

	pxdoc = PX_new();
	if(0 > PX_open_file(pxdoc, inputfile)) {
		fprintf(stderr, _("Could not open input file."));
		fprintf(stderr, "\n");
		exit(1);
	}
	pxh = pxdoc->px_head;
	if(targetencoding != NULL)
		PX_set_targetencoding(pxdoc, targetencoding);

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
		fprintf(outfp, _("File Version:        %1.1f\n"), (float) pxh->px_fileversion/10.0);
		fprintf(outfp, _("File Type:           "));
		switch(pxh->px_filetype) {
			case pxfFileTypIndexDB:
				fprintf(outfp, _("indexed .DB data file"));
				break;
			case pxfFileTypPrimIndex:
				fprintf(outfp, _("primary index .PX file"));
				break;
			case pxfFileTypNonIndexDB:
				fprintf(outfp, _("non-indexed .DB data file"));
				break;
			case pxfFileTypNonIncSecIndex:
				fprintf(outfp, _("non-incrementing secondary index .Xnn file"));
				break;
			case pxfFileTypSecIndex:
				fprintf(outfp, _("secondary index .Ynn file (inc or non-inc)"));
				break;
			case pxfFileTypIncSecIndex:
				fprintf(outfp, _("incrementing secondary index .Xnn file"));
				break;
			case pxfFileTypNonIncSecIndexG:
				fprintf(outfp, _("non-incrementing secondary index .XGn file"));
				break;
			case pxfFileTypSecIndexG:
				fprintf(outfp, _("secondary index .YGn file (inc or non inc)"));
				break;
			case pxfFileTypIncSecIndexG:
				fprintf(outfp, _("incrementing secondary index .XGn file"));
				break;
		}
		fprintf(outfp, "\n");
		fprintf(outfp, _("Tablename:           %s\n"), pxh->px_tablename);
		fprintf(outfp, _("Num. of Records:     %d\n"), pxh->px_numrecords);
		fprintf(outfp, _("Num. of Fields:      %d\n"), pxh->px_numfields);
		fprintf(outfp, _("Header size:         %d (0x%X)\n"), pxh->px_headersize, pxh->px_headersize);
		fprintf(outfp, _("Max. Table size:     %d (0x%X)\n"), pxh->px_maxtablesize, pxh->px_maxtablesize*0x400);
		fprintf(outfp, _("Num. of Data Blocks: %d\n"), pxh->px_fileblocks);
		if((pxh->px_filetype == pxfFileTypNonIncSecIndex) ||
			 (pxh->px_filetype == pxfFileTypIncSecIndex))
			fprintf(outfp, _("Num. of Index Field: %d\n"), pxh->px_indexfieldnumber);
		fprintf(outfp, _("Num. of prim. Key fields: %d\n"), pxh->px_primarykeyfields);
		fprintf(outfp, _("Write protected:     %d\n"), pxh->px_writeprotected);
		fprintf(outfp, _("Code Page:           %d (0x%X)\n"), pxh->px_doscodepage, pxh->px_doscodepage);
		if(verbose) {
			fprintf(outfp, _("Record size:         %d (0x%X)\n"), pxh->px_recordsize, pxh->px_recordsize);
			fprintf(outfp, _("Sort order:          %d (0x%X)\n"), pxh->px_sortorder, pxh->px_sortorder);
			fprintf(outfp, _("Auto increment:      %d (0x%X)\n"), pxh->px_autoinc, pxh->px_autoinc);
		}
		fprintf(outfp, "\n");

		fprintf(outfp, _("Fieldname          | Type\n"));
		fprintf(outfp, "------------------------------------\n");
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			fprintf(outfp, "%18s | ", pxf->px_fname);
			switch(pxf->px_ftype) {
				case pxfAlpha:
					fprintf(outfp, "char(%d)\n", pxf->px_flen);
					break;
				case pxfDate:
					fprintf(outfp, "date(%d)\n", pxf->px_flen);
					break;
				case pxfShort:
					fprintf(outfp, "int(%d)\n", pxf->px_flen);
					break;
				case pxfLong:
					fprintf(outfp, "int(%d)\n", pxf->px_flen);
					break;
				case pxfCurrency:
					fprintf(outfp, "currency(%d)\n", pxf->px_flen);
					break;
				case pxfNumber:
					fprintf(outfp, "double(%d)\n", pxf->px_flen);
					break;
				case pxfLogical:
					fprintf(outfp, "boolean(%d)\n", pxf->px_flen);
					break;
				case pxfMemoBLOb:
					fprintf(outfp, "blob(%d)\n", pxf->px_flen);
					break;
				case pxfBLOb:
					fprintf(outfp, "blob(%d)\n", pxf->px_flen);
					break;
				case pxfFmtMemoBLOb:
					fprintf(outfp, "blob(%d)\n", pxf->px_flen);
					break;
				case pxfOLE:
					fprintf(outfp, "ole(%d)\n", pxf->px_flen);
					break;
				case pxfGraphic:
					fprintf(outfp, "graphic(%d)\n", pxf->px_flen);
					break;
				case pxfTime:
					fprintf(outfp, "time(%d)\n", pxf->px_flen);
					break;
				case pxfTimestamp:
					fprintf(outfp, "timestamp(%d)\n", pxf->px_flen);
					break;
				case pxfAutoInc:
					fprintf(outfp, "autoinc(%d)\n", pxf->px_flen);
					break;
				case pxfBCD:
					fprintf(outfp, "decimal(17,%d)\n", pxf->px_flen);
					break;
				case pxfBytes:
					fprintf(outfp, "bytes(%d)\n", pxf->px_flen);
					break;
				default:
					fprintf(outfp, "%c(%d)\n", pxf->px_ftype, pxf->px_flen);
			}
			pxf++;
		}
		fprintf(outfp, "------------------------------------\n");
	}

	if(outputschema) {
		int sumlen = 0;
		if(tablename)
			fprintf(outfp, "[%s]\n", tablename);
		else
			fprintf(outfp, "[%s]\n", pxh->px_tablename);
		fprintf(outfp, "Filetype=Delimited\n");
		fprintf(outfp, "Delimiter=%c\n", enclosure);
		fprintf(outfp, "Separator=%c\n", delimiter);
		fprintf(outfp, "CharSet=ANSIINTL\n");
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
					fprintf(outfp, "Field%d=", i+1);
					fprintf(outfp, "%s,", pxf->px_fname);
					break;
			}
			switch(pxf->px_ftype) {
				case pxfAlpha:
					fprintf(outfp, "Char,%d,00,%d\n", pxf->px_flen, sumlen);
					sumlen += pxf->px_flen;
					break;
				case pxfDate:
					fprintf(outfp, "ADate,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfShort:
					fprintf(outfp, "Short Integer,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfAutoInc:
				case pxfLong:
					fprintf(outfp, "Long Integer,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfCurrency:
					fprintf(outfp, "Currency,20,02,%d\n", sumlen);
					sumlen += 20;
					break;
				case pxfNumber:
					fprintf(outfp, "Float,20,02,%d\n", sumlen);
					sumlen += 20;
					break;
				case pxfLogical:
					fprintf(outfp, "Boolean,%d,00,%d\n", pxf->px_flen, sumlen);
					sumlen += pxf->px_flen;
					break;
				case pxfMemoBLOb:
				case pxfBLOb:
				case pxfFmtMemoBLOb:
				case pxfOLE:
				case pxfGraphic:
					break;
				case pxfTime:
					fprintf(outfp, "ATime,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfTimestamp:
					fprintf(outfp, "ATimestamp,11,00,%d\n", sumlen);
					sumlen += 11;
					break;
				case pxfBCD:
					fprintf(outfp, "Float,17,%d,%d\n", pxf->px_flen, sumlen);
					sumlen += 17;
					break;
				case pxfBytes:
					fprintf(outfp, "Char,%d,00,%d\n", pxf->px_flen, sumlen);
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
		if((selectedfields = (char *) pxdoc->malloc(pxdoc, pxh->px_numfields, _("Could not allocate memory for array of selected fields."))) == NULL) {
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
		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
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
					if(fieldregex == NULL || selectedfields[i]) {
						if(first == 1)
							fprintf(outfp, "%c", delimiter);
						switch(pxf->px_ftype) {
							case pxfAlpha: {
								char *value;
								if(PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
									if(enclosure && strchr(value, delimiter))
										fprintf(outfp, "%c%s%c", enclosure, value, enclosure);
									else
										fprintf(outfp, "%s", value);
								}
								first = 1;
								break;
							}
							case pxfDate: {
								long value;
								int year, month, day;
								if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									PX_SdnToGregorian(value+1721425, &year, &month, &day);
									fprintf(outfp, "%02d.%02d.%04d", day, month, year);
								}
								first = 1;
								break;
								}
							case pxfShort: {
								short int value;
								if(PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%d", value);
								}
								first = 1;
								break;
								}
							case pxfAutoInc:
							case pxfLong: {
								long value;
								if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%ld", value);
								}
								first = 1;
								break;
								}
							case pxfTime: {
								long value;
								if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
								}
								first = 1;
								break;
								}
							case pxfCurrency:
							case pxfNumber: {
								double value;
								if(PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value))
									fprintf(outfp, "%f", value);
								} 
								first = 1;
								break;
							case pxfLogical:
								if(*((char *)(&data[offset])) & 0x80) {
									data[offset] &= 0x7f;
									if(data[offset])
										fprintf(outfp, "1");
									else
										fprintf(outfp, "0");
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
									fprintf(outfp, "offset=%ld ", boffset);
									fprintf(outfp, "size=%ld ", size);
									fprintf(outfp, "mod_nr=%d ", mod_nr);
									blobdata = PX_read_blobdata(pxblob, boffset, size);
									if(blobdata) {
										sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
										fp = fopen(filename, "w");
										if(fp) {
											fwrite(blobdata, size, 1, fp);
											fclose(fp);
										} else {
											fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
										}
									} else {
										fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
									}

								} else {
									fprintf(outfp, "offset=%ld ", get_long(&data[offset]) & 0xffffff00);
									fprintf(outfp, "size=%ld ", get_long(&data[offset+4]));
									fprintf(outfp, "mod_nr=%d ", get_short(&data[offset+8]));
									hex_dump(outfp, &data[offset], pxf->px_flen);
								}
								first = 1;
								break;
							default:
								fprintf(outfp, "");
						}
					}
					offset += pxf->px_flen;
					pxf++;
				}
				if(pxh->px_filetype == pxfFileTypPrimIndex) {
					short int value;
					if(PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
					}
					offset += 2;
					if(PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
					}
					offset += 2;
					if(PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
					}
				}
				fprintf(outfp, "\n");
			} else {
				fprintf(stderr, _("Couldn't get record\n"));
			}
		}
		px_free(pxdoc, data);
	}

	/* Output data as sql statements */
	if(outputsql) {
		/* check if existing table shall be delete */
		if(deletetable) {
			if(tablename)
				fprintf(outfp, "DELETE TABLE %s;\n", tablename);
			else
				fprintf(outfp, "DELETE TABLE %s;\n", pxh->px_tablename);
		}
		/* Output table schema */
		if(tablename)
			fprintf(outfp, "CREATE TABLE %s (\n", tablename);
		else
			fprintf(outfp, "CREATE TABLE %s (\n", pxh->px_tablename);
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				if(first == 1)
					fprintf(outfp, ",\n");
				switch(pxf->px_ftype) {
					case pxfAlpha:
					case pxfDate:
					case pxfShort:
					case pxfLong:
					case pxfAutoInc:
					case pxfCurrency:
					case pxfNumber:
					case pxfLogical:
					case pxfTime:
					case pxfTimestamp:
					case pxfBCD:
					case pxfBytes:
						fprintf(outfp, "  %s ", pxf->px_fname);
						first = 1;
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs) {
							fprintf(outfp, "  %s ", pxf->px_fname);
							first = 1;
						}
						break;
				}
				switch(pxf->px_ftype) {
					case pxfAlpha:
						fprintf(outfp, "char(%d)", pxf->px_flen);
						break;
					case pxfDate:
						fprintf(outfp, "date");
						break;
					case pxfShort:
						fprintf(outfp, "smallint");
						break;
					case pxfLong:
					case pxfAutoInc:
						fprintf(outfp, "integer");
						break;
					case pxfCurrency:
					case pxfNumber:
						fprintf(outfp, "decimal(20,2)");
						break;
					case pxfLogical:
						fprintf(outfp, "boolean");
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs)
							fprintf(outfp, "oid");
						break;
					case pxfOLE:
						break;
					case pxfTime:
						fprintf(outfp, "time");
						break;
					case pxfTimestamp:
						fprintf(outfp, "timestamp");
						break;
					case pxfBCD:
						fprintf(outfp, "decimal(17,%d)", pxf->px_flen);
						break;
					case pxfBytes:
						fprintf(outfp, "char(%d)", pxf->px_flen);
						break;
					default:
						break;
				}
			}
			pxf++;
		}
		fprintf(outfp, "\n);\n\n");

		/* Only output data if we have at least one record */
		if(pxh->px_numrecords > 0) {
			if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
				if(selectedfields)
					px_free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}

			if(tablename)
				fprintf(outfp, "COPY %s (", tablename);
			else
				fprintf(outfp, "COPY %s (", pxh->px_tablename);
			first = 0;  // set to 1 when first field has been output
			pxf = pxh->px_fields;
			/* output field name */
			for(i=0; i<pxh->px_numfields; i++) {
				if(fieldregex == NULL ||  selectedfields[i]) {
					if(first == 1)
						fprintf(outfp, ", ");
					switch(pxf->px_ftype) {
						case pxfAlpha:
						case pxfDate:
						case pxfShort:
						case pxfLong:
						case pxfAutoInc:
						case pxfTime:
						case pxfCurrency:
						case pxfNumber:
						case pxfLogical:
							fprintf(outfp, "%s", pxf->px_fname);
							first = 1;
							break;
						case pxfMemoBLOb:
						case pxfBLOb:
						case pxfFmtMemoBLOb:
						case pxfGraphic:
							if(includeblobs) {
								fprintf(outfp, "%s", pxf->px_fname);
								first = 1;
							}
							break;
					}
				}
				pxf++;
			}
			fprintf(outfp, ") FROM stdin;\n");
			for(j=0; j<pxh->px_numrecords; j++) {
				int offset;
				if(PX_get_record(pxdoc, j, data)) {
					first = 0;  // set to 1 when first field has been output
					offset = 0;
					pxf = pxh->px_fields;
					for(i=0; i<pxh->px_numfields; i++) {
						if(fieldregex == NULL ||  selectedfields[i]) {
							if(first == 1)
								fprintf(outfp, "\t");
							switch(pxf->px_ftype) {
								case pxfAlpha: {
									char *value;
									if(PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%s", value);
									}
									first = 1;

									break;
								}
								case pxfDate: {
									long value;
									if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%ld", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfShort: {
									short int value;
									if(PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%d", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfAutoInc:
								case pxfLong: {
									long value;
									if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%ld", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfTime: {
									long value;
									if(PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
									}
								case pxfCurrency:
								case pxfNumber: {
									double value;
									if(PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%f", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
									}
								case pxfLogical:
									if(*((char *)(&data[offset])) & 0x80) {
										data[offset] &= 0x7f;
										if(data[offset])
											fprintf(outfp, "TRUE");
										else
											fprintf(outfp, "FALSE");
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								case pxfMemoBLOb:
								case pxfBLOb:
								case pxfFmtMemoBLOb:
								case pxfGraphic:
									if(includeblobs) {
										fprintf(outfp, "\\N");
										first = 1;
									}
									break;
								default:
									fprintf(outfp, "");
							}
						}
						offset += pxf->px_flen;
						pxf++;
					}
					fprintf(outfp, "\n");
				} else {
					fprintf(stderr, _("Couldn't get record\n"));
				}
			}
			fprintf(outfp, "\\.\n");
			px_free(pxdoc, data);
		}
	}

	if(outputdebug) {
		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
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
					if(fieldregex == NULL || selectedfields[i]) {
						fprintf(outfp, "%s: ", pxf->px_fname);
						hex_dump(outfp, &data[offset], pxf->px_flen);
						fprintf(outfp, "\n");
					}
					offset += pxf->px_flen;
					pxf++;
				}
				fprintf(outfp, "\n");
			} else {
				fprintf(stderr, _("Couldn't get record\n"));
			}
		}
		px_free(pxdoc, data);
	}

	if(selectedfields)
		px_free(pxdoc, selectedfields);

	PX_close(pxdoc);

	exit(0);
}
