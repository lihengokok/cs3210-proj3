#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fuse.h>

#include "date_utils.h"
#include "log.h"

struct tm zeroDate = {0};

int isZero(struct tm* date)
{
	if(date->tm_mday == zeroDate.tm_mday
	&& date->tm_mon == zeroDate.tm_mon
	&& date->tm_year == zeroDate.tm_year)
	return 1;
	return 0;
}

struct tm pathToTm(const char * path)
{
	struct tm result = { 0 }, zeroResult = { 0 };
	
	char year[5] = "";
	char month[12] = "";
	char day[3] = "";
	int yearInt, monthInt, dayInt;
	
	const char* nextIndex;
	const char* prevIndex;
	prevIndex = path + 1;
	
	
	/*First, attempt the year*/
	if((nextIndex = strchr(prevIndex, '/')) != NULL)
	{
		log_msg("\nIndiced Diff: %i\n", nextIndex - prevIndex);
		if(nextIndex - prevIndex == 4)
		{
			memcpy(year, prevIndex, 4);
			year[4] = '\0';
			prevIndex = nextIndex + 1;
			/*If a year was correctly received, check for a month*/
			if((nextIndex = strchr(prevIndex, '/')) != NULL)
			{
				if(nextIndex - prevIndex < 11)
				{
					memcpy(month, prevIndex, nextIndex - prevIndex);
					month[nextIndex - prevIndex] = '\0';
					prevIndex = nextIndex + 1;
					/*Now we check for a date*/
					if((nextIndex = strchr(prevIndex, '/')) != NULL)
					{
						if(nextIndex - prevIndex == 2)
						{
							//log_msg("\nDay, inside \n");
							memcpy(day, prevIndex, 2);
							day[2] = '\0';
						}
					}
				}
			}
		}
	}
	if(strlen(year) > 0)
	{
		log_msg("\nYear is good\n");
		if((yearInt = atoi(year)) != 0)
		{
			result.tm_year = yearInt - 1900;
			log_msg("\nMonth string: %s\n", month);
			if(strlen(month) > 0)
			{
				if((monthInt = monthParse(month)) != -1)
				{
					result.tm_mon = monthInt;
					if(strlen(day) > 0)
					{
						if((dayInt = atoi(day)) != 0)
						{
							result.tm_mday = dayInt;
							return result;
						} else return zeroResult;
					} else 
					{
						result.tm_mday = -1;
						return result;
					}
				} else return zeroResult;
			} else
			{
				result.tm_mon = -1;
				return result;
			}
		} else return zeroResult;
	} else 
	{
		result.tm_year = -1;
		return zeroResult;
	}
}

int monthParse(char* monthName)
{
	int i;
	const char * monthNames[12];
	monthNames[0] = "jan";
	monthNames[1] = "feb";
	monthNames[2] = "mar";
	monthNames[3] = "apr";
	monthNames[4] = "may";
	monthNames[5] = "jun";
	monthNames[6] = "jul";
	monthNames[7] = "aug";
	monthNames[8] = "sep";
	monthNames[9] = "oct";
	monthNames[10] = "nov";
	monthNames[11] = "dec";
	for(i = 0; i < 12; i++)
	{
		if(strcmp(monthNames[i], monthName) == 0)
			return i;
	}
	return -1;
}

struct tm pathToTmComplete(const char * path)
{
	char *pathFixed;
	struct tm date;
	// Test it out
	pathFixed = malloc(strlen(path) + 2);
	strcpy(pathFixed, path);
	pathFixed[strlen(path)] = '/';
	pathFixed[strlen(path) + 1] = '\0';
	date = pathToTm(pathFixed);
	log_msg("\nFixed Path: %s, Year: %i, Month: %i, Day: %i\n", 
			pathFixed, date.tm_year, date.tm_mon, date.tm_mday);
	free(pathFixed);
	return date;
}
