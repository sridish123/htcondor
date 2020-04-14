#ifndef _CONDOR_FORK_UTILS_H
#define _CONDOR_FORK_UTILS_H

//
// #include <set>
// #include "fork_utils.h"
//

// This only works for single-threaded programs.
void close_all_fds( const std::set<int> & exceptions );

#endif /* _CONDOR_FORK_UTILS_H */
