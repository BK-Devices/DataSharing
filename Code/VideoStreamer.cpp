// ============================================================================
// Name		: VideoStreamer.cpp
// Date		: Mar 18, 2026
// ============================================================================


// ---------- Header Inclusion ----------
#include "VideoStreamer.h"


#pragma pack(1)

// ---------- Streams & Clients Data ----------
struct VideoStreamer :: _Streams_
{
	bool StreamStatus;

	Size VideoSize;
	uint8_t VideoFps;
	enum ImgColour Colour;
	enum ImgQuality Quality;

	Mat VideoFrame;
	bool NewFrameFlag;
	uint32_t FrameNo;
	struct timeval StreamTime;
	pthread_mutex_t VideoFrameLock;

	struct _Clients_
	{
		bool ClientStatus;

		int8_t IPAddr[16];
		uint16_t RecvPort;
		uint16_t SendPort;

		struct sockaddr_in CAddr;
		struct timeval HealthTime;

		_Clients_()
		{
			ClientStatus = false;
			RecvPort = -1;
			SendPort = -1;
		}

	} Clients[3];

	_Streams_()
	{
		StreamStatus = false;
		VideoFps = 0;
		Colour = Gray;
		Quality = Q8;
		NewFrameFlag = false;
		FrameNo = 0;
	}

	~_Streams_()
	{
		VideoFrame.release();
		pthread_mutex_destroy(&VideoFrameLock);
	}
};


// ---------- Video Packet ----------
union VideoStreamer :: _VideoPacket_
{
	uint8_t DataBuff[_Size_64KB_];

	struct _VideoData_
	{
		uint8_t Header[3];
		uint16_t DataLen;

		uint16_t ServerPort;
		uint8_t StreamID;

		uint32_t FrameNo;
		uint8_t PacketCnt;
		uint8_t PacketNo;

		uint8_t VideoData[_Size_63KB_];

		uint8_t CheckSum;
	} VideoData;
};


// ---------- Video Commands ----------
union VideoStreamer :: _VideoCmds_
{
	uint8_t DataBuff[40];

	struct _HealthData_
	{
		uint8_t Header[3];
		uint16_t DataLen;

		uint8_t StreamID[3];
		uint16_t CliPort;

		uint8_t CheckSum;
	} HealthData;

	struct _ConnectCmd_
	{
		uint8_t Header[3];
		uint16_t DataLen;

		uint8_t StreamID;
		uint16_t CliPort;

		uint8_t CheckSum;
	} ConnectCmd;
};


// ---------- Video Commands ----------
union VideoStreamer :: _VideoResp_
{
	uint8_t DataBuff[40];

	struct _ConnectResp_
	{
		uint8_t Header[3];
		uint16_t DataLen;

		uint16_t ServerPort;
		uint8_t StreamID;
		uint8_t CmdAck[2];

		uint16_t VideoSize[2];
		uint8_t VideoFps;
		uint8_t Colour;

		uint8_t CheckSum;
	} ConnectResp;
};

#pragma pack()


// ---------- Release UDP Port ----------
void VideoStreamer :: releaseUDPPort(uint16_t Port)
{
	string Command = "";

	Command = "fuser -k " + to_string(Port) + "/udp >/dev/null 2>&1";
	system(Command.c_str());

	usleep(10 * 1000);

	return;
}

// ---------- Connect Clients ----------
void VideoStreamer :: connectClients(sockaddr_in CAddr)
{
	uint8_t IPAddr[16];
	uint16_t CliPort = 0;
	uint8_t ID = 0, CL = 0, Ind = 0;

	struct timespec ts1;
	ts1.tv_sec = 0;				// seconds
	ts1.tv_nsec = 1000 * 5;		// 5 microseconds


	// Extract Client IPAddr & Port No
	CliPort = ntohs(CAddr.sin_port);
	inet_ntop(AF_INET, &CAddr.sin_addr, (char *)IPAddr, sizeof(IPAddr));

	memset((void *)VideoResp->DataBuff, 0, sizeof(VideoResp->DataBuff));

	VideoResp->ConnectResp.Header[0] = StreamerID;
	VideoResp->ConnectResp.Header[1] = ReceiverID;
	VideoResp->ConnectResp.Header[2] = ConnectID;

	VideoResp->ConnectResp.DataLen = sizeof(VideoResp->ConnectResp) - 6U;

	VideoResp->ConnectResp.ServerPort = RecvPort;
	VideoResp->ConnectResp.StreamID = VideoCmds->ConnectCmd.StreamID;

	// Invalid Parameters
	if(VideoCmds->ConnectCmd.StreamID != S1 && VideoCmds->ConnectCmd.StreamID != S2 && VideoCmds->ConnectCmd.StreamID != S3)
	{
		VideoResp->ConnectResp.CmdAck[0] = 0xB2U;
		VideoResp->ConnectResp.CmdAck[1] = 0x01U;

		pthread_mutex_lock(&UdpLock);
		CAddr.sin_port = htons(VideoCmds->ConnectCmd.CliPort);
		sendto(SendSock, (void *)VideoResp->DataBuff, sizeof(VideoResp->ConnectResp), 0, (sockaddr *)&CAddr, SockLen);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
		pthread_mutex_unlock(&UdpLock);

		return;
	}

	ID = VideoCmds->ConnectCmd.StreamID;

	// Stream Inactive
	if(Streams[ID]->StreamStatus == false)
	{
		VideoResp->ConnectResp.CmdAck[0] = 0xB2U;
		VideoResp->ConnectResp.CmdAck[1] = 0x02U;

		pthread_mutex_lock(&UdpLock);
		CAddr.sin_port = htons(VideoCmds->ConnectCmd.CliPort);
		sendto(SendSock, (void *)VideoResp->DataBuff, sizeof(VideoResp->ConnectResp), 0, (sockaddr *)&CAddr, SockLen);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
		pthread_mutex_unlock(&UdpLock);

		return;
	}

	Ind = 3;

	for(CL = 0; CL < 3; CL++)
	{
		if(Streams[ID]->Clients[CL].ClientStatus == false)
		{
			Ind = CL;
			continue;
		}

		if(strcmp((char *)IPAddr, (char *)Streams[ID]->Clients[CL].IPAddr) == 0 &&
			Streams[ID]->Clients[CL].SendPort == VideoCmds->ConnectCmd.CliPort &&
			Streams[ID]->Clients[CL].RecvPort == CliPort)
		{
			Ind = CL;
			break;
		}
	}

	// Client Full
	if(Ind == 3)
	{
		VideoResp->ConnectResp.CmdAck[0] = 0xB2U;
		VideoResp->ConnectResp.CmdAck[1] = 0x03U;

		pthread_mutex_lock(&UdpLock);
		CAddr.sin_port = htons(VideoCmds->ConnectCmd.CliPort);
		sendto(SendSock, (void *)VideoResp->DataBuff, sizeof(VideoResp->ConnectResp), 0, (sockaddr *)&CAddr, SockLen);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
		pthread_mutex_unlock(&UdpLock);

		return;
	}

	// Connect or Overwrite Client
	memcpy((void *)Streams[ID]->Clients[CL].IPAddr, (void *)IPAddr, 16);
	Streams[ID]->Clients[CL].SendPort = VideoCmds->ConnectCmd.CliPort;
	Streams[ID]->Clients[CL].RecvPort = CliPort;

	Streams[ID]->Clients[CL].CAddr = CAddr;
	Streams[ID]->Clients[CL].CAddr.sin_port = htons(Streams[ID]->Clients[CL].SendPort);
	gettimeofday(&Streams[ID]->Clients[CL].HealthTime, NULL);

	Streams[ID]->Clients[CL].ClientStatus = true;

	VideoResp->ConnectResp.CmdAck[0] = 0xE5U;
	VideoResp->ConnectResp.CmdAck[1] = 0x00U;

	VideoResp->ConnectResp.VideoSize[0] = Streams[ID]->VideoSize.width;
	VideoResp->ConnectResp.VideoSize[1] = Streams[ID]->VideoSize.height;
	VideoResp->ConnectResp.VideoFps = Streams[ID]->VideoFps;
	VideoResp->ConnectResp.Colour = (uint8_t)Streams[ID]->Colour;

	pthread_mutex_lock(&UdpLock);
	sendto(SendSock, (void *)VideoResp->DataBuff, sizeof(VideoResp->ConnectResp), 0, (sockaddr *)&Streams[ID]->Clients[CL].CAddr, SockLen);
	clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
	pthread_mutex_unlock(&UdpLock);

	return;
}


// ---------- Client Health ----------
void VideoStreamer :: clientHealth(sockaddr_in CAddr)
{
	uint8_t IPAddr[16];
	uint16_t CliPort = 0;
	uint8_t ID = 0, CL = 0;

	// Extract Client IPAddr & Port No
	CliPort = ntohs(CAddr.sin_port);
	inet_ntop(AF_INET, &CAddr.sin_addr, (char *)IPAddr, sizeof(IPAddr));

	for(ID = 0; ID < 3; ID++)
	{
		if(VideoCmds->HealthData.StreamID[ID] == 0U)
		{
			continue;
		}

		if(Streams[ID]->StreamStatus == false)
		{
			continue;
		}

		for(CL = 0; CL < 3; CL++)
		{
			if(Streams[ID]->Clients[CL].ClientStatus == false)
			{
				continue;
			}

			if(strcmp((char *)IPAddr, (char *)Streams[ID]->Clients[CL].IPAddr) == 0 &&
				Streams[ID]->Clients[CL].SendPort == VideoCmds->HealthData.CliPort &&
				Streams[ID]->Clients[CL].RecvPort == CliPort)
			{
				gettimeofday(&Streams[ID]->Clients[CL].HealthTime, NULL);
				break;
			}
		}
	}

	return;
}


// ---------- Server Manager ----------
void VideoStreamer :: serverManager(void)
{
	int32_t ePollFD;
	epoll_event event;

	int64_t timeCount = 0;
	uint8_t ID = 0, CL = 0;
	int32_t RetVal = -1, nfds = -1;

	struct timespec ts1;
	struct timeval CurrTime;

	ts1.tv_sec = 0;				// seconds
	ts1.tv_nsec = 1000 * 5;		// 5 microseconds

	ePollFD = epoll_create1(0);
	event.events = EPOLLIN;
	event.data.fd = RecvSock;

	// Empty the socket first
	while(ServerStatus == true)
	{
		RetVal = recvfrom(RecvSock, VideoCmds->DataBuff, sizeof(VideoCmds->DataBuff), MSG_DONTWAIT, nullptr, nullptr);
		if(RetVal <= 0)
		{
			break;
		}
	}

	// Register ePoll Event
	epoll_ctl(ePollFD, EPOLL_CTL_ADD, RecvSock, &event);

	while(ServerStatus == true)
	{
		clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);

		nfds = epoll_wait(ePollFD, &event, 1, 2000);
		if(nfds <= 0)
		{
			gettimeofday(&CurrTime, NULL);

			for(ID = 0; ID < 3; ID++)
			{
				if(Streams[ID]->StreamStatus == false)
				{
					continue;
				}

				for(CL = 0; CL < 3; CL++)
				{
					if(Streams[ID]->Clients[CL].ClientStatus == false)
					{
						continue;
					}

					// Calculate the execution time in milliseconds
					timeCount = ((CurrTime.tv_sec - Streams[ID]->Clients[CL].HealthTime.tv_sec) * 1000L) +
								((CurrTime.tv_usec - Streams[ID]->Clients[CL].HealthTime.tv_usec) / 1000L);

					if(CurrTime.tv_usec < Streams[ID]->Clients[CL].HealthTime.tv_usec)
					{
						timeCount -= 1;  // adjust for borrow
					}

					// Disconnect Client
					if(timeCount >= ClientTimeOut)
					{
						Streams[ID]->Clients[CL].ClientStatus = false;
						memset((void *)&Streams[ID]->Clients[CL], 0, sizeof(Streams[ID]->Clients[CL]));
					}
				}
			}

			continue;
		}

		memset((void *)VideoCmds->DataBuff, 0, sizeof(VideoCmds->DataBuff));
		RetVal = recvfrom(RecvSock, VideoCmds->DataBuff, sizeof(VideoCmds->DataBuff), MSG_DONTWAIT, (sockaddr *)&Recv_CAddr, &SockLen);
		if(RetVal <= 0)
		{
			continue;
		}

		if(VideoCmds->DataBuff[0] != ReceiverID || VideoCmds->DataBuff[1] != StreamerID)
		{
			continue;
		}

		if(VideoCmds->DataBuff[2] == ConnectID)
		{
			connectClients(Recv_CAddr);
		}
		else if(VideoCmds->DataBuff[2] == HealthID)
		{
			clientHealth(Recv_CAddr);
		}
		else
		{

		}
	}

	return;
}


// ---------- Stream Video Frames ----------
void VideoStreamer :: streamVideoFrames(void)
{
	uint8_t ID = 0;
	int64_t timeCount = 0;

	struct timespec ts1, ts2;
	struct timeval CurrTime;

	vector<uint8_t> FrameBuff;
	uint8_t PacketNo = 0, PacketCnt = 0;
	uint32_t FrameLength = 0, PacketStart = 0, PacketSize = 0;

	ts1.tv_sec = 0;				// seconds
	ts1.tv_nsec = 1000 * 5;		// 5 microseconds

	ts2.tv_sec = 0;				// seconds
	ts2.tv_nsec = 1000 * 500;	// 500 microseconds

	while(ServerStatus == true)
	{
		for(ID = 0; ID < 3; ID++)
		{
			clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);

			if(Streams[ID]->StreamStatus == false)
			{
				continue;
			}

			// Close Stream if timeout of 15 Sec
			if(Streams[ID]->NewFrameFlag == false)
			{
				gettimeofday(&CurrTime, NULL);

				// Calculate the execution time in milliseconds
				timeCount = ((CurrTime.tv_sec - Streams[ID]->StreamTime.tv_sec) * 1000L) +
						    ((CurrTime.tv_usec - Streams[ID]->StreamTime.tv_usec) / 1000L);

				if(CurrTime.tv_usec < Streams[ID]->StreamTime.tv_usec)
				{
					timeCount -= 1;  // adjust for borrow
				}

				// DisActive VideoStream
				if(timeCount >= VideoTimeOut)
				{
					clearStreamParam((enum StreamIDs)ID);
				}

				continue;
			}

			if(Streams[ID]->Clients[0].ClientStatus == false && Streams[ID]->Clients[1].ClientStatus == false && Streams[ID]->Clients[2].ClientStatus == false)
			{
				Streams[ID]->VideoFrame.release();
				Streams[ID]->NewFrameFlag = false;
				continue;
			}

			pthread_mutex_lock(&Streams[ID]->VideoFrameLock);

			// Resizing Image
			if((Streams[ID]->VideoSize.width < Streams[ID]->VideoFrame.cols) || (Streams[ID]->VideoSize.height < Streams[ID]->VideoFrame.rows))
			{
				resize(Streams[ID]->VideoFrame, Streams[ID]->VideoFrame, Streams[ID]->VideoSize, 0, 0, INTER_AREA);
			}
			else
			{
				resize(Streams[ID]->VideoFrame, Streams[ID]->VideoFrame, Streams[ID]->VideoSize, 0, 0, INTER_LINEAR);
			}

			// Colour Conversion
			if(Streams[ID]->Colour == Gray && Streams[ID]->VideoFrame.channels() == 3)
			{
				cvtColor(Streams[ID]->VideoFrame, Streams[ID]->VideoFrame, COLOR_BGR2GRAY);
			}
			else if(Streams[ID]->Colour == Bgr && Streams[ID]->VideoFrame.channels() == 1)
			{
				cvtColor(Streams[ID]->VideoFrame, Streams[ID]->VideoFrame, COLOR_GRAY2BGR);
			}
			else
			{

			}

			PacketNo = 0;
			PacketCnt = 0;
			PacketSize = 0;
			PacketStart = 0;
			FrameLength = 0;

			// Compress VideoFrame
			imencode(".jpeg", Streams[ID]->VideoFrame, FrameBuff, {IMWRITE_JPEG_QUALITY, Streams[ID]->Quality});
			FrameLength = FrameBuff.size();
			PacketCnt = (FrameLength + (_Size_63KB_ - 1)) / _Size_63KB_;

			// Clear Video Frame
			Streams[ID]->VideoFrame.release();
			Streams[ID]->NewFrameFlag = false;

			pthread_mutex_unlock(&Streams[ID]->VideoFrameLock);

			// Split Video data in packets and sent to clients
			for(PacketNo = 0; PacketNo < PacketCnt; PacketNo++)
			{
				memset((void *)VideoPacket->DataBuff, 0, _Size_64KB_);

				// Split The VideoFrame data
				PacketStart = PacketNo * _Size_63KB_;
				PacketSize = min(PacketStart + _Size_63KB_, FrameLength) - PacketStart;

				VideoPacket->VideoData.Header[0] = StreamerID;
				VideoPacket->VideoData.Header[2] = ReceiverID;
				VideoPacket->VideoData.Header[3] = VideoDataID;

				VideoPacket->VideoData.ServerPort = (uint16_t)RecvPort;
				VideoPacket->VideoData.StreamID = ID;

				VideoPacket->VideoData.FrameNo = Streams[ID]->FrameNo;
				VideoPacket->VideoData.PacketCnt = PacketCnt;
				VideoPacket->VideoData.PacketNo = PacketNo;

				memcpy((void *)VideoPacket->VideoData.VideoData, (void *)(FrameBuff.data()+PacketStart), PacketSize);

				VideoPacket->VideoData.CheckSum = 0x00U;
				VideoPacket->VideoData.DataLen = PacketSize + (15U - 6U);

				pthread_mutex_lock(&UdpLock);

				if(Streams[ID]->Clients[0].ClientStatus == true)
				{
					sendto(SendSock, (void *)VideoPacket->DataBuff, (PacketSize + 15U), 0, (sockaddr *)&Streams[ID]->Clients[0].CAddr, SockLen);
					clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
				}

				if(Streams[ID]->Clients[1].ClientStatus == true)
				{
					sendto(SendSock, (void *)VideoPacket->DataBuff, (PacketSize + 15U), 0, (sockaddr *)&Streams[ID]->Clients[1].CAddr, SockLen);
					clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
				}

				if(Streams[ID]->Clients[2].ClientStatus == true)
				{
					sendto(SendSock, (void *)VideoPacket->DataBuff, (PacketSize + 15U), 0, (sockaddr *)&Streams[ID]->Clients[2].CAddr, SockLen);
					clock_nanosleep(CLOCK_MONOTONIC, 0, &ts1, nullptr);
				}

				pthread_mutex_unlock(&UdpLock);

				cout << "Frame Sent ID-" << (uint32_t)ID << " F-" << Streams[ID]->FrameNo << " PC-" << (uint32_t)PacketCnt << " PN-" <<
				(uint32_t)(PacketNo + 1U) << " FL-" << FrameLength << " PL-" << (PacketSize + 15U) << " to all clients..." << endl << flush;

				clock_nanosleep(CLOCK_MONOTONIC, 0, &ts2, nullptr);
			}

			Streams[ID]->FrameNo++;

			FrameBuff.clear();
			FrameBuff.shrink_to_fit();
		}
	}

	return;
}


// ---------- Server Manager Thread ----------
void * VideoStreamer :: serverManagerThread(void *args)
{
	// Enable deferred cancellation and set type to deferred
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

	VideoStreamer *This = static_cast<VideoStreamer *>(args);

	This->serverManager();

	pthread_exit(NULL);
	return nullptr;
}


// ---------- Stream Video Frames Thread ----------
void * VideoStreamer :: streamVideoFramesThread(void *args)
{
	// Enable deferred cancellation and set type to deferred
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

	VideoStreamer *This = static_cast<VideoStreamer *>(args);

	This->streamVideoFrames();

	pthread_exit(NULL);
	return nullptr;
}


// ---------- Constructor ----------
VideoStreamer :: VideoStreamer(void)
{
	// Server Details
	ServerStatus = false;

	RecvSock = -1;
	SendSock = -1;
	RecvPort = -1;
	SendPort = -1;

	SockLen = sizeof(Recv_SAddr);

	// Thread Details
	ServerManagerThread = -1;
	StreamVideoFramesThread = -1;

	Streams[0] = new _Streams_;
	Streams[1] = new _Streams_;
	Streams[2] = new _Streams_;
	VideoPacket = new _VideoPacket_;
	VideoCmds = new _VideoCmds_;
	VideoResp = new _VideoResp_;

	return;
}


// ---------- Destructor ----------
VideoStreamer :: ~VideoStreamer(void)
{
	// Stop Stream Server
	stopStreamServer();

	delete Streams[0];
	delete Streams[1];
	delete Streams[2];
	delete VideoPacket;
	delete VideoCmds;
	delete VideoResp;

	return;
}


// ---------- Start Stream Server ----------
uint8_t VideoStreamer :: startStreamServer(uint16_t ServerPort)
{
	if(ServerStatus == true)
	{
		cout << "Error : Stream Server is Already Running..." << endl << flush;
		return 1U;
	}

	RecvPort = ServerPort;

	RecvSock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	SendSock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

	if(RecvSock < 0 || SendSock < 0)
	{
		cout << "Error : Socket Creation Failed..." << endl << flush;
		return 2U;
	}

	SockLen = sizeof(Recv_SAddr);

	memset((void *)&Recv_SAddr, 0, SockLen);
	memset((void *)&Recv_CAddr, 0, SockLen);
	memset((void *)&Send_SAddr, 0, SockLen);
	memset((void *)&Send_CAddr, 0, SockLen);


	Recv_SAddr.sin_family = AF_INET;
	Recv_SAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	Recv_SAddr.sin_port = htons(RecvPort);

	Send_SAddr.sin_family = AF_INET;
	Send_SAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	Send_SAddr.sin_port = 0;

	releaseUDPPort(RecvPort);

	if(bind(RecvSock, (struct sockaddr*)&Recv_SAddr, SockLen) < 0 ||
	   bind(SendSock, (struct sockaddr*)&Send_SAddr, SockLen) < 0)
	{
		cout << "Error : Socket Binding Failed..." << endl << flush;
		return 3U;
	}

	SendPort = ntohs(Send_SAddr.sin_port);
	ServerStatus = true;

	pthread_mutex_init(&UdpLock, NULL);

	if(pthread_create(&ServerManagerThread, NULL, this->serverManagerThread, this) != 0 ||
	   pthread_create(&StreamVideoFramesThread, NULL, this->streamVideoFramesThread, this) != 0)
	{
		ServerStatus = false;

		cout << "Error : Thread Creation Failed..." << endl << flush;
		return 4U;
	}

	cout << "Video Streamer Started on '0.0.0.0:" << RecvPort << " ..." << endl << flush;

	return 0U;
}


// ---------- Set Stream Parameters ----------
uint8_t VideoStreamer :: setStreamParam(enum StreamIDs ID, Size VideoSize, uint8_t VideoFps, enum ImgColour Colour, enum ImgQuality Quality)
{
	if(ServerStatus == false)
	{
		cout << "Error : Stream Server is Not Running..." << endl << flush;
		return 1U;
	}

	if((S1 != ID && S2 != ID && S3 != ID) || (Colour != Gray && Colour != Bgr) || (Q1 != Quality && Q2 != Quality
		&& Q3 != Quality && Q4 != Quality && Q5 != Quality && Q6 != Quality && Q7 != Quality && Q8 != Quality))
	{
		cout << "Error : Wrong Parameters..." << endl << flush;
		return 2U;
	}

	if(Streams[ID]->StreamStatus == true)
	{
		cout << "Error : Stream is Already Active..." << endl << flush;
		return 3U;
	}

	pthread_mutex_init(&Streams[ID]->VideoFrameLock, NULL);

	Streams[ID]->VideoSize = VideoSize;
	Streams[ID]->VideoFps = VideoFps;
	Streams[ID]->Colour = Colour;
	Streams[ID]->Quality = Quality;

	Streams[ID]->StreamStatus = true;

	return 0U;
}


// ---------- Clear Stream Parameters ----------
void VideoStreamer :: clearStreamParam(enum StreamIDs ID)
{
	if(ServerStatus == false)
	{
		return;
	}

	if(S1 != ID && S2 != ID && S3 != ID)
	{
		return;
	}

	if(Streams[ID]->StreamStatus == false)
	{
		return;
	}

	Streams[ID]->StreamStatus = false;
	Streams[ID]->VideoFrame.release();
	pthread_mutex_destroy(&Streams[ID]->VideoFrameLock);

	memset((void *)&Streams[ID], 0, sizeof(Streams[ID]));

	return;
}


// ---------- Stream Video ----------
uint8_t VideoStreamer :: streamVideo(enum StreamIDs ID, Mat &VideoFrame)
{
	if(ServerStatus == false)
	{
		cout << "Error : Stream Server is Not Running..." << endl << flush;
		return 1U;
	}

	if((S1 != ID && S2 != ID && S3 != ID) || (VideoFrame.cols == 0 || VideoFrame.rows == 0))
	{
		cout << "Error : Wrong Parameters..." << endl << flush;
		return 2U;
	}

	if(Streams[ID]->StreamStatus == false)
	{
		cout << "Error : Stream is Not Active..." << endl << flush;
		return 3U;
	}

	pthread_mutex_lock(&Streams[ID]->VideoFrameLock);

	if(Streams[ID]->NewFrameFlag == true)
	{
		Streams[ID]->NewFrameFlag = false;
		Streams[ID]->VideoFrame.release();
	}

	Streams[ID]->VideoFrame = VideoFrame.clone();
	gettimeofday(&Streams[ID]->StreamTime, NULL);
	Streams[ID]->NewFrameFlag = true;

	pthread_mutex_unlock(&Streams[ID]->VideoFrameLock);

	return 0U;
}


// ---------- Stop Stream Server ----------
void VideoStreamer :: stopStreamServer(void)
{
	if(ServerStatus == false)
	{
		return;
	}

	ServerStatus = false;

	pthread_mutex_destroy(&UdpLock);
	pthread_mutex_destroy(&Streams[0]->VideoFrameLock);
	pthread_mutex_destroy(&Streams[1]->VideoFrameLock);
	pthread_mutex_destroy(&Streams[2]->VideoFrameLock);

	Streams[0]->VideoFrame.release();
	Streams[1]->VideoFrame.release();
	Streams[2]->VideoFrame.release();

	close(RecvSock);
	close(SendSock);

	pthread_cancel(ServerManagerThread);
	pthread_cancel(StreamVideoFramesThread);

	memset((void *)Streams, 0, sizeof(Streams));

	return;
}




