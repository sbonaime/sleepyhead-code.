#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Contec CMS 50F firmware 3.7 sample duration test


data = [
dict(a='08 80 80 80 a4 80 80 80', l=6),
dict(a='08 80 80 80 d2 80 80 80', l=14),
dict(a='08 80 80 80 c8 80 80 80', l=12),
dict(a='08 84 80 80 ca 80 80 80', l=34),
dict(a='08 80 80 80 a2 80 80 80', l=6),
dict(a='08 80 80 80 a4 81 80 80', l=49),
dict(a='08 84 80 80 98 80 80 80', l=26),
dict(a='08 84 80 80 c2 80 80 80', l=33),
dict(a='08 84 80 80 d2 84 80 80', l=206),
dict(a='08 84 80 80 aa 81 80 80', l=71),
dict(a='08 80 80 80 90 80 80 80', l=3),
dict(a='08 84 80 80 84 80 80 80', l=22),
dict(a='08 84 80 80 c0 93 80 80', l=843),

dict(a='08 80 80 84 9c 82 80 80', l=0),
dict(a='08 80 80 85 a8 80 80 80', l=0),
dict(a='08 84 80 86 ec 80 80 80', l=0),
dict(a='08 84 80 80 bc 84 80 80', l=0),
dict(a='08 80 80 81 f0 81 80 80', l=0),
dict(a='08 80 80 82 ac 80 80 80', l=0),
dict(a='08 80 80 83 b8 80 80 80', l=0),
dict(a='08 80 80 84 9c 82 80 80', l=0),
dict(a='08 80 80 85 a8 80 80 80', l=0),
dict(a='08 84 80 86 ec 80 80 80', l=0)
#dict(a='08 80 80 80 93 c0 80 80', l=0)
#dict(a='08 80 80 80 93 c0 80 80', l=0)
];

print data

for entry in data:
	d=entry['a'].replace(' ', '').decode("hex")
	bytes = [ord(d[i]) for i in range(0, len(d) - 1)]
	#print bytes
	print entry['a']

	duration = ( ( (bytes[1] & 4) << 5)  | (bytes[4] ^ 0x80) | ((bytes[5] ^ 0x80) << 8));

	bit = duration % 2;
	duration /= 2;

	print "d: %d [%02d:%02d] l: %d b: %d" % (duration, duration /60, duration%60, entry['l'] * 3, bit)

