import classad
import htcondor
import os
import time

from PersonalCondor import PersonalCondor
from Globals import *
from Utils import Utils


class CondorJob(object):

    def __init__(self, job_args):
        self._cluster_id = None
        self._job_args = job_args

        # If no log file was specified in job_args, we need to add it
        # Need a better way to generate unique log files, since the executable
        # name might be "/bin/echo" and that would blow up here
        if "log" in job_args:
            self._log = job_args["log"]
        else:
            self._log = str(job_args["executable"] + "." + str(os.getpid()) + ".log")
            self._job_args["log"] = self._log

    def FailureCallback(self):
        Utils.TLog("Job cluster " + str(self._cluster_id) + " failure callback invoked")

    def SubmitCallback(self):
        Utils.TLog("Job cluster " + str(self._cluster_id) + " submit callback invoked")

    def SuccessCallback(self):
        Utils.TLog("Job cluster " + str(self._cluster_id) + " success callback invoked")


    def Submit(self, wait=False):

        # Submit the job defined by submit_args
        Utils.TLog("Submitting job with arguments: " + str(self._job_args))
        schedd = htcondor.Schedd()
        submit = htcondor.Submit(self._job_args)
        try:
            with schedd.transaction() as txn:
                self._cluster_id = submit.queue(txn)
        except:
            print("Job submission failed for an unknown error")
            return JOB_FAILURE

        Utils.TLog("Job running on cluster " + str(self._cluster_id))

        # Wait until job has finished running?
        if wait is True:
            submit_result = self.WaitForFinish()
            return submit_result

        # If we aren't waiting for finish, return None
        return None


    def WaitForFinish(self, timeout=240):

        event_log = htcondor.JobEventLog(self._log)

        if event_log.isInitialized() is False:
            Utils.TLog("Event log not initialized. Should we abort here?")

        for event in event_log.follow():
            Utils.TLog("New event logged: " + str(event.type))
            if event.type is htcondor.JobEventType.JOB_TERMINATED:
                self.SuccessCallback()
                return SUCCESS
            elif event.type is htcondor.JobEventType.JOB_HELD:
                self.FailureCallback()
                return JOB_FAILURE

        # If we got this far, we hit the timeout and job did not complete
        Utils.TLog("Job failed to complete with timeout = " + str(timeout))
        return JOB_FAILURE


    def RegisterSubmit(self, submit_callback_fn):
        self.SubmitCallback = submit_callback_fn


    def RegisterSuccess(self, success_callback_fn):
        self.SuccessCallback = success_callback_fn


    def RegisterFailure(self, failure_callback_fn):
        self.FailureCallback = failure_callback_fn
