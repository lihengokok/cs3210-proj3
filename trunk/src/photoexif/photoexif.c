
#define _GNU_SOURCE 500/* for strptime()*/
#include <stdio.h>
#include <string.h>
#include <libexif/exif-data.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    ExifData *ed;
    ExifEntry *entry;
    struct tm *photo_time;
    char *isGood;
	char env[20] = "  \%T";
	putenv(env);

	photo_time = malloc(sizeof(struct tm));

    if (argc < 2) {
        printf("Usage: %s image.jpg\n", argv[0]);
        return 1;
    }

    /* Load an ExifData object from an EXIF file */
    ed = exif_data_new_from_file(argv[1]);
    if (!ed) {
        printf("File not readable or no EXIF data in file %s\n", argv[1]);
        return 2;
    }

    /* Get the exif entry */
    entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_DATE_TIME);
   
    /*
    isGood = getdate_r(entry->data,photo_time);
    if (isGood!=0)
    {
		printf("getdate_r() failed with error: %d \n", isGood);
	}
	*/
	
	isGood = strptime(entry->data, "%Y:%m:%d %H:%M:%S", photo_time);
	if (isGood == NULL){
		printf("Time/date from picture is outside expected format of '2000:09:02 14:30:10' \n The date was %s\n", entry->data);
	} 
	printf("Date is %s\n", entry->data);
	
    
	printf("seconds after the minute (0 to 61): %d \n",photo_time->tm_sec);
	printf("minutes after the hour (0 to 59): %d \n",photo_time->tm_min);
	printf("hours since midnight (0 to 23): %d \n",photo_time->tm_hour);
	printf("day of the month (1 to 31): %d \n",photo_time->tm_mday);
	printf("months since January (0 to 11): %d \n",photo_time->tm_mon);
	printf("years since 1900: %d \n",photo_time->tm_year);
	printf("days since Sunday (0 to 6 Sunday=0): %d \n",photo_time->tm_wday);
	printf("days since January 1 (0 to 365): %d \n",photo_time->tm_yday);
	printf("Daylight Savings Time: %d \n",photo_time->tm_isdst);

	
	
    /* Free the EXIF data */
    exif_data_unref(ed);
	free(photo_time);
    return 0;
}
