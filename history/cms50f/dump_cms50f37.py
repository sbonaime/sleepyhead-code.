#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Contec CMS 50F firmware 3.7 dumper
# (C) 2014, François Revol <revol@free.fr>

#cf. http://elinux.org/Serial_port_programming

import serial
import sys
import time


def send_cmd(p, c):
	print "> %s" % c
	c = c.replace(' ', '')
	c = c.decode("hex")
	#print c.encode("hex")
	p.write(c)
	#p.flush()

def recv_data(p, l=None):
	data = ''
	while l is None or len(data) < l:
		time.sleep(0.5)
		if p.inWaiting() < 1:
			print "No more data..."
			# XXX: do something?
			if l is None:
				break
		want = max(p.inWaiting(), 1)
		if l is not None:
			want = min(want, l - len(data))
		# just for nicer hex dump, wrap at 8 bytes
		want = min(want, 8)
		#print len
		d = p.read(want)
		hx = d.encode("hex")
		print "< %s" % " ".join(hx[i:i+2] for i in range(0, len(hx), 2))
		data += d
	print
	return data

if len(sys.argv) < 2:
	print "Usage: %s serialdev" % sys.argv[0]
	exit(1)

#, parity=serial.PARITY_ODD
with serial.Serial(port=sys.argv[1], baudrate=115200) as p:

	send_cmd(p, '7d 81 a2 80 80 80 80 80 80   7d 81 a7 80 80 80 80 80 80   7d 81 a8 80 80 80 80 80 80   7d 81 a9 80 80 80 80 80 80   7d 81 aa 80 80 80 80 80 80   7d 81 b0 80 80 80 80 80 80')
	recv_data(p)



	send_cmd(p, '7d 81 a2 80 80 80 80 80 80')
	recv_data(p)
	send_cmd(p, '7d 81 a7 80 80 80 80 80 80')
	recv_data(p)
	send_cmd(p, '7d 81 a8 80 80 80 80 80 80')
	recv_data(p)
	send_cmd(p, '7d 81 a9 80 80 80 80 80 80')
	recv_data(p)
	send_cmd(p, '7d 81 aa 80 80 80 80 80 80')
	recv_data(p)
	send_cmd(p, '7d 81 b0 80 80 80 80 80 80')
	recv_data(p)


	print "------------"

	send_cmd(p, '7d 81 a7 80 80 80 80 80 80')
	recv_data(p, 2)
	send_cmd(p, '7d 81 a2 80 80 80 80 80 80')
	recv_data(p, 2)
	send_cmd(p, '7d 81 a0 80 80 80 80 80 80')
	recv_data(p, 4)

	send_cmd(p, '7d 81 b0 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 ac 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 b3 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 ad 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 a3 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 ab 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 a4 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 a5 80 80 80 80 80 80')
	d = recv_data(p)
	print "timestamp:"
	packet = d[0:8]
	if packet[0] == '\x07':
		print "%02d%02d-%02d-%02d" % (
			ord(packet[4]) & ~0x80,
			ord(packet[5]) & ~0x80,
			ord(packet[6]) & ~0x80,
			ord(packet[7]) & ~0x80)
	packet = d[8:16]
	if packet[0] == '\x12':
		print "%02d:%02d:%02d.%02d" % (
			ord(packet[4]) & ~0x80,
			ord(packet[5]) & ~0x80,
			ord(packet[6]) & ~0x80,
			ord(packet[7]) & ~0x80)

	send_cmd(p, '7d 81 af 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 a7 80 80 80 80 80 80')
	recv_data(p)

	send_cmd(p, '7d 81 a2 80 80 80 80 80 80')
	recv_data(p)


	send_cmd(p, '7d 81 a6 80 80 80 80 80 80')
	d = recv_data(p)
	print "data:"
	for i in range(0, len(d) / 8):
		packet = d[i*8:i*8+8]
		print "packet %d" % (i)
		if packet[0] != '\x0f':
			print "invalid data packet:"
			print packet.encode("hex")
			continue
		if packet[1] != '\x80':
			print "unknown code for packet: %02x" % ord(packet[1])

		print "%d%% %dbpm" % (ord(packet[2]) & ~0x80, ord(packet[3]) & ~0x80)
		print "%d%% %dbpm" % (ord(packet[4]) & ~0x80, ord(packet[5]) & ~0x80)
		print "%d%% %dbpm" % (ord(packet[6]) & ~0x80, ord(packet[7]) & ~0x80)

