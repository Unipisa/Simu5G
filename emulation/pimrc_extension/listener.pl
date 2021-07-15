#!/usr/bin/perl -w

##############################################################################
# This script open a TCP socket, listening to the given port (default: 2021) #
# Upon the reception of the data, it launches the traffic generator          #
# and keep listening for new incoming connections.                           #
##############################################################################

use IO::Socket::INET;

use Getopt::Long;        # for options parsing

my $senderCmd = "./emulation_sender";	
my $receiverCmd = "./emulation_receiver";

my $portNumber = "2021";  
my $proto = "tcp";

# auto-flush on socket
$| = 1;

# creating a listening socket
my $socket = new IO::Socket::INET (
    LocalHost => '0.0.0.0',
    LocalPort => $portNumber,
    Proto => 'tcp',
    Listen => 5,
    Reuse => 1
);
die "cannot create socket $!\n" unless $socket;

# file descriptor for the output file (needed by SimuLTE)
my $out;

#while(1)
#{
    # waiting for a new client connection
    my $client_socket = $socket->accept();

    print "\nListener waiting for client connection on port $portNumber\n";

    # get information about a newly connected client
    my $client_address = $client_socket->peerhost();
    my $client_port = $client_socket->peerport();
    print "connection from $client_address:$client_port\n";

    my $pid = fork();
    if ($pid == 0)
    {
        # child process - start receiver
        print " --- RECEIVER STARTED --- \n\n";
        my $cmd = "$receiverCmd -p udp";
        `$cmd`;
        print " --- RECEIVER TERMINATED --- \n\n";
        
        exit;
    }
    else
    {
        # parent process - start sender
        sleep(1);
        
        print " --- Configuring the sender --- \n";
        
        # read message length
        my $msgLen = "";
        $client_socket->read($msgLen, 4);  # 4 chars
        print "--> msg len set to $msgLen B\n";
        
        # read send interval 
        my $sndInt = "";
        $client_socket->read($sndInt, 4);  # 4 chars
        print "--> snd int set to $sndInt s\n";
        
        # read string from the connected client
        my $outputfolder = "";
        $client_socket->read($outputfolder, 2048);
        print "--> outputfolder set to $outputfolder\n";
        
        # launch traffic and save statistics in the appropriate folder
        print " --- SENDER STARTED --- \n";
        my $cmd = "$senderCmd -h 10.0.2.1 -p udp -s $msgLen -t $sndInt -d 120 -o $outputfolder";
        `$cmd`;
        print " --- SENDER TERMINATED --- \n\n";
        
        # notify client that response has been sent
        shutdown($client_socket, 1);
    }
                   

#}
$socket->close();
