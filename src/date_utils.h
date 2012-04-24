#include <time.h>

int monthParse(char* monthName);
struct tm pathToTm(const char * path);
struct tm pathToTmComplete(const char * path);
int isZero(struct tm* date);
int daysInMonth(struct tm date);
