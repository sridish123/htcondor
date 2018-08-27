import datetime
import errno
import os
import shutil
import sys
import time

class Utils(object):


    @staticmethod
    def Log(message):
        print("\t" + message)


    @staticmethod
    def TLog(message):
        timestamp = time.time()
        timestamp_str = datetime.datetime.fromtimestamp(timestamp).strftime('%d/%m/%y %H:%M:%S')
        print(timestamp_str + " " + message)


    @staticmethod
    def RunCondorTool(cmd):
        output = os.popen(cmd).read()
        return output


    @staticmethod
    def MakedirsIgnoreExist(directory):
        try:
            os.makedirs(directory)
        except:
            exctype, oe = sys.exc_info()[:2]
            if not issubclass(exctype, OSError): raise
            if oe.errno != errno.EEXIST:
                raise


    @staticmethod
    def RemoveIgnoreMissing(path):
        try:
            if os.path.isfile(path):
                os.unlink(path)
            elif os.path.isdir(path):
                shutil.rmtree(path)
        except:
            exctype, oe = sys.exc_info()[:2]
            if not issubclass(exctype, OSError): raise
            if oe.errno != errno.ENOENT:
                raise


    @staticmethod
    def IsWindows():
        if os.name == 'nt':
            return True
        return False
