#include <iostream>
#include <sstream>
#include <string>
#include <winsock2.h>

using namespace std;

WSADATA WSAData;    //WSADATA Structure - Contains info about Windows Sockets
WORD wVersionRequested = MAKEWORD(2, 2);    //version 2.0 of windows sockets

/**********************************************************************
*
*  Name:       InitiateWinsock()
*
*  Author:     Ross Spencer
*
*  Purpose:    Run through the initialization of Winsock
*
**********************************************************************/
int InitiateWinsock()
{
	if(WSAStartup(wVersionRequested, &WSAData))
	{
		cout << "couldn't start winsock" << endl;
		return -1;
	}
}

/**********************************************************************
*
*  Name:       main()
*
*  Author:     Ross Spencer
*
*  Purpose:    Entry point for SIP message app, contains the sending and
*              recieving mechanism for SIP messages. 
*
**********************************************************************/
int main(int argc, char *argv[]) {

   const int SIZE_EXTENSION = 5;
   const int SIZE_IP_ADDR = 15;
   const int SIZE_MESSAGE = 250;
   const int SIZE_RECV_BUFFER = 1000;
   const int RESPONSE_HEADER_LEN = 15;

   char szExtension[SIZE_EXTENSION] = "";
   char szRemoteIP[SIZE_IP_ADDR] = "";
   char szMessage[SIZE_MESSAGE] = "";

   if(argv[1])
   {
      if(*argv[1] == 'E')
      {
         cout << endl << "Input Phone Extension: ";
         cin.getline(szExtension, SIZE_IP_ADDR);
      }
   }

   cout << "Input IP Address to send message to: "; 
   cin.getline(szRemoteIP, SIZE_IP_ADDR);

   cout << "Input message to send: ";
   cin.getline(szMessage, SIZE_MESSAGE);

   cout << endl;

   InitiateWinsock();

   const int SIZE_HOSTNAME = 50;

   char szHostName[SIZE_HOSTNAME] = "";
   gethostname(szHostName, SIZE_HOSTNAME);
   
   hostent *myHost = gethostbyname(szHostName);

   if(myHost)
   {
      unsigned long ulHostIP = *reinterpret_cast<unsigned long*>(myHost->h_addr_list[0]);
      stringstream hostIP;
      hostIP << ((ulHostIP & 0x000000FF)) << "." << ((ulHostIP & 0x0000FF00) >> 8) << "."
         << ((ulHostIP & 0x00FF0000) >> 16) << "." << ((ulHostIP & 0xFF000000) >> 24);

      stringstream sip_message;
      sip_message << "MESSAGE sip:";

      if(*szExtension)
      {
         sip_message << szExtension << "@";
      }
      sip_message << szRemoteIP << " SIP/2.0" << endl; 
      sip_message << "Via: SIP/2.0/UDP " << hostIP.str() << ";branch=z9hG4bK776sgdkse" << endl;   
      sip_message << "Max-Forwards: 70" << endl;
      sip_message << "From: sip:2040@" << hostIP.str() << ";tag=49583" << endl;
      sip_message << "To: sip:" << szExtension << "@" << szRemoteIP << endl;
      sip_message << "Call-ID: abcd123@" << hostIP.str() << endl;  
      sip_message << "User-Agent: exponentialdecay.co.uk softSIP" << endl;
      sip_message << "CSeq: 1 MESSAGE" << endl << "Allow: ACK" << endl
         << "Content-Type: text/plain" << endl;
   
      sip_message << "Content-Length: " << strlen(szMessage)+1 << endl;
   
      sip_message << endl << " " << szMessage << endl;
   
      cout << sip_message.str() << endl;

   	SOCKET Sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);  //address family | type | protocol
   	SOCKADDR_IN	SockAddr;
   	memset(&SockAddr,0, sizeof(SockAddr));
   	SockAddr.sin_family = AF_INET;
   	SockAddr.sin_addr.s_addr = INADDR_ANY;
   	SockAddr.sin_port = htons(5060);
   
   	// Bind to the address and port
   	int iBindRet = bind(Sock, (SOCKADDR*)&SockAddr, sizeof(SockAddr));
   
   	if (iBindRet != 0)
   	{
   		cout << "cannot bind to port" << endl;
   		return -1;
   	}
   
   	SOCKADDR_IN dest;
   	memset(&dest,0,sizeof(SOCKADDR_IN));
   	dest.sin_addr.s_addr = inet_addr(szRemoteIP);
   	dest.sin_family = AF_INET;
   	dest.sin_port = htons(5060);
   
   	int bytes_sent = 0;
      bytes_sent = sendto(Sock, sip_message.str().c_str(), sip_message.str().length(), 0, (SOCKADDR*)&dest, sizeof(SOCKADDR_IN));
     
      if (bytes_sent)
      {
         cout << bytes_sent << " bytes, message sent... OK" << endl;

         fd_set readSet;
         FD_ZERO(&readSet);
         FD_SET(Sock, &readSet);
         struct timeval timeout;
         timeout.tv_sec = 0;
         timeout.tv_usec = 10000; 

         //throw socket into non-blocking mode...   
         unsigned long non_blocking = 1;
         ioctlsocket(Sock, FIONBIO, &non_blocking);

         int iLen = 0;
         int iCount = 0;
         int iTime = time(NULL);

         //give recv() 3 seconds max to timeout...
         const int TIMEOUT = 3;

         while(iLen <= 0 & iCount < iTime + TIMEOUT)
   		{
      		if(select(1, &readSet, NULL, NULL, &timeout))
      		{
               char szBuff[SIZE_RECV_BUFFER] = "";

         		iLen = recv(Sock, szBuff, SIZE_RECV_BUFFER, 0);

               if(iLen != -1)
               {
                  if(strlen(szBuff) > RESPONSE_HEADER_LEN)
                  {
                     stringstream response_code;
                     szBuff[RESPONSE_HEADER_LEN-1] = '\0';
                     response_code << szBuff;
      
                     if(response_code.str() == "SIP/2.0 200 OK")
                     {
                        cout << iLen << " bytes, recieved... OK" << endl;
                        break;
                     }
                  }
               }
            }

            iCount = time(NULL);
         }
      }
      else
      {
         cout << "message sending failed, error: " << WSAGetLastError() << endl;
      }
   }

	WSACleanup();

   return 0;
}
