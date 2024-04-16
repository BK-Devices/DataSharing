#include "Header.h"

u32 preNavTime=0;
u32 RcvCnt=0, SndCnt=0;
u32 flag=0, alarmflag=0;
u32 mode=0, flagCnt=0;

// Output files file descriptor
u32 dlinkRawFD=0, ulinkRAWFD=0;
u32 batteryFD=0, posvelFD=0;
u32 strainFD=0, ratesFD=0;
u32 vib3FD=0, vib4FD=0;
u32 emaFD=0, ulinkFD=0;

// Serial Variables
u32 USB=0;
struct termios tty;
//memset (&tty, 0, sizeof(tty));
// struct termios tty={0};

// Ethernet variables
u32 SockFD;
struct sockaddr_in servAddr;

int main()
{
	printf("RS232/RS422 - 1     Ethernet - 2     Default is RS232/RS422\n");
	printf("Please enter the mode for receiving data : ");
	scanf("%d", &mode);

	if(mode!=2)
	{
		mode=1;
	}

	openFiles(); // Open output files
	signal(SIGALRM, sigalrm_handler); // Alarm handler for sending data
	
	if(mode==1)
	{
		configUsbPort(); // Configure USB Port
	}
	else
	{
		
	}
	
	printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Start !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

	while(1)
	{
		if(mode==1)
		{
			read(USB, &DLIN , sizeof(DLIN));
		}
		else
		{
			recv(sockD, &DLIN , sizeof(DLIN), 0); 
		}

		if(alarmflag==0 && DLIN.NavTime!=0)
		{
			sendData();
			ualarm(SEND_DELAY*1000);
			alarmflag=1;
		}
		
		if(DLIN.NavTime==preNavTime)
		{
			flagCnt++;
			if(flagCnt>=5)
			{
				if(prerNavTime!=0)
				{
					break;
				}
				flag=1;
			}
		}
		else
		{
			flag=0;
			flagCnt=0;
		}	

		if(flag==0)
		{
			// Print data
			// Save data in raw file
			// decode data
			// Save data in decoded file
			
			RcvCnt++;
		}
	}

	printf("\n\n");
	printf("Data Received %d times\n", RcvCnt);
	printf("Data Send %d times\n", SndCnt);
	
	printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Run Over !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
	return 0;

}


void sigalrm_handler(int sig)
{
	sendData();
	ualarm(SEND_DELAY*1000);
}

void sendData()
{
	//Load data in structure
	// save data in raw file
	// Save data in decoded file
	
	
	if(mode==1)
	{
		write(USB, &ULIN, sizeof(ULIN));
	}
	else
	{
		send(SockFD, &ULIN, sizeof(ULIN), 0);
	}

	SndCnt++;
}

void configUsbPort()
{
	USB = open(USBPORT, O_RDWR|O_NONBLOCK|O_NDELAY); // Open File Descriptor of serial port
	if(USB<0)
	{
		printf("Error %d opening %s: %s\n", errno, PORT, strerror(errno));
		exit(1);
	}

	if(tcgetattr(USB, &tty)!=0)
	{
		printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
	}

	// Set Baud Rate
	cfsetospeed(&tty, BAUD);
	cfsetispeed(&tty, BAUD);

	// Setting other Port Stuff
	tty.c_cflag     &=  ~PARENB;		// Make 8n1
	tty.c_cflag     &=  ~CSTOPB;
	tty.c_cflag     &=  ~CSIZE;
	tty.c_cflag     |=  CS8;
	tty.c_cflag     &=  ~CRTSCTS;		// no flow control
	tty.c_lflag     =   0;			// no signaling chars, no echo, no canonical processing
	tty.c_oflag     =   0;                  // no remapping, no delays
	tty.c_cc[VMIN]      =   0;		// read doesn't block
	tty.c_cc[VTIME]     =   5;		// 0.5 seconds read timeout

	tty.c_cflag     |=  CREAD | CLOCAL;			// turn on READ & ignore ctrl lines
	tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);		// turn off s/w flow ctrl
	tty.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG);	// make raw
	tty.c_oflag     &=  ~OPOST;				// make raw

	// Flush Port, then applies attributes 
	tcflush(USB, TCIFLUSH);
	if(tcsetattr(USB, TCSANOW, &tty) != 0)
	{
		printf("Error %d from tcsetattr\n", errno);
	}
}

void configSocket()
{
	SockFD = socket(AF_INET, SOCK_STREAM, 0);

	servAddr.sin_family = AF_INET; 
	servAddr.sin_port = htons(ServerPORT);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	int connectStatus = connect(SockFD, (struct sockaddr*)&servAddr, sizeof(servAddr));
}

void openFiles()
{
	// Opening output files
	dlinkRawFD = open(path1, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	ulinkRawFD = open(path2, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	batteryFD  = open(path3, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	posvelFD   = open(path4, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	strainFD   = open(path5, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	ratesFD    = open(path6, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	vib3FD     = open(path8, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	vib4FD     = open(path9, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	emaFD      = open(path7, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	ulinkFD    = open(path10, O_CREAT|O_TRUNC|O_WRONLY, 0664);
	
}






