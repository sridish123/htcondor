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


#include "condor_common.h"
#include "condor_config.h"
#include "read_multiple_logs.h"
#include "basename.h"
#include "tmp_dir.h"
#include "dagman_multi_dag.h"
#include "condor_getcwd.h"

// Just so we can link in the ReadMultipleUserLogs class.
MULTI_LOG_HASH_INSTANCE;

//-------------------------------------------------------------------------
int
FindLastRescueDagNum( const char *primaryDagFile, bool multiDags,
			int maxRescueDagNum )
{
	int lastRescue = 0;

	for ( int test = 1; test <= maxRescueDagNum; test++ ) {
		MyString testName = RescueDagName( primaryDagFile, multiDags,
					test );
		if ( access( testName.Value(), F_OK ) == 0 ) {
			if ( test > lastRescue + 1 ) {
					// This should probably be a fatal error if
					// DAGMAN_USE_STRICT is set, but I'm avoiding
					// that for now because the fact that this code
					// is used in both condor_dagman and condor_submit_dag
					// makes that harder to implement. wenger 2011-01-28
				dprintf( D_ALWAYS, "Warning: found rescue DAG "
							"number %d, but not rescue DAG number %d\n",
							test, test - 1);
			}
			lastRescue = test;
		}
	}
	
	if ( lastRescue >= maxRescueDagNum ) {
		dprintf( D_ALWAYS,
					"Warning: FindLastRescueDagNum() hit maximum "
					"rescue DAG number: %d\n", maxRescueDagNum );
	}

	return lastRescue;
}

//-------------------------------------------------------------------------
MyString
RescueDagName(const char *primaryDagFile, bool multiDags,
			int rescueDagNum)
{
	ASSERT( rescueDagNum >= 1 );

	MyString fileName(primaryDagFile);
	if ( multiDags ) {
		fileName += "_multi";
	}
	fileName += ".rescue";
	fileName.formatstr_cat( "%.3d", rescueDagNum );

	return fileName;
}

//-------------------------------------------------------------------------
void
RenameRescueDagsAfter(const char *primaryDagFile, bool multiDags,
			int rescueDagNum, int maxRescueDagNum)
{
		// Need to allow 0 here so condor_submit_dag -f can rename all
		// rescue DAGs.
	ASSERT( rescueDagNum >= 0 );

	dprintf( D_ALWAYS, "Renaming rescue DAGs newer than number %d\n",
				rescueDagNum );

	int firstToDelete = rescueDagNum + 1;
	int lastToDelete = FindLastRescueDagNum( primaryDagFile, multiDags,
				maxRescueDagNum );

	for ( int rescueNum = firstToDelete; rescueNum <= lastToDelete;
				rescueNum++ ) {
		MyString rescueDagName = RescueDagName( primaryDagFile, multiDags,
					rescueNum );
		dprintf( D_ALWAYS, "Renaming %s\n", rescueDagName.Value() );
		MyString newName = rescueDagName + ".old";
			// Unlink here to be safe on Windows.
		tolerant_unlink( newName.Value() );
		if ( rename( rescueDagName.Value(), newName.Value() ) != 0 ) {
			EXCEPT( "Fatal error: unable to rename old rescue file "
						"%s: error %d (%s)\n", rescueDagName.Value(),
						errno, strerror( errno ) );
		}
	}
}

//-------------------------------------------------------------------------
MyString
HaltFileName( const MyString &primaryDagFile )
{
	MyString haltFile = primaryDagFile + ".halt";

	return haltFile;
}

//-------------------------------------------------------------------------
void
tolerant_unlink( const char *pathname )
{
	if ( unlink( pathname ) != 0 ) {
		if ( errno == ENOENT ) {
			dprintf( D_SYSCALLS,
						"Warning: failure (%d (%s)) attempting to unlink file %s\n",
						errno, strerror( errno ), pathname );
		} else {
			dprintf( D_ALWAYS,
						"Error (%d (%s)) attempting to unlink file %s\n",
						errno, strerror( errno ), pathname );

		}
	}
}
