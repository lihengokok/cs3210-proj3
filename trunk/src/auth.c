#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "auth.h"
 
void response_handle(void *ptr, size_t size, size_t nmemb, void *stream){
    //printf("\nThe response was \n %s \n", (char *)ptr);
}

void makePOST(char *url, char *data){
	CURL *curl;
	CURLcode sentOK;
	char err_mess[50];

	curl = curl_easy_init();
	if(curl) {
		/*curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");*/

		/* Add the parameters */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		/* Give it the URL */
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/* Setup callback function to handle http response */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_handle);

		/* Setup error */
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,err_mess);

		#ifdef SKIP_PEER_VERIFICATION
		/*
		 * If you want to connect to a site who isn't using a certificate that is
		 * signed by one of the certs in the CA bundle you have, you can skip the
		 * verification of the server's certificate. This makes the connection
		 * A LOT LESS SECURE.
		 *
		 * If you have a CA cert for the server stored someplace else than in the
		 * default bundle, then the CURLOPT_CAPATH option might come handy for
		 * you.
		 */ 
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		#endif

		#ifdef SKIP_HOSTNAME_VERFICATION
		/*
		 * If the site you're connecting to uses a different host name that what
		 * they have mentioned in their server certificate's commonName (or
		 * subjectAltName) fields, libcurl will refuse to connect. You can skip
		 * this check, but this will make the connection less secure.
		 */ 
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		#endif

		
		sentOK = curl_easy_perform(curl);
		/*make
		if (sentOK!=0){
			//If error was "Failed writing body" see sendf.c file 
			 
			printf("Http request failed: response was %d \n Error message: %s\n", sentOK, err_mess);
		}
		*/
		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
}


void exifGpsDataToStr(char* data, char* myText, int neg)
{
	float a, b, c;
	
	a = (float)((int)(*(data + 3)));
	b = (float)((int)(*(data + 11)));
	c = (float)((int)(*(data + 19)));
	
	if(neg)
	{
		a *= -1.0;
		b *= -1.0;
		c *= -1.0;
	}
	
	b += (c / 60.0);
	a += (b / 60.0);
	
	sprintf(myText, "%.8g", a);
}

void convertGpsInPlace(char* latitude, char* longitude)
{
	//Format: 33 deg 49' 18.00" N
	float latA, latB, latC, longA, longB, longC;
	printf("Lat: %s\n", latitude);
	printf("Long: %s\n", longitude);
	//Break up the string
	latitude[2] = '\0';
	longitude[2] = '\0';
	latitude[9] = '\0';
	longitude[9] = '\0';
	latitude[16] = '\0';
	longitude[16] = '\0';
	latA = atof(latitude);
	longA = atof(longitude);
	latB = atof(latitude + 7);
	longB = atof(longitude + 7);
	latC = atof(latitude + 11);
	longC = atof(longitude + 11);
	if(latA != 0 && longA != 0)
	{
		//then these are valid
		if(latitude[18] == 'S')
		{
			latA = -latA;
			latB = -latB;
			latC = -latC;
		}
		if(longitude[18] == 'W')
		{
			longA = -longA;
			longB = -longB;
			longC = -longC;
		}
		latB += (latC / 60.0);
		latA += (latB / 60.0);
		
		longB += (longC / 60.0);
		longA += (longB / 60.0);
		
		sprintf(latitude, "%.8g", latA);
		sprintf(longitude, "%.8g", longA);
		
		printf("Lat: %s\n", latitude);
		printf("Long: %s\n", longitude);
	}
	
}

void makeEasyPOST(const char* latitude, const char* longitude, const char* name, const char* url)
{
	char* data;
	data = malloc(strlen(latitude) + strlen(longitude) + strlen(name) + 1 + 15);
	strcpy(data, "lat=");
	strcat(data, latitude);
	strcat(data, "&desc=");
	strcat(data, name);
	strcat(data, "&lng=");
	strcat(data, longitude);
	
	makePOST(url,data);
	
	free(data);
}

/*int main(void)
{
	
	char data[50] = {"lat=40.680638&desc=New York&lng=-73.959961"};
	char data[50] = {"lat=39.402244&desc=Maryland&lng=-76.530762"};
	char data[50] = {"lat=37.879478&desc=Virginia&lng=-77.387695"};
	
	char data[50] = {"lat=39.740986&desc=New Jersey&lng=-74.750977"};
	
	char url[100] = {"http://m.cip.gatech.edu/developer/imf/api/helper/coord/"};
	
	makePOST(url,data);
	
  return 0;
}*/
