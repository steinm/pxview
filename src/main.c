#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libintl.h>
#include <paradox.h>
#include "config.h"
#define _(String) gettext(String)

void usage(char *progname) {
	printf(_("%s reads a paradox file and outputs information about the file\nor dumps the content in CSV format.\n\n"), progname);
	printf(_("Usage: %s [OPTIONS] [PARADOX FILE]"), progname);
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
	printf(_("  -o, --output=FILE   output dump into file instead of stdout."));
	printf("\n");
	printf(_("  -f, --file=FILE     read input data from file."));
	printf("\n");
	printf(_("  -b, --blobfile=FILE read blob data from file."));
	printf("\n");
	printf(_("  -p, --blobprefix=FILE prefix for all created files with blob data."));
	printf("\n");
}

int main(int argc, char *argv[]) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	pxdoc_t *pxdoc;
	pxblob_t *pxblob;
	char *data, buffer[1000];
	int i, j, c;
	int outputcsv = 0;
	int outputinfo = 1;
	int outputsql = 0;
	char delimiter = ';';
	char enclosure = '"';
	char *inputfile = NULL;
	char *blobfile = NULL;
	char *blobprefix = NULL;

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
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
			{"file", 1, 0, 'f'},
			{"blobfile", 1, 0, 'b'},
			{"blobprefix", 1, 0, 'p'},
			{"output", 1, 0, 'o'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "icsf:b:p:o:h",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'f':
				inputfile = strdup(optarg);
				break;
			case 'b':
				blobfile = strdup(optarg);
				break;
			case 'p':
				blobprefix = strdup(optarg);
				break;
			case 'o':
				break;
			case 'i':
				outputinfo = 1;
				break;
			case 'c':
				outputcsv = 1;
				outputinfo = 0;
				break;
			case 's':
				outputsql = 1;
				outputinfo = 0;
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
					printf("bool(%d)\n", pxf->px_flen);
					break;
				case pxfMemoBLOb:
					printf("blob(%d)\n", pxf->px_flen);
					break;
				case pxfBLOb:
					printf("blob(%d)\n", pxf->px_flen);
					break;
				case pxfFmtMemoBLOb:
					printf("bool(%d)\n", pxf->px_flen);
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
					printf("timestamp(%d)\n", pxf->px_flen);
					break;
				case pxfBCD:
					printf("timestamp(%d)\n", pxf->px_flen);
					break;
				case pxfBytes:
					printf("timestamp(%d)\n", pxf->px_flen);
					break;
				default:
					printf("%c(%d)\n", pxf->px_ftype, pxf->px_flen);
			}
			pxf++;
		}
	}

	if(outputcsv) {
		if((data = (char *) px_malloc(pxdoc, pxh->px_recordsize, _("Could not allocate memory for record."))) == NULL) {
			exit(1);
		}

		for(j=0; j<pxh->px_numrecords; j++) {
			int offset;
			if(PX_get_record(pxdoc, j, data)) {
				pxf = pxh->px_fields;
				offset = 0;
				for(i=0; i<pxh->px_numfields; i++) {
					switch(pxf->px_ftype) {
						case pxfAlpha:
							memcpy(buffer, &data[offset], pxf->px_flen);
							buffer[pxf->px_flen] = '\0';
							printf("%c%s%c", enclosure, buffer, enclosure);
							break;
						case pxfDate:
							data[offset] ^= data[offset];
							printf("%d", *((int *)(&data[offset])));
							break;
						case pxfShort:
							data[offset] ^= data[offset];
							printf("%d", *((short int *)(&data[offset])));
							break;
						case pxfLong:
							data[offset] ^= data[offset];
							printf("%ld", *((long int *)(&data[offset])));
							break;
						case pxfNumber:
							printf("%f", *((double *)(&data[offset])));
							break;
						case pxfGraphic:
						case pxfBLOb:
//							printf("%ld ", *((long int *)(&data[offset])));
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
							break;
						default:
							printf("");
					}
					if(i < (pxh->px_numfields-1))
						printf("%c", delimiter);
					offset += pxf->px_flen;
					pxf++;
				}
				printf("\n");
			} else {
				fprintf(stderr, _("Couldn't get record\n"));
			}
		}
		px_free(data);
	}

	PX_close(pxdoc);
}
