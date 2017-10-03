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



#include "security-toy-client-app.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"
#include "utils/ndn-ns3-packet-tag.hpp"
#include "ns3/log.h"
#include "ns3/simulator.h" //needed for properties in the simulator
#include "ns3/packet.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include <iostream>

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.SecurityToyClientApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(SecurityToyClientApp);

TypeId
SecurityToyClientApp::GetTypeId()
{
   //already has parent attributes
   static TypeId tid = TypeId("ns3::ndn::SecurityToyClientApp")
     .SetGroupName("Ndn")
     .SetParent<Consumer>()
     .AddConstructor<SecurityToyClientApp>()
     .AddAttribute("WaitTime", "Wait Time in seconds after a Packet is Verified to send out the next packet", 
                  StringValue("2.0"), MakeDoubleAccessor(&SecurityToyClientApp::m_waitTime), 
                  MakeDoubleChecker<double>())
     .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&SecurityToyClientApp::m_seqMax), MakeIntegerChecker<uint32_t>())
     .AddAttribute("KeyName", "Name for the Key Verification Packet", 
                   StringValue("/"), MakeNameAccessor(&SecurityToyClientApp::m_keyName), MakeNameChecker())
     .AddAttribute("ReactionTime", "Wait time for how long client should wait to send Key Interest and EF Interest",
                   StringValue("1.0"), MakeDoubleAccessor(&SecurityToyClientApp::m_reactionTime), 
                   MakeDoubleChecker<double>())
     .AddAttribute("GoodDataSize", "Payload Size for the Good Data Packets. Should be different from Evil", 
                   StringValue("1024"), MakeIntegerAccessor(&SecurityToyClientApp::m_goodDataSize), 
                   MakeIntegerChecker<uint32_t>())
     .AddAttribute("DelayStart", "Seconds for how long client should wait to send first interest", StringValue("0"), 
                   MakeDoubleAccessor(&SecurityToyClientApp::m_delayStartTime), MakeDoubleChecker<double>());
   return tid;
}

SecurityToyClientApp::~SecurityToyClientApp()
{
}


SecurityToyClientApp::SecurityToyClientApp()
{
   //constructor. Should generate the random function and packet sequence number (if needed)
   NS_LOG_FUNCTION_NOARGS();
   m_waitTime = 2.0; //suggested is reaction time is at least half of this wait time else infinite loop mode...
   m_reactionTime = 1.0;
   m_delayStartTime = 0;
   m_firstTime = true;
   m_verificationMode = false;
   m_pursuitMode = false;
   m_goodDataSize = 1024;
   m_seqMax = std::numeric_limits<uint32_t>::max(); //needed to be able to send the packets
   m_seq = 1; //start at 1 for the data packets
   m_keyRequestInterestSeq = 0;
}


//logic:
//  if in verification mode
//      create interest for the key
//  if in pursuit mode
//      create interest for the original data with EF flag for old data set
//  else
//      create normal interest
void
SecurityToyClientApp::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Interest> interest;

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

    //NS_LOG_INFO("Max Sequence number is" << m_seqMax << " and my sequence is: " << m_seq);

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    if(!m_verificationMode && !m_pursuitMode)
    {
       seq = m_seq++;
    }
    else
    {
       if(m_pursuitMode && !m_verificationMode)
       {
          seq = m_originalSequenceNumber;
       }
       else
       {
          seq = m_keyRequestInterestSeq; 
       }
    }
    //cout << "Sequence number for next packet is: " << seq << ". Following packet will be: " << m_seq << endl;
  }

  if(!m_verificationMode && !m_pursuitMode)
  {
   //NS_LOG_INFO("Making a Packet w/ sequence number" << seq);

   shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
   nameWithSequence->appendSequenceNumber(seq);

   m_originalInterestName = nameWithSequence;

   // shared_ptr<Interest> interest = make_shared<Interest> ();
   interest = make_shared<Interest>();
   m_originalNonce = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max());
   interest->setNonce(m_originalNonce);
   interest->setName(*nameWithSequence);
   time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
   interest->setInterestLifetime(interestLifeTime);

   // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
   //this will now make every even interest a request for data...
   NS_LOG_INFO("> Interest for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
   NS_LOG_DEBUG("Trying to add " << seq << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

   //NS_LOG_INFO("I currently have app link: " << m_appLink);
  }
  else 
  {
    if(m_pursuitMode && !m_verificationMode)
    {
      cout << "in pursuit mode!" << endl;
      //I received a bad data packet => I retransmit my previous interest but with EF flag set! (And I have not started 
      //verification yet)
      interest = make_shared<Interest>();
      interest->setNonce(m_originalNonce);
      interest->setName(*m_originalInterestName);
      time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
      interest->setInterestLifetime(interestLifeTime);

      Exclude excludeSection = Exclude();
      Name evilDataName = m_evilPacket->getName();
      name::Component excludeItem = evilDataName.get(3);
      excludeSection.excludeOne(excludeItem);
      interest->setExclude(excludeSection);

      NS_LOG_DEBUG("Interest with Exclude: " << interest->toUri()); 

      NS_LOG_INFO("> Requesting new data for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId());
      //cout << "> Requesting new data for " << seq << ", Total: " << m_seq << ", face: " << m_face->getId() << endl;
    }
    else
    {
      interest = make_shared<Interest>();
      interest->setNonce(m_rand->GetValue(0,std::numeric_limits<uint32_t>::max()));
      interest->setName(m_keyName);
      time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
      interest->setInterestLifetime(interestLifeTime);
      NS_LOG_INFO("> Interest for " << seq << ", is a Key Request Interest");
      //cout << "> Interest for " << seq << ", is a Key Request Interest" << endl;
      
      m_seqRetxCounts[seq] = 0;
    }
  }
  m_seqTimeouts.insert(SeqTimeout(seq, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqLastDelay.erase(seq);
  m_seqLastDelay.insert(SeqTimeout(seq, Simulator::Now()));

  m_seqRetxCounts[seq]++;

  m_rtt->SentSeq(SequenceNumber32(seq), 1);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  //NS_LOG_INFO("Created Interest: " << *interest);

  SecurityToyClientApp::ScheduleNextPacket();
}

void
SecurityToyClientApp::OnData(shared_ptr<const Data> data)
{

  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  //If in verification mode:
  //   if in pursuit mode: (already received bad packet & getting the verification packet)
  //      scenario complete! (successful increment) & reset verification mode & pursuit mode
  //   else
  //      check if originally received packet was evil packet
  //      if yes => in pursuit mode = true
  //      if no => scenario complete (successful increment) & reset verification mode & pursuit mode
  //else:
  //   store packet for later/mark if it was evil or not & set in verification mode to true

  if(!m_verificationMode)
  {
     uint32_t seq = data->getName().at(2).toSequenceNumber();
     NS_LOG_INFO("< DATA for " << seq << " with name " << data->getName());
     cout << "< DATA for " << seq << " with name" << data->getName() << endl;
     int hopCount = 0;
     auto hopCountTag = data->getTag<lp::HopCountTag>();
     if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
        hopCount = *hopCountTag;
      }
      NS_LOG_DEBUG("Hop count: " << hopCount);

     SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
     if (entry != m_seqLastDelay.end()) {
        m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
     }

     entry = m_seqFullDelay.find(seq);
     if (entry != m_seqFullDelay.end()) {
         m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
     }

     m_originalSequenceNumber = seq;

     m_seqRetxCounts.erase(seq);
     m_seqFullDelay.erase(seq);
     m_seqLastDelay.erase(seq);

     m_seqTimeouts.erase(seq);
     m_retxSeqs.erase(seq);

     m_verificationMode = true;

     Block dataContent = data->getContent();
     //this seems really really hacky... but sadly I don't have a way around this b/c somehow returned data will have an extra 4 bytes...
     if(dataContent.size() <= m_goodDataSize)
     {
        //somehow evil packet... let's use block size since not easy to store a string in the block.
        NS_LOG_DEBUG("Received Evil Packet for " << seq << " with size " << dataContent.size());
        m_evilPacket = data;
        m_lastPacketEvil = true;
     }
     else
     {
        NS_LOG_DEBUG("Received Good Packet for " << seq << " with size " << dataContent.size());
        m_lastPacketEvil = false;
     }

  }
  else
  { //in verification mode => assumes data received is the key...

     NS_LOG_INFO("> Data for Key Request " << m_keyRequestInterestSeq << " for Data packet " << m_originalSequenceNumber);
     int hopCount = 0;
     auto hopCountTag = data->getTag<lp::HopCountTag>();
     if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
        hopCount = *hopCountTag;
     }
     NS_LOG_DEBUG("Hop count: " << hopCount);

    m_seqRetxCounts.erase(m_keyRequestInterestSeq);
    m_seqFullDelay.erase(m_keyRequestInterestSeq);
    m_seqLastDelay.erase(m_keyRequestInterestSeq);

    m_seqTimeouts.erase(m_keyRequestInterestSeq);
    m_retxSeqs.erase(m_keyRequestInterestSeq);
    //ack receive of key interest data response (else it gets retransmitted)
    m_rtt->AckSeq(SequenceNumber32(m_keyRequestInterestSeq));

    m_verificationMode = false;

    //should only ACK original packet if it wasn't evil... else set pursuit mode to true
    if(!m_lastPacketEvil)
    {

       //to true (and have the previously sent data stored for exclude later)
       NS_LOG_INFO("Acknowledging:" << m_originalSequenceNumber);

       //ack original packet
       m_rtt->AckSeq(SequenceNumber32(m_originalSequenceNumber));

       m_pursuitMode = false;
    }
    else
    {
       NS_LOG_INFO("Pursuing new packet for " << m_originalSequenceNumber);
       m_pursuitMode = true;
    }
  } 
}

void
SecurityToyClientApp::OnNack(shared_ptr<const lp::Nack> nack)
{
  Consumer::OnNack(nack);//for the logging...
  //don't retransmit. Wait and schedule next packet

  //if in verification Mode and received a NACK for the signature, set verification mode to false. (failed verification)
  if(m_verificationMode)
  {
	m_verificationMode = false;
  }

  //if received a NACK while in pursuit mode => there's no other packet in network. so give up.
  if(m_pursuitMode)
  {
        m_pursuitMode = false;
  }
}

void
SecurityToyClientApp::ScheduleNextPacket()
{
   if(m_firstTime)
   {
	m_sendEvent = Simulator::Schedule(Seconds(m_delayStartTime), &SecurityToyClientApp::SendPacket, this);
        m_firstTime = false;
   }
   else if(!m_sendEvent.IsRunning())
   {
        if(m_verificationMode || m_pursuitMode)
	{
	  //it seems I have to wait, else infinite loop => new field m_reactionTime
          m_sendEvent = Simulator::Schedule(Seconds(m_reactionTime), &SecurityToyClientApp::SendPacket, this);
	}
	else
	{
	   //wait for given wait time parameter before sending the next interest
           m_sendEvent = Simulator::Schedule(Seconds(m_waitTime), &SecurityToyClientApp::SendPacket, this);
	}
   }
}

//will call parent StartApplication & StopApplication


} // namespace ndn
} // namespace ns3
