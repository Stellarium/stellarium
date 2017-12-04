/////////////////////////////////////////////////////////////
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.
//
// ApnCamData.h:  Interface file for the CApnCamData class.
//
// Copyright (c) 2003-2007 Apogee Instruments, Inc.
//
/////////////////////////////////////////////////////////////

#if !defined(APN_CAMDATA_H__)
#define APN_CAMDATA_H__

#include <string>
#include <stdint.h>
#include "CamCfgMatrix.h"
#include "DefDllExport.h"

class DLL_EXPORT CApnCamData  
{
    public:

	    CApnCamData();
        CApnCamData(const CamCfg::APN_CAMERA_METADATA & meta,
            const CamCfg::APN_VPATTERN_FILE & vert,
            const CamCfg::APN_HPATTERN_FILE & clampNorm,
            const CamCfg::APN_HPATTERN_FILE & skipNorm,
            const CamCfg::APN_HPATTERN_FILE & roiNorm,
            const CamCfg::APN_HPATTERN_FILE & clampFast,
            const CamCfg::APN_HPATTERN_FILE & skipFast,
            const CamCfg::APN_HPATTERN_FILE & roiFast,
            const CamCfg::APN_VPATTERN_FILE & vertVideo,
            const CamCfg::APN_HPATTERN_FILE & clampVideo,
            const CamCfg::APN_HPATTERN_FILE & skipVideo,
            const CamCfg::APN_HPATTERN_FILE & roiVideo,
            const CamCfg::APN_HPATTERN_FILE & clampNormDual,
            const CamCfg::APN_HPATTERN_FILE & skipNormDual,
            const CamCfg::APN_HPATTERN_FILE & roiNormDual,
            const CamCfg::APN_HPATTERN_FILE & clampFastDual,
            const CamCfg::APN_HPATTERN_FILE & skipFastDual,
            const CamCfg::APN_HPATTERN_FILE & roiFastDual );

        CApnCamData( const CApnCamData &rhs );
        CApnCamData& operator=(CApnCamData const&d);
       
	    virtual ~CApnCamData();

        void Set(const std::string & path, 
                      const std::string & cfgFile,
                      uint16_t CamId);
        void Clear();
		void Write2File( const std::string & fname );

        CamCfg::APN_CAMERA_METADATA m_MetaData;

	    // Pattern Files
        CamCfg::APN_VPATTERN_FILE m_VerticalPattern;
	    CamCfg::APN_HPATTERN_FILE m_ClampPatternNormal;
	    CamCfg::APN_HPATTERN_FILE m_SkipPatternNormal;
	    CamCfg::APN_HPATTERN_FILE m_RoiPatternNormal;

	    CamCfg::APN_HPATTERN_FILE m_ClampPatternFast;
	    CamCfg::APN_HPATTERN_FILE m_SkipPatternFast;
	    CamCfg::APN_HPATTERN_FILE m_RoiPatternFast;

        CamCfg::APN_VPATTERN_FILE m_VerticalPatternVideo;
	    CamCfg::APN_HPATTERN_FILE m_ClampPatternVideo;
	    CamCfg::APN_HPATTERN_FILE m_SkipPatternVideo;
	    CamCfg::APN_HPATTERN_FILE m_RoiPatternVideo;

        CamCfg::APN_HPATTERN_FILE m_ClampPatternNormalDual;
	    CamCfg::APN_HPATTERN_FILE m_SkipPatternNormalDual;
	    CamCfg::APN_HPATTERN_FILE m_RoiPatternNormalDual;

	    CamCfg::APN_HPATTERN_FILE m_ClampPatternFastDual;
	    CamCfg::APN_HPATTERN_FILE m_SkipPatternFastDual;
	    CamCfg::APN_HPATTERN_FILE m_RoiPatternFastDual;

    private:
        std::string m_FileName;

        void LoadVertical( const std::string & name, 
            CamCfg::APN_VPATTERN_FILE & vpattern );

        void LoadHorizontal( const std::string & name, 
            CamCfg::APN_HPATTERN_FILE & hpattern );

        void WriteMeta( const std::string & fname );
        void WriteVPattern( const std::string & fname, 
            const CamCfg::APN_VPATTERN_FILE & vert );
        void WriteHPattern( const std::string & fname,
                                const CamCfg::APN_HPATTERN_FILE & horiztonal);

       
    

};

#endif // !defined APN_CAMDATA_H__
