#!/usr/bin/perl -w
use strict;
use warnings;
use Getopt::Long;	# for options parsing

# File descriptors
my $in;

# Input parameters
my $numMessages = "";
my $computeRtt = "";
my $countMsg = ""
my $inputFileRtt = "rtt";
my $inputFileAckedMsg = "ackedMsg";


# parsing command line options
GetOptions('num:i' => \$numMessages, 'rtt' => \$computeRtt, 'countMsg' => \$countMsg); 
if ($num eq "")
{
    print "ERROR: You must specify the name of the number of sent messages (-n option)";
    exit -1;
}
if (($computeRtt eq "") and ($countMsg eq ""))
{
    print "ERROR: You must specify what you want to do (-r or -c option)";
    exit -1;
}

if ($computeRtt)
{
    ### PARSE RTT ###

    open($in, "<", $inputFileRtt) or die "Cannot open rtt file";

    my @samples = ();
    my $sum = 0;
    my $count = 0;

    # read file
    my $line;
    while ($line = <$in>)
    {
            # lines contain values separated by blank spaces
            my @values = split('\t', $line);  
            
            # column number 1 contains the value of interest
            $sum = $sum + $values[1];
            
            # store the samples, so as to compute confidence intervals
            push @samples,$values[1];
            
            $count = $count + 1;
    }

    # compute mean
    if ($count eq 0)
    {
        print "NaN\tNaN";
    }
    else
    {
        my $mean = $sum / $count;

        # compute the confidence of the mean
        $sum = 0;
        my $stdDev;
        my $confidence;
        for (my $i=0; $i < 1; $i++)
        {
            $sum = $sum + ($samples[$i] - $mean)**2;
        }
        $stdDev = sqrt($sum/1);
        $confidence = 1.96*($stdDev/sqrt($runs));  # alpha = 0.95

        print $mean."\t".$confidence;
    }

    close $in;
}
elsif ($countMsg)
{
    ### PARSE ACKED ###

    open($in, "<", $inputFileAckedMsg) or die "Cannot open ackedMsg file";

    # read file
    my $line;
    $count = 0;
    while ($line = <$in>)
    {      
        $count = $count + 1;
    }
    my $ratio = $count/$num;
    
    print $ratio;
    
    close $in;
}
