#ifdef WIN32
#define CURL_STATICLIB // this has to match the way the curl library was built.
#endif

#include <curl/curl.h>

using namespace std;

struct transfer_request {
    MyString download_file_name;
};

static FileTransferStats* _global_ft_stats;

class CurlPlugin {
  
  public:
 
    CurlPlugin( int diagnostic );
    ~CurlPlugin();

    int DownloadFile( const char* url, const char* download_file_name );
    int DownloadMultipleFiles( string input_filename );
    int UploadFile( const char* url, const char* download_file_name );
    int server_supports_resume( const char* url );
    static size_t header_callback( char* buffer, size_t size, size_t nitems );
    static size_t ftp_write_callback( void* buffer, size_t size, size_t nmemb, void* stream );
    string GetStats() { return _all_stats; }
    void init_stats( char* request_url );
    void SetDiagnostic( int diagnostic ) { _diagnostic = diagnostic; };
    
  private:

    CURL* _handle = NULL;
    FileTransferStats* _file_transfer_stats;
    int _diagnostic = 0;
    string _all_stats = "";

};