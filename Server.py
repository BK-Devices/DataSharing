import cv2
import time
import socket
import base64
import imutils

SIZE = 65536
WIDTH = 400
PORT = 8080
IP = '172.17.212.16'
FPS = 25 #29.97
DLY = 1000/FPS

if __name__=="__main__":
	serSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	serSock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, SIZE)
	serSock.bind((IP, PORT))

	print('Listening at:', (IP, PORT))

	vid = cv2.VideoCapture(0) #  replace 'Video.mp4' with 0 for webcam

	# Receiving request
	while True:
		msg, cliAddr = serSock.recvfrom(SIZE)
		print('Got connection from: ', cliAddr)
		if msg==b'Send Video': break
	
	serSock.sendto(str(FPS).encode(), cliAddr)
	
	start=time.time()

	while(vid.isOpened()):
		ret,frame = vid.read()
		if not ret: break

		#frame = imutils.resize(frame, width=WIDTH)
		encoded, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 60])
		msg = base64.b64encode(buffer)

		serSock.sendto(msg, cliAddr)

		key = cv2.waitKey(1) & 0xFF
		if key == ord('q'):
			serSock.close()
			break

		while DLY>((time.time()-start)*1000): pass
		start=time.time()
	
	serSock.close()
	vid.release()
