#source: ovl.s
#ld: -N -T ovl.lnk
#objdump: -D

.*elf32-spu

Disassembly of section \.text:

00000100 <_start>:
 100:	1c f8 00 81 	ai	\$1,\$1,-32
 104:	48 20 00 00 	xor	\$0,\$0,\$0
 108:	24 00 00 80 	stqd	\$0,0\(\$1\)
 10c:	24 00 40 80 	stqd	\$0,16\(\$1\)
 110:	33 00 04 00 	brsl	\$0,130 <f0\+0x4>	# 130
 114:	33 00 04 80 	brsl	\$0,138 <f0\+0xc>	# 138
 118:	33 00 07 00 	brsl	\$0,150 <f0\+0x24>	# 150
 11c:	42 00 ac 09 	ila	\$9,344	# 158
 120:	35 20 04 80 	bisl	\$0,\$9
 124:	1c 08 00 81 	ai	\$1,\$1,32	# 20
 128:	32 7f fb 00 	br	100 <_start>	# 100

0000012c <f0>:
 12c:	35 00 00 00 	bi	\$0
 130:	42 02 00 4f 	ila	\$79,1024	# 400
 134:	32 00 02 80 	br	148 <f0\+0x1c>	# 148
 138:	42 02 02 4f 	ila	\$79,1028	# 404
 13c:	32 00 01 80 	br	148 <f0\+0x1c>	# 148
 140:	42 02 08 4f 	ila	\$79,1040	# 410
 144:	40 20 00 00 	nop	\$0
 148:	42 00 00 ce 	ila	\$78,1
 14c:	32 00 0a 80 	br	1a0 <__ovly_load>	# 1a0
 150:	42 02 00 4f 	ila	\$79,1024	# 400
 154:	32 00 02 80 	br	168 <f0\+0x3c>	# 168
 158:	42 02 12 4f 	ila	\$79,1060	# 424
 15c:	32 00 01 80 	br	168 <f0\+0x3c>	# 168
 160:	42 02 1a 4f 	ila	\$79,1076	# 434
 164:	40 20 00 00 	nop	\$0
 168:	42 00 01 4e 	ila	\$78,2
 16c:	32 00 06 80 	br	1a0 <__ovly_load>	# 1a0
#...
[0-9a-f]+ <__ovly_return>:
[0-9a-f ]+:	3f e1 00 4e 	shlqbyi	\$78,\$0,4
[0-9a-f ]+:	3f e2 00 4f 	shlqbyi	\$79,\$0,8
[0-9a-f ]+:	25 00 27 ce 	biz	\$78,\$79

[0-9a-f]+ <__ovly_load>:
#...
[0-9a-f]+ <_ovly_debug_event>:
#...
Disassembly of section \.ov_a1:

00000400 <f1_a1>:
 400:	32 00 01 80 	br	40c <f3_a1>	# 40c

00000404 <f2_a1>:
 404:	42 00 a0 03 	ila	\$3,320	# 140
 408:	35 00 00 00 	bi	\$0

0000040c <f3_a1>:
 40c:	35 00 00 00 	bi	\$0

00000410 <f4_a1>:
 410:	35 00 00 00 	bi	\$0
	\.\.\.
Disassembly of section \.ov_a2:

00000400 <f1_a2>:
 400:	24 00 40 80 	stqd	\$0,16\(\$1\)
 404:	24 ff 80 81 	stqd	\$1,-32\(\$1\)
 408:	1c f8 00 81 	ai	\$1,\$1,-32
 40c:	33 7f a4 00 	brsl	\$0,12c <f0>	# 12c
 410:	33 7f a4 00 	brsl	\$0,130 <f0\+0x4>	# 130
 414:	33 00 03 80 	brsl	\$0,430 <f3_a2>	# 430
 418:	34 00 c0 80 	lqd	\$0,48\(\$1\)	# 30
 41c:	1c 08 00 81 	ai	\$1,\$1,32	# 20
 420:	35 00 00 00 	bi	\$0

00000424 <f2_a2>:
 424:	41 00 00 03 	ilhu	\$3,0
 428:	60 80 b0 03 	iohl	\$3,352	# 160
 42c:	35 00 00 00 	bi	\$0

00000430 <f3_a2>:
 430:	35 00 00 00 	bi	\$0

00000434 <f4_a2>:
 434:	35 00 00 00 	bi	\$0
	\.\.\.
Disassembly of section .data:

00000440 <_ovly_table>:
 440:	00 00 04 00 .*
 444:	00 00 00 20 .*
 448:	00 00 02 e0 .*
 44c:	00 00 00 01 .*
 450:	00 00 04 00 .*
 454:	00 00 00 40 .*
 458:	00 00 03 00 .*
 45c:	00 00 00 01 .*

00000460 <_ovly_buf_table>:
 460:	00 00 00 00 .*
Disassembly of section \.toe:

00000470 <_EAR_>:
	\.\.\.
Disassembly of section \.note\.spu_name:

.* <\.note\.spu_name>:
.*:	00 00 00 08 .*
.*:	00 00 00 0c .*
.*:	00 00 00 01 .*
.*:	53 50 55 4e .*
.*:	41 4d 45 00 .*
.*:	74 6d 70 64 .*
.*:	69 72 2f 64 .*
.*:	75 6d 70 00 .*
