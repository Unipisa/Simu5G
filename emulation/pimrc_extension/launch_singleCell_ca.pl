#!/usr/bin/perl -w

##############################################################################
# This script open a TCP socket, listening to the given port (default: 2021) #
# Upon the reception of the data, it launches the traffic generator          #
# and keep listening for new incoming connections.                           #
##############################################################################

use Socket;

sub startTraffic
{
    my $par_ca = $_[0];
    my $par_numerology = $_[1];
    my $par_rbs = $_[2];
    my $par_ues = $_[3];
    my $par_extCells = $_[4];
    my $par_sndInt = $_[5];
    my $par_run = $_[6];
      
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
    print SOCKET "ca=$par_ca-u=$par_numerology-rbs=$par_rbs-sndInt=$par_sndInt-bkUEs=$par_ues-#$par_run\n";
                 
    close SOCKET or die "close: $!";
}


my $out;

# iteration variables
my @ca = (2);
my @numerology = (4);   
my @rbs = (25);
my @ues = (15..19);
my @extCells = (0);
my @sndInterval = (0.04);
my @runs = (0);
foreach my $ca (@ca)
{
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
	                        
	                        open ($out,">","scenario.ini") or die "ERROR: Unable to open output file"; 
	                        print $out "*.numBkUEs = \${numBkUEs=$ues}\n";
	                        print $out "*.carrierAggregation.numComponentCarriers = \${ca=$ca}\n";
	                        print $out "*.carrierAggregation.componentCarrier[*].numBands = \${rbs=$rbs}\n";
	                        print $out "*.carrierAggregation.componentCarrier[*].numerologyIndex = \${u=$numerology}\n";
	                        print $out "*.numExtCells = \${numExtCells=$extCells}\n";
	                        print $out "*.bkUe[*].app[*].sendInterval = \${sndInt=${sndInt}}s\n";
	                        
	                        close $out;
	                        
	                        my $pid = fork();
	                        if ($pid == 0)
	                        {
	                            # child process - start simulation
	                            print " --- Starting simulation ---\n\n";
	                            system("./run_wSocket.sh");
	                            
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
	                                startTraffic($ca, $numerology, $rbs, $ues, $extCells, $sndInt, $run);
	                            
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