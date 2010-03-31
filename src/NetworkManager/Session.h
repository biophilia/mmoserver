/*
---------------------------------------------------------------------------------------
This source file is part of swgANH (Star Wars Galaxies - A New Hope - Server Emulator)
For more information, see http://www.swganh.org


Copyright (c) 2006 - 2010 The swgANH Team

---------------------------------------------------------------------------------------
*/

#ifndef ANH_NETWORKMANAGER_SESSION_H
#define ANH_NETWORKMANAGER_SESSION_H

#include "CompCryptor.h"
#include "GameService.h"
#include "NetConfig.h"

#include "Common/Message.h"
#include "Utils/clock.h"
#include "Utils/typedefs.h"

#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>

#include <list>
#include <queue>

//======================================================================================================================

class NetworkClient;
class PacketFactory;
class MessageFactory;
class Packet;
class SessionPacket;

//======================================================================================================================

typedef std::map< uint16, Packet* >											SequencedPacketMap;
typedef std::queue<Packet*>													PacketQueue;
typedef std::priority_queue<Message*, std::vector<Message*>, CompareMsg>	MessageQueue;

//======================================================================================================================

enum SessionStatus
{
	SSTAT_Initialize = 0,
	SSTAT_Connecting,
	SSTAT_Connected,
	SSTAT_Disconnecting,
	SSTAT_Disconnected,
	SSTAT_Destroy,
	SSTAT_Timeout,
	SSTAT_Error
};

enum SessionCommand
{
	SCOM_None = 0,
	SCOM_Connect,
	SCOM_Disconnect
};

//======================================================================================================================

class Session
{
	public:
								  Session(void);
								  ~Session(void);

	  void                        ProcessWrite(void);
	  void						  Update(void);

	  void                        HandleSessionPacket(Packet* packet);
	  void                        SortSessionPacket(Packet* packet, uint16 type);
	  void                        HandleFastpathPacket(Packet* packet);
	  
	  void                        SendChannelA(Message* message);
	
	  void						  SendChannelAUnreliable(Message* message);
	  void                        DestroyIncomingMessage(Message* message);
	  void                        DestroyPacket(Packet* packet);
	  
	  
	  // Accessor methods
	  NetworkClient*              getClient(void)                                 { return mClient; }
	  GameService*                getService(void)                                { return mService; }
	  uint32                      getId(void)                                     { return mId; }
	  uint16                      getPortHost(void);
	  uint32                      getIncomingQueueMessageCount()    { return mIncomingMessageQueue.size(); }
	  Message*                    getIncomingQueueMessage();
	  uint32                      getEncryptKey(void)                             { return mEncryptKey; }
	  SessionStatus               getStatus(void)                                 { return mStatus; }
	  SessionCommand              getCommand(void)                                { return mCommand; }
	  boost::asio::ip::udp::endpoint getRemoteEndpoint(void)					  { return mRemoteEndpoint; }

	  void                        setClient(NetworkClient* client)                { mClient = client; }
	  void                        setService(GameService* service)                    { mService = service; }
	  void                        setPacketFactory(PacketFactory* factory)        { mPacketFactory = factory; }
	  void                        setMessageFactory(MessageFactory* factory)      { mMessageFactory = factory; }
	  void                        setId(uint32 id)                                { mId = id; }
	  void						  setRemoteEndpoint( boost::asio::ip::udp::endpoint ep ) { mRemoteEndpoint = ep; }
	  void                        setEncryptKey(uint32 key)                       { mEncryptKey = key; }
	  void                        setStatus(SessionStatus status)                 { mStatus = status; }
	  void                        setCommand(SessionCommand command)              { mCommand = command; }
	  void                        setPacketSize(uint16 size)					  { mMaxPacketSize = size;}
	  void                        setUnreliableSize(uint16 size)				  { mMaxUnreliableSize = size;}

	  void						  QueueIncomingPacket(Packet* packet)			  { mIncomingPackets.push(packet); }

private:
	  void                        _processSessionRequestPacket(Packet* packet);
	  void                        _processDisconnectPacket(Packet* packet);
	  void                        _processMultiPacket(Packet* packet);
	  void                        _processNetStatRequestPacket(Packet* packet);
	  
	  void                        _processDataChannelPacket(Packet* packet);	  
	  void                        _processDataOrderPacket(Packet* packet);
	  void                        _processDataChannelAck(Packet* packet);
	  void                        _processFragmentedPacket(Packet* packet);
	  void                        _processPingPacket(Packet* packet);

	  void                        _processConnectCommand(void);
	  void                        _processDisconnectCommand(void);

	  void                        _addOutgoingMessage(Message* message, uint8 priority, bool fastpath);
	  void                        _addIncomingMessage(Message* message, uint8 priority);

	  uint32					  _buildPackets();
	  uint32					  _buildPacketsUnreliable();


	  void						  _buildMultiDataPacket();//fastpath
	  void						  _buildRoutedMultiDataPacket();
	  void						  _buildUnreliableMultiDataPacket();

	  void                        _buildOutgoingReliablePackets(Message* message);
	  void						  _buildOutgoingReliableRoutedPackets(Message* message);
	  void                        _buildOutgoingUnreliablePackets(Message* message);
	  void                        _addOutgoingReliablePacket(Packet* packet);
	  void                        _addOutgoingUnreliablePacket(Packet* packet);
	  void                        _resendOutgoingPackets(void);
	  void                        _sendPingPacket(void);

	  void						  _handleOutOfOrderPacket(uint16 packet);

	  bool						  CheckSessionTimeOut( void );

	  //
	  // Temp Functions. These should probably exist else where. - Dead1ock
	  //
	  bool						  ValidatePacketCrc(Packet* packet);
	  void						  DeCompCryptPacket(Packet* packet);

	  
	  uint16					  mMaxPacketSize;
	  uint16					  mMaxUnreliableSize;

	  GameService*                mService;
	  NetworkClient*              mClient;
	  PacketFactory*              mPacketFactory;
	  MessageFactory*             mMessageFactory;
	  
	  uint32						mId;
	  uint32						mEncryptKey;
	  uint32						mRequestId;
	  uint32						mOutgoingPingSequence;
	  boost::asio::ip::udp::endpoint mRemoteEndpoint;

	  // Incoming fragmented packet processing.
	  uint32                      mFragmentedPacketTotalSize;
	  uint32                      mFragmentedPacketCurrentSize;
	  uint16					  mFragmentedPacketStartSequence;
	  uint16					  mFragmentedPacketCurrentSequence;

	  uint32                      mRoutedFragmentedPacketTotalSize;
	  uint32                      mRoutedFragmentedPacketCurrentSize;
	  uint16					  mRoutedFragmentedPacketStartSequence;
	  uint16					  mRoutedFragmentedPacketCurrentSequence;

	  uint64                      mConnectStartEvent;       // For SCOM_Connect commands
	  uint64                      mLastConnectRequestSent;  
	  uint64                      mLastPacketReceived;      // General session timeout
	  uint64                      mLastPacketSent;          // General session timeout
	  uint64                      mLastPingPacketSent;          // General session timeout

	  // Netstats
	  uint32				      mServerTickCount;
	  uint32                      mLastRoundtripTime;
	  uint32                      mAverageRoundtripTime;
	  uint32                      mShortestRoundtripTime;
	  uint32                      mLongestRoundtripTime;
	  uint64                      mClientPacketsSent;
	  uint64                      mClientPacketsReceived;
	  uint64                      mServerPacketsSent;
	  uint64                      mServerPacketsReceived;

	  // Reliability
	  uint16                      mOutSequenceNext;
	  uint16                      mInSequenceNext;

	  bool                        mSendDelayedAck;        // We processed some incoming packets, send an ack
	  uint16                      mLastSequenceAcked;

	  SessionStatus               mStatus;
	  SessionCommand              mCommand;

	  // Message queues.
	  MessageQueue                mOutgoingMessageQueue;		//here we store the messages given to us by the messagelib
	  MessageQueue                mUnreliableMessageQueue;

	  MessageQueue                mIncomingMessageQueue;
	  MessageQueue				  mMultiMessageQueue;
	  MessageQueue				  mRoutedMultiMessageQueue;
	  MessageQueue				  mMultiUnreliableQueue;

	  // Packet queues.
	  PacketQueue				  mIncomingPackets;
	  PacketQueue				  mOutgoingPackets;
	  SequencedPacketMap		  mOutgoingPacketCache; // Cache packets until they are acknowledged.

	  PacketQueue                 mIncomingFragmentedPacketQueue;
	  PacketQueue                 mIncomingRoutedFragmentedPacketQueue;		

	  uint64					  lasttime;
	  uint64					  avgTime;
	  uint32                      avgPacketsbuild;
	  uint32                      avgUnreliablesbuild;

	  uint64					  mPacketBuildTimeLimit;
	  uint64					  mLastWriteThreadTime;

	  uint32					  endCount;
	  uint16					  lowest;// the lowest packet requested from the server
	  uint16					  lowestCount;// counts the requests up

	  CompCryptor				  mCompCryptor;
	  
};


//======================================================================================================================

#endif //ANH_NETWORKMANAGER_SESSION_H





