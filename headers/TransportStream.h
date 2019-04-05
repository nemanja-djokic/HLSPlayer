#ifndef _TRANSPORT_STREAM_H_
#define _TRANSPORT_STREAM_H_

#include <cstdint>

class TransportStream
{
    private:
        bool _validSyncByte;
        bool _transportErrorIndicator;
        bool _payloadUnitStartIndicator;
        bool _transportPriority;
        uint8_t _pid[2]; //Actually 13 bits, LSB aligned, 51 is video, 64 is audio
        bool _scrambled;
        bool _hasAdaptationField;
        uint8_t _continuityCounter; //Actually 4 bits, LSB aligned
        uint8_t _payload[184];
    public:
};

#endif