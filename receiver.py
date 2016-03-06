#! /usr/bin/python
#usage command |--------->  python receiver.py <NAME_OF_FILE_TO_RECEIVE>

import socket      #import the socket library
import hashlib,sys,os

if len(sys.argv) > 3:
	print '\nWrong args. Usage: python receiver.py <NAME_OF_FILE_TO_RECEIVE> <PORT_NUMBER>\n'
	exit(1)
elif len(sys.argv) == 2:
	print '\nWrong args. Usage: python receiver.py <NAME_OF_FILE_TO_RECEIVE> <PORT_NUMBER>\n\n\tDefault port 64000 is used anyway'
	PORT = 64000 
#else:
#	PORT = sys.argv[2]    #arbitrary port not currently in use


HOST = '127.0.0.1'    #host
ADDR = (HOST,PORT)    #we need a tuple for the address
BUFSIZE = 4096    #reasonably sized buffer for data

# create a new socket object (serv)
try:
	serv = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
	serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)    
except socket.error:
		print 'Failed to create a socket'
		sys.exit(1)

#bind socket to the address
serv.bind((ADDR))    #the double parens are to create a tuple with one element
serv.listen(3)    #1 is the max number of queued connections allowed

print '\nListening...'
 
conn,addr = serv.accept() #accept the connection
print 'from client ' + str(addr)
print '...connected!'

def md5Checksum(filePath):
	f = open(filePath.name, 'r')#open file (received) and set read mode
	m = hashlib.md5()#assign haslib's md5 method to m
	while True:#loop to read all data
		data = f.read(BUFSIZE)
		if not data:
			break
		m.update(data)#parsing data
	return m.hexdigest()#and outputing it in hexadecimals

def recvall(the_socket):
    #the_socket.setblocking(0)
    total_data=[];data=''
    #begin=time.time()
    data=the_socket.recv(BUFSIZE)
    while data!='':
        #print data    
        total_data.append(data)
        data=the_socket.recv(BUFSIZE)
        if data=='':
			break
    result=''.join(total_data)
    return result

#start_time = time.time()#start to count time
data = recvall(conn)
conn.send('File Transfered')#send confirm to client

	

fileSent = open(sys.argv[1], 'wb')#create file and set write mode
fileSent.write(data)#write stream received inside the file
fileSent.close()#close file


conn.close()#close connection
print
print 'File Transfered..'
#print time.time() - start_time, 'seconds..'#print time elapsed
print 'to port: ', PORT
print
print 'Checksum of file is ', md5Checksum(fileSent)
print
print 'Size of data: ', os.path.getsize(sys.argv[1])
print

