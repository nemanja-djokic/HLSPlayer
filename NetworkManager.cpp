#include "headers/NetworkManager.h"
#include <thread>
#include <iostream>
#include <iomanip>
#include <algorithm>

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
    this->_lastBitrate = 0;
    this->_bitrateDiscontinuity = false;
    this->_automaticAdaptiveBitrate = true;
    this->_manualSelectedBitrateIndex = -1;
    for(size_t i = 0; i < this->_playlists->at(0)->getSegments()->size(); i++)
    {
        this->_videoSegmentWeights.insert(this->_videoSegmentWeights.end(), 0);
    }
    this->_referenceBitrate = INT32_MAX;
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
        if(localSegment->getIsLoaded() && _bitrates->at(i) > returnBitrate)
        {
            toReturn = localSegment;
            returnBitrate = _bitrates->at(i);
        }
    }
    if(returnBitrate > 0)
    {
        if(_lastBitrate != 0 && returnBitrate != this->_lastBitrate)
        {
            this->_bitrateDiscontinuity = true;
            this->_lastBitrate = returnBitrate;
        }
        else
        {
            this->_lastBitrate = returnBitrate;
        }
        return toReturn;
    }
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
        if(returnBitrate > 0)
        {
            if(!toReturn->getIsLoaded())toReturn->loadSegment();
            if(_lastBitrate != 0 && returnBitrate != this->_lastBitrate)
            {
                this->_bitrateDiscontinuity = true;
                this->_lastBitrate = returnBitrate;
            }
            else
            {
                this->_lastBitrate = returnBitrate;
            }
            return toReturn;
        }
        else
        {
            returnBitrate = INT32_MAX;
            for(int32_t i = 0; i < (int32_t)_playlists->size(); i++)
            {
                PlaylistSegment* localSegment = _playlists->at(i)->getSegments()->at(pos);
                if(_bitrates->at(i) < returnBitrate)
                {
                    toReturn = localSegment;
                    returnBitrate = _bitrates->at(i);
                }
            }
            if(!toReturn->getIsLoaded())toReturn->loadSegment();
            if(_lastBitrate != 0 && returnBitrate != this->_lastBitrate)
            {
                this->_bitrateDiscontinuity = true;
                this->_lastBitrate = returnBitrate;
            }
            else
            {
                this->_lastBitrate = returnBitrate;
            }
            return toReturn;
        }
        
    }
    else
    {
        if(!toReturn->getIsLoaded())toReturn->loadSegment();
        if(_lastBitrate != 0 && returnBitrate != this->_lastBitrate)
        {
            this->_bitrateDiscontinuity = true;
            this->_lastBitrate = returnBitrate;
        }
        else
        {
            this->_lastBitrate = returnBitrate;
        }
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

int32_t NetworkManager::setManualBitrate(int32_t whence)
{
    this->_automaticAdaptiveBitrate = false;
    if(this->_lastBitrate == 0)
    {
        std::cout << "last bitrate 0" << std::endl;
        if(whence > 0)
            this->_manualSelectedBitrateIndex = (int32_t)this->_bitrates->size() - 1;
        else
            this->_manualSelectedBitrateIndex = ((int32_t)this->_bitrates->size() - 2 >= 0)?(int32_t)this->_bitrates->size() - 2 : 0;
    }
    else
    {
        if(this->_manualSelectedBitrateIndex == -1)
        {
            for(int32_t i = 0 ; i < (int32_t)this->_bitrates->size(); i++)
            {
                if(this->_bitrates->at(i) == this->_lastBitrate)
                {
                    this->_manualSelectedBitrateIndex = i;
                    break;
                }
            }
        }
        if(whence > 0)
            this->_manualSelectedBitrateIndex = (this->_manualSelectedBitrateIndex + 1 > (int32_t)this->_bitrates->size() - 1)?
                this->_bitrates->size() - 1 : this->_manualSelectedBitrateIndex + 1;
        else
            this->_manualSelectedBitrateIndex = (this->_manualSelectedBitrateIndex - 1 < 0)?
            0:this->_manualSelectedBitrateIndex - 1;
    }
    std::cout << "BITRATE:" << this->_bitrates->at(this->_manualSelectedBitrateIndex) / 1000 << " Kbps" << std::endl;
    return this->_bitrates->at(this->_manualSelectedBitrateIndex);
}

void NetworkManager::setAutomaticBitrate()
{
    std::cout << "BITRATE:AUTOMATIC" << std::endl;
    this->_automaticAdaptiveBitrate = true;
    this->_manualSelectedBitrateIndex = -1;
}

void networkManagerThread(NetworkManager* networkManager)
{
    while(true)
    {
        int32_t start = (networkManager->_currentSegment - 30 < 0)?0:networkManager->_currentSegment - 30;
        int32_t end = (networkManager->_currentSegment + 30 > (int32_t)networkManager->_videoSegmentWeights.size())?
            networkManager->_videoSegmentWeights.size():networkManager->_currentSegment + 30;
        std::cout << "===== " << networkManager->_referenceBitrate / 1024 << " Kbps =====" << std::endl;
        for(int32_t j = 0; j < (int32_t)networkManager->_playlists->size(); j++)
        {
            std::cout << std::setw(15) << networkManager->_bitrates->at(j) / 1000 << " Kbps: " << start << " ";
            for(int32_t i = start;  i < end; i++)
            {
                PlaylistSegment* localSegment = networkManager->_playlists->at(j)->getSegments()->at(i);
                if(localSegment->getIsLoaded())std::cout << "*";
                else std::cout << "-";
                if(i == networkManager->_currentSegment)std::cout << "#";
            }
            std::cout << " " << end << std::endl;
        }
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
                
                if(networkManager->_automaticAdaptiveBitrate)
                {
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
                        if(networkManager->_referenceBitrate == INT32_MAX)networkManager->_referenceBitrate = networkManager->_playlists->at(selectedIndex)->getSegments()->at(i)->loadSegment();
                        else
                        {
                            networkManager->_referenceBitrate += networkManager->_playlists->at(selectedIndex)->getSegments()->at(i)->loadSegment();
                            networkManager->_referenceBitrate *= 0.4;
                        }
                    }
                }
                else
                {
                    bool hasLoadedHigherQuality = false;
                    for(int32_t j = 0; j < (int32_t)networkManager->_bitrates->size(); j++)
                    {
                        if(networkManager->_bitrates->at(networkManager->_manualSelectedBitrateIndex) <= networkManager->_bitrates->at(j) &&
                        networkManager->_playlists->at(j)->getSegments()->at(i)->getIsLoaded())
                            hasLoadedHigherQuality = true;
                    }
                    if(!hasLoadedHigherQuality)
                    {
                        if(networkManager->_referenceBitrate == INT32_MAX)
                        {
                            networkManager->_referenceBitrate = networkManager->_playlists->at(networkManager->_manualSelectedBitrateIndex)->getSegments()->at(i)->loadSegment();
                        }
                        else
                        {
                            networkManager->_referenceBitrate += networkManager->_playlists->at(networkManager->_manualSelectedBitrateIndex)->getSegments()->at(i)->loadSegment();
                            networkManager->_referenceBitrate *= 0.4;
                        }
                    }
                }
            }
        }
    }
}