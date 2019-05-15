#ifndef _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

extern "C"
{
    #include <stdint.h>
    #include "SDL2/SDL_mutex.h"
}
#include "PlaylistSegment.h"
#include "Playlist.h"
#include <vector>

class NetworkManager
{
private:
    int32_t _currentSegment;
    int32_t _averageDownBitrate;
    int32_t _countedBitrates;
    int32_t _maxPriority;
    int32_t _priorityDecayGainRate;
    int32_t _referenceBitrate;
    int32_t _maxMemory;
    int32_t _lastBitrate;
    bool _automaticAdaptiveBitrate;
    int32_t _manualSelectedBitrateIndex;
    bool _bitrateDiscontinuity;
    std::vector<int32_t> _videoSegmentWeights;
    std::vector<Playlist*>* _playlists;
    std::vector<int32_t>* _bitrates;
    SDL_semaphore* _blockEndSemaphore;
    int32_t getUsedMemory();
    bool positionHasALoadedSegment(int32_t);
public:
    NetworkManager(std::vector<Playlist*>*, std::vector<int32_t>*, int32_t, int32_t, int32_t);
    void start();
    void updateCurrentSegment(int32_t);
    double getBlockDuration(int32_t);
    PlaylistSegment* getSegment(int32_t);
    void setManualBitrate(int32_t);
    void setAutomaticBitrate();
    inline int32_t isBitrateDiscontinuity(){return _bitrateDiscontinuity;};
    inline void clearBitrateDiscontinuity(){_bitrateDiscontinuity = false;};
    inline int32_t getLastBitrate(){return _lastBitrate;};
    inline int32_t getSegmentsSize(){return _playlists->at(0)->getSegments()->size();};
    SDL_semaphore* getBlockEndSemaphore();
    friend void networkManagerThread(NetworkManager*);
};

#endif