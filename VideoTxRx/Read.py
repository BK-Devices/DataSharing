import cv2
import signal
import sys

# OUT_FILE = 'HTTP_OUT.avi'
OUT_FILE = 'HTTP_OUT.mp4'
fps = 25

vid = cv2.VideoCapture('http://172.17.212.16:5000/video')
# vid = cv2.VideoCapture(0)

# fourcc = cv2.VideoWriter_fourcc(*'XVID')
fourcc = cv2.VideoWriter_fourcc(*'X264')
out=None
flag=1

while True:
    ret,frame = vid.read()
    if not ret: break

    if flag:
        out=cv2.VideoWriter(OUT_FILE, fourcc, fps, (len(frame[0]), len(frame)))
        flag=0

    cv2.imshow("Video", frame)
    out.write(frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
            break

out.release()
vid.release()
cv2.destroyAllWindows()



# def signal_handler(sig, frame):
#     print('You pressed Ctrl+C!')
#     sys.exit(0)

# signal.signal(signal.SIGINT, signal_handler)