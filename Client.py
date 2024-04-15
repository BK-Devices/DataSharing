import cv2
import socket
import base64
import imutils
import numpy as np

SIZE = 65536
WIDTH = 800
PORT = 8080
IP = '172.17.212.16'
OUT_FILE = 'Output.avi' # For fourcc = XVID
# OUT_FILE = 'Output.mp4' # For fourcc = X264

if __name__=="__main__":
	cliSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	cliSock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, SIZE)
	cliSock.connect((IP, PORT))

	print('Connected to:', (IP, PORT))

	# requesting video
	cliSock.sendto(b'Send Video', (IP, PORT))

	# fps received
	packet,_ = cliSock.recvfrom(SIZE)
	fps=float(packet.decode())

	fourcc = cv2.VideoWriter_fourcc(*'XVID')
	# fourcc = cv2.VideoWriter_fourcc(*'X264')
	out=None
	flag=1

	cliSock.settimeout(3)
	while True:
		try:
			packet,_ = cliSock.recvfrom(SIZE)
		except TimeoutError:
			break

		data = base64.b64decode(packet,' /')
		# npdata = np.fromstring(data,dtype=np.uint8)
		npdata = np.frombuffer(data,dtype=np.uint8)
		frame = cv2.imdecode(npdata,1)

		# frame = imutils.resize(frame, width=WIDTH)
		cv2.imshow("RECEIVING VIDEO",frame)

		if flag:
			out=cv2.VideoWriter(OUT_FILE, fourcc, fps, (len(frame[0]), len(frame)))
			flag=0
		
		out.write(frame)

		key = cv2.waitKey(1) & 0xFF
		if key == ord('q'):
			cliSock.close()
			break

	cliSock.close()
	out.release()