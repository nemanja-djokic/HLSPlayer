#include "headers/NetworkManager.h"
#include <thread>
#include <iostream>

void networkManagerThread(NetworkManager*);

NetworkManager::NetworkManager(std::vector<PlaylistSegment*>* videoSegments, int32_t maxPriority = 10, int32_t priorityDecayGainRate = 3)
{
    this->_blockEndSemaphore = SDL_CreateSemaphore(1);
    this->_videoSegments = videoSegments;
    this->_maxPriority = maxPriority;
    this->_priorityDecayGainRate = priorityDecayGainRate;
    this->_currentSegment = 0;
    for(size_t i = 0; i < this->_videoSegments->size(); i++)
    {
        this->_videoSegmentWeights.insert(this->_videoSegmentWeights.end(), 0);
    }
}

void NetworkManager::start()
{
    std::thread(networkManagerThread, this).detach();
}

void NetworkManager::updateCurrentSegment(int32_t currentSegment)
{
    this->_currentSegment = currentSegment;
}

SDL_semaphore* NetworkManager::getBlockEndSemaphore()
{
    return this->_blockEndSemaphore;
}

void networkManagerThread(NetworkManager* networkManager)
{
    while(true)
    {
        SDL_SemWait(networkManager->_blockEndSemaphore);
        networkManager->_videoSegmentWeights.at(networkManager->_currentSegment) = networkManager->_maxPriority;
        for(size_t i = 0; i < networkManager->_videoSegmentWeights.size(); i++)
        {
            if((int32_t)i < networkManager->_currentSegment)
            {
                networkManager->_videoSegmentWeights.at(i) -= (networkManager->_currentSegment - i) * networkManager->_priorityDecayGainRate;
                if(networkManager->_videoSegmentWeights.at(i) < 0)networkManager->_videoSegmentWeights.at(i) = 0;
            }
            else if((int32_t)i > networkManager->_currentSegment)
            {
                int32_t gain = networkManager->_maxPriority - (i - networkManager->_currentSegment) * networkManager->_priorityDecayGainRate;
                if(gain > -networkManager->_maxPriority)
                    networkManager->_videoSegmentWeights.at(i) += gain;
                if(networkManager->_videoSegmentWeights.at(i) < 0)
                    networkManager->_videoSegmentWeights.at(i) = 0;
            }
        }
        for(size_t i = 0; i < networkManager->_videoSegmentWeights.size(); i++)
        {
            if(networkManager->_videoSegmentWeights.at(i) == 0 && networkManager->_videoSegments->at(i)->getIsLoaded())
            {
                networkManager->_videoSegments->at(i)->unloadSegment();
            }
            if(networkManager->_videoSegmentWeights.at(i) > 0 && !networkManager->_videoSegments->at(i)->getIsLoaded())
            {
                networkManager->_videoSegments->at(i)->loadSegment();
            }
        }
    }
}