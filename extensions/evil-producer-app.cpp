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


#include "evil-producer-app.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp" //it's an L...
#include "helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/ndn-cxx/name.hpp" //for name component creation...
#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.EvilProducerApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(EvilProducerApp);

TypeId
EvilProducerApp::GetTypeId(void)
{
   //currently signature produced is default and keylocator is default (no methods to set them yet)
   static TypeId typeId = TypeId("ns3::ndn::EvilProducerApp")
    .SetGroupName("Ndn")
    .SetParent<App>()
    .AddConstructor<EvilProducerApp>()
    .AddAttribute("Prefix", "Prefix for evil producer interests", StringValue("/"),
                   MakeNameAccessor(&EvilProducerApp::m_prefix), MakeNameChecker())
    .AddAttribute("PayloadSize", "Payload size for Content packets", UintegerValue(512), 
                  MakeUintegerAccessor(&EvilProducerApp::m_payloadSize),
                  MakeUintegerChecker<uint32_t>())
    .AddAttribute("Freshness", "Freshness of the data packets", TimeValue(Seconds(0)), 
                  MakeTimeAccessor(&EvilProducerApp::m_freshness), MakeTimeChecker())
    .AddAttribute("Signature", "Bad Signature that won't verify", UintegerValue(0), 
                  MakeUintegerAccessor(&EvilProducerApp::m_signature), 
                  MakeUintegerChecker<uint32_t>())
    .AddAttribute("KeyLocator", "Name for the Key Locator", NameValue(), 
                  MakeNameAccessor(&EvilProducerApp::m_keyLocator), MakeNameChecker());

   return typeId;
}

EvilProducerApp::EvilProducerApp()
{
   NS_LOG_FUNCTION_NOARGS();
}

void
EvilProducerApp::OnInterest(shared_ptr<const Interest> interest)
{
  //log that I received the interest
  App::OnInterest(interest);
  NS_LOG_FUNCTION(this << interest);

  //if I am not active, exit
  if(!m_active)
    return;

   //do data creation
   Name dataName(interest->getName());
   //no need for "/" because append already adds the slash...
   name::Component suffixName("evil");
   dataName.append(suffixName);
   auto data = make_shared<Data>();
   data->setName(dataName);
   data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
   data->setContent(make_shared<::ndn::Buffer>(m_payloadSize));
   Signature sig;
   SignatureInfo sigInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));

   if(m_keyLocator.size() > 0)
   {
     sigInfo.setKeyLocator(m_keyLocator);
   }
   sig.setInfo(sigInfo);
   sig.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
   data->setSignature(sig);
  
  //log that I am sending the data
  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data:" << data->getName());

  data->wireEncode(); 

  //transmit the data
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

//inherited from application (overriding them to do more than just app stuff)
void
EvilProducerApp::StartApplication()
{
   //original logs, calls parent start, and then call FibHelper to add route to self
   NS_LOG_FUNCTION_NOARGS();
   App::StartApplication();
   FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
EvilProducerApp::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    App::StopApplication();
}

} // namespace ndn
} // namespace ns3
