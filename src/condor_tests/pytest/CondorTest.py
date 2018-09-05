import atexit
import sys

from PersonalCondor import PersonalCondor
from Globals import *
from Utils import Utils

class CondorTest(object):

    #
    # Personal Condor management
    #
    _personal_condors = { }

    # @return A PersonalCondor object.  The corresponding master daemon
    # has been started.
    #
    # (FIXME) In the default case, locate the "current"/"default" personal
    # condor that the test suite should have already started for us.
    # Also, if CONDOR_CONFIG is not set already, complain.
    #
    # Otherwise, construct a new personal condor and register an
    # atexit function to make sure we shut it down (FIXME: aggressively).
    #
    # Make the returned PersonalCondor the 'active' object (that is, set
    # its CONDOR_CONFIG in the enviroment).
    #
    @staticmethod
    StartPersonalCondor( name, params=None, ordered_params=None):
        pc = CondorTest._personal_condors.get( name )
        if pc is None:
            pc = PersonalCondor( name, params, ordered_params )
            CondorTest._personal_condors[ name ] = pc
        atexit.register( CondorTest.ExitHandler )
        pc.Start()
        return pc

    @staticmethod
    StopPersonalCondor( name ):
        pc = CondorTest._personal_condors.get( name )
        if pc is not None:
            pc.Stop()

    @staticmethod
    StopAllPersonalCondors():
        for pc in CondorTest._personal_condors:
            pc.Stop()


    #
    # Subtest reporting.  The idea here is two-fold: (1) to abstract away
    # reporting (e.g., for Metronome or for CTests or for Jenkins) and
    # (2) to help test authors catch typos and accidentally skipped tests.
    #
    _tests = { }

    # @param subtest An arbitrary string denoting which part of the test
    #                passed or failed, or the name of test if it's indivisible.
    # @param message An arbitrary string explaining why the (sub)test failed.
    @staticmethod
    def RegisterFailure( subtest, message ):
        Utils.TLog( " [" + subtest + "] FAILURE: " + message )
        CondorTest._tests[ subtest ] = ( TEST_FAILURE, message )
        return None

    @staticmethod
    def RegisterSuccess( subtest, message ):
        Utils.TLog( " [" + subtest + "] SUCCESS: " + message )
        CondorTest._tests[ subtest ] = ( TEST_SUCCESS, message )
        return None

    # @param list A list of arbitrary strings corresponding to parts of the
    #             test.  The test will fail if any member of this list has
    #             not been the first argument to RegisterSuccess().
    @staticmethod
    def RegisterTests( list ):
        for test in list:
            CondorTest._tests[ test ] = ( TEST_SKIPPED, "[initial state]" )


    #
    # Exit handling.
    #
    @staticmethod
    def ExitHandler():
        # We're headed out the door, start killing our PCs.
        for pc in CondorTest._personal_condors:
            pc.BeginStopping()

        # The reporting here is a trifle simplicistic.
        rv = TEST_SUCCESS
        for subtest in CondorTest._tests.keys():
            record = CondorTest._tests[subtest]
            if record[0] != TEST_SUCCESS:
                Utils.TLog( "[" + subtest + "] did not succeed, failing test!" )
                rv = TEST_FAILURE

        # Make sure the PCs are really gone.
        for pc in CondorTest._personal_condors:
            pc.FinishStopping()

        # If we're the last registered exit handler, this determines the
        # interpreter's exit code.  [TLM: or so I boldly claim.]
        raise SystemExit( rv )

    # The 'early-out' method.  For now, it has no particular semantic
    # meaning, but it might later.
    @staticmethod
    def exit( code ):
        sys.exit( code )
