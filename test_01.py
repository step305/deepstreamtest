import cv2

cam = cv2.VideoCapture('rtsp://192.168.0.22:8554/ds-test')

while cam:
	try:
		ret, frame = cam.read()
		if ret:
			cv2.imshow('vid', frame)
			if cv2.waitKey(1) == ord('q'):
				break
	except:
		break
cam.release()
cv2.destroyAllWindows()

