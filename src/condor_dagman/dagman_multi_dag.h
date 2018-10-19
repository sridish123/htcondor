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


// Functions for multi-DAG support.

// Note that this is used by both condor_submit_dag and condor_dagman
// itself.

#ifndef DAGMAN_MULTI_DAG_H
#define DAGMAN_MULTI_DAG_H

	// The default maximum rescue DAG number.
const int MAX_RESCUE_DAG_DEFAULT = 100;

	// The absolute maximum allowed rescue DAG number (the real maximum
	// is normally configured lower).
const int ABS_MAX_RESCUE_DAG_NUM = 999;

/** Finds the number of the last existing rescue DAG file for the
	given "primary" DAG.
	@param primaryDagFile The primary DAG file name
	@param multiDags Whether we have multiple DAGs
	@param maxRescueDagNum the maximum legal rescue DAG number
	@return The number of the last existing rescue DAG (0 if there
		is none)
*/
int FindLastRescueDagNum(const char *primaryDagFile,
			bool multiDags, int maxRescueDagNum);

/** Creates a rescue DAG name, given a primary DAG name and rescue
	DAG number
	@param primaryDagFile The primary DAG file name
	@param multiDags Whether we have multiple DAGs
	@param rescueDagNum The rescue DAG number
	@return The full name of the rescue DAG
*/
MyString RescueDagName(const char *primaryDagFile,
			bool multiDags, int rescueDagNum);

/** Renames all rescue DAG files for this primary DAG after the
	given one (as long as the numbers are contiguous).  For example,
	if rescueDagNum is 3, we will rename .rescue4, .rescue5, etc.
	@param primaryDagFile The primary DAG file name
	@param multiDags Whether we have multiple DAGs
	@param rescueDagNum The rescue DAG number to rename *after*
	@param maxRescueDagNum the maximum legal rescue DAG number
*/
void RenameRescueDagsAfter(const char *primaryDagFile,
			bool multiDags, int rescueDagNum, int maxRescueDagNum);

/** Generates the halt file name based on the primary DAG name.
	@return The halt file name.
*/
MyString HaltFileName( const MyString &primaryDagFile );

/** Attempts to unlink the given file, and prints an appropriate error
	message if this fails (but doesn't return an error, so only call
	this if a failure of the unlink is okay).
	@param pathname The path of the file to unlink
*/
void tolerant_unlink( const char *pathname );

#endif /* #ifndef DAGMAN_MULTI_DAG_H */
