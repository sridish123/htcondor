import htcondor
import os

from Globals import *
from Utils import Utils
from CondorCluster import CondorCluster
from PersonalCondor import PersonalCondor

from htcondor import JobEventLog
from htcondor import JobEventType

#
# If this object stays this simple, we should probably just make it a
# static method on CondorCluster, instead.
#
class CondorSubmit(object):

    # For internal use only.  Use PersonalCondor.GetScheduler() instead.
    def __init__(self, schedd):
        self._schedd = schedd

	# @return The corresponding htcondor.Schedd object.
    def Schedd(self):
        return self._schedd

	# @return The specified job ClassAd.
    def GetJobAd(self, cluster, proc=0):
        try:
            return self._schedd.xquery( requirements = "ClusterID == {0} " +
                "&& ProcID == {1}".format( cluster, proc ) ).next()
        except StopIteration as si:
            return None

	# @return The corresponding CondorCluster object or None.
    def Submit(self, job_args, count=1):
        # It's easier to smash the case of the keys (since ClassAds and the
        # submit language don't care) than to do the case-insensitive compare.
        self.job_args = dict([(k.lower(), v) for k, v in self.job_args.items()])

        # Extract the event log filename, or insert one if none.
        log = self.job_args.setdefault( "log", "test-{0}.log".format( os.getpid() ) )
        log = os.path.abspath( log )

        # Submit the job defined by submit_args
        Utils.TLog("Submitting job with arguments: " + str(self.job_args))
        submit = htcondor.Submit(self.job_args)
        try:
            with self._schedd.transaction() as txn:
                cluster_id = submit.queue(txn, count)
        except Exception as e:
            print( "Job submission failed for an unknown error: " + str(e) )
            return None

        Utils.TLog("Job submitted succeeded with cluster ID " + str(cluster_id))

        jel = JobEventLog( log )
        if not jel.isInitialized():
            print( "Unable to initialize job event log " + log )
            return None

        return CondorCluster( cluster_id, log, count, jel )
