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

#ifdef ENABLE_NLS
#define _(String) gettext(String)
#else
#define _(String) String
#endif

/* strrep() {{{
 * Replace a char c1 with c2
 */
void strrep(char *str, char c1, char c2) {
	char *ptr = str;

	while(*ptr != '\0') {
		if(*ptr == c1)
			*ptr = c2;
		ptr++;
	}
}
/* }}} */

/* str_replace() {{{
 * Replace th first occurence of a substring s1 in str with the string s2
 * Returns the new string
 */
char * str_replace(char *str, char *s1, char *s2) {
	char *newstring, *newptr;
	char *firstoccurence;
	int index; /* Position of first occurence of s1 */
	int rep_len, find_len; /* Length of s2 and s1 */

	if(NULL == str || NULL == s1)
		return(NULL);
	if(NULL == s2)
		rep_len = 0;
	else
		rep_len = strlen(s2);
	find_len = strlen(s1);
	newstring = malloc(strlen(str) + rep_len - find_len + 1);
	firstoccurence = strstr(str, s1);
	if(NULL == firstoccurence) {
		memcpy(newstring, str, strlen(str)+1);
		return(newstring);
	}
	newptr = newstring;
	index = firstoccurence-str;
	memcpy(newptr, str, index); /* Copy string up to start of s1 */
	newptr += index;
	memcpy(newptr, s2, rep_len); /* Copy replacement string */
	newptr += rep_len;
	memcpy(newptr, firstoccurence+find_len, rep_len + 1); /* Copy rest of string */
	return(newstring);
}
/* }}} */

struct str_buffer {
	char *buffer;
	size_t cur;
	size_t size;
};

/* str_buffer_new() {{{
 * Create a new string buffer with the given initial size
 */
struct str_buffer *str_buffer_new(pxdoc_t *pxdoc, size_t size) {
	struct str_buffer *sb;
	if(NULL == (sb = pxdoc->malloc(pxdoc, sizeof(struct str_buffer), _("Allocate memory for string buffer"))))
		return NULL;
	if(size > 0) {
		if(NULL == (sb->buffer = pxdoc->malloc(pxdoc, size, _("Allocate memory for string buffer")))) {
			pxdoc->free(pxdoc, sb);
			return NULL;
		}
		sb->buffer[0] = '\0';
	} else {
		sb->buffer = NULL;
	}
	sb->size = size;
	sb->cur = 0;
	return(sb);
}
/* }}} */

/* str_buffer_delete() {{{
 * Frees the memory occupied by the string buffer
 */
void str_buffer_delete(pxdoc_t *pxdoc, struct str_buffer *sb) {
	if(sb->buffer)
		pxdoc->free(pxdoc, sb->buffer);
	pxdoc->free(pxdoc, sb);
}
/* }}} */

#define MSG_BUFSIZE 256
/* str_buffer_print() {{{
 * print a string at the end of a buffer
 */
void str_buffer_print(pxdoc_t *pxdoc, struct str_buffer *sb, const char *fmt, ...) {
	char msg[MSG_BUFSIZE];
	va_list ap;
	int written;

	va_start(ap, fmt);
	written = vsnprintf(msg, MSG_BUFSIZE, fmt, ap);
	if(written >= MSG_BUFSIZE) {
		fprintf(stderr, "Fatal Error: Format string is to short\n");
		exit(1);
	}

	/* Enlarge memory for buffer
	 * Enlarging it MSG_BUFSIZE ensure that the space is in any case
	 * sufficient to add the new string.
	 */
	if((sb->cur + written + 1) > sb->size) {
		sb->buffer = pxdoc->realloc(pxdoc, sb->buffer, sb->size+MSG_BUFSIZE, _("Get more memory for string buffer."));
		sb->size += MSG_BUFSIZE;
	}
	strcpy(&(sb->buffer[sb->cur]), msg);
	sb->cur += written;

	va_end(ap);
}
/* }}} */
#undef MSG_BUFSIZE

/* str_buffer_get() {{{
 * Returns a pointer to current buffer
 */
const char *str_buffer_get(pxdoc_t *pxdoc, struct str_buffer *sb) {
	return(sb->buffer);
}
/* }}} */

/* str_buffer_len() {{{
 * Returns len of string buffer
 */
size_t str_buffer_len(pxdoc_t *pxdoc, struct str_buffer *sb) {
	return(sb->cur);
}
/* }}} */

/* str_buffer_clear() {{{
 * Clears the string buffer but will not free the memory
 */
void str_buffer_clear(pxdoc_t *pxdoc, struct str_buffer *sb) {
	sb->cur = 0;
	sb->buffer[0] = '\0';
}
/* }}} */

/* str_buffer_printmask() {{{
 * Prints str to buffer and masks each occurence of c1 with c2.
 * Returns the number of written chars.
 */
int str_buffer_printmask(pxdoc_t *pxdoc, struct str_buffer *sb, char *str, char c1, char c2 ) {
	char *ptr, *dst;
	int len = 0;
	int c = 0;

	/* Count occurences of c1 */
	ptr = str;
	while(*ptr != '\0') {
		if(*ptr == c1)
			c++;
	}

	c += sb->cur + strlen(str) + 1;
	if(c > sb->size) {
		 sb->buffer = pxdoc->realloc(pxdoc, sb->buffer, c, _("Get more memory for string buffer."));
		 sb->size += c;
	}
	dst = &(sb->buffer[sb->cur]);
	ptr = str;
	while(*ptr != '\0') {
		if(*ptr == c1) {
			*dst++ = c2;
			len ++;
		} 
		*dst++ = *ptr++;
		len++;
	}
	*dst = '\0';
	sb->cur += len;
	return(len);
}
/* }}} */

/* printmask() {{{
 * Prints str and masks each occurence of c1 with c2.
 * Returns the number of written chars.
 */
int printmask(FILE *outfp, char *str, char c1, char c2 ) {
	char *ptr;
	int len = 0;
	ptr = str;
	while(*ptr != '\0') {
		if(*ptr == c1) {
			fprintf(outfp, "%c", c2);
			len ++;
		} 
		fprintf(outfp, "%c", *ptr);
		len++;
		ptr++;
	}
	return(len);
}
/* }}} */

struct sql_type_map {
	char *pxtype;
	char *sqltype;
};

/* set_default_sql_types() {{{
 */
void set_default_sql_types(struct sql_type_map *typemap) {
	memset(typemap, 0, (pxfBytes+1) * sizeof(struct sql_type_map));
	typemap[0].pxtype = NULL;
	typemap[0].sqltype = NULL;
	typemap[pxfAlpha].pxtype = strdup("alpha");
	typemap[pxfAlpha].sqltype = strdup("char(%d)");
	typemap[pxfDate].pxtype = strdup("date");
	typemap[pxfDate].sqltype = strdup("date");
	typemap[pxfShort].pxtype = strdup("short");
	typemap[pxfShort].sqltype = strdup("integer");
	typemap[pxfLong].pxtype = strdup("long");
	typemap[pxfLong].sqltype = strdup("integer");
	typemap[pxfCurrency].pxtype = strdup("currency");
	typemap[pxfCurrency].sqltype = strdup("decimal(20,2)");
	typemap[pxfNumber].pxtype = strdup("number");
	typemap[pxfNumber].sqltype = strdup("real");
	typemap[pxfLogical].pxtype = strdup("logical");
	typemap[pxfLogical].sqltype = strdup("boolean");
	typemap[pxfMemoBLOb].pxtype = strdup("memoblob");
	typemap[pxfMemoBLOb].sqltype = strdup("text");
	typemap[pxfBLOb].pxtype = strdup("blob");
	typemap[pxfBLOb].sqltype = strdup("text");
	typemap[pxfFmtMemoBLOb].pxtype = strdup("fmtmemoblob");
	typemap[pxfFmtMemoBLOb].sqltype = strdup("text");
	typemap[pxfOLE].pxtype = strdup("ole");
	typemap[pxfOLE].sqltype = strdup("text");
	typemap[pxfGraphic].pxtype = strdup("graphic");
	typemap[pxfGraphic].sqltype = strdup("text");
	typemap[pxfTime].pxtype = strdup("time");
	typemap[pxfTime].sqltype = strdup("time");
	typemap[pxfTimestamp].pxtype = strdup("timestamp");
	typemap[pxfTimestamp].sqltype = strdup("timestamp");
	typemap[pxfAutoInc].pxtype = strdup("autoinc");
	typemap[pxfAutoInc].sqltype = strdup("integer");
	typemap[pxfBCD].pxtype = strdup("bcd");
	typemap[pxfBCD].sqltype = strdup("decimal(34,%d)");
	typemap[pxfBytes].pxtype = strdup("bytes");
	typemap[pxfBytes].sqltype = strdup("text");
}
/* }}} */

/* set_sql_type() {{{
 */
void set_sql_type(struct sql_type_map *typemap, int pxtype, char *sqltype) {
	if(pxtype < 1 || pxtype > pxfBytes) {
		return;
	}
	if(sqltype == NULL) {
		return;
	}
	if(typemap[pxtype].sqltype)
		free(typemap[pxtype].sqltype);
	typemap[pxtype].sqltype = strdup(sqltype);
}
/* }}} */

/* get_sql_type() {{{
 */
char *get_sql_type(struct sql_type_map *typemap, int pxtype, int len) {
	static char buffer[200];
	if(pxtype < 1 || pxtype > pxfBytes) {
		return NULL;
	}
	snprintf(buffer, 200, typemap[pxtype].sqltype, len);
	return(buffer);
}
/* }}} */

/* errorhandler() {{{
 */
void errorhandler(pxdoc_t *p, int error, const char *str, void *data) {
	  fprintf(stderr, "PXLib: %s\n", str);
}
/* }}} */

/* usage() {{{
 * Output usage information
 */
void usage(char *progname) {
	int recode;

	printf(_("Version: %s %s http://sourceforge.net/projects/pxlib"), progname, VERSION);
	printf("\n");
	printf(_("Copyright: Copyright (C) 2003, 2004 Uwe Steinmann <uwe@steinmann.cx>"));
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
	printf(_("General options:"));
	printf("\n");
	printf(_("  -h, --help          this usage information."));
	printf("\n");
	printf(_("  --version           show version information."));
	printf("\n");
	printf(_("  -v, --verbose       be more verbose."));
	printf("\n");
#ifdef HAVE_GSF
	if(PX_has_gsf_support()) {
		printf(_("  --use-gsf           use gsf library to read input file."));
		printf("\n");
	}
#endif

	printf("\n");
	printf(_("Options to select output mode:"));
	printf("\n");
	if(!strcmp(progname, "pxview")) {
		printf(_("  -i, --info          show information about file."));
		printf("\n");
		printf(_("  -c, --csv           dump records in CSV format."));
		printf("\n");
		printf(_("  -s, --sql           dump records in SQL format."));
		printf("\n");
#ifdef HAVE_SQLITE
		printf(_("  -q, --sqlite        dump records into sqlite database."));
		printf("\n");
#endif
		printf(_("  -x, --html          dump records in HTML format."));
		printf("\n");
		printf(_("  -t, --shema         output schema of database."));
		printf("\n");
		printf(_("  --mode=MODE         set output mode (info, csv, sql, sqlite, html or schema)."));
		printf("\n");
	}
	printf(_("  -o, --output-file=FILE output data into file instead of stdout."));
	printf("\n");
	printf(_("  --output-deleted    output also records which were deleted."));
	printf("\n");
	printf(_("  --fields=REGEX      extended regular expression to select fields."));
	printf("\n");
	printf(_("  -r, --recode=ENCODING sets the target encoding."));
	printf("\n");
	printf(_("  -n, --primary-index-file=FILE read primary index from file."));
	printf("\n");

	printf("\n");
	printf(_("Options to handle blob files:"));
	printf("\n");
	printf(_("  --include-blobs     add blob fields in sql output."));
	printf("\n");
	printf(_("  -b, --blobfile=FILE read blob data from file."));
	printf("\n");
	printf(_("  -p, --blobprefix=PREFIX prefix for all created files with blob data."));
	printf("\n");

	if(!strcmp(progname, "px2html") || !strcmp(progname, "pxview")) {
		printf("\n");
		printf(_("Options for html ouput:"));
		printf("\n");
		printf(_("  --tablename=NAME    overwrite name of database table."));
		printf("\n");
		printf(_("  --mark-deleted      add extra column with 1 for deleted records."));
		printf("\n");
	}

	if(!strcmp(progname, "px2sql") || !strcmp(progname, "pxview") || !strcmp(progname, "px2sqlite")) {
		printf("\n");
		printf(_("Options for sql and sqlite ouput:"));
		printf("\n");
		printf(_("  --tablename=NAME    overwrite name of database table."));
		printf("\n");
		printf(_("  --delete-table      delete existing sql database table."));
		printf("\n");
		printf(_("  --skip-schema       do not output database table schema."));
		printf("\n");
		printf(_("  --short-insert      use short insert statements."));
		printf("\n");
		printf(_("  --set-sql-type=SPEC sets the type for a sql field."));
		printf("\n");
		printf(_("  --empty-string-is-null tread empty string as null."));
		printf("\n");
	}
	if(!strcmp(progname, "px2sql") || !strcmp(progname, "pxview")) {
		printf("\n");
		printf(_("Options for sql output:"));
		printf("\n");
		printf(_("  --use-copy          use COPY instead of INSERT statement."));
		printf("\n");
	}

	if(!strcmp(progname, "px2csv") || !strcmp(progname, "pxview")) {
		printf("\n");
		printf(_("Options for csv ouput:"));
		printf("\n");
		printf(_("  --separator=CHAR    character used to separate field values\n                      (default is ',')."));
		printf("\n");
		printf(_("  --enclosure=CHAR    character used to enclose field values\n                      (default is '\"')."));
		printf("\n");
		printf(_("  --without-head      Turn off first line with field names."));
		printf("\n");
		printf(_("  --mark-deleted      add extra column with 1 for deleted records."));
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
	int skipschema = 0;
	int shortinsert = 0;
	int outputdeleted = 0;
	int markdeleted = 0;
	int usecopy = 0;
	int usegsf = 0;
	int verbose = 0;
	int withouthead = 0;
	int emptystringisnull = 0;
	char delimiter = ',';
	char enclosure = '"';
	char *inputfile = NULL;
	char *outputfile = NULL;
	char *blobfile = NULL;
	char *pindexfile = NULL;
	char *blobprefix = NULL;
	char *fieldregex = NULL;
	char *tablename = NULL;
	char *targetencoding = NULL;
	struct sql_type_map *typemap;
	FILE *outfp = NULL;

	/* allocate 1 more struct because the first one is not used */
	typemap = malloc((pxfBytes+1) * sizeof(struct sql_type_map));
	set_default_sql_types(typemap);

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
			{"use-copy", 0, 0, 9},
			{"without-head", 0, 0, 10},
			{"skip-schema", 0, 0, 12},
			{"short-insert", 0, 0, 13},
			{"set-sql-type", 1, 0, 14},
			{"empty-string-is-null", 0, 0, 15},
			{"primary-index-file", 1, 0, 'n'},
			{"version", 0, 0, 11},
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
				if(!strcmp(optarg, "info")) {
					outputinfo = 1;
				} else if(!strcmp(optarg, "csv")) {
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
			case 9:
				usecopy = 1;
				break;
			case 10:
				withouthead = 1;
				break;
			case 11:
				fprintf(stdout, "%s\n", VERSION);
				exit(0);
				break;
			case 12:
				skipschema = 1;
				break;
			case 13:
				shortinsert = 1;
				break;
			case 14: {
				char *delimptr;
				int typelen;
				int index = 0;
				delimptr = strchr(optarg, ':');
				if(NULL != delimptr) {
					typelen = delimptr-optarg;
					for(i=1; i<=pxfBytes; i++) {
						if(typemap[i].pxtype) {
							if(!strncmp(typemap[i].pxtype, optarg, typelen)) {
								index = i;
							}
						}
					}
					if(index) {
						typemap[index].sqltype = strdup(delimptr+1);
					} else {
						fprintf(stderr, _("Unknown paradox type specified with --set-sql-type."));
						fprintf(stderr, "\n");
					}
				} else {
					fprintf(stderr, _("Argument of --set-sql-type does not contain the delimiting character ':'."));
					fprintf(stderr, "\n");
					exit;
				}
				break;
			}
			case 15:
				emptystringisnull = 1;
				break;
			case 'h':
				usage(progname);
				printf(_("Predefined paradox to sql field type mapping:"));
				printf("\n");
				for(i=1; i<pxfBytes; i++) {
					if(typemap[i].sqltype)
						printf("%s:%s\n", typemap[i].pxtype, typemap[i].sqltype);
				}
				printf("\n");
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
			fprintf(stderr, "\n");
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
		if(pxh->px_filetype == pxfFileTypPrimIndex) {
			fprintf(outfp, _("Root index block number:  %d\n"), pxh->px_indexroot);
			fprintf(outfp, _("Num. of index levels:     %d\n"), pxh->px_numindexlevels);
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
					fprintf(outfp, "memoblob(%d)\n", pxf->px_flen);
					break;
				case pxfBLOb:
					fprintf(outfp, "blob(%d)\n", pxf->px_flen);
					break;
				case pxfFmtMemoBLOb:
					fprintf(outfp, "fmtmemoblob(%d)\n", pxf->px_flen);
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
					fprintf(outfp, "decimal(34,%d)\n", pxf->px_flen);
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

	/* Check which fields shall be shown in output {{{
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
		if(!withouthead) {
			first = 0;  // set to 1 when first field has been output
			pxf = pxh->px_fields;
			for(i=0; i<pxh->px_numfields; i++) {
				if(fieldregex == NULL || selectedfields[i]) {
					if(first == 1)
						fprintf(outfp, "%c", delimiter);
					if(delimiter == ',')
						fprintf(outfp, "%c", enclosure);
					if(strlen(pxf->px_fname))
						fprintf(outfp, "%s", pxf->px_fname);
					else
						fprintf(outfp, "column%d", i+1);
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
			if(pxh->px_filetype == pxfFileTypPrimIndex) {
				fprintf(outfp, "%c", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "blocknr,I,4");
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "%c", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "count,I,4");
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "%c", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "dummy,I,4");
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "%c", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "thisblocknr,I,4");
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
			}
			if(markdeleted) {
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
				fprintf(outfp, "%cdeleted,L,1", delimiter);
				if(delimiter == ',')
					fprintf(outfp, "%c", enclosure);
			}
			fprintf(outfp, "\n");
		}

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
									if(enclosure && strchr(value, delimiter)) {
										fprintf(outfp, "%c", enclosure);
										if(strchr(value, enclosure))
											printmask(outfp, value, enclosure, enclosure);
										else
											fprintf(outfp, "%s", value);
										fprintf(outfp, "%c", enclosure);
									} else {
										if(strchr(value, enclosure)) {
											fprintf(outfp, "%c", enclosure);
											printmask(outfp, value, enclosure, enclosure);
											fprintf(outfp, "%c", enclosure);
										} else
											fprintf(outfp, "%s", value);
									}
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
									fprintf(outfp, "%g", value);
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
							case pxfMemoBLOb:
							case pxfOLE:
								if(pxblob) {
									char *blobdata;
									char filename[200];
									FILE *fp;
									int mod_nr, size;
									blobdata = PX_read_blobdata(pxblob, &data[offset], pxf->px_flen, &mod_nr, &size);
									if(size) {
										if(blobdata) {
											if(pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
												int i;
												for(i=0; i<size; i++) {
													fputc(blobdata[i], outfp);
												}
											} else {
												sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
												fp = fopen(filename, "w");
												if(fp) {
													fwrite(blobdata, size, 1, fp);
													fclose(fp);
													fprintf(outfp, "%s", filename);
												} else {
													fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
												}
											}
										} else {
											fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
										}
									}
									if(blobdata)
										pxblob->pxdoc->free(pxblob->pxdoc, blobdata);

								} else {
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
		struct str_buffer *sbuf;
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

		/* Allocate memory for string buffer.
		 */
		if((sbuf = str_buffer_new(pxdoc, 20)) == NULL) {
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
			str_buffer_print(pxdoc, sbuf, "DROP TABLE %s;\n", tablename);
			if(SQLITE_OK != sqlite_exec(sql, str_buffer_get(pxdoc, sbuf), NULL, NULL, &sqlerror)) {
				fprintf(stderr, "%s\n", sqlerror);
				sqlite_close(sql);
				str_buffer_delete(pxdoc, sbuf);
				pxdoc->free(pxdoc, data);
				if(selectedfields)
					pxdoc->free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}
		}
		/* Output table schema */
		if(!skipschema) {
			str_buffer_clear(pxdoc, sbuf);
			str_buffer_print(pxdoc, sbuf, "CREATE TABLE %s (\n", tablename);
			first = 0;  // set to 1 when first field has been output
			pxf = pxh->px_fields;
			for(i=0; i<pxh->px_numfields; i++) {
				if(fieldregex == NULL ||  selectedfields[i]) {
					strrep(pxf->px_fname, ' ', '_');
					if(first == 1)
						str_buffer_print(pxdoc, sbuf, ",\n");
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
							str_buffer_print(pxdoc, sbuf, "  %s ", pxf->px_fname);
							str_buffer_print(pxdoc, sbuf, "%s", get_sql_type(typemap, pxf->px_ftype, pxf->px_flen));
							first = 1;
							break;
						case pxfMemoBLOb:
						case pxfBLOb:
						case pxfFmtMemoBLOb:
						case pxfGraphic:
						case pxfOLE:
							if(includeblobs) {
								str_buffer_print(pxdoc, sbuf, "  %s ", pxf->px_fname);
								str_buffer_print(pxdoc, sbuf, "%s", get_sql_type(typemap, pxf->px_ftype, pxf->px_flen));
								first = 1;
							} else {
								first = 0;
							}
							break;
					}
					if(i < pxh->px_primarykeyfields)
						str_buffer_print(pxdoc, sbuf, " unique");
				}
				pxf++;
			}
			str_buffer_print(pxdoc, sbuf, ");");
			if(SQLITE_OK != sqlite_exec(sql, str_buffer_get(pxdoc, sbuf), NULL, NULL, &sqlerror)) {
				sqlite_close(sql);
				fprintf(stderr, "%s\n", sqlerror);
				str_buffer_delete(pxdoc, sbuf);
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
					str_buffer_clear(pxdoc, sbuf);
					str_buffer_print(pxdoc, sbuf, "CREATE INDEX %s_%s_index on %s (%s);", tablename, pxf->px_fname, tablename, pxf->px_fname);
					if(SQLITE_OK != sqlite_exec(sql, str_buffer_get(pxdoc, sbuf), NULL, NULL, &sqlerror)) {
						sqlite_close(sql);
						fprintf(stderr, "%s\n", sqlerror);
						str_buffer_delete(pxdoc, sbuf);
						pxdoc->free(pxdoc, data);
						if(selectedfields)
							pxdoc->free(pxdoc, selectedfields);
						PX_close(pxdoc);
						exit(1);
					}
				}
				pxf++;
			}
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
				str_buffer_clear(pxdoc, sbuf);
				str_buffer_print(pxdoc, sbuf, "INSERT INTO %s VALUES (", tablename);
				if(PX_get_record(pxdoc, j, data)) {
					first = 0;  // set to 1 when first field has been output
					offset = 0;
					pxf = pxh->px_fields;
					for(i=0; i<pxh->px_numfields; i++) {
						if(fieldregex == NULL ||  selectedfields[i]) {
							if(first == 1)
								str_buffer_print(pxdoc, sbuf, ",");
							switch(pxf->px_ftype) {
								case pxfAlpha: {
									char *value;
									if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
										if(strchr(value, '\'')) {
											str_buffer_print(pxdoc, sbuf, "'");
											str_buffer_printmask(pxdoc, sbuf, value, '\'', '\\');
											str_buffer_print(pxdoc, sbuf, "'");
										} else
											str_buffer_print(pxdoc, sbuf, "'%s'", value);
										pxdoc->free(pxdoc, value);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;

									break;
								}
								case pxfDate: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										str_buffer_print(pxdoc, sbuf, "%ld", value);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfShort: {
									short int value;
									if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
										str_buffer_print(pxdoc, sbuf, "%d", value);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfAutoInc:
								case pxfTimestamp:
								case pxfLong: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										str_buffer_print(pxdoc, sbuf, "%ld", value);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfTime: {
									long value;
									if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
										str_buffer_print(pxdoc, sbuf, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfCurrency:
								case pxfNumber: {
									double value;
									if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
										str_buffer_print(pxdoc, sbuf, "%g", value);
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfLogical: {
									char value;
									if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
										if(value)
											str_buffer_print(pxdoc, sbuf, "1");
										else
											str_buffer_print(pxdoc, sbuf, "0");
									} else {
										str_buffer_print(pxdoc, sbuf, "NULL");
									}
									first = 1;
									break;
								}
								case pxfMemoBLOb:
								case pxfBLOb:
								case pxfFmtMemoBLOb:
								case pxfGraphic:
								case pxfOLE:
									if(includeblobs) {
										if(pxblob) {
											char *blobdata;
											char filename[200];
											FILE *fp;
											int mod_nr, size;
											blobdata = PX_read_blobdata(pxblob, &data[offset], pxf->px_flen, &mod_nr, &size);
											str_buffer_print(pxdoc, sbuf, "'");
											if(size) {
												if(blobdata) {
													if(pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
														int i;
														for(i=0; i<size; i++) {
															if(blobdata[i] == '\'')

																str_buffer_print(pxdoc, sbuf, "'");
															str_buffer_print(pxdoc, sbuf, "%c", blobdata[i]);
														}
													} else {
														sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
														fp = fopen(filename, "w");
														if(fp) {
															fwrite(blobdata, size, 1, fp);
															fclose(fp);
															str_buffer_print(pxdoc, sbuf, "%s", filename);
														} else {
															fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
														}
													}
												} else {
													fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
												}
											}
											str_buffer_print(pxdoc, sbuf, "'");
											if(blobdata)
												pxblob->pxdoc->free(pxblob->pxdoc, blobdata);

										} else {
											hex_dump(outfp, &data[offset], pxf->px_flen);
										}
										first = 1;
									} else {
										first = 0;
									}
									break;
								case pxfBCD:
									str_buffer_print(pxdoc, sbuf, "NULL");
									break;
								default:
									str_buffer_print(pxdoc, sbuf, "NULL");
							}
						}
						offset += pxf->px_flen;
						pxf++;
					}
					str_buffer_print(pxdoc, sbuf, ");\n");
				} else {
					fprintf(stderr, _("Couldn't get record number %d\n"), j);
				}
printf("%s", str_buffer_get(pxdoc, sbuf));

				if(SQLITE_OK != sqlite_exec(sql, str_buffer_get(pxdoc, sbuf), NULL, NULL, &sqlerror)) {
					sqlite_close(sql);
					fprintf(stderr, "%s\n", sqlerror);
					str_buffer_delete(pxdoc, sbuf);
					pxdoc->free(pxdoc, data);
					if(selectedfields)
						pxdoc->free(pxdoc, selectedfields);
					PX_close(pxdoc);
					exit(1);
				}
			}
		}
		str_buffer_delete(pxdoc, sbuf);
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
		fprintf(outfp, " <caption>%s</caption>\n", tablename);
		fprintf(outfp, " <tr>\n");

		/* output field name */
		pxf = pxh->px_fields;
		for(i=0; i<pxh->px_numfields; i++) {
			if(fieldregex == NULL ||  selectedfields[i]) {
				fprintf(outfp, "  <th>");
				if(strlen(pxf->px_fname))
					fprintf(outfp, "%s", pxf->px_fname);
				else
					fprintf(outfp, "column%d", i+1);
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
				fprintf(outfp, "</th>\n");
			}
			pxf++;
		}
		if(markdeleted) {
			fprintf(outfp, "  <th>deleted</th>\n", isdeleted);
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
									fprintf(outfp, "%g", value);
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
							case pxfFmtMemoBLOb:
							case pxfMemoBLOb:
							case pxfOLE:
								if(pxblob) {
									char *blobdata;
									char filename[200];
									FILE *fp;
									int mod_nr, size;
									blobdata = PX_read_blobdata(pxblob, &data[offset], pxf->px_flen, &mod_nr, &size);
									if(size) {
										if(blobdata) {
											if(pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
												int i;
												for(i=0; i<size; i++) {
													fputc(blobdata[i], outfp);
												}
											} else {
												sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
												fp = fopen(filename, "w");
												if(fp) {
													fwrite(blobdata, size, 1, fp);
													fclose(fp);
													fprintf(outfp, "%s", filename);
												} else {
													fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
												}
											}
										} else {
											fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
										}
									}
									if(blobdata)
										pxblob->pxdoc->free(pxblob->pxdoc, blobdata);

								} else {
									hex_dump(outfp, &data[offset], pxf->px_flen);
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
		if(!skipschema) {
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
							fprintf(outfp, "%s", get_sql_type(typemap, pxf->px_ftype, pxf->px_flen));
							first = 1;
							break;
						case pxfMemoBLOb:
						case pxfBLOb:
						case pxfFmtMemoBLOb:
						case pxfGraphic:
						case pxfOLE:
							if(includeblobs) {
								fprintf(outfp, "  %s ", pxf->px_fname);
								fprintf(outfp, "%s", get_sql_type(typemap, pxf->px_ftype, pxf->px_flen));
								first = 1;
							} else {
								first = 0;
							}
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
		}

		/* Only output data if we have at least one record */
		if(pxh->px_numrecords > 0) {
			if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
				if(selectedfields)
					pxdoc->free(pxdoc, selectedfields);
				PX_close(pxdoc);
				exit(1);
			}

			if(usecopy) {
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
							case pxfOLE:
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
											if(strchr(value, '\t'))
												printmask(outfp, value, '\t', '\\');
											else
												fprintf(outfp, "%s", value);
											pxdoc->free(pxdoc, value);
										} else {
											if(emptystringisnull)
												fprintf(outfp, "\\N");
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
											fprintf(outfp, "%02d:%02d:%02.3f", value/3600000, value/60000%60, value%60000/1000.0);
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
											fprintf(outfp, "%g", value);
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
									case pxfBLOb:
									case pxfGraphic:
									case pxfOLE:
									case pxfMemoBLOb:
									case pxfFmtMemoBLOb:
										if(includeblobs) {
											if(pxblob) {
												char *blobdata;
												char filename[200];
												FILE *fp;
												int mod_nr, size;
												blobdata = PX_read_blobdata(pxblob, &data[offset], pxf->px_flen, &mod_nr, &size);
												if(size) {
													if(blobdata) {
														if(pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
															int i;
															for(i=0; i<size; i++) {
																fputc(blobdata[i], outfp);
															}
														} else {
															sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
															fp = fopen(filename, "w");
															if(fp) {
																fwrite(blobdata, size, 1, fp);
																fclose(fp);
																fprintf(outfp, "%s", filename);
															} else {
																fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
															}
														}
													} else {
														fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
													}
												}
												if(blobdata)
													pxblob->pxdoc->free(pxblob->pxdoc, blobdata);

											} else {
												hex_dump(outfp, &data[offset], pxf->px_flen);
											}
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
			} else {
				struct str_buffer *sbuf;
				if(!shortinsert) {
					if((sbuf = str_buffer_new(pxdoc, 20)) == NULL) {
						if(selectedfields)
							pxdoc->free(pxdoc, selectedfields);
						PX_close(pxdoc);
						exit(1);
					}
					str_buffer_print(pxdoc, sbuf, "(");
					first = 0;  // set to 1 when first field has been output
					pxf = pxh->px_fields;
					/* output field name */
					for(i=0; i<pxh->px_numfields; i++) {
						if(fieldregex == NULL ||  selectedfields[i]) {
							if(first == 1)
								str_buffer_print(pxdoc, sbuf, ", ");
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
									str_buffer_print(pxdoc, sbuf, "%s", pxf->px_fname);
									first = 1;
									break;
								case pxfMemoBLOb:
								case pxfFmtMemoBLOb:
								case pxfBLOb:
								case pxfGraphic:
								case pxfOLE:
									if(includeblobs) {
										str_buffer_print(pxdoc, sbuf, "%s", pxf->px_fname);
										first = 1;
									} else {
										first = 0;
									}
									break;
							}
						}
						pxf++;
					}
					str_buffer_print(pxdoc, sbuf, ")");
				}
				for(j=0; j<pxh->px_numrecords; j++) {
					int offset;
					if(PX_get_record(pxdoc, j, data)) {
						first = 0;  // set to 1 when first field has been output
						offset = 0;
						if(shortinsert)
							fprintf(outfp, "insert into %s values (", tablename);
						else
							fprintf(outfp, "insert into %s %s values (", tablename, str_buffer_get(pxdoc, sbuf));
						pxf = pxh->px_fields;
						for(i=0; i<pxh->px_numfields; i++) {
							if(fieldregex == NULL ||  selectedfields[i]) {
								if(first == 1)
									fprintf(outfp, ", ");
								switch(pxf->px_ftype) {
									case pxfAlpha: {
										char *value;
										if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
											if(strchr(value, '\'')) {
												fprintf(outfp, "'");
												printmask(outfp, value, '\'', '\\');
												fprintf(outfp, "'");
											} else
												fprintf(outfp, "'%s'", value);
											pxdoc->free(pxdoc, value);
										} else {
											if(emptystringisnull)
												fprintf(outfp, "NULL", value);
											else
												fprintf(outfp, "''", value);
										}
										first = 1;

										break;
									}
									case pxfDate: {
										long value;
										int year, month, day;
										if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
											PX_SdnToGregorian(value+1721425, &year, &month, &day);
											fprintf(outfp, "'%02d.%02d.%04d'", day, month, year);
										} else {
											fprintf(outfp, "NULL");
										}
										first = 1;
										break;
									}
									case pxfShort: {
										short int value;
										if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
											fprintf(outfp, "%d", value);
										} else {
											fprintf(outfp, "NULL");
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
											fprintf(outfp, "NULL");
										}
										first = 1;
										break;
									}
									case pxfTime: {
										long value;
										if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
											fprintf(outfp, "'%02d:%02d:%02.3f'", value/3600000, value/60000%60, value%60000/1000.0);
										} else {
											fprintf(outfp, "NULL");
										}
										first = 1;
										break;
									}
									case pxfCurrency:
									case pxfNumber: {
										double value;
										if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
											fprintf(outfp, "%g", value);
										} else {
											fprintf(outfp, "NULL");
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
											fprintf(outfp, "NULL");
										}
										first = 1;
										break;
									}
									case pxfBLOb:
									case pxfGraphic:
									case pxfOLE:
									case pxfMemoBLOb:
									case pxfFmtMemoBLOb:
										if(includeblobs) {
											if(pxblob) {
												char *blobdata;
												char filename[200];
												FILE *fp;
												int mod_nr, size;
												blobdata = PX_read_blobdata(pxblob, &data[offset], pxf->px_flen, &mod_nr, &size);
												fputc('\'', outfp);
												if(size) {
													if(blobdata) {
														if(pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
															int i;
															for(i=0; i<size; i++) {
																fputc(blobdata[i], outfp);
															}
														} else {
															sprintf(filename, "%s_%d.blob", blobprefix, mod_nr);
															fp = fopen(filename, "w");
															if(fp) {
																fwrite(blobdata, size, 1, fp);
																fclose(fp);
																fprintf(outfp, "%s", filename);
															} else {
																fprintf(stderr, "Couldn't open file '%s' for blob data\n", filename);
															}
														}
													} else {
														fprintf(stderr, "Couldn't get blob data for %d\n", mod_nr);
													}
												}
												if(blobdata)
													pxblob->pxdoc->free(pxblob->pxdoc, blobdata);
												fputc('\'', outfp);

											} else {
												hex_dump(outfp, &data[offset], pxf->px_flen);
											}
											first = 1;
										} else {
											first = 0;
										}
										break;
									case pxfBCD:
									case pxfBytes:
										fprintf(outfp, "NULL");
										first = 1;
										break;
									default:
										fprintf(outfp, "");
								}
							}
							offset += pxf->px_flen;
							pxf++;
						}
						fprintf(outfp, ");\n");
					} else {
						fprintf(stderr, _("Couldn't get record number %d\n"), j);
					}
				}
				if(!shortinsert)
					str_buffer_delete(pxdoc, sbuf);
			}
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

	/* FIXME: not to free typemap->sqltype */
	free(typemap);

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
