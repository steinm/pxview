#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <getopt.h>
#include <libintl.h>
#include <sys/types.h>
#include <regex.h>
#include <libgen.h>
#include "config.h"
#ifdef HAVE_GSF
#include <paradox-gsf.h>
#else
#include <paradox.h>
#endif

#ifdef MEMORY_DEBUGGING
#include <paradox-mp.h>
#endif

#ifdef HAVE_SQLITE
#include <sqlite.h>
#endif
#define _(String) gettext(String)

void strrep(char *str, char c1, char c2) {
	char *ptr = str;

	while(*ptr != '\0') {
		if(*ptr == c1)
			*ptr = c2;
		ptr++;
	}
}

/* pbuffer() {{{
 * print a string at the end of a buffer
 */
void pbuffer(char *buffer, const char *fmt, ...) {
	char msg[256];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);

	strcat(buffer, msg);

	va_end(ap);
}
/* }}} */

void errorhandler(pxdoc_t *p, int error, const char *str, void *data) {
	  fprintf(stderr, "PXLib: %s\n", str);
}

/* usage() {{{
 * Output usage information
 */
void usage(char *progname) {
	int recode;

	printf(_("Version: %s %s http://sourceforge.net/projects/pxlib"), progname, VERSION);
	printf("\n");
	printf(_("Copyright: Copyright (C) 2003 Uwe Steinmann <uwe@steinmann.cx>"));
	printf("\n\n");
	if(!strcmp(progname, "px2csv")) {
		printf(_("%s reads a paradox file and outputs the file in CSV format."), progname);
	} else if(!strcmp(progname, "px2sql")) {
		printf(_("%s reads a paradox file and outputs the file in SQL format."), progname);
	} else if(!strcmp(progname, "px2html")) {
		printf(_("%s reads a paradox file and outputs the file in HTML format."), progname);
	} else if(!strcmp(progname, "px2sqlite")) {
		printf(_("%s reads a paradox file and writes the output into a sqlite database."), progname);
	} else {
		printf(_("%s reads a paradox file and outputs information about the file\nor dumps the content in CSV, HTML, SQL or sqlite format."), progname);
	}
	printf("\n\n");
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
		printf(_("  -q, --sqlite        dump records into sqlite database."));
		printf("\n");
		printf(_("  -x, --html          dump records in HTML format."));
		printf("\n");
		printf(_("  -t, --shema         output schema of database."));
		printf("\n");
		printf(_("  --mode=MODE         set output mode (csv, sql, sqlite, html or schema)."));
		printf("\n");
	}
	printf(_("  -o, --output-file=FILE output data into file instead of stdout."));
	printf("\n");
	printf(_("  -b, --blobfile=FILE read blob data from file."));
	printf("\n");
	printf(_("  -p, --blobprefix=PREFIX prefix for all created files with blob data."));
	printf("\n");
	printf(_("  -n, --primary-index-file=FILE read primary index from file."));
	printf("\n");
	printf(_("  -r, --recode=ENCODING sets the target encoding."));
	printf("\n");
	if(!strcmp(progname, "px2csv") || !strcmp(progname, "pxview")) {
		printf(_("  --separator=CHAR    character used to separate field values."));
		printf("\n");
		printf(_("  --enclosure=CHAR    character used to enclose field values."));
		printf("\n");
	}
	if(!strcmp(progname, "px2csv") || !strcmp(progname, "px2html") || !strcmp(progname, "pxview")) {
		printf(_("  --mark-deleted      add extra column with 1 for deleted records."));
		printf("\n");
	}
	printf(_("  --include-blobs     add blob fields in sql output."));
	printf("\n");
	printf(_("  --fields=REGEX      extended regular expression to select fields."));
	printf("\n");
	printf(_("  --output-deleted    output also records which were deleted."));
	printf("\n");
#ifdef HAVE_GSF
	if(PX_has_gsf_support()) {
		printf(_("  --use-gsf           use gsf library to read input file."));
		printf("\n");
	}
#endif
	if(strcmp(progname, "px2csv") && strcmp(progname, "px2html")) {
		printf(_("  --tablename=NAME    overwrite name of database table."));
		printf("\n");
	}
	if(!strcmp(progname, "px2sql") || !strcmp(progname, "pxview") || !strcmp(progname, "px2sqlite")) {
		printf(_("  --delete-table      delete existing sql database table."));
		printf("\n");
	}
	printf("\n");
	if(!strcmp(progname, "pxview")) {
		printf(_("If you do not specify any of the options -i, -c, -s, -x, -q or -t\nthen -i will be used."));
		printf("\n\n");
	}
	if(!strcmp(progname, "pxview")) {
		printf(_("The option --fields will only affect csv, html, sql and sqlite output."));
		printf("\n\n");
		printf(_("The options --separator, --enclosure and --mark-deleted will only\naffect csv output."));
		printf("\n\n");
	}
	if(!strcmp(progname, "px2csv")) {
		printf(_("If exporting csv format fields will be separated by tabulator\nand enclosed into \"."));
		printf("\n\n");
	}

	printf(_("Supported output formats: "));
	printf(_("csv")); printf(" ");
	printf(_("html")); printf(" ");
	printf(_("sql")); printf(" ");
#ifdef HAVE_SQLITE
	printf(_("sqlite")); printf(" ");
#endif
	printf("\n\n");
	recode = PX_has_recode_support();
	switch(recode) {
		case 1:
			printf(_("libpx uses librecode for recoding."));
			break;
		case 2:
			printf(_("libpx uses iconv for recoding."));
			break;
		case 0:
			printf(_("libpx has no support for recoding."));
			break;
	}
	printf("\n");
	if(PX_is_bigendian())
		printf(_("libpx has been compiled for big endian architecture."));
	else
		printf(_("libpx has been compiled for little endian architecture."));
	printf("\n");
	printf(_("libpx has gsf support: %s"), PX_has_gsf_support() == 1 ? _("Yes") : _("No"));
	printf("\n");
	printf(_("libpx has version: %d.%d.%d"), PX_get_majorversion(), PX_get_minorversion(), PX_get_subminorversion());
	printf("\n\n");
}
/* }}} */

/* main() {{{
 */
int main(int argc, char *argv[]) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	pxdoc_t *pxdoc = NULL;
	pxdoc_t *pindexdoc = NULL;
	pxblob_t *pxblob = NULL;
	char *progname = NULL;
	char *selectedfields = NULL;
	char *data, *buffer = NULL;
	int i, j, c; // general counters
	int first; // used to indicate if output has started or not
	int outputcsv = 0;
	int outputhtml = 0;
	int outputinfo = 0;
	int outputsql = 0;
	int outputsqlite = 0;
	int outputschema = 0;
	int outputdebug = 0;
	int includeblobs = 0;
	int deletetable = 0;
	int outputdeleted = 0;
	int markdeleted = 0;
	int usegsf = 0;
	int verbose = 0;
	char delimiter = '\t';
	char enclosure = '"';
	char *inputfile = NULL;
	char *outputfile = NULL;
	char *blobfile = NULL;
	char *pindexfile = NULL;
	char *blobprefix = NULL;
	char *fieldregex = NULL;
	char *tablename = NULL;
	char *targetencoding = NULL;
	FILE *outfp = NULL;

#ifdef MEMORY_DEBUGGING
	PX_mp_init();
#endif

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	setlocale (LC_NUMERIC, "C");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (GETTEXT_PACKAGE);
#endif

	/* Handle program options {{{
	 */
	progname = basename(strdup(argv[0]));
	while(1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"info", 0, 0, 'i'},
			{"csv", 0, 0, 'c'},
			{"sql", 0, 0, 's'},
			{"html", 0, 0, 'x'},
			{"schema", 0, 0, 't'},
			{"sqlite", 0, 0, 'q'},
			{"verbose", 0, 0, 'v'},
			{"blobfile", 1, 0, 'b'},
			{"blobprefix", 1, 0, 'p'},
			{"recode", 1, 0, 'r'},
			{"output-file", 1, 0, 'o'},
			{"help", 0, 0, 'h'},
			{"separator", 1, 0, 0},
			{"enclosure", 1, 0, 1},
			{"includeblobs", 0, 0, 2},
			{"include-blobs", 0, 0, 2},
			{"fields", 1, 0, 'f'},
			{"tablename", 1, 0, 3},
			{"mode", 1, 0, 4},
			{"deletetable", 0, 0, 5},
			{"delete-table", 0, 0, 5},
			{"outputdeleted", 0, 0, 6},
			{"output-deleted", 0, 0, 6},
			{"markdeleted", 0, 0, 7},
			{"mark-deleted", 0, 0, 7},
			{"use-gsf", 0, 0, 8},
			{"primary-index-file", 1, 0, 'n'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "icsxqvtf:b:r:p:o:n:h",
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
				} else if(!strcmp(optarg, "sqlite")) {
#ifdef HAVE_SQLITE
					outputsqlite = 1;
#else
					printf(_("No sqlite support available."));
					exit(1);
#endif
				} else if(!strcmp(optarg, "html")) {
					outputhtml = 1;
				} else if(!strcmp(optarg, "schema")) {
					outputschema = 1;
				} else if(!strcmp(optarg, "debug")) {
					outputdebug = 1;
				}
				break;
			case 5:
				deletetable = 1;
				break;
			case 6:
				outputdeleted = 1;
				break;
			case 7:
				markdeleted = 1;
				break;
			case 8:
				usegsf = 1;
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
			case 'x':
				outputhtml = 1;
				break;
			case 'q':
#ifdef HAVE_SQLITE
				outputsqlite = 1;
#else
				printf(_("No sqlite support available."));
				exit(1);
#endif
				break;
			case 'n':
				pindexfile = strdup(optarg);
				break;
		}
	}

	if (optind < argc) {
		inputfile = strdup(argv[optind]);
	}

	if(!inputfile) {
		fprintf(stderr, _("You must at least specify an input file."));
		fprintf(stderr, "\n");
		fprintf(stderr, "\n");
		usage(progname);
		exit(1);
	}
	/* }}} */

	/* Handle different program names {{{
	 */
	if(!strcmp(progname, "px2sql")) {
		outputinfo = 0;
		outputcsv = 0;
		outputschema = 0;
		outputsql = 1;
		outputsqlite = 0;
		outputhtml = 0;
	} else if(!strcmp(progname, "px2csv")) {
		outputinfo = 0;
		outputcsv = 1;
		outputschema = 0;
		outputsql = 0;
		outputsqlite = 0;
		outputhtml = 0;
	} else if(!strcmp(progname, "px2html")) {
		outputinfo = 0;
		outputcsv = 0;
		outputschema = 0;
		outputsql = 0;
		outputsqlite = 0;
		outputhtml = 1;
	} else if(!strcmp(progname, "px2sqlite")) {
		outputinfo = 0;
		outputcsv = 0;
		outputschema = 0;
		outputsql = 0;
		outputsqlite = 1;
		outputhtml = 0;
	}
	/* }}} */

	/* if none the output modes is selected then display info */
	if(outputinfo == 0 && outputcsv == 0 && outputschema == 0 && outputsql == 0 && outputdebug == 0 && outputhtml == 0 && outputsqlite == 0)
		outputinfo = 1;

	/* Create output file {{{
	 */
	if((outputfile == NULL) || !strcmp(outputfile, "-")) {
		if(outputsqlite) {
			fprintf(stderr, _("sqlite database cannot be written to stdout."));
			exit(1);
		} else {
			outfp = stdout;
		}
	} else {
		if(!outputsqlite) {
			outfp = fopen(outputfile, "w");
			if(outfp == NULL) {
				fprintf(stderr, _("Could not open output file."));
				fprintf(stderr, "\n");
				exit(1);
			}
		}
	}
	/* }}} */

	/* Open input file {{{
	 */
#ifdef MEMORY_DEBUGGING
	if(NULL == (pxdoc = PX_new2(errorhandler, PX_mp_malloc, PX_mp_realloc, PX_mp_free))) {
#else
	if(NULL == (pxdoc = PX_new2(errorhandler, NULL, NULL, NULL))) {
#endif
		fprintf(stderr, _("Could not create new paradox instance."));
		fprintf(stderr, "\n");
		exit(1);
	}

#ifdef HAVE_GSF
	if(PX_has_gsf_support() && usegsf) {
		GsfInput *input = NULL;
		GsfInputStdio  *in_stdio;
		GsfInputMemory *in_mem;
		GError *gerr = NULL;
		fprintf(stderr, "Inputfile:  %s\n", inputfile);
		gsf_init ();
		in_mem = gsf_input_mmap_new (inputfile, NULL);
		if (in_mem == NULL) {
			in_stdio = gsf_input_stdio_new(inputfile, &gerr);
			if(in_stdio != NULL)
				input = GSF_INPUT (in_stdio);
			else {
				fprintf(stderr, _("Could not open gsf input file."));
				fprintf(stderr, "\n");
				g_object_unref (G_OBJECT (input));
				exit(1);
			}
		} else {
			input = GSF_INPUT (in_mem);
		}
		if(0 > PX_open_gsf(pxdoc, input)) {
			fprintf(stderr, _("Could not open input file."));
			fprintf(stderr, "\n");
			exit(1);
		}
	} else {
#endif
		if(0 > PX_open_file(pxdoc, inputfile)) {
			fprintf(stderr, _("Could not open input file."));
			fprintf(stderr, "\n");
			exit(1);
		}
#ifdef HAVE_GSF
	}
#endif
	/* }}} */

	/* Open primary index file {{{
	 */
	if(pindexfile) {
		pindexdoc = PX_new2(errorhandler, NULL, NULL, NULL);
		if(0 > PX_open_file(pindexdoc, pindexfile)) {
			fprintf(stderr, _("Could not open primary index file."));
			fprintf(stderr, "\n");
			exit(1);
		}
		if(0 > PX_read_primary_index(pindexdoc)) {
			fprintf(stderr, _("Could not read primary index file."));
			fprintf(stderr, "\n");
			exit(1);
		}
		if(0 > PX_add_primary_index(pxdoc, pindexdoc)) {
			fprintf(stderr, _("Could not add primary index file."));
			fprintf(stderr, "\n");
			exit(1);
		}
	}
	/* }}} */

	pxh = pxdoc->px_head;
	if(targetencoding != NULL)
		PX_set_targetencoding(pxdoc, targetencoding);

	/* Set tablename to the one in the header if it wasn't set before */
	if(tablename == NULL)
		tablename = pxh->px_tablename;
	strrep(tablename, '.', '_');
	strrep(tablename, ' ', '_');

	/* Open the file containing the blobs if one is given {{{
	 */
	if(blobfile) {
		pxblob = PX_new_blob(pxdoc);
		if(0 > PX_open_blob_file(pxblob, blobfile)) {
			fprintf(stderr, _("Could not open blob file."));
			fprintf(stderr, "\n");
			PX_close(pxdoc);
			exit(1);
		}
	}
	/* }}} */

	/* Output info {{{
	 */
	if(outputinfo) {
		int reclen;
		struct tm time_tm;
		fprintf(outfp, _("File Version:            %1.1f\n"), (float) pxh->px_fileversion/10.0);
		fprintf(outfp, _("File Type:               "));
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
		fprintf(outfp, _("Tablename:               %s\n"), pxh->px_tablename);
		fprintf(outfp, _("Num. of Records:         %d\n"), pxh->px_numrecords);
		fprintf(outfp, _("Theor. Num. of Rec.:     %d\n"), pxh->px_theonumrecords);
		fprintf(outfp, _("Num. of Fields:          %d\n"), pxh->px_numfields);
		fprintf(outfp, _("Header size:             %d (0x%X)\n"), pxh->px_headersize, pxh->px_headersize);
		fprintf(outfp, _("Max. Table size:         %d (0x%X)\n"), pxh->px_maxtablesize, pxh->px_maxtablesize*0x400);
		fprintf(outfp, _("Num. of Data Blocks:     %d\n"), pxh->px_fileblocks);
		fprintf(outfp, _("Num. of 1st Data Block:  %d\n"), pxh->px_firstblock);
		fprintf(outfp, _("Num. of last Data Block: %d\n"), pxh->px_lastblock);
		if((pxh->px_filetype == pxfFileTypNonIncSecIndex) ||
		   (pxh->px_filetype == pxfFileTypIncSecIndex)) {
			fprintf(outfp, _("Num. of Index Field:     %d\n"), pxh->px_indexfieldnumber);
			fprintf(outfp, _("Sort order of Field:     %d\n"), pxh->px_refintegrity);
		}
		if((pxh->px_filetype == pxfFileTypIndexDB) ||
		   (pxh->px_filetype == pxfFileTypNonIndexDB)) {
			fprintf(outfp, _("Num. of prim. Key fields: %d\n"), pxh->px_primarykeyfields);
			fprintf(outfp, _("Next auto inc. value:    %d\n"), pxh->px_autoinc);
		}
		fprintf(outfp, _("Write protected:         %d\n"), pxh->px_writeprotected);
		fprintf(outfp, _("Code Page:               %d (0x%X)\n"), pxh->px_doscodepage, pxh->px_doscodepage);
		localtime_r((time_t *) &(pxh->px_fileupdatetime), &time_tm);
		fprintf(outfp, _("Update time:             %d.%d.%d %d:%02d:%02d (%d)\n"), time_tm.tm_mday, time_tm.tm_mon+1, time_tm.tm_year+1900, time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, pxh->px_fileupdatetime);
		if(verbose) {
			fprintf(outfp, _("Record size:             %d (0x%X)\n"), pxh->px_recordsize, pxh->px_recordsize);
			fprintf(outfp, _("Sort order:              %d (0x%X)\n"), pxh->px_sortorder, pxh->px_sortorder);
			fprintf(outfp, _("Auto increment:          %d (0x%X)\n"), pxh->px_autoinc, pxh->px_autoinc);
			fprintf(outfp, _("Modified Flags 1:        %d (0x%X)\n"), pxh->px_modifiedflags1, pxh->px_modifiedflags1);
			fprintf(outfp, _("Modified Flags 2:        %d (0x%X)\n"), pxh->px_modifiedflags2, pxh->px_modifiedflags2);
		}
		fprintf(outfp, "\n");

		fprintf(outfp, _("Fieldname          | Type\n"));
		fprintf(outfp, "------------------------------------\n");
		pxf = pxh->px_fields;
		reclen = 0;
		for(i=0; i<pxh->px_numfields; i++) {
			reclen += pxf->px_flen;
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
		fprintf(outfp, _("     Record length | %d (0x%X)\n"), reclen, reclen);
	}
	/* }}} */

	/* Output Schema {{{
	 */
	if(outputschema) {
		int sumlen = 0;

		if((pxh->px_filetype != pxfFileTypIndexDB) && 
		   (pxh->px_filetype != pxfFileTypNonIndexDB)) {
			fprintf(stderr, _("Schema output is only reasonable for DB files."));
			fprintf(stderr, "\n");
			PX_close(pxdoc);
			exit(1);
		}

		fprintf(outfp, "[%s]\n", tablename);
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
				case pxfAutoInc:
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
	/* }}} *?

	/* Check which fields shall be shown in sql or csv output {{{
	 */
	if(fieldregex) {
		regex_t preg;
		if(regcomp(&preg, fieldregex, REG_NOSUB|REG_EXTENDED|REG_ICASE)) {
			fprintf(stderr, _("Could not compile regular expression to select fields."));
			PX_close(pxdoc);
			exit(1);
		}
		/* allocate memory for selected field array */
		if((selectedfields = (char *) pxdoc->malloc(pxdoc, pxh->px_numfields, _("Could not allocate memory for array of selected fields."))) == NULL) {
			PX_close(pxdoc);
			exit(1);
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
	/* }}} */

	/* Output data as comma separated values {{{ */
	if(outputcsv) {
		int numrecords, ireccounter = 0;
		int isdeleted, presetdeleted;

		/* Output first line with column names */
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL || selectedfields[i]) {
				if(first == 1)
					fprintf(outfp, "%c", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "%s", pxf->px_fname);
				switch(pxf->px_ftype) {
					case pxfAlpha:
						fprintf(outfp, ",A,%d", pxf->px_flen);
						break;
					case pxfDate:
						fprintf(outfp, ",D,%d", pxf->px_flen);
						break;
					case pxfShort:
						fprintf(outfp, ",S,%d", pxf->px_flen);
						break;
					case pxfAutoInc:
						fprintf(outfp, ",+,%d", pxf->px_flen);
						break;
					case pxfTimestamp:
						fprintf(outfp, ",@,%d", pxf->px_flen);
						break;
					case pxfLong:
						fprintf(outfp, ",I,%d", pxf->px_flen);
						break;
					case pxfTime:
						fprintf(outfp, ",T,%d", pxf->px_flen);
						break;
					case pxfCurrency:
						fprintf(outfp, ",$,%d", pxf->px_flen);
						break;
					case pxfNumber:
						fprintf(outfp, ",N,%d", pxf->px_flen);
						break;
					case pxfLogical:
						fprintf(outfp, ",L,%d", pxf->px_flen);
						break;
					case pxfGraphic:
						fprintf(outfp, ",G,%d", pxf->px_flen);
						break;
					case pxfBLOb:
						fprintf(outfp, ",B,%d", pxf->px_flen);
						break;
					case pxfOLE:
						fprintf(outfp, ",O,%d", pxf->px_flen);
						break;
					case pxfFmtMemoBLOb:
						fprintf(outfp, ",F,%d", pxf->px_flen);
						break;
					case pxfMemoBLOb:
						fprintf(outfp, ",F,%d", pxf->px_flen);
						break;
					case pxfBytes:
						fprintf(outfp, ",Y,%d", pxf->px_flen);
						break;
					case pxfBCD:
						fprintf(outfp, ",#,%d", pxf->px_flen);
						break;
				}
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				first = 1;
			}
			pxf++;
		}
		fprintf(outfp, "\n");

		/* Allocate memory for record */
		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Allocate memory for record."))) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		if(outputdeleted) {
			numrecords = pxh->px_theonumrecords;
			presetdeleted = 1;
		} else {
			numrecords = pxh->px_numrecords;
			presetdeleted = 0;
		}
		/* Output records */
		for(j=0; j<numrecords; j++) {
			int offset;
			pxdatablockinfo_t pxdbinfo;
			isdeleted = presetdeleted;
			if(NULL != PX_get_record2(pxdoc, j, data, &isdeleted, &pxdbinfo)) {
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
								if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
									if(enclosure && strchr(value, delimiter))
										fprintf(outfp, "%c%s%c", enclosure, value, enclosure);
									else
										fprintf(outfp, "%s", value);
									pxdoc->free(pxdoc, value);
								}
								first = 1;
								break;
							}
							case pxfDate: {
								long value;
								int year, month, day;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									PX_SdnToGregorian(value+1721425, &year, &month, &day);
									fprintf(outfp, "%02d.%02d.%04d", day, month, year);
								}
								first = 1;
								break;
								}
							case pxfShort: {
								short int value;
								if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%d", value);
								}
								first = 1;
								break;
								}
							case pxfAutoInc:
							case pxfTimestamp:
							case pxfLong: {
								long value;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%ld", value);
								}
								first = 1;
								break;
								}
							case pxfTime: {
								long value;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
								}
								first = 1;
								break;
								}
							case pxfCurrency:
							case pxfNumber: {
								double value;
								if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%f", value);
								} 
								first = 1;
								break;
								} 
							case pxfLogical: {
								char value;
								if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
									if(value)
										fprintf(outfp, "1");
									else
										fprintf(outfp, "0");
								}
								first = 1;
								break;
								}
							case pxfGraphic:
							case pxfBLOb:
							case pxfFmtMemoBLOb:
							case pxfOLE:
								if(pxblob) {
									char *blobdata;
									char filename[200];
									FILE *fp;
									size_t size, boffset, mod_nr;
									size = get_long_le(&data[offset+4]);
									boffset = get_long_le(&data[offset]) & 0xffffff00;
									mod_nr = get_short_le(&data[offset+8]);
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
									fprintf(outfp, "offset=%ld ", get_long_le(&data[offset]) & 0xffffff00);
									fprintf(outfp, "size=%ld ", get_long_le(&data[offset+4]));
									fprintf(outfp, "mod_nr=%d ", get_short_le(&data[offset+8]));
									hex_dump(outfp, &data[offset], pxf->px_flen);
								}
								first = 1;
								break;
							case pxfBytes:
								hex_dump(outfp, &data[offset], pxf->px_flen);
								first = 1;
								break;
							case pxfBCD:
								hex_dump(outfp, &data[offset], pxf->px_flen);
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
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
					}
					offset += 2;
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
						ireccounter += value;
					}
					offset += 2;
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "%c", delimiter);
						fprintf(outfp, "%d", value);
					}
					fprintf(outfp, "%c", delimiter);
					fprintf(outfp, "%d", pxdbinfo.number);
				}
				if(markdeleted) {
					fprintf(outfp, "%c", delimiter);
					fprintf(outfp, "%d", isdeleted);
				}
				fprintf(outfp, "\n");
			} else {
				fprintf(stderr, _("Couldn't get record number %d\n"), j);
			}
		}
		/* Print sum over all records */
		if(pxh->px_filetype == pxfFileTypPrimIndex) {
			for(i=0; i<pxh->px_numfields; i++)
				fprintf(outfp, "%c", delimiter);
			fprintf(outfp, "%c", delimiter);
			fprintf(outfp, "%d", ireccounter);
			fprintf(outfp, "%c", delimiter);
			fprintf(outfp, "\n");
		}
		pxdoc->free(pxdoc, data);
	}
	/* }}} */

#ifdef HAVE_SQLITE
	/* Output data into sqlite database {{{
	 */
	if(outputsqlite) {
		int numrecords;
		sqlite *sql;
		char *buffer;
		char *sqlerror;

		if((pxh->px_filetype != pxfFileTypIndexDB) && 
		   (pxh->px_filetype != pxfFileTypNonIndexDB)) {
			fprintf(stderr, _("SQL output is only reasonable for DB files."));
			fprintf(stderr, "\n");
			PX_close(pxdoc);
			exit(1);
		}

		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		/* Allocate memory for string buffer */
		if((buffer = (char *) pxdoc->malloc(pxdoc, 2*pxh->px_recordsize, _("Could not allocate memory for string buffer."))) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		if((sql = sqlite_open(outputfile, 0, NULL)) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		/* check if existing table shall be delete */
		if(deletetable) {
			sprintf(buffer, "DROP TABLE %s;\n", tablename);
			if(SQLITE_OK != sqlite_exec(sql, buffer, NULL, NULL, &sqlerror)) {
				fprintf(stderr, "%s\n", sqlerror);
				sqlite_close(sql);
				pxdoc->free(pxdoc, buffer);
				pxdoc->free(pxdoc, data);
				if(selectedfields)
					pxdoc->free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}
		}
		/* Output table schema */
		buffer[0] = '\0';
		pbuffer(buffer, "CREATE TABLE %s (\n", tablename);
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				strrep(pxf->px_fname, ' ', '_');
				if(first == 1)
					pbuffer(buffer, ",\n");
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
						pbuffer(buffer, "  %s ", pxf->px_fname);
						first = 1;
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs) {
							pbuffer(buffer, "  %s ", pxf->px_fname);
							first = 1;
						} else {
							first = 0;
						}
						break;
				}
				switch(pxf->px_ftype) {
					case pxfAlpha:
						pbuffer(buffer, "char(%d)", pxf->px_flen);
						break;
					case pxfDate:
						pbuffer(buffer, "date");
						break;
					case pxfShort:
						pbuffer(buffer, "smallint");
						break;
					case pxfLong:
					case pxfAutoInc:
						pbuffer(buffer, "integer");
						break;
					case pxfCurrency:
					case pxfNumber:
						pbuffer(buffer, "decimal(20,2)");
						break;
					case pxfLogical:
						pbuffer(buffer, "boolean");
						break;
					case pxfMemoBLOb:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfGraphic:
						if(includeblobs)
							pbuffer(buffer, "oid");
						break;
					case pxfOLE:
						break;
					case pxfTime:
						pbuffer(buffer, "time");
						break;
					case pxfTimestamp:
						pbuffer(buffer, "timestamp");
						break;
					case pxfBCD:
						pbuffer(buffer, "decimal(34,%d)", pxf->px_flen);
						break;
					case pxfBytes:
						pbuffer(buffer, "char(%d)", pxf->px_flen);
						break;
					default:
						break;
				}
				if(i < pxh->px_primarykeyfields)
					pbuffer(buffer, " unique");
			}
			pxf++;
		}
		pbuffer(buffer, "\n);\n");
		if(SQLITE_OK != sqlite_exec(sql, buffer, NULL, NULL, &sqlerror)) {
			sqlite_close(sql);
			fprintf(stderr, "%s\n", sqlerror);
			pxdoc->free(pxdoc, buffer);
			pxdoc->free(pxdoc, data);
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		/* Create the indexes */
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_primarykeyfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				strrep(pxf->px_fname, ' ', '_');
				buffer[0] = '\0';
				pbuffer(buffer, "CREATE INDEX %s_%s_index on %s (%s);", tablename, pxf->px_fname, tablename, pxf->px_fname);
				if(SQLITE_OK != sqlite_exec(sql, buffer, NULL, NULL, &sqlerror)) {
					sqlite_close(sql);
					fprintf(stderr, "%s\n", sqlerror);
					pxdoc->free(pxdoc, buffer);
					pxdoc->free(pxdoc, data);
					if(selectedfields)
						pxdoc->free(pxdoc, selectedfields);
					PX_close(pxdoc);
					exit(1);
				}
			}
			pxf++;
		}

		/* Only output data if we have at least one record */
		if(pxh->px_numrecords > 0) {
			if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
				if(selectedfields)
					pxdoc->free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}

			for(j=0; j<pxh->px_numrecords; j++) {
				int offset;
				buffer[0] = '\0';
				pbuffer(buffer, "INSERT INTO %s VALUES (", tablename);
				if(PX_get_record(pxdoc, j, data)) {
					first = 0;  // set to 1 when first field has been output
					offset = 0;
					pxf = pxh->px_fields;
					for(i=0; i<pxh->px_numfields; i++) {
						if(fieldregex == NULL ||  selectedfields[i]) {
							if(first == 1)
								pbuffer(buffer, ",");
							switch(pxf->px_ftype) {
								case pxfAlpha: {
									char *value;
									if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "'%s'", value);
										pxdoc->free(pxdoc, value);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;

									break;
								}
								case pxfDate: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "%ld", value);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfShort: {
									short int value;
									if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "%d", value);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfAutoInc:
								case pxfTimestamp:
								case pxfLong: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "%ld", value);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfTime: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfCurrency:
								case pxfNumber: {
									double value;
									if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
										pbuffer(buffer, "%f", value);
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfLogical: {
									char value;
									if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
										if(value)
											pbuffer(buffer, "TRUE");
										else
											pbuffer(buffer, "FALSE");
									} else {
										pbuffer(buffer, "NULL");
									}
									first = 1;
									break;
								}
								case pxfMemoBLOb:
								case pxfBLOb:
								case pxfFmtMemoBLOb:
								case pxfGraphic:
									if(includeblobs) {
										pbuffer(buffer, "NULL");
										first = 1;
									} else {
										first = 0;
									}
									break;
								case pxfBCD:
									pbuffer(buffer, "NULL");
									break;
								default:
									pbuffer(buffer, "NULL");
							}
						}
						offset += pxf->px_flen;
						pxf++;
					}
					pbuffer(buffer, ");\n");
				} else {
					fprintf(stderr, _("Couldn't get record number %d\n"), j);
				}

				if(SQLITE_OK != sqlite_exec(sql, buffer, NULL, NULL, &sqlerror)) {
					sqlite_close(sql);
					fprintf(stderr, "%s\n", sqlerror);
					pxdoc->free(pxdoc, buffer);
					pxdoc->free(pxdoc, data);
					if(selectedfields)
						pxdoc->free(pxdoc, selectedfields);
					PX_close(pxdoc);
					exit(1);
				}
			}
		}
		pxdoc->free(pxdoc, buffer);
		pxdoc->free(pxdoc, data);

		sqlite_close(sql);
	}
	/* }}} */
#endif

	/* Output HTML Table {{{
	 */
	if(outputhtml) {
		int numrecords;
		int isdeleted, presetdeleted;

		/* Allocate memory for record data */
		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		if(outputdeleted) {
			numrecords = pxh->px_theonumrecords;
			presetdeleted = 1;
		} else {
			numrecords = pxh->px_numrecords;
			presetdeleted = 0;
		}

		fprintf(outfp, "<table>\n");
		fprintf(outfp, " <caption>%s</caption>\n", pxh->px_tablename);
		fprintf(outfp, " <tr>\n");

		/* output field name */
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				fprintf(outfp, "  <td><b>");
				fprintf(outfp, "%s", pxf->px_fname);
				switch(pxf->px_ftype) {
					case pxfAlpha:
						fprintf(outfp, ",A,%d", pxf->px_flen);
						break;
					case pxfDate:
						fprintf(outfp, ",D,%d", pxf->px_flen);
						break;
					case pxfShort:
						fprintf(outfp, ",S,%d", pxf->px_flen);
						break;
					case pxfAutoInc:
						fprintf(outfp, ",+,%d", pxf->px_flen);
						break;
					case pxfTimestamp:
						fprintf(outfp, ",@,%d", pxf->px_flen);
						break;
					case pxfLong:
						fprintf(outfp, ",I,%d", pxf->px_flen);
						break;
					case pxfTime:
						fprintf(outfp, ",T,%d", pxf->px_flen);
						break;
					case pxfCurrency:
						fprintf(outfp, ",$,%d", pxf->px_flen);
						break;
					case pxfNumber:
						fprintf(outfp, ",N,%d", pxf->px_flen);
						break;
					case pxfLogical:
						fprintf(outfp, ",L,%d", pxf->px_flen);
						break;
					case pxfGraphic:
						fprintf(outfp, ",G,%d", pxf->px_flen);
						break;
					case pxfBLOb:
						fprintf(outfp, ",B,%d", pxf->px_flen);
						break;
					case pxfOLE:
						fprintf(outfp, ",O,%d", pxf->px_flen);
						break;
					case pxfFmtMemoBLOb:
						fprintf(outfp, ",F,%d", pxf->px_flen);
						break;
					case pxfMemoBLOb:
						fprintf(outfp, ",F,%d", pxf->px_flen);
						break;
					case pxfBytes:
						fprintf(outfp, ",Y,%d", pxf->px_flen);
						break;
					case pxfBCD:
						fprintf(outfp, ",#,%d", pxf->px_flen);
						break;
				}
				fprintf(outfp, "</b></td>\n");
			}
			pxf++;
		}
		if(markdeleted) {
			fprintf(outfp, "  <td><b>deleted</b></td>\n", isdeleted);
		}
		fprintf(outfp, " </tr>\n");

		for(j=0; j<numrecords; j++) {
			int offset;
			isdeleted = presetdeleted;
			if(NULL != PX_get_record2(pxdoc, j, data, &isdeleted, NULL)) {
				pxf = pxh->px_fields;
				offset = 0;
				fprintf(outfp, " <tr valign=\"top\">\n");
				for(i=0; i<pxh->px_numfields; i++) {
					if(fieldregex == NULL || selectedfields[i]) {
						fprintf(outfp, "  <td>");
						switch(pxf->px_ftype) {
							case pxfAlpha: {
								char *value;
								if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%s", value);
									pxdoc->free(pxdoc, value);
								}
								break;
							}
							case pxfDate: {
								long value;
								int year, month, day;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									PX_SdnToGregorian(value+1721425, &year, &month, &day);
									fprintf(outfp, "%02d.%02d.%04d", day, month, year);
								}
								break;
								}
							case pxfShort: {
								short int value;
								if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%d", value);
								}
								break;
								}
							case pxfAutoInc:
							case pxfLong: {
								long value;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%ld", value);
								}
								break;
							}
							case pxfTime: {
								long value;
								if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
								}
								break;
							}
							case pxfCurrency:
							case pxfNumber: {
								double value;
								if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
									fprintf(outfp, "%f", value);
								} 
								break;
							} 
							case pxfLogical: {
								char value;
								if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
									if(value)
										fprintf(outfp, "1");
									else
										fprintf(outfp, "0");
								}
								break;
							}
							case pxfGraphic:
							case pxfBLOb:
								if(pxblob) {
									char *blobdata;
									char filename[200];
									FILE *fp;
									size_t size, boffset, mod_nr;
									size = get_long_le(&data[offset+4]);
									boffset = get_long_le(&data[offset]) & 0xffffff00;
									mod_nr = get_short_le(&data[offset+8]);
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
										fprintf(outfp, "%s", filename);
									} else {
										fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
									}

								} else {
									fprintf(outfp, "offset=%ld ", get_long_le(&data[offset]) & 0xffffff00);
									fprintf(outfp, "size=%ld ", get_long_le(&data[offset+4]));
									fprintf(outfp, "mod_nr=%d", get_short_le(&data[offset+8]));
								}
								break;
							default:
								fprintf(outfp, "");
						}
						fprintf(outfp, "</td>\n");
					}
					offset += pxf->px_flen;
					pxf++;
				}
				if(pxh->px_filetype == pxfFileTypPrimIndex) {
					short int value;
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "  <td>%d</td>\n", value);
					}
					offset += 2;
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "  <td>%d</td>\n", value);
					}
					offset += 2;
					if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
						fprintf(outfp, "  <td>%d</td>\n", value);
					}
				}
				if(markdeleted) {
					fprintf(outfp, "  <td>%d</td>\n", isdeleted);
				}
				fprintf(outfp, " <tr>\n");
			} else {
				fprintf(stderr, _("Couldn't get record number %d\n"), j);
			}
		}
		fprintf(outfp, "</table>\n");
		pxdoc->free(pxdoc, data);
	}
	/* }}} */

	/* Output data as sql statements {{{
	 */
	if(outputsql) {
		if((pxh->px_filetype != pxfFileTypIndexDB) && 
		   (pxh->px_filetype != pxfFileTypNonIndexDB)) {
			fprintf(stderr, _("SQL output is only reasonable for DB files."));
			fprintf(stderr, "\n");
			PX_close(pxdoc);
			exit(1);
		}

		/* check if existing table shall be delete */
		if(deletetable) {
			fprintf(outfp, "DROP TABLE %s;\n", tablename);
		}
		/* Output table schema */
		fprintf(outfp, "CREATE TABLE %s (\n", tablename);
		first = 0;  // set to 1 when first field has been output
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				strrep(pxf->px_fname, ' ', '_');
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
						} else {
							first = 0;
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
				if(i < pxh->px_primarykeyfields)
					fprintf(outfp, " unique");
			}
			pxf++;
		}
		fprintf(outfp, "\n);\n");

		/* Create the indexes */
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_primarykeyfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				strrep(pxf->px_fname, ' ', '_');
				fprintf(outfp, "CREATE INDEX %s_%s_index on %s (%s);\n", tablename, pxf->px_fname, tablename, pxf->px_fname);
			}
			pxf++;
		}

		/* Only output data if we have at least one record */
		if(pxh->px_numrecords > 0) {
			if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
				if(selectedfields)
					pxdoc->free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}

			fprintf(outfp, "COPY %s (", tablename);
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
						case pxfBCD:
						case pxfTimestamp:
						case pxfBytes:
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
							} else {
								first = 0;
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
									if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%s", value);
										pxdoc->free(pxdoc, value);
									}
									first = 1;

									break;
								}
								case pxfDate: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%ld", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfShort: {
									short int value;
									if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%d", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfAutoInc:
								case pxfTimestamp:
								case pxfLong: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%ld", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfTime: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
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
									if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
										fprintf(outfp, "%f", value);
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfLogical: {
									char value;
									if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
										if(value)
											fprintf(outfp, "TRUE");
										else
											fprintf(outfp, "FALSE");
									} else {
										fprintf(outfp, "\\N");
									}
									first = 1;
									break;
								}
								case pxfMemoBLOb:
								case pxfBLOb:
								case pxfFmtMemoBLOb:
								case pxfGraphic:
									if(includeblobs) {
										fprintf(outfp, "\\N");
										first = 1;
									} else {
										first = 0;
									}
									break;
								case pxfBCD:
								case pxfBytes:
									fprintf(outfp, "\\N");
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
					fprintf(stderr, _("Couldn't get record number %d\n"), j);
				}
			}
			fprintf(outfp, "\\.\n");
			pxdoc->free(pxdoc, data);
		}
	}
	/* }}} */

	/* Output debug {{{
	 */
	if(outputdebug) {
		int numrecords;
		int isdeleted, presetdeleted;
		if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			if(selectedfields)
				pxdoc->free(pxdoc, selectedfields);
			PX_close(pxdoc);
			exit(1);
		}

		if(outputdeleted) {
			numrecords = pxh->px_theonumrecords;
			presetdeleted = 1;
		} else {
			numrecords = pxh->px_numrecords;
			presetdeleted = 0;
		}

		for(j=0; j<numrecords; j++) {
			int offset;
			pxdatablockinfo_t pxdbinfo;
			isdeleted = presetdeleted;
			if(PX_get_record2(pxdoc, j, data, &isdeleted, &pxdbinfo)) {
				fprintf(outfp, _("Previous block number according to header: "));
				fprintf(outfp, "%d\n", pxdbinfo.prev);
				fprintf(outfp, _("Next block number according to header: "));
				fprintf(outfp, "%d\n", pxdbinfo.next);
				fprintf(outfp, _("Real block number in file: "));
				fprintf(outfp, "%d\n", pxdbinfo.number);
				fprintf(outfp, _("Block size: "));
				fprintf(outfp, "%d\n", pxdbinfo.size);
				fprintf(outfp, _("Record number in block: "));
				fprintf(outfp, "%d\n", pxdbinfo.recno);
				fprintf(outfp, _("Number of records in block: "));
				fprintf(outfp, "%d\n", pxdbinfo.numrecords);
				fprintf(outfp, _("Block position in file: "));
				fprintf(outfp, "%d (0x%X)\n", pxdbinfo.blockpos, pxdbinfo.blockpos);
				fprintf(outfp, _("Record position in file: "));
				fprintf(outfp, "%d (0x%X)\n", pxdbinfo.recordpos, pxdbinfo.recordpos);
				if(markdeleted) {
					fprintf(outfp, _("Record deleted: "));
					fprintf(outfp, "%d\n", isdeleted);
				}
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
				fprintf(stderr, _("Couldn't get record number %d\n"), j);
			}
		}
		pxdoc->free(pxdoc, data);
	}
	/* }}} */

	/* Free resources and close files {{{
	 */
	if(selectedfields)
		pxdoc->free(pxdoc, selectedfields);

	if(pindexfile) {
		PX_close(pindexdoc);
		PX_delete(pindexdoc);
	}

	PX_close(pxdoc);
	PX_delete(pxdoc);

#ifdef HAVE_GSF
	if(PX_has_gsf_support() && usegsf) {
		gsf_shutdown();
	}
#endif
	/* }}} */

#ifdef MEMORY_DEBUGGING
	PX_mp_list_unfreed();
#endif

	exit(0);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
