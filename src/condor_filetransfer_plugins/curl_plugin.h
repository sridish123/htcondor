#ifdef WIN32
#define CURL_STATICLIB // this has to match the way the curl library was built.
#endif

#include <curl/curl.h>

using namespace std;

struct transfer_request {
    MyString download_file_name;
};

int send_curl_request( char** argv, int diagnostic, CURL* handle, 
    FileTransferStats* stats );

int server_supports_resume( CURL* handle, char* url );

void init_stats( char* request_url );

static size_t header_callback( char* buffer, size_t size, size_t nitems );

static size_t ftp_write_callback( void* buffer, size_t size, size_t nmemb, 
    void* stream );