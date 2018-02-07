#ifdef WIN32
#define CURL_STATICLIB // this has to match the way the curl library was built.
#endif

#include <curl/curl.h>

using namespace std;

struct transfer_request {
    MyString local_file_name;
};

static FileTransferStats* _global_ft_stats;

class CurlPlugin {
  
  public:
 
    CurlPlugin( int diagnostic );
    ~CurlPlugin();

    int DownloadFile( const char* url, const char* local_file_name );
    int DownloadMultipleFiles( string input_filename );
    int UploadFile( const char* url, const char* local_file_name );
    int ServerSupportsResume( const char* url );
    static size_t HeaderCallback( char* buffer, size_t size, size_t nitems );
    static size_t FtpWriteCallback( void* buffer, size_t size, size_t nmemb, void* stream );
    void InitializeStats( char* request_url );

    CURL* GetHandle() { return _handle; };
    string GetStats() { return _all_stats; }
    

  private:

    CURL* _handle;
    FileTransferStats* _file_transfer_stats;
    int _diagnostic;
    string _all_stats;

};