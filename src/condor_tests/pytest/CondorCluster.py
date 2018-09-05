import classad
import htcondor
import os
import time

from PersonalCondor import PersonalCondor
from Globals import *
from Utils import Utils

from htcondor import JobEventLog
from htcondor import JobEventType

class CondorCluster(object):

	# For internal use only.  Use CondorScheduler.Submit() instead.
    def __init__(self, cluster_id, log, count, jel):
        self._cluster_id = cluster_id
        self._log = log
        self._count = count
        self._jel = jel
        self._callbacks = { }

    def ClusterID(self):
        return self._cluster_id

    #
    # The timeout for these functions is in seconds, and applies to the
    # whole process, not any individual read.
    #

    def WaitUntilJobTerminated( self, timeout = 240, proc = 0, count = 0 ):
        return self.WaitUntil( [ JobEventType.JOB_TERMINATED ],
            [ JobEventType.EXECUTE, JobEventType.SUBMIT,
              JobEventType.IMAGE_SIZE ], timeout, proc, count )

    def WaitUntilExecute( self, timeout = 240, proc = 0, count = 0 ):
        return self.WaitUntil( [ JobEventType.EXECUTE ],
            [ JobEventType.SUBMIT ], timeout, proc, count )

    def WaitUntilJobHeld( self, timeout = 240, proc = 0, count = 0 ):
        return self.WaitUntil( [ JobEventType.JOB_HELD ],
            [ JobEventType.EXECUTE, JobEventType.SUBMIT,
              JobEventType.IMAGE_SIZE, JobEventType.SHADOW_EXCEPTION ],
            timeout, proc, count )

        # FIXME: "look ahead" five seconds to see if we find the
        # hold event, o/w fail.  TODO: file a bug to prevent the
        # shadow exception event from entering the user log when
        # a job goes on hold.

    # WaitUntilAll*() won't work with 'advanced queue statements' because we
    # don't know how many procs to expect.  That's OK for now, since this
    # class doesn't support AQSs... yet.
    def WaitUntilAllJobsTerminated( self, timeout = 240 ):
        return self.WaitUntil( [ JobEventType.JOB_TERMINATED ],
            [ JobEventType.EXECUTE, JobEventType.SUBMIT,
              JobEventType.IMAGE_SIZE ], timeout, -1, self._count )

    def WaitUntilAllExecute( self, timeout = 240 ):
        return self.WaitUntil( [ JobEventType.EXECUTE ],
            [ JobEventType.SUBMIT ], timeout, -1, self._count )

    def WaitUntilAllJobsHeld( self, timeout = 240 ):
        return self.WaitUntil( [ JobEventType.JOB_HELD ],
            [ JobEventType.EXECUTE, JobEventType.SUBMIT,
              JobEventType.IMAGE_SIZE, JobEventType.SHADOW_EXCEPTION ],
            timeout, -1, self._count )


    # An event type not listed in successEvents or ignoreeEvents is a failure.
    def WaitUntil( self, successEvents, ignoreEvents, timeout = 240, proc = 0, count = 0 ):
        deadline = time.time() + timeout;
        Utils.TLog( "[cluster " + str(self._cluster_id) + "] Waiting for " + ",".join( [str(x) for x in successEvents] ) )

        successes = 0
        for event in self._jel.follow( int(timeout * 1000) ):
            if event.cluster == self._cluster_id and (count > 0 or event.proc == proc):
                if( not self._callbacks.get( event.type ) is None ):
                    self._callbacks[ event.type ]()

                if( event.type == JobEventType.NONE ):
                    Utils.TLog( "[cluster " + str(self._cluster_id) + "] Found relevant event of type " + str(event.type) + " (ignore)" )
                    pass
                elif( event.type in successEvents ):
                    Utils.TLog( "[cluster " + str(self._cluster_id) + "] Found relevant event of type " + str(event.type) + " (succeed)" )
                    successes += 1
                    if count <= 0 or successes == count:
                        return True
                elif( event.type in ignoreEvents ):
                    Utils.TLog( "[cluster " + str(self._cluster_id) + "] Found relevant event of type " + str(event.type) + " (ignore)" )
                    pass
                else:
                    Utils.TLog( "[cluster " + str(self._cluster_id) + "] Found relevant event of disallowed type " + str(event.type) + " (fail)" )
                    return False

            difference = deadline - time.time();
            if difference <= 0:
                Utils.TLog( "[cluster " + str(self._cluster_id) + "] Timed out waiting for " + ",".join( [str(x) for x in successEvents] ) )
                return False
            else:
                self._jel.setFollowTimeout( int(difference * 1000) )
                continue

    #
    # The callback-based interface.  The first set respond to events.  We'll
    # add semantic callbacks if it turns out those are useful to anyone.
    #

    def RegisterSubmit( self, submit_callback_fn ):
        self._callbacks[ JobEventType.SUBMIT ] = submit_callback_fn

    def RegisterJobTerminated( self, job_terminated_callback_fn ):
        self._callbacks[ JobEventType.JOB_TERMINATED ] = job_terminated_callback_fn

    def RegisterJobHeld( self, job_held_callback_fn ):
        self._callbacks[ JobEventType.JOB_HELD ] = job_held_callback_fn
