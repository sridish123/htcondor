/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#ifndef _FILE_TRANSFER_H
#define _FILE_TRANSFER_H

#include "condor_common.h"
#include "condor_daemon_core.h"
#include "MyString.h"
#include "HashTable.h"
#ifdef WIN32
#include "perm.h"
#endif
#include "condor_uid.h"
#include "condor_ver_info.h"
#include "condor_classad.h"
#include "dc_transfer_queue.h"
#include <vector>

extern const char * const StdoutRemapName;
extern const char * const StderrRemapName;

class FileTransfer;	// forward declatation
class FileTransferItem;
typedef std::vector<FileTransferItem> FileTransferList;


struct CatalogEntry {
	time_t		modification_time;
	filesize_t	filesize;

	// if support for hashes is added, it requires memory management of the
	// data pointed to.  (hash table iterator copies Values, so you'll need a
	// copy operator that does a deep copy, or use a large enough static array)
	//
	//void		*hash;
	//void		hash[SIZE_OF_HASH];
	//
	//
	// likewise if you add signatures and all that PKI jazz.
	//
	// ZKM
};


typedef HashTable <MyString, FileTransfer *> TranskeyHashTable;
typedef HashTable <int, FileTransfer *> TransThreadHashTable;
typedef HashTable <MyString, CatalogEntry *> FileCatalogHashTable;
typedef HashTable <MyString, MyString> PluginHashTable;

typedef int		(Service::*FileTransferHandlerCpp)(FileTransfer *);
typedef int		(*FileTransferHandler)(FileTransfer *);

enum FileTransferStatus {
	XFER_STATUS_UNKNOWN,
	XFER_STATUS_QUEUED,
	XFER_STATUS_ACTIVE,
	XFER_STATUS_DONE
};


namespace htcondor {
class DataReuseDirectory;
}


class FileTransfer final: public Service {

  public:
    // Used by the Condor-C GAHP, the job router, and the submit utils to
    // avoid certain problems when dealing with spooling files.
	static bool ExpandInputFileList( ClassAd *job, MyString &error_msg );
	static bool ExpandInputFileList( char const *input_list, char const *iwd, MyString &expanded_list, MyString &error_msg );

    // Used by the shadow.
	static bool IsDataflowJob(ClassAd *job_ad);



	FileTransfer();

	~FileTransfer();

    // ...
	int Init( ClassAd *Ad, bool check_file_perms = false,
			  priv_state priv = PRIV_UNKNOWN,
			  bool use_file_catalog = true);

    // ...
	int SimpleInit(ClassAd *Ad, bool want_check_perms, bool is_server,
						 ReliSock *sock_to_use = NULL,
						 priv_state priv = PRIV_UNKNOWN,
						 bool use_file_catalog = true,
						 bool is_spool = false);

    // ...
	int DownloadFiles(bool blocking=true);

	// ...
	int UploadFiles(bool blocking=true, bool final_transfer=true);



    // Used by the starter to advertise its capabilities.
    MyString GetSupportedMethods();

    // Used by the starter.  Should probably be included in init().
	int InitializePlugins(CondorError &e);

    // Used by the starter to include (renamed) core files.  This also
    // seems way more general than it needs to be.  [FIXME?]
	bool addOutputFile( const char* filename );

    // Used by the starter for file transfer plug-ins.
	void setRuntimeAds(const std::string &job_ad, const std::string &machine_ad)
	{m_job_ad = job_ad; m_machine_ad = machine_ad;}

    // Used by the starter.
	void setCredsDir(const std::string &cred_dir) {m_cred_dir = cred_dir;}

    // Used by the starter.
	void setDataReuseDirectory(htcondor::DataReuseDirectory &reuse_dir) {m_reuse_dir = &reuse_dir;}

    // Used by the starter during reconnect.
	bool changeServer( const char* transkey, const char* transsock );

    // Used by the starter to tolerate long i/o delays.
	int	setClientSocketTimeout(int timeout);

    // Used by the starter to clean up the transfer output list.
	bool addFileToExceptionList( const char* filename );

    // Used by the starter when catching DC_SIGSUSPEND.
	int Suspend();

    // Used by the starter when catching DC_SIGCONTINUE.
	int Continue();

    // Used by the shadow and the starter.
	void RegisterCallback(FileTransferHandler handler, bool want_status_updates=false)
		{
			ClientCallback = handler;
			ClientCallbackWantsStatusUpdates = want_status_updates;
		}

    // Used by the shadow and the starter.
	void RegisterCallback(FileTransferHandlerCpp handler, Service* handlerclass, bool want_status_updates=false)
		{
			ClientCallbackCpp = handler;
			ClientCallbackClass = handlerclass;
			ClientCallbackWantsStatusUpdates = want_status_updates;
		}

    // Used by the shadow.
	void setMaxUploadBytes(filesize_t MaxUploadBytes);
	void setMaxDownloadBytes(filesize_t MaxUploadBytes);

    // Used by the shadow for reporting.
	int GetUploadTimestamps(time_t * pStart, time_t * pEnd = NULL) {
		if (uploadStartTime < 0)
			return false;
		if (pEnd) *pEnd = (time_t)uploadEndTime;
		if (pStart) *pStart = (time_t)uploadStartTime;
		return true;
	}

    // Used by the shadow for reporting.
	bool GetDownloadTimestamps(time_t * pStart, time_t * pEnd = NULL) {
		if (downloadStartTime < 0)
			return false;
		if (pEnd) *pEnd = (time_t)downloadEndTime;
		if (pStart) *pStart = (time_t)downloadStartTime;
		return true;
	}

    // Used by the shadow and the starter for reporting.
	float TotalBytesSent() { return bytesSent; }
	float TotalBytesReceived() { return bytesRcvd; };

    // Used by the shadow for unknown reasons.
	void setTransferQueueContactInfo(char const *contact);

    // Used by the shadow to see if it's safe to shut down.
	bool transferIsInProgress() { return ActiveTransferTid != -1; }

    // Used by the shadow when aborting file transfers.
	void stopServer();

    // Used by the grid manager for INFN batch jobs to remap stdout & stderr.
    // Should be replaced by commands specifically for that.  [FIXME?]
	void AddDownloadFilenameRemap(char const *source_name,char const *target_name);

    // Used by the shadow on Windows, because it doesn't have fork().
    // Used by the grid manager for INFN batch jobs for unknown reasons.
	static bool SetServerShouldBlock( bool block );

    // Used by the starter when dealing with shadow reconnects, and in
    // some (unknown) cases in the FT GAHP.
	void setSecuritySession(char const *session_id);

    // Used by dc_schedd, dc_transferd, and jic_shadow.  Activates the
    // output file remaps, since we don't always use them (e.g., for
    // intermediate file transfer).
	int InitDownloadFilenameRemaps(ClassAd *Ad);

    // Why isn't this always handled internally?
	void setPeerVersion( const char *peer_version );
	void setPeerVersion( const CondorVersionInfo &peer_version );

	enum TransferType { NoType, DownloadFilesType, UploadFilesType };
	struct FileTransferInfo {
		FileTransferInfo() : bytes(0), duration(0), type(NoType),
		    success(true), in_progress(false), xfer_status(XFER_STATUS_UNKNOWN),
			try_again(true), hold_code(0), hold_subcode(0) {}

		FileTransferInfo(const FileTransferInfo &rhs) {
			bytes = rhs.bytes;
			duration = rhs.duration;
			type = rhs.type;
			success = rhs.success;
			in_progress = rhs.in_progress;
			xfer_status = rhs.xfer_status;
			try_again = rhs.try_again;
			hold_code = rhs.hold_code;
			hold_subcode = rhs.hold_subcode;
			error_desc = rhs.error_desc;
			spooled_files = rhs.spooled_files;
			tcp_stats = rhs.tcp_stats;
		}
		void addSpooledFile(char const *name_in_spool);

		filesize_t bytes;
		time_t duration;
		TransferType type;
		bool success;
		bool in_progress;
		FileTransferStatus xfer_status;
		bool try_again;
		int hold_code;
		int hold_subcode;
		MyString error_desc;
			// List of files we created in remote spool.
			// This is intended to become SpooledOutputFiles.
		MyString spooled_files;
		MyString tcp_stats;
	};
	FileTransferInfo GetInfo() { return Info; }

  private:
	inline bool IsServer() {return user_supplied_key == FALSE;}

	inline bool IsClient() {return user_supplied_key == TRUE;}

	static int HandleCommands(Service *,int command,Stream *s);

	static int Reaper(Service *, int pid, int exit_status);

		// If this file transfer object has a transfer running in
		// the background, kill it.
	void abortActiveTransfer();

	void RemoveInputFiles(const char *sandbox_path = NULL);

	void setTransferFilePermissions( bool value )
		{ TransferFilePermissions = value; }

	priv_state getDesiredPrivState( void ) { return desired_priv_state; };

	void InsertPluginMappings(MyString methods, MyString p);
	void SetPluginMappings( CondorError &e, const char* path );
	int InitializeJobPlugins(const ClassAd &job, CondorError &e, StringList &infiles);
	MyString DetermineFileTransferPlugin( CondorError &error, const char* source, const char* dest );
	int InvokeFileTransferPlugin(CondorError &e, const char* URL, const char* dest, ClassAd* plugin_stats, const char* proxy_filename = NULL);
	int InvokeMultipleFileTransferPlugin(CondorError &e, const std::string &plugin_path, const std::string &transfer_files_string, const char* proxy_filename, bool do_upload, std::vector<std::unique_ptr<ClassAd>> *);
	ssize_t InvokeMultiUploadPlugin(const std::string &plugin_path, const std::string &transfer_files_string, ReliSock &sock, bool send_trailing_eom, CondorError &err);
    int OutputFileTransferStats( ClassAd &stats );

	// Add any number of download remaps, encoded in the form:
	// "source1 = target1; source2 = target2; ..."
	// or in other words, the format expected by the util function
	// filename_remap_find().
	void AddDownloadFilenameRemaps(char const *remaps);

	ClassAd *GetJobAd();

  protected:

	int Download(ReliSock *s, bool blocking);
	int Upload(ReliSock *s, bool blocking);
	static int DownloadThread(void *arg, Stream *s);
	static int UploadThread(void *arg, Stream *s);
	int TransferPipeHandler(int p);
	bool ReadTransferPipeMsg();
	void UpdateXferStatus(FileTransferStatus status);

		/** Actually download the files.
			@return -1 on failure, bytes transferred otherwise
		*/
	int DoDownload( filesize_t *total_bytes, ReliSock *s);
	int DoUpload( filesize_t *total_bytes, ReliSock *s);

	double uploadStartTime{-1}, uploadEndTime{-1};
	double downloadStartTime{-1}, downloadEndTime{-1};

	void CommitFiles();
	void ComputeFilesToSend();
#ifdef HAVE_HTTP_PUBLIC_FILES
	int AddInputFilenameRemaps(ClassAd *Ad);
#endif
	uint64_t bytesSent{0}, bytesRcvd{0};
	StringList* InputFiles{nullptr};

  private:

	bool TransferFilePermissions{false};
	bool DelegateX509Credentials{false};
	bool PeerDoesTransferAck{false};
	bool PeerDoesGoAhead{false};
	bool PeerUnderstandsMkdir{false};
	bool PeerDoesXferInfo{false};
	bool PeerDoesReuseInfo{false};
	bool PeerDoesS3Urls{false};
	bool TransferUserLog{false};
	char* Iwd{nullptr};
	StringList* ExceptionFiles{nullptr};
	StringList* OutputFiles{nullptr};
	StringList* EncryptInputFiles{nullptr};
	StringList* EncryptOutputFiles{nullptr};
	StringList* DontEncryptInputFiles{nullptr};
	StringList* DontEncryptOutputFiles{nullptr};
	StringList* IntermediateFiles{nullptr};
	StringList* FilesToSend{nullptr};
	StringList* EncryptFiles{nullptr};
	StringList* DontEncryptFiles{nullptr};
	char* OutputDestination{nullptr};
	char* SpooledIntermediateFiles{nullptr};
	char* ExecFile{nullptr};
	char* UserLogFile{nullptr};
	char* X509UserProxy{nullptr};
	MyString JobStdoutFile;
	MyString JobStderrFile;
	char* TransSock{nullptr};
	char* TransKey{nullptr};
	char* SpoolSpace{nullptr};
	char* TmpSpoolSpace{nullptr};
	int user_supplied_key{false};
	bool upload_changed_files{false};
	int m_final_transfer_flag{false};
	time_t last_download_time{0};
	FileCatalogHashTable* last_download_catalog{nullptr};
	int ActiveTransferTid{-1};
	time_t TransferStart{0};
	int TransferPipe[2] {-1, -1};
	bool registered_xfer_pipe{false};
	FileTransferHandler ClientCallback{nullptr};
	FileTransferHandlerCpp ClientCallbackCpp{nullptr};
	Service* ClientCallbackClass{nullptr};
	bool ClientCallbackWantsStatusUpdates{false};
	FileTransferInfo Info;
	PluginHashTable* plugin_table{nullptr};
	std::map<MyString, bool> plugins_multifile_support;
	std::map<std::string, bool> plugins_from_job;
	bool I_support_filetransfer_plugins{false};
	bool I_support_S3{false};
	bool multifile_plugins_enabled{false};
#ifdef WIN32
	perm* perm_obj{nullptr};
#endif		
	priv_state desired_priv_state{PRIV_UNKNOWN};
	bool want_priv_change{false};
	static TranskeyHashTable* TranskeyTable;
	static TransThreadHashTable* TransThreadTable;
	static int CommandsRegistered;
	static int SequenceNum;
	static int ReaperId;
	static bool ServerShouldBlock;
	int clientSockTimeout{30};
	bool did_init{false};
	bool simple_init{true};
	ReliSock *simple_sock{nullptr};
	MyString download_filename_remaps;
	bool m_use_file_catalog{true};
	TransferQueueContactInfo m_xfer_queue_contact_info;
	MyString m_jobid; // what job we are working on, for informational purposes
	char *m_sec_session_id{nullptr};
	std::string m_cred_dir;
	std::string m_job_ad;
	std::string m_machine_ad;
	filesize_t MaxUploadBytes{-1};  // no limit by default
	filesize_t MaxDownloadBytes{-1};

	// stores the path to the proxy after one is received
	MyString LocalProxyName;

	// Object to manage reuse of any data locally.
	htcondor::DataReuseDirectory *m_reuse_dir{nullptr};

	// called to construct the catalog of files in a direcotry
	bool BuildFileCatalog(time_t spool_time = 0, const char* iwd = NULL, FileCatalogHashTable **catalog = NULL);

	// called to lookup the catalog entry of file
	bool LookupInFileCatalog(const char *fname, time_t *mod_time, filesize_t *filesize);

	// Called internally by DoUpload() in order to handle common wrapup tasks.
	int ExitDoUpload(filesize_t *total_bytes, int numFiles, ReliSock *s, priv_state saved_priv, bool socket_default_crypto, bool upload_success, bool do_upload_ack, bool do_download_ack, bool try_again, int hold_code, int hold_subcode, char const *upload_error_desc,int DoUpload_exit_line);

	// Send acknowledgment of success/failure after downloading files.
	void SendTransferAck(Stream *s,bool success,bool try_again,int hold_code,int hold_subcode,char const *hold_reason);

	// Receive acknowledgment of success/failure after downloading files.
	void GetTransferAck(Stream *s,bool &success,bool &try_again,int &hold_code,int &hold_subcode,MyString &error_desc);

	// Stash transfer success/failure info that will be propagated back to
	// caller of file transfer operation, using GetInfo().
	void SaveTransferInfo(bool success,bool try_again,int hold_code,int hold_subcode,char const *hold_reason);

	// Receive message indicating that the peer is ready to receive the file
	// and save failure information with SaveTransferInfo().
	bool ReceiveTransferGoAhead(Stream *s,char const *fname,bool downloading,bool &go_ahead_always,filesize_t &peer_max_transfer_bytes);

	// Receive message indicating that the peer is ready to receive the file.
	bool DoReceiveTransferGoAhead(Stream *s,char const *fname,bool downloading,bool &go_ahead_always,filesize_t &peer_max_transfer_bytes,bool &try_again,int &hold_code,int &hold_subcode,MyString &error_desc, int alive_interval);

	// Obtain permission to receive a file download and then tell our
	// peer to go ahead and send it.
	// Save failure information with SaveTransferInfo().
	bool ObtainAndSendTransferGoAhead(DCTransferQueue &xfer_queue,bool downloading,Stream *s,filesize_t sandbox_size,char const *full_fname,bool &go_ahead_always);

	bool DoObtainAndSendTransferGoAhead(DCTransferQueue &xfer_queue,bool downloading,Stream *s,filesize_t sandbox_size,char const *full_fname,bool &go_ahead_always,bool &try_again,int &hold_code,int &hold_subcode,MyString &error_desc);

	std::string GetTransferQueueUser();

	// Report information about completed transfer from child thread.
	bool WriteStatusToTransferPipe(filesize_t total_bytes);
	ClassAd jobAd;

	bool ExpandFileTransferList( StringList *input_list, FileTransferList &expanded_list, bool preserveRelativePaths );

		// This function generates a list of files to transfer, including
		// directories to create and their full contents.
		// Arguments:
		// src_path  - the path of the file to be transferred
		// dest_dir  - the directory to write the file to
		// iwd       - relative paths are relative to this path
		// max_depth - how deep to recurse (-1 for infinite)
		// expanded_list - the list of files to transfer
	static bool ExpandFileTransferList( char const *src_path, char const *dest_dir, char const *iwd, int max_depth, FileTransferList &expanded_list, bool preserveRelativePaths );


		// Returns true if path is a legal path for our peer to tell us it
		// wants us to write to.  It must be a relative path, containing
		// no ".." elements.
	bool LegalPathInSandbox(char const *path,char const *sandbox);

		// Returns true if specified path points into the spool directory.
		// This does not do an existence check for the file.
	bool outputFileIsSpooled(char const *fname);

	void callClientCallback();
};

// returns 0 if no expiration
time_t GetDesiredDelegatedJobCredentialExpiration(ClassAd *job);

// returns 0 if renewal of lifetime is not needed
time_t GetDelegatedProxyRenewalTime(time_t expiration_time);

void GetDelegatedProxyRenewalTime(ClassAd *jobAd);

#endif

