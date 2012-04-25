#include <stdio.h>
#include <curl/curl.h>
 
response_handle(void *ptr, size_t size, size_t nmemb, void *stream){
    printf("\nThe response was \n %s \n", (char *)ptr);
}

makePOST(char *url, char *data){
CURL *curl;
CURLcode sentOK;
char err_mess[50];

curl = curl_easy_init();
if(curl) {
	/*curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");*/

	/* Add the parameters */
	/*curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);*/
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

int main(void)
{
	/*
	char data[30] = {"user=dh&pw=11111"};
	char url[100] = {"http://m.cip.gatech.edu/developer/imf/api/RWMSERVER/login"};
	*/
	char data[30] = {""};
	
	char url[300] = {"http://www.panoramio.com/map/get_panoramas.php?set=6871797&from=0&to=20&minx=-180&miny=-90&maxx=180&maxy=90&size=medium&mapfilter=true"};
	
	makePOST(url,data);
	
  return 0;
}
