#ifndef _PSEGMENT_H_
#define _PSEGMENT_H_

#include <string>

class PlaylistSegment
{
    private:
        int _num;
        double _extinf;
        int _extXByterangeN;
        int _extXByterangeO;
        bool _isDiscontinuity;
        std::string _extXKey;
        std::string _extXMap;
        std::string _extXProgramDateTime;
        std::string _extXDateRange;
        void* _tsData;
        bool _isLoaded;

        std::string _endpoint;
    public:
        PlaylistSegment(int, std::string, std::string, std::string);

        static const std::string EXTINF_TAG;
        static const std::string EXT_X_BYTERANGE_TAG;
        static const std::string EXT_X_DISCONTINUITY_TAG;
        static const std::string EXT_X_KEY_TAG;
        static const std::string EXT_X_MAP_TAG;
        static const std::string EXT_X_PROGRAM_DATE_TIME_TAG;
        static const std::string EXT_X_DATERANGE_TAG;

        inline int getNum(){return _num;};
        inline double getExtInf(){return _extinf;};
        inline int getExtXByterangeN(){return _extXByterangeN;};
        inline int getExtXByteRangeO(){return _extXByterangeO;};
        inline bool getIsDiscontinuity(){return _isDiscontinuity;};
        inline std::string getExtXKey(){return _extXKey;};
        inline std::string getExtXMap(){return _extXMap;};
        inline std::string getExtXProgramDateTime(){return _extXProgramDateTime;};
        inline std::string getExtXDateRange(){return _extXDateRange;};

        inline std::string getEndpoint(){return _endpoint;};

        friend std::ostream& operator<<(std::ostream&, const PlaylistSegment&);
};

#endif