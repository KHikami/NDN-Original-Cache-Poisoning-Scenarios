/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created by KHikami (Rachel Chu) 2017
 **/


#ifndef SECURITYTOYCLIENT_H
#define SECURITYTOYCLIENT_H

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"
#include <vector>
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/security/key-chain.hpp>

//will act as the Consumer of our data packets... going to extend from consumer...
namespace ns3 {
namespace ndn {

class SecurityToyClientApp : public Consumer
{
  public:
     static TypeId
     GetTypeId();
    
     SecurityToyClientApp();

     virtual ~SecurityToyClientApp();

     virtual void
     SendPacket();

     virtual void
     OnData(shared_ptr<const Data> data);

     virtual void
     OnNack(shared_ptr<const lp::Nack> nack);

  protected:

     virtual void
     ScheduleNextPacket();

  protected:
    //have a lot of fields that are inherited from consumer
    //m_rand = nonce generator (a pointer to it)
    //EventID m_sendEvent = the current pending send packet event
    //Packet SendPacket = the packet to be sent
    
     bool m_firstTime; //boolean that says is this the first time this is running
     bool m_verificationMode; //boolean that says am I currently in verification mode
     bool m_pursuitMode; //boolean that says am I currently pursuing the correct packet
     bool m_lastPacketEvil; //boolean that remembers if the last data packet I received was bad

     double m_reactionTime; //time needed to wait from sending original request to verification etc.
     uint32_t m_keyRequestInterestSeq;
     uint32_t m_waitTime; //time from a packet being verified to sending the next interest
     uint32_t m_delayStartTime; //time to wait before sending first packet
     uint32_t m_originalNonce;//the original nonce of the Interest that got the bad packet back
     uint32_t m_originalSequenceNumber; //original sequence number of the packet I sent

     //to be used for if data is correct or not... strangely EF only is based on name and so this should check payload
     //also note that for a name verification => have to be sure the name component exists...
     uint32_t m_goodDataSize; //payload size of the good packets   

     Name m_keyName; //name of the verification packet that producer makes
     shared_ptr<const Data> m_evilPacket; //data of the evil packet...
     shared_ptr<Name> m_originalInterestName; //the original name of the packet sent out
   
};

} // namespace ndn
} // namespace ns3

#endif
