__author__ = "Raj Praveen Selvaraj"

'''
Test Client for MongoDB integration
Usage : Client has operations for insert, bulkinsert, findone, find (many), update and delete
        Invoked from the command line as python dyn_mongo_test.py -o [operation name]
        example: python dyn_mongo_test.py -o insert
                 python dyn_mongo_test.py -o bulk_insert
                 python dyn_mongo_test.py -o find_one
                 python dyn_mongo_test.py -o find
                 python dyn_mongo_test.py -o update
                 python dyn_mongo_test.py -o delete

Invoke the client in a sensible logical order and not random order
for example invoking update or delete or find without insert is absurd and so forth

if mongoDB runs on a different host and a different port
use the -H and -P to mention the same

client uses localhost and 8102 as the host and the port for mongoDB
log-file is not used in the implementation rightnow, just placed it as an option
'''

#!/usr/bin/env python

from optparse import OptionParser
import ConfigParser
import logging
import time
import os
import re
import sys
import errno
from datetime import datetime
from datetime import timedelta
import threading
import random

from logging import debug, info, warning, error


import pymongo
from pymongo import MongoClient


current_milli_time = lambda: int(round(time.time() * 1000))


def main():
    parser = OptionParser(usage="usage: %prog [options] filename",
                          version="%prog 1.0")
    parser.add_option("-o", "--operation",
                      action="store",
                      dest="operation",
                      default="insert",
                      help="Operation to perform: insert, bulk_insert, find_one, find (find many documents), delete and update")
    parser.add_option("-l", "--logfile",
                      action="store",
                      dest="logfle",
                      default="/tmp/dynomite-test.log",
                      help="log file location")
    parser.add_option("-H", "--host",
                      action="store",
                      dest="host",
                      default="127.0.0.1",
                      help="targe host ip")
    parser.add_option("-P", "--port",
                      action="store",
                      dest="port",
                      default="8102",
                      help="target port")

    if len(sys.argv) == 1:
         print "Learn some usages: " + sys.argv[0] + " -h"
         sys.exit(1)


    (options, args) = parser.parse_args()     



    #logger = logging.getLogger(log_name)
    #logger.setLevel(logging.DEBUG)
    #fh = logging.handlers.TimedRotatingFileHandler('/tmp/dynomite-test.log', when="midnight")
    #fh.setLevel(logging.DEBUG)
    #formatter = logging.Formatter('%(asctime)s:  %(name)s:  %(levelname)s: %(message)s')
    #fh.setFormatter(formatter)
    #logger.addHandler(fh)

    print options


    mongo = MongoClient(options.host, int(options.port) )
    db = mongo.test_people_db
    people = db.people

    num_docs = 100
    success = 0
    failure = 0

    if "insert" == options.operation:
        for i in range(1,num_docs+1):
            record = {
                        "name" : "person_" + str(i),
                        "description" : "description_" + str(i),
                        "timestamp" : datetime.now().strftime("%I:%M%p on %B %d, %Y")
                     }
            person_id = people.insert(record)
            if person_id is None:
                failure += 1
            else:
                success += 1
        print "Trying to Singe Insert : " + str(num_docs)
        print "Singe Insert Success : " + str(success)
        print "Singe Insert Failure : " + str(failure)

    elif "bulk_insert" == options.operation:
        records = []
        rand_prefix = random.randint(1000,100000)
        for i in range(1, num_docs + 1):
            record = {
                        "name" : "person_" + str(rand_prefix),
                        "description" : "description_" + str(rand_prefix),
                        "timestamp" : datetime.now().strftime("%I:%M%p on %B %d, %Y")
                     }
            rand_prefix += 1
            records.append(record)
        result = people.insert(records)
        print "Trying to Bulk Insert : " + str(num_docs)
        print "Bulk Insert Success : " + str(len(result))
        print "Bulk Insert Failure : " + str(num_docs - len(result))

    elif "find_one" == options.operation:
        for i in range(1, num_docs + 1):
            record = people.find_one( {"name":"person_" + str(i)} )
            if record is None:
                failure += 1
            else:
                success += 1
        print "Trying to single find : " + str(num_docs)
        print "Singe Find Success : " + str(success)
        print "Singe Find Failure : " + str(failure)
    elif "find" == options.operation:
        records = people.find( { "name" :  {'$regex': 'person*'} } ) #name like person
        count = people.find( { "name" : {'$regex': 'person*'} } ).count()
        print "Trying to bulk find name like 'person' "
        if count > 0:
            print "find multiple sucess : " + str(count)
        else:
            print "find multiple failure " + str(count)
    elif "delete" == options.operation:
        for i in range(1,num_docs+1):
            deletePerson = {"name" : "person_" + str(i) }
            people.remove(deletePerson)
            result = people.find_one(deletePerson)
            if result is None:
                success += 1
            else:
                failure += 1
        print "Trying to single delete : " + str(num_docs)
        print "Delete single person SUCCESS : " + str(success)
        print "Delete single person FAILURE : " + str(failure)
    elif "update" == options.operation:
        curr_timestamp = datetime.now().strftime("%I:%M%p on %B %d, %Y")
        for i in range(1,num_docs+1):
            new_record = {
                            "name" : "person_" + str(i),
                            "description" : "description_" + str(i),
                            "timestamp" : curr_timestamp
                        }
            record = people.find_one({"name":"person_" + str(i)})
            if not record is None:
                people.update({"_id":record['_id']}, new_record )
                result = people.find_one({"name":"person_" + str(i)})
                if result['timestamp'] == curr_timestamp:
                    success += 1
                else:
                    failure += 1
        print "Trying to single update : " + str(num_docs)
        print "Single Update Success : " + str(success)
        print "Singe Update Failure : " + str(failure)
    else:
        print "Invalid Operation see -h for help"

if __name__ == '__main__':
    main()

