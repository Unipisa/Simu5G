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
    my $par_fgMsgLen = $_[4];
    my $par_fgSndInt = $_[5];
    my $par_bgSndInt = $_[6];
    my $par_run = $_[7];
      
    # initialize host and port
    my $host = "localhost";
    my $port = 2021;
    my $server = "127.0.0.1";  # Host IP running the server

    # create the socket, connect to the port
    socket(SOCKET,PF_INET,SOCK_STREAM,(getprotobyname('tcp'))[2])
    or die "Can't create a socket $!\n";
    connect( SOCKET, pack_sockaddr_in($port, inet_aton($server)))
    or die "Can't connect to port $port! \n";
    
    print SOCKET $par_fgMsgLen; 
    print SOCKET $par_fgSndInt; 
    print SOCKET "AAAAAA-varFgBitrate-gnbs=$par_gnbs-u=$par_numerology-rbs=$par_rbs-msgLen=$par_fgMsgLen-sndInt=$par_fgSndInt-bkUEs=$par_ues-#$par_run\n";
                 
    close SOCKET or die "close: $!";
}


my $out;
my $configOut;

# iteration variables
my @ca = (1);
my @numerology = (0);   
my @rbs = (100);
my @ues = (100);
my @gnbs = (3);
my @fgMsgLength = (1000);
my @fgSndInterval = (0.01);
my $bgSndInt = 0.04;
my @runs = (0);
                
foreach my $ca (@ca)
{
	foreach my $numerology (@numerology)
	{
	    foreach my $rbs (@rbs)
	    {
	        foreach my $ues (@ues)
	        {
                    foreach my $gnbs (@gnbs)
                    {
                    	foreach my $fgMsgLen (@fgMsgLength)
	                {
                            foreach my $fgSndInt (@fgSndInterval)
                            {
                                foreach my $run (@runs)
                                {
                        
                                    # write to a INI file
                                    print "\n --- Configuring simulation: ";
                                    print "Numerology[$numerology] - RBs[$rbs] - UEs[$ues] - gNBs[$gnbs] - FgSndInt[$fgSndInt]\n\n";
                                    
                                    open ($out,">","scenario_multicell.ini") or die "ERROR: Unable to open output file"; 
                                    print $out "*.numBgCell = \${numBgCells=$gnbs}\n";
                                    print $out "*.gnb.cellularNic.bgTrafficGenerator[0].numBgUes = \${numBkUEs=$ues}\n";
                                    print $out "*.bgCell[*].bgTrafficGenerator.numBgUes = \${numBkUEs}\n";
                                    print $out "*.carrierAggregation.componentCarrier[0].numBands = \${rbs=$rbs}\n";
                                    print $out "*.carrierAggregation.componentCarrier[0].numerologyIndex = \${u=$numerology}\n";
                                    
                                    close $out;
                                    
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
                                        
                                            sleep(5); # wait simulation to start (is 5 seconds enough?)
                                                                                                # father process - start real traffic on the sender
                                            print " --- Starting traffic ---\n\n";
                                            startTraffic($gnbs, $numerology, $rbs, $ues, $fgMsgLen, $fgSndInt, $bgSndInt, $run);

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
	    }
    }
}