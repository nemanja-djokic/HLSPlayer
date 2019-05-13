#include "headers/NetworkManager.h"
#include <thread>
#include <iostream>

void networkManagerThread(NetworkManager*);

NetworkManager::NetworkManager(std::vector<Playlist*>* playlists, std::vector<int32_t>* bitrates, int32_t maxMemory, int32_t maxPriority = 10, int32_t priorityDecayGainRate = 3)
{
    this->_blockEndSemaphore = SDL_CreateSemaphore(1);
    this->_playlists = playlists;
    this->_bitrates = bitrates;
    this->_maxPriority = maxPriority;
    this->_priorityDecayGainRate = priorityDecayGainRate;
    this->_currentSegment = 0;
    this->_maxMemory = maxMemory;
    for(size_t i = 0; i < this->_playlists->at(0)->getSegments()->size(); i++)
    {
        this->_videoSegmentWeights.insert(this->_videoSegmentWeights.end(), 0);
    }
    _referenceBitrate = INT32_MAX;
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

PlaylistSegment* NetworkManager::getSegment(int32_t pos)
{
    PlaylistSegment* toReturn = nullptr;
    int32_t returnBitrate = -1;
    for(int32_t i = 0; i < (int32_t)_playlists->size(); i++)
    {
        PlaylistSegment* localSegment = _playlists->at(i)->getSegments()->at(pos);
        if(localSegment->getIsLoaded() && _bitrates->at(i) > returnBitrate && _bitrates->at(i) < _referenceBitrate)
        {
            toReturn = localSegment;
            returnBitrate = _bitrates->at(i);
        }
    }
    if(returnBitrate < 0)
    {
        for(int32_t i = 0; i < (int32_t)_playlists->size(); i++)
        {
            PlaylistSegment* localSegment = _playlists->at(i)->getSegments()->at(pos);
            if(_bitrates->at(i) > returnBitrate && _bitrates->at(i) < _referenceBitrate)
            {
                toReturn = localSegment;
                returnBitrate = _bitrates->at(i);
            }
        }
        return toReturn;
    }
    else
    {
        if(!toReturn->getIsLoaded())toReturn->loadSegment();
        return toReturn;
    }
}

double NetworkManager::getBlockDuration(int32_t pos)
{
    return _playlists->at(0)->getSegments()->at(pos)->getExtInf();
}

int32_t NetworkManager::getUsedMemory()
{
    int32_t usedBytes = 0;
    for(size_t i = 0; i < this->_playlists->size(); i++)
        for(size_t j = 0; j < this->_playlists->at(i)->getSegments()->size(); j++)
            usedBytes += this->_playlists->at(i)->getSegments()->at(j)->loadedSize();
    return usedBytes;
}

bool NetworkManager::positionHasALoadedSegment(int32_t pos)
{
    bool toReturn = false;
    for(size_t i = 0; i < _playlists->size(); i++)
    {
        if(_playlists->at(i)->getSegments()->at(pos)->getIsLoaded())
            toReturn = true;
    }
    return toReturn;
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
                if(gain < -networkManager->_maxPriority)gain = -networkManager->_maxPriority;
                networkManager->_videoSegmentWeights.at(i) += gain;
                if(networkManager->_videoSegmentWeights.at(i) < 0)
                    networkManager->_videoSegmentWeights.at(i) = 0;
            }
        }
        for(size_t i = 0; i < networkManager->_videoSegmentWeights.size(); i++)
        {
            if(networkManager->_videoSegmentWeights.at(i) == 0)
            {
                for(int32_t j = 0; j < (int32_t)networkManager->_playlists->size(); j++)
                {
                    if(networkManager->getUsedMemory() > networkManager->_maxMemory * 1024 * 1024
                    && networkManager->_playlists->at(j)->getSegments()->at(i)->getIsLoaded())
                    {
                        networkManager->_playlists->at(j)->getSegments()->at(i)->unloadSegment();
                    }
                }
            }
            if(networkManager->_videoSegmentWeights.at(i) > 0)
            {
                int32_t selectedBitrate = 0;
                int32_t selectedIndex = -1;
                
                if(networkManager->_referenceBitrate == 0)
                {
                    for(int32_t j = 0; j < (int32_t)networkManager->_bitrates->size(); j++)
                    {
                        if(networkManager->_bitrates->at(j) > selectedBitrate)
                        {
                            selectedBitrate = networkManager->_bitrates->at(j);
                            selectedIndex = j;
                        }
                    }
                }
                else
                {
                    for(int32_t j = 0; j < (int32_t)networkManager->_bitrates->size(); j++)
                    {
                        if(networkManager->_bitrates->at(j) > selectedBitrate 
                        && networkManager->_bitrates->at(j) <= networkManager->_referenceBitrate)
                        {
                            selectedBitrate = networkManager->_bitrates->at(j);
                            selectedIndex = j;
                        }
                    }
                    if(selectedIndex < 0)
                    {
                        selectedBitrate = INT32_MAX;
                        for(int32_t j = 0; j < (int32_t)networkManager->_bitrates->size(); j++)
                        {
                            if(networkManager->_bitrates->at(j) < selectedBitrate)
                            {
                                selectedBitrate = networkManager->_bitrates->at(j);
                                selectedIndex = j;
                            }
                        }
                    }
                }
                if(!networkManager->positionHasALoadedSegment(i))
                {
                    networkManager->_referenceBitrate = networkManager->_playlists->at(selectedIndex)->getSegments()->at(i)->loadSegment();
                }
            }
        }
    }
}