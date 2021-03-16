// @author Alessandro Noferi


packet HTTPRequestPacket
{
    @customize(true);
  	string method;
  	string body;
  	string uri;
  	string payload;

}