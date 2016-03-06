#! /usr/bin/python
#usage command |--------->  python sender.py <FILE_TO_TRANSFER> <HOST_NUMBER>

import socket
import hashlib,sys,os

if len(sys.argv) == 2:
         print '\nWrong args. Usage: python sender.py <FILE_TO_TRANSFER> <HOST_NUMBER>\n\n\tDefault <HOST_NUMBER> = 127.0.0.1\n'
	 HOST = '127.0.0.1'
elif len(sys.argv) == 1:
         print '\nWrong args. Usage: python sender.py <FILE_TO_TRANSFER> <HOST_NUMBER>\nAt least <FILE_TO_TRANSFER>'
	 print
	 exit(1)
elif len(sys.argv) > 3:
	print '\nWrong args. Usage: python sender.py <FILE_TO_TRANSFER> <HOST_NUMBER>\n'
	print
	exit(1)
else:
	HOST = sys.argv[2]

PORT = 59000 
ADDR = (HOST,PORT)
BUFSIZE = 4096
FILE = sys.argv[1]

# create a new socket object (cli)
try:
	cli = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
except socket.error:
		print 'Failed to create a socket'
		sys.exit(1)

#connect socket to the address
cli.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
cli.connect((ADDR))

def md5Checksum(filePath):
    f = open(filePath, 'r')#open file (to be sent) and set read mode
    m = hashlib.md5()#assign haslib's md5 method to m
    while True:#loop to read all data
        data = f.read(BUFSIZE)
        if not data:
            break
        m.update(data)#parsing data
    return m.hexdigest()#and outputing it in hexadecimals

print "\nThe md5checksum of your file is", md5Checksum(FILE)
print
print
fileBuff = open(FILE, 'r')#open file and set read mode
fileToSend = fileBuff.read()#read all and save it in variable
fileBuff.close()#close file

cli.send(fileToSend)#send file to server

#data = cli.recv(BUFSIZE)#recive response from client
#print
#print data
#print 
print 'Size of data: ', os.path.getsize(sys.argv[1])
cli.close()#close connection
