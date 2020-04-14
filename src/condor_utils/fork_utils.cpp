#include "condor_common.h"
#include "condor_debug.h"

#include <set>
#include "fork_utils.h"

#if defined(LINUX)
bool
close_all_fds_quickly( const std::set<int> & exceptions ) {
	DIR * dir = opendir( "/proc/self/fd" );
	if( dir == NULL ) { return false; }

	std::vector<int> fds;
	char * endptr = NULL;
	struct dirent * d = NULL;
	while( (d = readdir(dir)) != NULL ) {
		if( strcmp( d->d_name, "." ) == 0 || strcmp( d->d_name, ".." ) == 0 ) { continue; }
		int fd = (int)strtol( d->d_name, & endptr, 10 );
		ASSERT( * endptr == '\0' );

		if(exceptions.count(fd) == 0) {
			fds.push_back(fd);
		}
	}
	// We buffer the FDs to close primarily to avoid disturbing the
	// directory while we're iterating over it, but it also means that
	// we don't accidentally close dir's underlying FD early.  Since
	// we're ignoring close() failures anyway, don't bother to exclude
	// the FD we're about to close here from the loop below.
	(void)closedir(dir);

	for( auto i : fds ) {
		(void)close( i );
	}

	return true;
}
#endif

void
close_all_fds( const std::set<int> & exceptions ) {
#if defined(LINUX)
	if(close_all_fds_quickly( exceptions )) { return; }
#endif /* defined(LINUX) */

	int limit = getdtablesize();
	for (int jj=3; jj < limit; jj++) {
		if(exceptions.count(jj) == 0) {
			close(jj);
		}
	}
}
