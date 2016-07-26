require 'socket'

sock = UDPSocket.new
sock.send "Hello\0", 0, 'localhost', 3131
