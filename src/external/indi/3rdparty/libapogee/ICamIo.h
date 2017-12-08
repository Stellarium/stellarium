/*! 
* 
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class ICamIo 
* \brief interface class for all camera io 
* 
*/ 


#ifndef ICAMIO_INCLUDE_H__ 
#define ICAMIO_INCLUDE_H__ 


#include <string>
#include <vector>
#include <vector>
#include <stdint.h>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

#include "CameraStatusRegs.h" 

class ICamIo 
{ 
    public: 
        virtual ~ICamIo();
        
        virtual uint16_t GetFirmwareRev() = 0;

        /*!
         * Prime the IO interface for an upcoming image transfer.
         * \param[in] Rows Image height in pixels
         * \param[in] Cols Image width in pixels
         * \param[in] NumOfImages Number of images in the set
         */
         virtual void SetupImgXfer(uint16_t Rows, 
            uint16_t Cols, uint16_t NumOfImages, bool IsBulkSeq) = 0;

         /*!
         *  Cancel the pending image transfer.
         */
         virtual void CancelImgXfer() = 0;


        /*!
         *  Moves the data from the camera to the host PC.  Called at different
         *  times depending on the cameras acquistion mode
         * \param[in] NumberOfPixels Total number of pixels to transfer from the camera to host
         * \return ImageData Image from the camera
         */
        virtual void GetImageData( std::vector<uint16_t> & data ) = 0;	

        /*!
         *  Reads camera control registers
         * \param[in] reg Register to read.
         */
	    virtual uint16_t ReadReg( uint16_t reg ) const = 0;

         /*!
         *  Writes data to camera control registers
         * \param[in] reg Register to write to.
         * \param[out] val Data value in the register
         */
	    virtual void WriteReg( uint16_t reg, uint16_t val ) = 0;

        /*!
         *  Writes data to a Single Request Multiple Data (SRMD) controller on the camera
         * \param[in] reg Register to write to.
         * \param[out] data Data vector
         */
        virtual void WriteSRMD( uint16_t reg, const std::vector<uint16_t> & data ) = 0;

        /*!
         *  Writes data to a Multiple Request Multiple Data (MRMD) controller on the camera
         * \param[in] reg Register to write to.
         * \param[out] data Data vector
         */
        virtual void WriteMRMD( uint16_t reg, const std::vector<uint16_t> & data ) = 0;

        /*!
         *  Fetches the camera's status.  This function should be used on rev 15 firmware 
         *  or lower
         * \param[out] status Basic status structure
         */
        virtual void GetStatus(CameraStatusRegs::BasicStatus & status) = 0;

        /*!
         *  Fetches the camera's status.  This function should be used on rev 16 firmware 
         *  and higher
         * \param[out] status Advanced status structure
         */
        virtual void GetStatus(CameraStatusRegs::AdvStatus & status) = 0;

        virtual std::string GetInfo() = 0;

        virtual std::string GetDriverVersion() = 0;

        virtual bool IsError() = 0;
    protected:
        ICamIo();
}; 

#endif
