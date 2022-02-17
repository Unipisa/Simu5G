#!/usr/bin/perl

# This script computes the mean, together with variance, stddev and confidence interval (at 95%)
#
# Usage: ./getMean <config> <metric>
#

use strict;
use 5.010;
use Scalar::Util qw(looks_like_number);

my(@raw_data);
my($data_line);
my($fl_in);
my($sample);
my(@samples);
my($sum);
my($mean);
my($variance);
my($stddev);
my($confidence);

my($config) = $ARGV[0];
my($metric) = $ARGV[1];


# power model parameters
my($Pmax) = 1;        # 1 W = 30dBm
my($deltaP) = 588;
my($p0) = 114.5;
my($numSectors) = 1;
my($numCCs);
if ($config eq "CA-BestChannel")
{
    $numCCs = 2;
}
else
{
    $numCCs = 1;
}
my($totBlocks) = 25 * $numCCs;

$fl_in = "stats/$config/sca_$metric.csv";

# open and read CSV file 
open(DAT, $fl_in) || die("Could not open input file $fl_in");
@raw_data=<DAT>;
close(DAT);

# parse each line
my(@data_values);
my($lineCounter) = 0;
foreach $data_line (@raw_data)
{
    chomp($data_line);

    #skip the first line (header)
    if($lineCounter == 0)
    {
	$lineCounter++;
	next;
    }

    # retrieve column 'value' (5th column)
    @data_values = split(",",$data_line);

    $sample = $data_values[7];

    # compute the power consumption for this sample
    my($utilizationRatio) = $sample/$totBlocks; 
    my($sampleConsumption) = $numCCs * $numSectors * ($p0 + $deltaP * $Pmax * $utilizationRatio);

    $sum = $sum + $sampleConsumption;
    push @samples, $sampleConsumption;

}

# compute mean
$mean = $sum / ($#samples+1);

#compute variance,std dev and confidence interval
$sum = 0;
foreach $sample (@samples)
{
    $sum = $sum + ($sample - $mean)**2;
}
$variance = $sum/$#samples;
$stddev = sqrt($variance);
$confidence = 1.96 * ($stddev/sqrt($#samples));

		
print "$mean\n";
print "$variance\n";
print "$stddev\n";
print "$confidence\n";

