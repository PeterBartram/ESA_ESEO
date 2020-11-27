#**************************************************************************
#
#            Copyright (c) 2004 by electronic system design gmbh
#
#  This software is copyrighted by and is the sole property of 
#  esd gmbh.  All rights, title, ownership, or other interests
#  in the software remain the property of esd gmbh. This
#  software may only be used in accordance with the corresponding
#  license agreement.  Any unauthorized use, duplication, transmission,  
#  distribution, or disclosure of this software is expressly forbidden.
#
#  This Copyright notice may not be removed or modified without prior
#  written consent of esd gmbh.
#
#  esd gmbh, reserves the right to modify this software without notice.
#
#  electronic system design gmbh          Tel. +49-511-37298-0
#  Vahrenwalder Str 207                   Fax. +49-511-37298-68 
#  30165 Hannover                         http://www.esd-electronics.com
#  Germany                                sales@esd-electronics.com
#
#**************************************************************************
#
#     Module Name: main.py
#         Version: 1.00
#   Original Date: 28-Oct-2004
#          Author: Michael Schoppe
#        Language: python
# Compile Options: 
# Compile defines: 
#       Libraries: 
#    Link Options: 
#
#    Entry Points: 
#
#**************************************************************************
# Description
#
#                                                                        
#                                                                        
# Edit Date/Ver   Edit Description
# ==============  ===================================================
# MS   28/10/04   Written
# MS   14/03/04   add pdo.write method in class PDO to send data in one PDO with different lengths
#
#


import ntcan  #Importiere Wrapper fuer NTCAN.DLL
import time 
import getopt
import sys
import string
import os
import canopen
import _thread
import util
import threading
import types


def main():
	print(sys.path)				#
	print(os.path)				#
	print(sys.version)				# version number of the Python interpreter 
	print(sys.copyright)			# containing the copyright pertaining to the Python interpreter
	print("Time: ", end=' ')				#
	print(time.asctime(time.localtime()))
	
	
	
	# Open CAN Interface
	# ---> cif = ntcan.CIF( net, RxQueueSize, RxTimeOut, TxQueueSize, TxTimeOut, Flags)
	net=0					# logical CAN Network [0, 255]
	RxQS=2000			        # RxQueueSize [0, 10000]
	RxTO=2000				# RxTimeOut in Millisconds
	TxQS=1					# TxQueueSize [0, 10000]
	TxTO=1000				# TxTimeOut in Millseconds
	#cifFlags=				# Flags
	cif = ntcan.CIF(net,RxQS,RxTO,TxQS,TxTO)
	print (cif)
	util.print2lines()
	
	# set baudrate 0 = 1MBaud
	# CAN-API-Description
	cif.baudrate = 0
	
	# Erzeuge CAN-Messagestruktur
	cmsg = ntcan.CMSG()
	print((cmsg.msg_lost))
	print (cmsg)
	
	
	
	
	# # examples for ntcan ----------------------------------------------------------
	# #cif2 = ntcan.CIF(net,RxQS,RxTO,TxQS,TxTO)
	# cif2 = ntcan.CIF(net)
	# print (cif2)
	# print((cif2.net))
	# print((cif2.tx_timeout))
	# print((cif2.rx_timeout))
	# print((cif2.features))
	# util.print2lines()
	
	# # set baudrate 0 = 1MBaud
	# # CAN-API-Description
	# cif2.baudrate = 0
	
	# # Erzeuge CAN-Messagestruktur
	# cmsg2 = ntcan.CMSG()
	# print("cmsg lost: %d"%(cmsg2.msg_lost))
	# print("cmsg2 %s" %(cmsg2))
	
	
	# id = 123
	# # can write
	# # canWrite...(cif,can-id,len,data ...)
	# #cmsg2.canWriteByte(cif2)
	# cmsg2.canWriteByte(cif2,id,8,1,2,3,4,5,6,7,8)			
	# cmsg2.canWriteShort(cif2,id,8,0x11,0x22,0x33,0x44)
	# cmsg2.canWriteLong(cif2,id,8,0x12345678,0xabcdef)
	# #cmsg2.canWriteLong(cif2,id,8,0x12345678,-0x1)


	
	# # can read
	# cif2.canIdAdd(0x234)						# add id
	
	# # receive can message
	# try: 
		# cmsg2.canRead(cif2)
		# print(cmsg2) 
	# except IOError as xxx_todo_changeme:
		# (errno) = xxx_todo_changeme
		# print("I/O error(%s): " % (errno))
	
	# cif2.canIdDelete(0x234)					# delete id
	
	
	# # clean up
	# del cif2
	# del cmsg2
	
	
	
	
	# # examples for canopen --------------------------------------------------------
	# # example for SYNC
	# print()
	# print("example for SYNC")
	# canopen.SYNCsend(cif)
	# time.sleep(0.5)
	# canopen.SYNCsend(cif)
	# time.sleep(0.5)
	# canopen.SYNCsend(cif)
	
	
	# # example for SYNC with class SYNC
	# print()
	# print("example for SYNC with class SYNC")
	# sync = canopen.SYNC(0,canopen.CANOPEN_1000kBd,1,2000,1.5) 		# (net,baudrate,TxQS,TxTO,[time_s])
	# print(sync)
	# print(sync.cif)
	# sync.send()		# send SYNC fame
	# sync.send()		# send SYNC fame
	# sync.send()		# send SYNC fame
	
	
	
	# # example for NMT
	# print()
	# print("example for NMT")
	# time.sleep(0.5)
	# canopen.NMTstop (cif,1)
	# time.sleep(0.5)
	# canopen.NMTpreop (cif,1)
	# time.sleep(0.5)
	# canopen.NMTreset (cif,1)
	# time.sleep(0.5)
	# canopen.NMTresetcomm (cif,1)
	# time.sleep(0.5)
	# canopen.NMTstart (cif,0)
	
	
	
	# # example for NMT with class NMT
	# print()
	# print("example for NMT with class NMT")
	# nmt = canopen.NMT(0,canopen.CANOPEN_1000kBd,1,2000) 		# (net,baudrate,TxQS,TxTO)
	# print(nmt)
	# print(nmt.cif)
	# nmt.start(3)
	# nmt.stop(3)
	# nmt.preop(3)
	# nmt.reset(3)
	# nmt.resetcomm(3)
	# nmt.start(0)
	
	
	
	# # example for NODEGUARD 
	# print()
	# print("example for NODEGUARD ")
	# canopen.NODEGUARDrequest (cif,1)
	# canopen.NODEGUARDrequest (cif,2)
	# canopen.NODEGUARDrequest (cif,3)
	# canopen.NODEGUARDrequest (cif,3)
	
	
	# result,state = canopen.NODEGUARDrequest (cif,3)
	# if result == True:
		# print("State %d" %state)
	
	# # example for NODEGUARD with class NODEGUARD
	# print("example for NODEGUARD with class NODEGUARD")
	# nodeguard = canopen.NODEGUARD(0,canopen.CANOPEN_1000kBd,100,2000,1,2000) 		# (net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# print(nodeguard)
	# print(nodeguard.cif)
	# nodeguard.request(3)
	
	
	
	
	# # example for SDO
	# print()
	# print("example for SDO")
	# canopen.SDOread (cif,1,0x1000,0x00)		# read device type (node1,index:0x1000,subindex:0)
	# canopen.SDOread (cif,1,0x1001,0x00)
	# canopen.SDOread (cif,1,0x100,0x00)
	
	# result,data=canopen.SDOread (cif,1,0x1000,0)
	# if result == True:
		# print("Result %d  Data 0x%X" %(result,data))
	# result,data=canopen.SDOread (cif,2,0x1000,0)
	# if result == True:
		# print("Result %d  Data 0x%X" %(result,data))
		
	# result,data=canopen.SDOread (cif,3,0x1000,0)
	# if result == True:
		# print("Result %d  Data 0x%X" %(result,data))
	
	# #canopen.SDOwrite1Byte (cif,1,0x2400,0x01,0x12345678)
	# canopen.SDOwrite2Byte (cif,1,0x2400,0x07,0x12345678)
	# canopen.SDOwrite4Byte (cif,1,0x2008,0x01,0x12345678)
	
	# canopen.SDOwriteString (cif,1,0x2008,0x01,"a")
	# canopen.SDOwriteString (cif,1,0x2008,0x01,"ab")
	# canopen.SDOwriteString (cif,1,0x2008,0x01,"abc")
	# canopen.SDOwriteString (cif,1,0x2008,0x01,"abcd")
	# canopen.SDOwriteString (cif,1,0x2008,0x01,"abcdef")
	
	# canopen.SDOwriteString (cif,6,0x1010,0x01,"save")	# Object 1010h: store parameters
	# canopen.SDOwriteString (cif,6,0x1011,0x01,"load")  	# Object 1011h: restore default parameters
	
	
	# # example for SDO with class SDO
	# print()
	# print("example for SDO with class SDO")
	# sdo=canopen.SDO(0,canopen.CANOPEN_1000kBd,100,2000,1,2000) # (net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# sdo.read (1,0x1000,0x00)
	# sdo.read (1,0x1001,0x00)
	# sdo.read (1,0x1234,0x00)
	
	# result,data=sdo.read (1,0x1000,0)
	# print("Result %d  Data 0x%X" %(result,data))
	# if result == True:
		# print("Result %d  Data 0x%X" %(result,data))
	
		
	# result,data=sdo.read (2,0x1008,0)
	# if result == True:
		# if isinstance(data,str):	# string?
			# print(data)
		# else:	
			# print("0x%X"%data)
		
	
	
	
	# #sdo.write1Byte (1,0x2400,0x01,0x12345678)
	# sdo.write2Byte (1,0x2400,0x07,0x12345678)
	# sdo.write4Byte (1,0x2008,0x01,0x12345678)
	
	# sdo.writeString (1,0x2008,0x01,"a")
	# sdo.writeString (1,0x2008,0x01,"ab")
	# sdo.writeString (1,0x2008,0x01,"abc")
	# sdo.writeString (1,0x2008,0x01,"abcd")
	# sdo.writeString (1,0x2008,0x01,"abcdef")
	# sdo.writeString (6,0x2008,0x01,"save")
	# sdo.writeString (6,0x2008,0x01,"load")
	
	
	# # read CANopen SDOs
	# node = 2
	
	
	# # 1001 VAR error register UNSIGNED8 ro M
	# result,data=sdo.read (node,0x1001,0)		
	# # 1002 VAR manufacturer status register UNSIGNED32 ro O
	# result,data=sdo.read (node,0x1002,0)		
	# # 1003 ARRAY pre-defined error field UNSIGNED32 ro O
	# result,data=sdo.read (node,0x1003,0)		
	# # 1004 reserved for compatibility reasons
	# result,data=sdo.read (node,0x1004,0)		
	# # 1005 VAR COB-ID SYNC UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1005,0)		
	# # 1006 VAR communication cycle period UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1006,0)		
	# # 1007 VAR synchronous window length UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1007,0)		
	# # 1008 VAR manufacturer device name Vis-String const O
	# result,data=sdo.read (node,0x1008,0)		
	# # 1009 VAR manufacturer hardware version Vis-String const O
	# result,data=sdo.read (node,0x1009,0)		
	# # 100A VAR manufacturer software version Vis-String const O
	# result,data=sdo.read (node,0x100A,0)		
	# # 100B reserved for compatibility reasons
	# result,data=sdo.read (node,0x100B,0)		
	# # 100C VAR guard time UNSIGNED16 rw O
	# result,data=sdo.read (node,0x100C,0)		
	# # 100D VAR life time factor UNSIGNED8 rw O
	# result,data=sdo.read (node,0x100D,0)		
	# # 100E reserved for compatibility reasons
	# result,data=sdo.read (node,0x100E,0)		
	# # 100F reserved for compatibility reasons
	# result,data=sdo.read (node,0x100F,0)		
	# # 1010 ARRAY store parameters UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1010,0)		
	# # 1011 ARRAY restore default parameters UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1011,0)		
	# # 1012 VAR COB-ID TIME UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1012,0)		
	# # 1013 VAR high resolution time stamp UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1013,0)		
	# # 1014 VAR COB-ID EMCY UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1014,0)		
	# # 1015 VAR Inhibit Time EMCY UNSIGNED16 rw O
	# result,data=sdo.read (node,0x1015,0)		
	# # 1016 ARRAY Consumer heartbeat time UNSIGNED32 rw O
	# result,data=sdo.read (node,0x1016,0)		
	# # 1017 VAR Producer heartbeat time UNSIGNED16 rw O
	# result,data=sdo.read (node,0x1017,0)		
	# # 1018 RECORD Identity Object Identity (23h) ro M
	# result,data=sdo.read (node,0x1018,0)		
	# if result == True:
		# sdo.read (node,0x1018,1)		
		# sdo.read (node,0x1018,2)		
		# sdo.read (node,0x1018,3)		
		# sdo.read (node,0x1018,4)		
	
	
	# sdo.write2Byte (2,0x1800,0x05,0)
	# del sdo
	
	
	
	# # example for PDO
	# #  
	# # canopenPDO1writeByte (cif,node,len, data)
	# # len: 0...8 always number of bytes!!!!!
	# print()
	# print("example for PDO")
	
	
	# canopen.PDO1read(cif,3)				# cif,node
	# canopen.PDO2read(cif,4)
	# canopen.PDO3read(cif,5)
	# canopen.PDO4read(cif,127)
	
	canopen.PDO1writeByte (cif,1,8,d0=0x10)
	# canopen.PDO1writeByte (cif,1,4,0x11)
	
	# canopen.PDO1writeShort (cif,2,7,0x1234,0x2345)
	
	# canopen.PDO1writeLong (cif,3,2)
	
	# canopen.PDO1writeByte (cif,0x22,8,d0=0x10)
	# canopen.PDO1writeByte (cif,0x22,4,0x11)
	# canopen.PDO1writeShort (cif,0x22,0,0x1234,0x2345)
	# canopen.PDO1writeShort (cif,0x22,4,0x1234,0x2345)
	# canopen.PDO1writeShort (cif,0x22,5,0x1234,0x2345)
	# canopen.PDO1writeLong (cif,0x22,1)
	# # 
	# canopen.PDO1writeLong (cif,0x23,5,0x12345678,0x7fffffff)
	# canopen.PDO1writeLong (cif,0x23,5,0x12345678,-1) # 0xFFFFFFFF -> -1)
	
	
	# result,cmsg = canopen.PDO1read (cif,3)
	# if result == True:
		# print(cmsg)
	
	# result,cmsg = canopen.PDO1read (cif,9)
	# if result == True:
		# print(cmsg)
	
	# result,cmsg = canopen.PDO2read (cif,9)
	# if result == True:
		# print(cmsg)
	
	# result,cmsg = canopen.PDO3read (cif,9)
	# if result == True:
		# print(cmsg)
	
	# result,cmsg = canopen.PDO4read (cif,9)
	# if result == True:
		# print(cmsg)
	
	
	# # example for PDO with class PDO
	# print()
	# print("example for PDO with class PDO")
	# pdo1=canopen.PDO(1,0,canopen.CANOPEN_1000kBd,100,2000,1,2000) 		# (obj,net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# pdo1.writeByte(0x8,8,d0=0x10)
	# result,cmsg = pdo1.read (3)
	# if result == True:
		# print(cmsg)
	
	
	# pdo1.writeByte(2,1,0x01)
	# result,cmsg = pdo1.read (2)
	# if result == True:
		# print(cmsg)
	# pdo1.writeByte(2,1,0x02)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x04)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x08)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x10)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x20)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x40)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x80)
	# time.sleep(0.1)
	# pdo1.writeByte(2,1,0x00)
	
	# time.sleep(0.1)
	# pdo1.write(2,0x11223344,1)
	# time.sleep(0.1)
	# pdo1.write(2,0x11223344,2)
	# time.sleep(0.1)
	# pdo1.write(2,0x11223344,3)
	# time.sleep(0.1)
	# pdo1.write(2,0x11223344,4)
	# time.sleep(0.1)
	# pdo1.write(4,0x11223344,1,0x11223344,4,0x11223344,2)
	# time.sleep(0.1)
	# pdo1.write(4,0x11223344,4,0x11223344,4,0x11223344,2)
	# time.sleep(0.1)
	# pdo1.write(5,0xAABB,2,0x11223344,4,0xFF,1)
	
	
	# del pdo1
	
	# pdo2=canopen.PDO(2,0,canopen.CANOPEN_1000kBd,100,2000,1,2000) 		# (obj,net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# pdo2.writeByte(0x8,8,d0=0x10)
	# result,cmsg = pdo2.read (3)
	# if result == True:
		# print(cmsg)
	# del pdo2
	
	# pdo3=canopen.PDO(3,0,canopen.CANOPEN_1000kBd,100,2000,1,2000) 		# (obj,net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# pdo3.writeByte(0x8,8,d0=0x10)
	# result,cmsg = pdo3.read (3)
	# if result == True:
		# print(cmsg)
	# del pdo3
	
	# pdo4=canopen.PDO(4,0,canopen.CANOPEN_1000kBd,100,2000,1,2000) 		# (obj,net,baudrate,RxQS,RxTO,TxQS,TxTO)
	# pdo4.writeByte(0x8,8,d0=0x10)
	# result,cmsg = pdo4.read (3)
	# if result == True:
		# print(cmsg)
	# del pdo4
	
	# #sys.exit (3)
	
	
	
	# # examples with threads
	# print()
	# print("examples with threads")
	
	# #def __init__(self,net,baudrate,TxQS,TxTO,time_s=1.0):
	# sync = canopen.SYNC(0,canopen.CANOPEN_1000kBd,1,2,0.2) 		# net,time in s
	# sync.start()
	
	# #def __init__(self,net,baudrate,node,time_s,state=NODEGUARD_BOOTUP_STATE):
	# heartbeatpro = canopen.HeartbeatProducer(0,canopen.CANOPEN_1000kBd,8,3.9) 		# net,time in s
	# heartbeatpro.start()
	
	
	# heartbeatcon=canopen.HeartbeatConsumer (0,canopen.CANOPEN_1000kBd,6,5.0)
	# heartbeatcon.start()
	
	# heartbeatcon2=canopen.HeartbeatConsumer (0,canopen.CANOPEN_1000kBd,8,4.0)
	# heartbeatcon2.start()
	
	
	# print("Start script in new shell")
	# os.system ("python -u heartbeatconsumer_arg.py 123")
	
	
	
	# e = eval(input("Input: "))					# wait!!!!!
	
	# # clean up
	# del cif
	# del cmsg
	
	# #sys.exit(1)

if __name__ == '__main__':
    main()

