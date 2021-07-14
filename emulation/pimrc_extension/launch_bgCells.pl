#!/usr/bin/perl -w

##############################################################################
# This script open a TCP socket, listening to the given port (default: 2021) #
# Upon the reception of the data, it launches the traffic generator          #
# and keep listening for new incoming connections.                           #
##############################################################################

use Socket;

sub startTraffic
{
    my $par_gnbs = $_[0];
    my $par_numerology = $_[1];
    my $par_rbs = $_[2];
    my $par_ues = $_[3];
    my $par_sndInt = $_[4];
    my $par_run = $_[5];
      
    # initialize host and port
    my $host = "localhost";
    my $port = 2021;
    my $server = "127.0.0.1";  # Host IP running the server

    # create the socket, connect to the port
    socket(SOCKET,PF_INET,SOCK_STREAM,(getprotobyname('tcp'))[2])
    or die "Can't create a socket $!\n";
    connect( SOCKET, pack_sockaddr_in($port, inet_aton($server)))
    or die "Can't connect to port $port! \n";
    
    print SOCKET 1000;  
    print SOCKET $par_sndInt; 
    print SOCKET "gnbs=$par_gnbs-u=$par_numerology-rbs=$par_rbs-sndInt=$par_sndInt-bkUEs=$par_ues-#$par_run\n";
                 
    close SOCKET or die "close: $!";
}


my $out;
my $configOut;
my $sndInt = (0.04);

# iteration variables
my @numerology = (0);   
my @rbs = (100);
my @ues = (150);
my @gnbs = (6);
my @runs = (0);
foreach my $numerology (@numerology)
{
    foreach my $rbs (@rbs)
    {
        foreach my $ues (@ues)
        {
            foreach my $gnbs (@gnbs)
            {
                foreach my $run (@runs)
                {
                    # write to a INI file
                    print "\n --- Configuring simulation: ";
                    print "Numerology[$numerology] - RBs[$rbs] - UEs[$ues] - bgGnbs[$gnbs] - SndInt[$sndInt]\n\n";
                    
                    open ($out,">","scenario_multicell.ini") or die "ERROR: Unable to open output file"; 
                    print $out "*.numBgCell = \${numBgCells=$gnbs}\n";
                    print $out "*.gnb.cellularNic.bgTrafficGenerator[0].numBgUes = \${numBkUEs=$ues}\n";
                    print $out "*.bgCell[*].bgTrafficGenerator.numBgUes = \${numBkUEs}\n";
                    print $out "*.carrierAggregation.componentCarrier[0].numBands = \${rbs=$rbs}\n";
                    print $out "*.carrierAggregation.componentCarrier[0].numerologyIndex = \${u=$numerology}\n";
                    
                    close $out;
                    
                    
#                     # XXX
#                     # 
#                     # launch here a simulation with foreground cells to obtain average CQI and RTX
#                     # run simulation with foreground cells
#                     system("simu5g -u Cmdenv -c BgMulticell-simulated -r 0 --cmdenv-redirect-output=false omnetpp-simulated.ini");
#                     
                    open ($configOut,">","trafficGeneratorConfig.ini") or die "ERROR: Unable to open config output file"; 
                    print $configOut "include trafficGeneratorConfigs/config-u=${numerology}-rbs=${rbs}-numBgCells=${gnbs}-numBkUEs=${ues}-dist=50m-repetition=0.ini";
                    close $configOut;
                    
                    
                    my $pid = fork();
                    if ($pid == 0)
                    {
                        # child process - start simulation
                        print " --- Starting simulation ---\n\n";
                        system("./runMulticell_wSocket.sh");
                        
                        exit;
                    }
                    else
                    {
                        my $innerpid = fork();
                        if ($innerpid == 0)
                        {
                            system("./listener.pl");
                            exit;
                        }
                        else
                        {
                        
#                             sleep(60); # wait simulation to start 
                            sleep(5); # wait simulation to start 
                                                                                # father process - start real traffic on the sender
                            print " --- Starting traffic ---\n\n";
                            startTraffic($gnbs, $numerology, $rbs, $ues, $sndInt, $run);
                        
                            # wait child to finish
                            wait();
                        }
                        wait();
                    }
                }
            }

        }
    }
}