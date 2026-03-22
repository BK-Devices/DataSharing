// ============================================================================
// Name		: VideoStreamer.h
// Date		: Mar 18, 2026
// ============================================================================


#ifndef VIDEOSTREAMER_H_
#define VIDEOSTREAMER_H_


// ---------- Header Inclusion ----------
#include <vector>
#include <iostream>

#include <unistd.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <opencv2/opencv.hpp>


// ---------- Namespace ----------
using namespace std;
using namespace cv;


// ---------- Defines ----------
#define  StreamerID    0xAAU
#define  ReceiverID    0xBBU

#define  ConnectID     0x11U
#define  VideoDataID   0x12U
#define  HealthID      0x13U

#define  _Size_63KB_   (63U * 1024U)
#define  _Size_64KB_   (64U * 1024U)

#define  ClientTimeOut  10000
#define  VideoTimeOut   15000


// ---------- Enums ----------
enum ImgColour  {Gray = 0, Bgr = 1};
enum StreamIDs  {S1 = 0, S2 = 1, S3 = 3};
enum ImgQuality {Q1 = 65, Q2 = 60, Q3 = 55, Q4 = 50, Q5 = 45, Q6 = 40, Q7 = 35, Q8 = 30};



// ---------- Class Declaration ----------
class VideoStreamer
{
private:
	// Server Details
	bool ServerStatus;

	int32_t RecvSock, SendSock;
	uint16_t RecvPort, SendPort;

	uint32_t SockLen;
	struct sockaddr_in Recv_SAddr, Recv_CAddr;
	struct sockaddr_in Send_SAddr, Send_CAddr;

	// Thread Details
	pthread_mutex_t UdpLock;
	pthread_t ServerManagerThread;
	pthread_t StreamVideoFramesThread;

	// Structures & Union
	struct _Streams_;
	union _VideoPacket_;
	union _VideoCmds_;
	union _VideoResp_;

	// Structures-Union Variables
	_Streams_ *Streams[3];
	_VideoPacket_ *VideoPacket;
	_VideoCmds_ *VideoCmds;
	_VideoResp_ *VideoResp;


	// Release UDP Port
	void releaseUDPPort(uint16_t Port);

	// Connect Clients
	void connectClients(sockaddr_in CAddr);

	// Client Health
	void clientHealth(sockaddr_in CAddr);

	// Server Manager
	void serverManager(void);

	// Stream Video Frames
	void streamVideoFrames(void);

	// Server Manager Thread
	static void *serverManagerThread(void *args);

	// Stream Video Frames Thread
	static void *streamVideoFramesThread(void *args);


public:
	// Constructor
	VideoStreamer(void);

	// Destructor
	~VideoStreamer(void);

	// Start Stream Server
	uint8_t startStreamServer(uint16_t ServerPort = 4774);

	// Set Stream Parameters
	uint8_t setStreamParam(enum StreamIDs ID, Size VideoSize, uint8_t VideoFps, enum ImgColour Colour = Gray, enum ImgQuality Quality = Q8);

	// Clear Stream Parameters
	void clearStreamParam(enum StreamIDs ID);

	// Stream Video
	uint8_t streamVideo(enum StreamIDs ID, Mat &VideoFrame);

	// Stop Stream Server
	void stopStreamServer(void);

};


#endif /* VIDEOSTREAMER_H_ */




