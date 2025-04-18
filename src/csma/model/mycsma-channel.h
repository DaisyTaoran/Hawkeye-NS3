
#ifndef MYCSMA_CHANNEL_H
#define MYCSMA_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

namespace ns3 {

class Packet;

class MyCsmaNetDevice;


class MyCsmaDeviceRec {
public:
  Ptr< MyCsmaNetDevice > devicePtr; /// Pointer to the net device
  bool                       active;    /// Is net device enabled to TX/RX

  MyCsmaDeviceRec();
  MyCsmaDeviceRec(Ptr< MyCsmaNetDevice > device);
  MyCsmaDeviceRec (MyCsmaDeviceRec const &);

  bool IsActive ();
};

enum WireState
{
  IDLE,            /**< Channel is IDLE, no packet is being transmitted */
  TRANSMITTING,    /**< Channel is BUSY, a packet is being written by a net device */
  PROPAGATING      /**< Channel is BUSY, packet is propagating to all attached net devices */
};


class MyCsmaChannel : public Channel 
{
public:
  static TypeId GetTypeId (void);

  MyCsmaChannel ();
  virtual ~MyCsmaChannel ();

  int32_t Attach (Ptr<MyCsmaNetDevice> device);

  bool Detach (Ptr<MyCsmaNetDevice> device);

  bool Detach (uint32_t deviceId);

  bool Reattach (uint32_t deviceId);

  bool Reattach (Ptr<MyCsmaNetDevice> device);

  bool TransmitStart (Ptr<Packet> p, uint32_t srcId);

  bool TransmitEnd ();

  void PropagationCompleteEvent ();

  int32_t GetDeviceNum (Ptr<MyCsmaNetDevice> device);

  WireState GetState ();

  bool IsBusy ();

  bool IsActive (uint32_t deviceId);

  uint32_t GetNumActDevices (void);

  virtual uint32_t GetNDevices (void) const;

  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

  Ptr<MyCsmaNetDevice> GetCsmaDevice (uint32_t i) const;

  DataRate GetDataRate (void);

  Time GetDelay (void);

private:
  MyCsmaChannel (MyCsmaChannel const &);
  MyCsmaChannel &operator = (MyCsmaChannel const &);

  DataRate      m_bps;

  Time          m_delay;

  std::vector<MyCsmaDeviceRec> m_deviceList;

  Ptr<Packet> m_currentPkt;

  uint32_t                            m_currentSrc;

  WireState          m_state;
};

} // namespace ns3

#endif /* MYCSMA_CHANNEL_H */
