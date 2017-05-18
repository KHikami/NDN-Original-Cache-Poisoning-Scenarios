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


#ifndef EVILPRODUCERAPP_H
#define EVILPRODUCERAPP_H

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/nstime.h"

#include <ndn-cxx/security/key-chain.hpp> //for later use...

//from ndn-app.hpp, we get ndn-commmon, ndn-app-link-service, face, application, ptr, callback, and traced callback
#include "ns3/ndnSIM/apps/ndn-app.hpp"

namespace ns3{
namespace ndn{

//pretend App that calls on EvilProducer to generate packets

//extending the ndn-app class to create my producer apps...
class EvilProducerApp : public App
{
   public:
     static TypeId
     GetTypeId();

     EvilProducerApp();//the constructor

     virtual void
     OnInterest(shared_ptr<const Interest> interest);

   protected:
     //inherited from application (overriding them to do more than just app stuff)
     virtual void
     StartApplication();

     virtual void
     StopApplication();

   private:
     //data packet production details
     Name m_prefix; //my prefix
     Time m_freshness;
     Name m_keyLocator;
     uint32_t m_signature;
     uint32_t m_payloadSize;

     //from ndn_app, I auto get: m_face, m_active, and logging details of traced callback
};

}//namesapace ndn

}//namespace ns3

#endif
