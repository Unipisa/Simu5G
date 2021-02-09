#!/usr/bin/perl
use strict;
use 5.010;
use Scalar::Util qw(looks_like_number);

no warnings 'experimental::smartmatch';

my($fileIn);
my($fileOut);
my($numRows) = 1;


my($parameter);
my($value);
my($perc);


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
        when(/^-file/)
        {
            # assign
            $fileIn = $value;
        }

        when(/^-output/)
        {
            # assign
            $fileOut = $value;
        }

        when(/^-rows/)
        {
            # assign
            $numRows = $value;
        }

        default
        {
            die "<E> Unknown parameter \"$parameter\"! Run --h\n";
        }
    }
}

my($fl) = $fileIn;

# open input file
open(DAT, $fl) || die("Could not open file!");
my(@raw_data) = <DAT>;
close(DAT);

# skip the first line (header line)
my($data_line) = $raw_data[1];
chomp($data_line);

# split by commas
my(@data_values)=split(",",$data_line);

# field 11 is the count
# field 13 is the mean
# field 14 is the stddev
my($count) = $data_values[11];  # convert to ms
my($mean) = $data_values[13] * 1000;   # convert to ms
my($stddev) = $data_values[14] * 1000;
my($confidence) = 1.96*($stddev/sqrt($count));


# --- print output --- #
print $mean."\t".$confidence;


