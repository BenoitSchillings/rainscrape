
#include <stdio.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>

//---------------------------------------------------------------------------------


#include "./tiny/tinyxml2.cpp"

struct MemoryStruct {
    char* memory;
    size_t size;
    float  lat;
    float  lon;
};

//-----------------------------------------------------------------------------------

static size_t WriteMemoryCallback(void* contents,
                                  size_t size,
                                  size_t nmemb,
                                  void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;
    
    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        exit(-1);
    }
    
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}


void process(MemoryStruct *mem)
{
    printf("process %f %f (%d)\n", mem->lat, mem->lon, mem->size);
    if (mem->size > 0) { 
    
	tinyxml2::XMLDocument doc;
    	doc.Parse(mem->memory);
   	if (doc.ErrorID() == 0) {	
   		tinyxml2::XMLPrinter printer;
		doc.Print( &printer );
		exit(-1);	
	} 
    }
}

//---------------------------------------------------------------------------------


struct thread_info {/* Used as argument to thread_start() */
    pthread_t th;
    float lon;
};


//---------------------------------------------------------------------------------

static void* fetcher(void* arg) {
    CURL* curl;
    FILE* fp;
    CURLcode res;
    char fn[256];
    char url[256];
    float start_y;
    
    struct thread_info* tinfo = (thread_info*)arg;
    
    curl = curl_easy_init();
    
    if (curl) {
        float x, y;
        for (y = 34; y < (48); y += 0.02) {
            x = tinfo->lon;
            {
                struct MemoryStruct chunk;
                
                chunk.memory = (char*)malloc(1); /* will be grown as needed by the realloc above */
                chunk.size = 0;       /* no data at this point */
                chunk.lat = y;
		chunk.lon = x; 
                
		sprintf(url,
                        "http://forecast.weather.gov/"
                        "MapClick.php?lat=%2.2f&lon=%2.2f&FcstType=digitalDWML",
                        y, x);
                
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.47.1");
                curl_easy_setopt(curl, CURLOPT_URL, url);
                
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
                
                
                res = curl_easy_perform(curl);
           	
		process(&chunk); 
	    }
        }
    }
    curl_easy_cleanup(curl);
    return 0;
}

//---------------------------------------------------------------------------------

thread_info info[256];

//---------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    for (int i = 0; i < 100; i++) {
        info[i].lon = -120 + (0.02 * i);
        int ret = pthread_create(&info[i].th, NULL, &fetcher, &info[i]);
    }
    while (1)
        sleep(1000);
    return 0;
}

