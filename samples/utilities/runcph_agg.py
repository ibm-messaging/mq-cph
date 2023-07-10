#!/usr/bin/env python
import signal
import subprocess
import shlex
import threading
import argparse
import re

########################################################################################
#
# Start cph threads across multiple processes and monitor/process the output from them.
#
# Outputs summary rate at interval defined by -ss cph parm and a total for the job when it
# completes.
# 
# For usage:
# runcph_agg.py -h 
#
# Paul Harris - February 2023
########################################################################################

def stop_process(signum,frame):
   pass
                                                                                                     
def output_reader(proc, lastProcess, verbose):
    thisPid=""
    if(verbose > 0):
      thisPid = "pid:"+ str(proc[0].pid) + " "
    for line in iter(proc[0].stdout.readline, b''):
      line = line.decode('utf-8')
      #print('got line: {}'.format(line.strip()))
      match = regex_dict['idAndRate'].search(line)
      if(match):
         #'print('id: {}, rate:{}'.format(match.group(1),match.group(2)))
         #print('rate:{}'.format(match.group(2)))
         proc[1] = float(match.group(2))
         if(lastProcess):
            #print('{} (last process)'.format(line.strip()))
            printIntervalStats()
         if(verbose > 1):      
            print('{}{}'.format(thisPid,line.strip()))
      else:
         match = regex_dict['summaryLine'].search(line)
         if(match):
            proc[2] = int(match.group(1)) #totalIterations
            proc[3] = float(match.group(2)) #totalSeconds
            proc[4] = float(match.group(3)) #avgRate
            print('{}{}'.format(thisPid,line.strip()))
         else:
            print('{}{}'.format(thisPid,line.strip()))

def printIntervalStats():
   total = 0.00
   for i in range(numberOfProcesses):
      total += procs[i][1]
   print("rate={:.2f},threads={},processes={}".format(total,numberOfThreads,numberOfProcesses))

def getCphParmValueIndex(arrayIn,parm,defaultValue):
   try: 
      parmValueIndex = arrayIn.index(parm) +1
   except ValueError: 
      arrayIn.append(parm)
      arrayIn.append(defaultValue)
      parmValueIndex=len(arrayIn)-1
   return parmValueIndex


#regex expressions used to find the rate and summary messages printed by cph.
regex_dict = {
    'idAndRate': re.compile(r'id=(\d+).+rate=(\d+\.\d+)'),
    'summaryLine': re.compile(r'totalIterations=(\d+).+totalSeconds=(\d+\.\d+).+avgRate=(\d+\.\d+)')
}


def launch(cphCommandIn, numThreadsIn, numProcessesIn, verboseIn):
   #Global variables
   global numberOfProcesses
   global numberOfThreads
   global procs

   numberOfProcesses = numProcessesIn
   numberOfThreads = numThreadsIn
   procs = [None] * numberOfProcesses #This is going to hold the cph processes and their counters (rates etc)

   if(numberOfProcesses > numberOfThreads):
      raise Exception("Number of threads ({}) must be more than the number of processes ({})".format(numberOfThreads,numberOfProcesses))

   #Integer part of division
   lowThreadsPerProcess=numberOfThreads // numberOfProcesses

   #If threads doesn't divide by processes excactly, then the remaining processes will run one extra thread each to make up the total.
   remainder=numberOfThreads % numberOfProcesses
   hiThreadsPerProcess=lowThreadsPerProcess+1                 

   #print("Low threads per process: {}".format(lowThreadsPerProcess))
   #print("High threads per process: {}".format(hiThreadsPerProcess))
   #print("Remaining threads: {}".format(remainder))

   #Split the cph command into an array suitable for subprocess.Popen and find the -nt and -id parms (inserting them with a value of 0, if they're not present). We'll change the values of -nt and -id later.
   cphcmd = shlex.split(cphCommandIn)
   threadValueIndex = getCphParmValueIndex(cphcmd,"-nt","0")
   idValueIndex = getCphParmValueIndex(cphcmd,"-id","0")

   lastProcess = False

   cphcmd[threadValueIndex] = str(lowThreadsPerProcess) #set the correct value for -nt in the cph command

   for x in range(numberOfProcesses): 
      cphcmd[idValueIndex] = str(x) #set the cph -id parm for this process
      if(x == numberOfProcesses-remainder):
         cphcmd[threadValueIndex] = str(hiThreadsPerProcess)
      #procs.append([subprocess.Popen(cphcmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT),0,0,0,0])
      procs[x] = [subprocess.Popen(cphcmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT),0,0,0,0]
      if(x == numberOfProcesses-1): #last process - this is used to tell an output reader thread, it's the one to trigger the aggregate rate report.
         lastProcess = True
      threading.Thread(target=output_reader, args=(procs[x],lastProcess,verboseIn)).start()

   #This traps a ctrl-c so we can wait for the cph processes to complete, output the final summary, then end.
   sig_handler = signal.signal(signal.SIGINT, stop_process)

   #All the cph processes should now be running. We'll wait for them to stop (by time, #messages, terminal failure or ctrl-c )
   for i in range(numberOfProcesses):
      procs[i][0].wait()  

   #Print Summary of Process stats
   iterations = 0
   totalSeconds = 0  
   rateAccumulator = 0
   for i in range(numberOfProcesses):
      iterations += procs[i][2]
      if (procs[i][3] > totalSeconds):
         totalSeconds = procs[i][3]
      rateAccumulator += procs[i][4]
   print('---------------------------------------------------------------------------------')
   print('--- Summary for all processes')
   print('---------------------------------------------------------------------------------')
   print('--- Thread distribution (threads={}  processes={})'.format(numberOfThreads,numberOfProcesses))
   print('--- Processes 1 to {} running {} threads'.format(numberOfProcesses-remainder,lowThreadsPerProcess))
   if(remainder > 0):
      print('--- Processes {} to {}  (total: {})  running {} threads'.format(numberOfProcesses-remainder+1,numberOfProcesses,remainder,hiThreadsPerProcess))
   print('---------------------------------------------------------------------------------')
   print('totalJobIterations={},maxSeconds={},avgJobRate={:.2f},avgRatePerProcess={:.2f}'.format(iterations,totalSeconds,rateAccumulator, rateAccumulator / numberOfProcesses))


if __name__ == '__main__':
   # This block will only be run if runcph_agg.py is executed as the main process (i.e you could import this module instead)
   # and execute main
   parser = argparse.ArgumentParser()

   parser.add_argument("-c", "--cphcommand", dest="cmd", required=True,
                     help="A cph command (including any path). Note that -nt and -id parms will be ignored, but you can leave them in.", 
                     metavar="<cph command line>")
   parser.add_argument("-t", "--threads", dest="threads", required=True,
                     type=int,
                     help="Total number of cph threads to be started", metavar="<# number of threads>")              
   parser.add_argument("-p", "--processes", dest="processes", required=True,
                     type=int,
                     help="Number of processes threads are to be spread over", metavar="<# processes>")      
   parser.add_argument("-v", "--verbose", dest="verbose",
                     type=int,
                     default=0,
                     help="verbose levels: 0=minimal (default), 1=print pids, 2=print pids + individual rate messages",metavar="<verbose level>")       

   args = parser.parse_args()
   print(args.cmd)

   launch(args.cmd, args.threads, args.processes, args.verbose)