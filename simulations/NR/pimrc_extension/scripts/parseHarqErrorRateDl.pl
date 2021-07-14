#!/usr/bin/perl
use strict;
use 5.010;
use Scalar::Util qw(looks_like_number);

no warnings 'experimental::smartmatch';

my($fileIn);
my($fileOut);
my($out);
my($parameter);
my($value);

my($moduleName)="gnb.cellularNic.bgTrafficGenerator[0]";

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
        when(/^-i/)
        {
            # assign
            $fileIn = $value;
        }
        when(/^-o/)
        {
            # assign
            $fileOut = $value;
        }
        when(/^-module/)
        {
            # assign
            $moduleName = $value;
        }
        default
        {
            die "<E> Unknown parameter \"$parameter\"! Run --h\n";
        }
    }
}

my(@raw_data);
my($data_line);

open(DAT, $fileIn) || die("Could not open file!");
@raw_data=<DAT>;
close(DAT);

# open output file
open ($out,">",$fileOut) or die "ERROR: Unable to open temporary file"; 

my($lineCounter)=1;
my($index) = 0;
foreach $data_line (@raw_data)
{
    chomp($data_line);

    my(@scalar_line)=split("harqErrorRateDl:mean ",$data_line);
    if ($#scalar_line == 0)
    {
        $lineCounter++;
        next;
    }
    
    my $val = @scalar_line[1];
    if (!looks_like_number($val))  
    {
        next;
    }

    print $out "*.$moduleName.bgUE[$index].generator.rtxRateDl = $val\n";

    $lineCounter++;
    $index++;
}


