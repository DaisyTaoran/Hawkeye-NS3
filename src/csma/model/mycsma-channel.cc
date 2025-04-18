
#include "mycsma-channel.h"
#include "mycsma-net-device.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("MyCsmaChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MyCsmaChannel);

TypeId
MyCsmaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyCsmaChannel")
    .SetParent<Channel> ()
    .AddConstructor<MyCsmaChannel> ()
    .AddAttribute ("DataRate", 
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (0xffffffff)),
                   MakeDataRateAccessor (&MyCsmaChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&MyCsmaChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

MyCsmaChannel::MyCsmaChannel ()
  :
    Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = IDLE;
  m_deviceList.clear ();
}

MyCsmaChannel::~MyCsmaChannel ()
{
  NS_LOG_FUNCTION (this);
  m_deviceList.clear ();
}

int32_t
MyCsmaChannel::Attach (Ptr<MyCsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  MyCsmaDeviceRec rec (device);

  m_deviceList.push_back (rec);
  return (m_deviceList.size () - 1);
}

bool
MyCsmaChannel::Reattach (Ptr<MyCsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<MyCsmaDeviceRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end ( ); it++)
    {
      if (it->devicePtr == device) 
        {
          if (!it->active) 
            {
              it->active = true;
              return true;
            } 
          else 
            {
              return false;
            }
        }
    }
  return false;
}

bool
MyCsmaChannel::Reattach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  if (deviceId < m_deviceList.size ())
    {
      return false;
    }

  if (m_deviceList[deviceId].active)
    {
      return false;
    } 
  else 
    {
      m_deviceList[deviceId].active = true;
      return true;
    }
}

bool
MyCsmaChannel::Detach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  if (deviceId < m_deviceList.size ())
    {
      if (!m_deviceList[deviceId].active)
        {
          NS_LOG_WARN ("MyCsmaChannel::Detach(): Device is already detached (" << deviceId << ")");
          return false;
        }

      m_deviceList[deviceId].active = false;

      if ((m_state == TRANSMITTING) && (m_currentSrc == deviceId))
        {
          NS_LOG_WARN ("MyCsmaChannel::Detach(): Device is currently" << "transmitting (" << deviceId << ")");
        }

      return true;
    } 
  else 
    {
      return false;
    }
}

bool
MyCsmaChannel::Detach (Ptr<MyCsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  std::vector<MyCsmaDeviceRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if ((it->devicePtr == device) && (it->active)) 
        {
          it->active = false;
          return true;
        }
    }
  return false;
}

bool
MyCsmaChannel::TransmitStart (Ptr<Packet> p, uint32_t srcId)
{
  NS_LOG_FUNCTION (this << p << srcId);
  NS_LOG_INFO ("UID is " << p->GetUid () << ")");

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("MyCsmaChannel::TransmitStart(): State is not IDLE");
      return false;
    }

  if (!IsActive (srcId))
    {
      NS_LOG_ERROR ("MyCsmaChannel::TransmitStart(): Seclected source is not currently attached to network");
      return false;
    }

  NS_LOG_LOGIC ("switch to TRANSMITTING");
  m_currentPkt = p;
  m_currentSrc = srcId;
  m_state = TRANSMITTING;
  return true;
}

bool
MyCsmaChannel::IsActive (uint32_t deviceId)
{
  return (m_deviceList[deviceId].active);
}

bool
MyCsmaChannel::TransmitEnd ()
{
  NS_LOG_FUNCTION (this << m_currentPkt << m_currentSrc);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == TRANSMITTING);
  m_state = PROPAGATING;

  bool retVal = true;

  if (!IsActive (m_currentSrc))
    {
      NS_LOG_ERROR ("MyCsmaChannel::TransmitEnd(): Seclected source was detached before the end of the transmission");
      retVal = false;
    }

  NS_LOG_LOGIC ("Schedule event in " << m_delay.GetSeconds () << " sec");


  NS_LOG_LOGIC ("Receive");

  std::vector<MyCsmaDeviceRec>::iterator it;
  uint32_t devId = 0;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++)
    {
      if (it->IsActive ())
        {
          // schedule reception events
          Simulator::ScheduleWithContext (it->devicePtr->GetNode ()->GetId (),
                                          m_delay,
                                          &MyCsmaNetDevice::Receive, it->devicePtr,
                                          m_currentPkt->Copy (), m_deviceList[m_currentSrc].devicePtr);
        }
      devId++;
    }

  // also schedule for the tx side to go back to IDLE
  Simulator::Schedule (m_delay, &MyCsmaChannel::PropagationCompleteEvent,
                       this);
  return retVal;
}

void
MyCsmaChannel::PropagationCompleteEvent ()
{
  NS_LOG_FUNCTION (this << m_currentPkt);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == PROPAGATING);
  m_state = IDLE;
}

uint32_t
MyCsmaChannel::GetNumActDevices (void)
{
  int numActDevices = 0;
  std::vector<MyCsmaDeviceRec>::iterator it;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if (it->active)
        {
          numActDevices++;
        }
    }
  return numActDevices;
}

uint32_t
MyCsmaChannel::GetNDevices (void) const
{
  return (m_deviceList.size ());
}

Ptr<MyCsmaNetDevice>
MyCsmaChannel::GetCsmaDevice (uint32_t i) const
{
  Ptr<MyCsmaNetDevice> netDevice = m_deviceList[i].devicePtr;
  return netDevice;
}

int32_t
MyCsmaChannel::GetDeviceNum (Ptr<MyCsmaNetDevice> device)
{
  std::vector<MyCsmaDeviceRec>::iterator it;
  int i = 0;
  for (it = m_deviceList.begin (); it < m_deviceList.end (); it++) 
    {
      if (it->devicePtr == device)
        {
          if (it->active) 
            {
              return i;
            } 
          else 
            {
              return -2;
            }
        }
      i++;
    }
  return -1;
}

bool
MyCsmaChannel::IsBusy (void)
{
  if (m_state == IDLE) 
    {
      return false;
    } 
  else 
    {
      return true;
    }
}

DataRate
MyCsmaChannel::GetDataRate (void)
{
  return m_bps;
}

Time
MyCsmaChannel::GetDelay (void)
{
  return m_delay;
}

WireState
MyCsmaChannel::GetState (void)
{
  return m_state;
}

Ptr<NetDevice>
MyCsmaChannel::GetDevice (uint32_t i) const
{
  return GetCsmaDevice (i);
}

MyCsmaDeviceRec::MyCsmaDeviceRec ()
{
  active = false;
}

MyCsmaDeviceRec::MyCsmaDeviceRec (Ptr<MyCsmaNetDevice> device)
{
  devicePtr = device; 
  active = true;
}

MyCsmaDeviceRec::MyCsmaDeviceRec (MyCsmaDeviceRec const &deviceRec)
{
  devicePtr = deviceRec.devicePtr;
  active = deviceRec.active;
}

bool
MyCsmaDeviceRec::IsActive () 
{
  return active;
}

} // namespace ns3
