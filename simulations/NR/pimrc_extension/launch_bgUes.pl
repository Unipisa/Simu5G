#!/usr/bin/perl -w


# iteration variables
my @numerology = (0);   
my @rbs = (100);
my @ues = (0,25,50,75,100,125,150,175,200);
my @extCells = (0);
my @sndInterval = (0.04);
my @runs = (0,1,2,3,4,5,6,7,8,9);
my $baseRun = 0;
foreach my $numerology (@numerology)
{
    foreach my $rbs (@rbs)
    {
        foreach my $ues (@ues)
        {
            foreach my $extCells (@extCells)
            {
                foreach my $sndInt (@sndInterval)
                {
                    foreach my $run (@runs)
                    {
            
                        # write to a INI file
                        print "\n --- Configuring simulation: ";
                        print "Numerology[$numerology] - RBs[$rbs] - UEs[$ues] - ExtCells[$extCells] - SndInt[$sndInt]\n\n";
                        
                        open ($configOut,">","trafficGeneratorConfig.ini") or die "ERROR: Unable to open config output file"; 
                        print $configOut "include trafficGeneratorConfigs/config-u=${numerology}-rbs=${rbs}-numBgCells=0-numBkUEs=${ues}-dist=300m-repetition=$run.ini";
                        close $configOut;

                        my $r = $baseRun + $run;
                        
                        system("simu5g -r $r -c BgSingleCellValidation-generated omnetpp.ini --cmdenv-redirect-output=false");
                        
                    }
                    $baseRun = $baseRun + 10;
                }
            }

        }
    }
}