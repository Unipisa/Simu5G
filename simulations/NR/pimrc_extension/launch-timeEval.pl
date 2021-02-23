#!/usr/bin/perl
use strict;
use 5.010;

no warnings 'experimental::smartmatch';


my($i);
my($j);
my($pid);
my($myPid);

my($parameter);
my($value);
my($config);


while(@ARGV)
{
    # ...we'll extract a parameter...
    $parameter = shift @ARGV;

    # ...and its value!
    $value = shift @ARGV;

    # it must be a value!
    if(!defined($value)){
        die "<E> Wrong Format!!!\n";
    }

    # store the value into the correct variable
    given( $parameter ) 
    {
        # parameter file to be opened
        when(/^-config/)
        {
            # assign
            $config = $value; 
        }

        default
        {
            die "<E> Unknown parameter \"$parameter\"! Run --h\n";
        }
    }
}


# configuration names in the .ini file
my(@configs) = 
(
    "BgTrafficScalabilityEval-generated" , 
    "BgTrafficScalabilityEval-simulated" 
);

my($timeResultsDir) = "timeResults";
mkdir($timeResultsDir);
my($countProc) = 0;
my($iniFile) = "omnetpp.ini";
my($maxProc) = 16;
my($runs) = 10;
my($parameters) = 9;
my($total);
my($decreasing);
my($timeCmd) = "/usr/bin/time -f %e";
$total = @configs * $parameters * $runs;
$decreasing = $total;
print "Total expected Runs: $total\n";

foreach $config (@configs)
{
    $timeResultsDir = "timeResults/$config";    
    mkdir($timeResultsDir);
    for( $j = 0 ; $j<$parameters; $j++ )
    {
        my($numUes) = $j * 25;
	# bound i to the number of per-configuration repetitions
	for( $i = 0 ; $i<$runs ; $i++ )
	{
	    $pid=fork();
	    if($pid==0)
	    {
	        $myPid = $$;
                my($runnumber) = $j*$runs + $i;
	        print "CHILD: I am process $myPid, now running [Config $config, run $runnumber]\n";
	        system "$timeCmd simu5g -u Cmdenv -c $config -r $runnumber $iniFile 2>> $timeResultsDir/time-numBgUes=$numUes >/dev/null";
	        print "CHILD: I am process $myPid and I am terminating\n";
	        exit;
	    }
	    else
	    {
	        $countProc++;
	        print "PARENT: run $i with process $pid started. Total running: $countProc\n";
	        if($countProc == $maxProc)
	        {
	            print "PARENT: waiting\n";
	            $pid = wait;
	            print "PARENT: $pid stopped ($decreasing\\$total), starting a new one\n";
	            $countProc--;
	            $decreasing--;
	        }
	    }  
	}
		
    } 
    print "PARENT: no more processes\n";
}

for( $i = $countProc ; $i>0 ; $i-- )
{
    print "PARENT: Final waiting\n";
    $pid = wait;
    print "PARENT: $pid stopped ($decreasing\\$total)\n";
    $countProc--;
    $decreasing--;
}

print "PARENT: no more processes\n";

    
