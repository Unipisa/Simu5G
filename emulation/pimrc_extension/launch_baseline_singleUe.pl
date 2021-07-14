#!/usr/bin/perl -w

##############################################################################
# This script open a TCP socket, listening to the given port (default: 2021) #
# Upon the reception of the data, it launches the traffic generator          #
# and keep listening for new incoming connections.                           #
##############################################################################

use Socket;

sub startTraffic
{
    my $par_sndInt = $_[0];
      
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
    print SOCKET "baseline-singleUe\n";
                 
    close SOCKET or die "close: $!";
}


# iteration variables
my @runs = (0);
my @sndIntArray = (0.04,0.02,0.01,0.005);
foreach my $run (@runs)
{
    foreach my $sndInt (@sndIntArray)
    {

        my $pid = fork();
        if ($pid == 0)
        {
            # child process - start simulation
            print " --- Starting simulation ---\n\n";
            system("./run_baseline_singleUe.sh");
            
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
                startTraffic($sndInt);
            
                # wait child to finish
                wait();
            }
            wait();
        }
    }
}
