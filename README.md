Project for the Netrworks course.
"Ritardatore" is provided by the teacher. It is a sort of simulator of the internet.


|----------|                                  |----------|
|	FILE   |                                  |   FILE   |
|----------|                                  |----------!
 	 |                                              ^
	 |MD5sum                                        |MD5sum check
	 V                                              |
  sender.py                                      receiver.py
	 |                                              ^
TCP  | send file to                            TCP  | send file to
	 V                                              |
               UDP                   UDP   
ProxySender.c -------> Ritardatore ---------> ProxyReceiver.c
