/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \brief This name space takes file names as input an pass out camera data structs 
* 
*/ 

#include "parseCfgTabDelim.h"
#include "helpers.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <bitset>

#ifdef WIN_OS
#include <regex>
#define OS_REGEX std::
#else
#include <boost/regex.hpp>
#define OS_REGEX boost
#endif


namespace
{
    const int APN_MAX_HBINNING = 20;
    const int APN_MAX_PATTERN_ENTRIES = 1024;


    //  READ    FILE
    std::string ReadFile( const std::string & fileName )
    {
        std::ifstream theFile;
        theFile.open( fileName.c_str() );

        if( theFile.fail() )
        {
            theFile.close();
            std::string msg = "Failed to open file " + fileName;
            std::runtime_error err( msg );
            throw( err );
        }

        std::stringstream buffer;
        buffer << theFile.rdbuf();
        theFile.close();

        std::string str( buffer.str() );

        return str;
    }
    
    //      TOKEN       NEW        LINES
    std::vector<std::string> TokenNewLines( std::string const & str )
    { 
		#ifdef WIN_OS
			return help::MakeTokens( str, "\n" );
		#else
			return help::MakeTokens( str, "\r\n" );
		#endif

    }

    // STR   2  BOOL
    bool Str2Bool(const std::string & str )
    {
        return ( 0 == str.compare("TRUE") ? true : false );
    }

    // HEX   STR  2   USHORT
    uint16_t HexStr2uShort(const std::string & str)
    {
        uint16_t val = 0;
        std::stringstream is( str );
        is >> std::hex >> val;
        return val;
    }

    //      STR  2  APN   AD    TYPE
    CamCfg::ApnAdType Str2ApnAdType(const std::string & str)
    {
        int val = 0;
        std::stringstream is( str );
        is >> val;

        switch( val )
        {
            case( CamCfg::ApnAdType_Alta_Sixteen ):
                return CamCfg::ApnAdType_Alta_Sixteen;
            break;

            case( CamCfg::ApnAdType_Alta_Twelve ):
                return CamCfg::ApnAdType_Alta_Twelve;
            break;

            case( CamCfg::ApnAdType_Ascent_Sixteen ):
                return CamCfg::ApnAdType_Ascent_Sixteen;
            break;

            default:
                return CamCfg::ApnAdType_None;
            break;   
        }

    }

    // CONVERT   BIN       LINE  2   DATA
    uint16_t ConvertBinLine2Data( const std::string & line )
    {
        const size_t LINE_LEN = 20;
        const int DATA_LEN = 16;
        const int START_POS = 3;
        const int STOP_POS = START_POS+DATA_LEN;

        std::vector<std::string> items = help::MakeTokens( line, "\t");

        if( items.size() != LINE_LEN )
        {
            std::string msg = "invalid number of items in line\n" + line;
            std::runtime_error err( msg );
            throw( err );
        }

        //copy bin pattern into a single string
        std::stringstream ss;
        std::copy(items.begin()+START_POS, items.begin()+STOP_POS,
            std::ostream_iterator<std::string>(ss)); 

        //convert binary string 
        std::bitset<16> str2Value( ss.str() );
        return( static_cast<uint16_t >(
            str2Value.to_ulong()) ); 
    }

    //  CREATE     DATA    VECT
    std::vector<uint16_t> CreateDataVect( const std::string & binStr )
    {
        std::vector<std::string> binLines = TokenNewLines( binStr );


        if( binLines.size() > APN_MAX_PATTERN_ENTRIES)
        {
            std::string msg("Too many pattern file entries");
            std::runtime_error err( msg );
            throw( err );
        }

        std::vector<uint16_t> binVect;
        std::vector<std::string>::iterator lineIter;

        for(lineIter = binLines.begin(); lineIter != binLines.end(); ++lineIter)
        {
            //check if it is a null string
            if( (*lineIter).size() > 0 )
            {
                binVect.push_back( ConvertBinLine2Data( (*lineIter) ) );
            }
        }

        return binVect;
    }

    //      GET      REGEX       MATCHES
    std::vector<std::string> GetRegExMatches(const std::string & pattern, std::string data)
    {
        OS_REGEX::regex re(pattern);
        OS_REGEX::sregex_iterator m1(data.begin(), data.end(), re);
        OS_REGEX::sregex_iterator m2;

        OS_REGEX::sregex_iterator iter;
        std::vector<std::string> matchStrs;
        for(iter = m1; iter != m2; ++iter)
        {
            matchStrs.push_back( (*iter).str() ); 
        }

        return matchStrs;
    }

    //      GET      MASK
    uint16_t GetMask(const std::string & str )
    {
        std::string pattern = ("\tMask.*?\\r?\\n");
        OS_REGEX::regex re(pattern);
        OS_REGEX::smatch theMatch;
        bool result = OS_REGEX::regex_search(str, theMatch, re);

        if( result )
        {
            return ConvertBinLine2Data( theMatch[0] );
        }

        return 0;
    }

}

//--------------------------------------------------------------------------------

//////////////////////////// 
//      IS     PATTERN    FILE
bool parseCfgTabDelim::IsPatternFile( const std::string & fileName )
{
    std::string str = ReadFile( fileName );
    std::string pattern = ("Pattern Set");
    OS_REGEX::regex re(pattern);
    OS_REGEX::smatch theMatch;
    return( OS_REGEX::regex_search(str, theMatch, re) );
}
//////////////////////////// 
//      IS     VERTICAL    FILE
bool parseCfgTabDelim::IsVerticalFile( const std::string & fileName )
{
    std::string str = ReadFile( fileName );
    std::string pattern = ("Vertical");
    OS_REGEX::regex re( pattern );
    OS_REGEX::smatch theMatch;
    return( OS_REGEX::regex_search(str, theMatch, re) );
}

//////////////////////////// 
//      IS     CFG    FILE
bool parseCfgTabDelim::IsCfgFile( const std::string & fileName )
{
    std::string str = ReadFile( fileName );
    std::string pattern = ("Configuration Matrix");
    OS_REGEX::regex re( pattern );
    OS_REGEX::smatch theMatch;
    return( OS_REGEX::regex_search(str, theMatch, re) );
}


//////////////////////////// 
// FETCH        HORIZONTAL     PATTERN
CamCfg::APN_HPATTERN_FILE parseCfgTabDelim::FetchHorizontalPattern(
    const std::string & fileName)
{
    
    //verifying the file is good to use
    if( !IsPatternFile( fileName ) )
    {
        std::string msg = " error file " + fileName + " is not a pattern file.";
        std::runtime_error err( msg );
        throw( err );
    }

    if( IsVerticalFile( fileName ) )
    {
        std::string msg = " error file " + fileName + " is not a horizontal file.";
        std::runtime_error err( msg );
        throw( err );
    }

     //get the file contents
     std::string str = ReadFile( fileName );

    CamCfg::APN_HPATTERN_FILE result;
    try
    {
        //set the mask data
        result.Mask = GetMask( str );
    
        //fill out the bin data
        std::vector<std::string> binMatches = GetRegExMatches("\tBIN\\s*[0-9]+\t(.|\\r?\\n)*?END.*?\\r?\\n", str );  

        //make sure we don't over run the number of
        //h bin
        if( APN_MAX_HBINNING < binMatches.size() )
        {
            std::string msg("Too many h bins");
            std::runtime_error err( msg );
            throw( err );
        }

        //convert each bin section into a data
        //vector and save it
        std::vector<std::string>::iterator binIter;
        for(binIter = binMatches.begin(); binIter != binMatches.end(); 
            ++binIter)
        {
            result.BinPatternData.push_back( 
                CreateDataVect( (*binIter) ) );
        }
   
        //fill out the ref data
        
        std::vector<std::string> refMatches = GetRegExMatches("\tREFERENCE(.|\\r?\\n)*?END.*?\\r?\\n", str );

        if( refMatches.size() > 1 )
        {
            std::string msg("Too many references");
            std::runtime_error err( msg );
            throw( err );
        }

        if( refMatches.size() == 1)
        {
            result.RefPatternData =  CreateDataVect( refMatches.at(0) );
        }

         //fill out the signal data
        
        std::vector<std::string> sigMatches = GetRegExMatches("\tSIGNAL+(.|\\r?\\n)*?END.*?\\r?\\n", str );

        if( sigMatches.size() > 1 )
        {
            std::string msg("Too many signals");
            std::runtime_error err( msg );
            throw( err );
        }

        if( sigMatches.size() == 1)
        {
            result.SigPatternData =  CreateDataVect( sigMatches.at(0) );
        }
     }
    catch( std::exception & e )
    {
        std::string msg( e.what() ); 
        msg += " : error in file " + fileName;
        std::runtime_error err( msg );
        throw( err );
    }

    return result;
}

//////////////////////////// 
// FETCH        VERTICAL      PATTERN
CamCfg::APN_VPATTERN_FILE parseCfgTabDelim::FetchVerticalPattern( 
       const std::string & fileName)
{
    //verifying the file is good to use
    if( !IsPatternFile( fileName ) )
    {
        std::string msg = " error file " + fileName + " is not a pattern file.";
        std::runtime_error err( msg );
        throw( err );
    }

    if( !IsVerticalFile( fileName ) )
    {
        std::string msg = " error file " + fileName + " is not a vertical file.";
        std::runtime_error err( msg );
        throw( err );
    }

     //get the file contents
     std::string str = ReadFile( fileName );
   
    //set the mask data
    CamCfg::APN_VPATTERN_FILE result;

    try
    {
        result.Mask = GetMask( str );

         //fill out the vertical data
        std::vector<std::string> vertMatches = GetRegExMatches("\tStart+(.|\\r?\\n)*?END.*?\\r?\\n", str );

        if( vertMatches.size() > 1 )
        {
            std::string msg("Too many vertical sections");
            std::runtime_error err( msg );
            throw( err );
        }

        if( vertMatches.size() == 1)
        {
            result.PatternData =  CreateDataVect( vertMatches.at(0) );
        }
    }
    catch( std::exception & e )
    {
        std::string msg( e.what() ); 
        msg += " : error in file " + fileName;
        std::runtime_error err( msg );
        throw( err );
    }

    return result;
}

//////////////////////////// 
// FETCH        META        DATA
void parseCfgTabDelim::FetchMetaData(const std::string & fileName,
        std::vector< std::shared_ptr<CamCfg::APN_CAMERA_METADATA> > & out)
{
    if( !IsCfgFile( fileName ) )
    {
        std::string msg = " error file " + fileName + " is a configuration matrix file.";
        std::runtime_error err( msg );
        throw( err );
    }

    //get the file contents
    std::string str = ReadFile( fileName );

    std::vector<std::string> lines = TokenNewLines( str );

    //iterate through all of the lines the file
    std::vector<std::string>::iterator lineIter;
    for( lineIter = lines.begin(); lineIter != lines.end(); ++lineIter)
    {
        std::vector<std::string> item = help::MakeTokens( (*lineIter), "\t");

        if( item.size() > 0 )
        {
            std::string firstItem(  item.at(0) );

            //if the line starts with the "StartCcd" tag
            //create a new sensor data object and push back
            //on the output vector
            if( 0 ==  firstItem.compare("StartCcd"))
            {
                std::shared_ptr<CamCfg::APN_CAMERA_METADATA> cfgData(
                    new CamCfg::APN_CAMERA_METADATA() );
               
                cfgData->Sensor = item.at(1);
                cfgData->CameraId = help::Str2uShort( item.at(2) );
                cfgData->CameraLine = item.at(3);
                cfgData->CameraModel = item.at(4);
                cfgData->InterlineCCD = Str2Bool( item.at(5) );
                cfgData->SupportsSerialA = Str2Bool( item.at(6) );
                cfgData->SupportsSerialB = Str2Bool( item.at(7) );
                cfgData->SensorTypeCCD = Str2Bool( item.at(8) );
                cfgData->TotalColumns = help::Str2uShort( item.at(9) );
                cfgData->ImagingColumns = help::Str2uShort( item.at(10) );
                cfgData->ClampColumns = help::Str2uShort( item.at(11) );
                cfgData->PreRoiSkipColumns = help::Str2uShort( item.at(12) );
                cfgData->PostRoiSkipColumns = help::Str2uShort( item.at(13) );
                cfgData->OverscanColumns = help::Str2uShort( item.at(14) );
                cfgData->TotalRows = help::Str2uShort( item.at(15) );
                cfgData->ImagingRows = help::Str2uShort( item.at(16) );
                cfgData->UnderscanRows = help::Str2uShort( item.at(17) );
                cfgData->OverscanRows = help::Str2uShort(item.at(18) );
                cfgData->VFlushBinning= help::Str2uShort( item.at(19) );
                cfgData->EnableSingleRowOffset = Str2Bool( item.at(20) );
	            cfgData->RowOffsetBinning = help::Str2uShort( item.at(21) );
	            cfgData->HFlushDisable= Str2Bool( item.at(22) );
	            cfgData->ShutterCloseDelay = help::Str2uShort( item.at(23) );
                cfgData->PixelSizeX = help::Str2Double( item.at(24) );
	            cfgData->PixelSizeY = help::Str2Double( item.at(25) );
	            cfgData->Color = Str2Bool( item.at(26) );
	            cfgData->ReportedGainSixteenBit = help::Str2Double( item.at(27) );
	            cfgData->MinSuggestedExpTime = help::Str2Double( item.at(28) );
	            cfgData->CoolingSupported= Str2Bool( item.at(29) );
	            cfgData->RegulatedCoolingSupported = Str2Bool( item.at(30) );
	            cfgData->TempSetPoint= help::Str2Double( item.at(31) );
	            cfgData->TempRampRateOne = help::Str2uShort( item.at(32) );
	            cfgData->TempRampRateTwo = help::Str2uShort( item.at(33) );
	            cfgData->TempBackoffPoint = help::Str2Double( item.at(34) );
	            cfgData->PrimaryADType = Str2ApnAdType( item.at(35) );
	            cfgData->AlternativeADType= Str2ApnAdType( item.at(36) );
	            cfgData->PrimaryADLatency = help::Str2uShort( item.at(37) );
	            cfgData->AlternativeADLatency= help::Str2uShort( item.at(38) );
	            cfgData->IRPreflashTime= help::Str2Double( item.at(39) );
                cfgData->AdCfg = help::Str2uShort( item.at(40) );
	            cfgData->DefaultGainLeft= help::Str2uShort( item.at(41) );
	            cfgData->DefaultOffsetLeft= help::Str2uShort( item.at(42) );
	            cfgData->DefaultGainRight= help::Str2uShort( item.at(43) );
	            cfgData->DefaultOffsetRight= help::Str2uShort( item.at(44) );
	            cfgData->DefaultRVoltage= help::Str2uShort( item.at(45) );
	            cfgData->DefaultDataReduction= Str2Bool( item.at(46) );
                cfgData->VideoSubSample= help::Str2uShort( item.at(47) );
                cfgData->AmpCutoffDisable= help::Str2uShort( item.at(48) );
                cfgData->NumAdOutputs = help::Str2uShort( item.at(49) );
                cfgData->SupportsSingleDualReadoutSwitching = Str2Bool( item.at(50) );
                cfgData->VerticalPattern= item.at(51);
                cfgData->SkipPatternNormal= item.at(52);
                cfgData->ClampPatternNormal= item.at(53);
                cfgData->RoiPatternNormal= item.at(54);
                cfgData->SkipPatternFast= item.at(55);
                cfgData->ClampPatternFast= item.at(56);
                cfgData->RoiPatternFast= item.at(57);
                cfgData->VerticalPatternVideo = item.at(58);
                cfgData->SkipPatternVideo= item.at(59);
                cfgData->ClampPatternVideo= item.at(60);
                cfgData->RoiPatternVideo= item.at(61);
                cfgData->SkipPatternNormalDual= item.at(62);
                cfgData->ClampPatternNormalDual= item.at(63);
                cfgData->RoiPatternNormalDual= item.at(64);
                cfgData->SkipPatternFastDual= item.at(65);
                cfgData->ClampPatternFastDual= item.at(66);
                cfgData->RoiPatternFastDual= item.at(67);

                out.push_back( cfgData );
            }
        }
    }
}

//////////////////////////// 
// FETCH        META        DATA
CamCfg::APN_CAMERA_METADATA parseCfgTabDelim::FetchMetaData(
        const std::string & fileName, uint16_t CamId )
{
     std::vector<std::shared_ptr<CamCfg::APN_CAMERA_METADATA> > newMetaVect;

    parseCfgTabDelim::FetchMetaData( fileName, newMetaVect );

    //search for the input cam id
    bool found = false;
    CamCfg::APN_CAMERA_METADATA result;
    std::vector<std::shared_ptr<CamCfg::APN_CAMERA_METADATA> >::iterator iter;
    for( iter = newMetaVect.begin(); iter != newMetaVect.end(); ++iter )
    {
        if( CamId == (*iter)->CameraId )
        {
            found = true;
            result = *(*iter);
            break;
        }
    }

    if( !found )
    {
        std::stringstream msg;
        msg << "Error did not find camera id " << CamId;
        msg << " in configuration file " << fileName.c_str();
        
        std::runtime_error err( msg.str() );
        throw( err );
    }

    return result;
}
