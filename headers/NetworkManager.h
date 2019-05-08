#ifndef _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

extern "C"
{
    #include <stdint.h>
    #include "SDL2/SDL_mutex.h"
}
#include "PlaylistSegment.h"
#include <vector>

class NetworkManager
{
private:
    int32_t _currentSegment;
    int32_t _averageDownBitrate;
    int32_t _countedBitrates;
    int32_t _maxPriority;
    int32_t _priorityDecayGainRate;
    std::vector<PlaylistSegment*>* _videoSegments;
    std::vector<int32_t> _videoSegmentWeights;
    SDL_semaphore* _blockEndSemaphore;
public:
    NetworkManager(std::vector<PlaylistSegment*>*, int32_t, int32_t);
    void start();
    void updateCurrentSegment(int32_t);
    SDL_semaphore* getBlockEndSemaphore();
    friend void networkManagerThread(NetworkManager*);
};

#endif