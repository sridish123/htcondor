/***************************************************************
 *
 * Copyright (C) 1990-2010, Condor Team, Computer Sciences Department,
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

#ifndef _STARTD_CRON_JOB_PARAMS_H
#define _STARTD_CRON_JOB_PARAMS_H

#include "classad_cron_job.h"
#include <list>

// Define a "ClassAd" cron job parameter object
class StartdCronJobParams : public ClassAdCronJobParams
{
  public:
  	typedef std::set< std::string > Metrics;

	StartdCronJobParams( const char			*job_name,
						 const CronJobMgr	&mgr );
	~StartdCronJobParams( void ) { };

	// Finish initialization
	bool Initialize( void );
	bool InSlotList( unsigned slot ) const;

	bool addMetric( const char * metricType, const char * attributeName );
	bool isSumMetric( const std::string & attributeName ) const { return sumMetrics.find( attributeName ) != sumMetrics.end(); }
	bool isPeakMetric( const std::string & attributeName ) const { return peakMetrics.find( attributeName ) != peakMetrics.end(); }
	bool isResourceMonitor( void ) const { return (sumMetrics.size() + peakMetrics.size()) > 0; }

  private:
  	Metrics sumMetrics;
  	Metrics peakMetrics;

	std::list<unsigned>	m_slots;
};

#endif /* _STARTD_CRON_JOB_PARAMS_H */
