/*      dabd -- a DAB radio backend daemon for the Raspberry Pi
 *                Copyright  (C) 2019 schlizbaeda
 *                 mailto:himself@schlizbaeda.de
 * 
 * dabd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License
 * or any later version.
 * 
 * dabd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with dabgui.py. If not, see <http://www.gnu.org/licenses/>.
 *
 * 
 * 
 * This project is based on the MonkeyBoard DAB+/FM Digital Radio
 * Development Board Pro with Slideshow which can be found at
 * https://www.monkeyboard.org/products/85-developmentboard
 * 
 * TODO:
 * -----
 *   The MOT slideshow feature isn't implemented yet
 *   because there are some issues. It doesn't work properly!
 */

#define VERSION "0.1"

#include <iomanip>  // Manipulators like std::setw or std::setbase
#include <iostream> // std::cin and std::cout
#include <vector>   // "dynamic array" to hold substrings from ::split()

#include <iconv.h>  // https://www.gnu.org/software/libiconv/ 



/********** functions for another thread to read from stdin ***********/
//#include <chrono>
#include <thread>
#include <mutex>

std::mutex  stdinthr_mutex;
bool        stdinthr_reading;
std::string stdinthr_buf;

void stdinthr_readparallel() {
    // create "scoped locking" in C++ style:
    //std::lock_guard<mutex> lock(stdinthr_mutex); // doesn't work
    char c;
    stdinthr_reading = true;
    while(stdinthr_reading && std::cin.good()) {
        std::cin.get(c);
        stdinthr_mutex.lock();
        stdinthr_buf += c;
        stdinthr_mutex.unlock();
    }
}

int stdinthr_peek() {
    int len;
    stdinthr_mutex.lock();
    len = stdinthr_buf.length();
    stdinthr_mutex.unlock();
    return len;
}

std::string stdinthr_readline() {
    std::string lin;
    auto pos = stdinthr_buf.find("\n");
    if (pos == std::string::npos) { // buf contains no line feed
        stdinthr_mutex.lock();
        lin = stdinthr_buf;
        stdinthr_buf = "";
        stdinthr_mutex.unlock();
    } else {
        stdinthr_mutex.lock();
        lin = stdinthr_buf.substr(0, pos + 1);
        stdinthr_buf = stdinthr_buf.substr(pos + 1);
        stdinthr_mutex.unlock();
    }
    return lin;
}
    





/***************************** DAB radio  *****************************/
#include "../KeyStoneCOMM/KeyStoneCOMM.h"


#define RES_PASS 0
#define RES_WARN_NOTRUN 1
#define RES_WARN_OLDTEXT 2
#define RES_WARN_NONE 32767
#define RES_ERR_FAIL -1
#define RES_ERR_OPEN -2
#define RES_ERR_CLOSE -3
#define RES_ERR_SYNTAX -4
#define RES_ERR_TODO -5

#define VERBOSITY_NONE 0
#define VERBOSITY_FUNCT 1
#define VERBOSITY_RES 2
#define VERBOSITY_ERR 3
#define VERBOSITY_WARN 4
#define VERBOSITY_MSG 5
#define VERBOSITY_DETAIL 6
#define VERBOSITY_PROGRESS 7
#define VERBOSITY_DEBUG 8


#define DAB_MUXBLOCKS 41
#define KEYSTONE_BUFFER_SIZE 300

class KeyStone {
public:
    KeyStone(int verbosity) {
        m_verbosity = verbosity;
        m_serialopen = false;
        m_serialname = "/dev/ttyACM0";
        m_playmode = (char)0; // DAB mode
        
        m_programtext = "";
    }
    ~KeyStone() {
        int res;
        if (m_serialopen) {
            res = CloseSerial();
            if (res < RES_PASS) {
                std::cout << "*TODO: close anyway due to fatal error!!!"
                          << std::endl;
            }
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: Serial closing was initiated by "
                          << "the destructor ~KeyStone()!"
                          << std::endl;
            }
        }
    }
    
    static std::string DABBlockName(int idx) {
        std::string blockname;
        switch (idx) {
            case  0: blockname = "5A";  break;
            case  1: blockname = "5B";  break;
            case  2: blockname = "5C";  break;
            case  3: blockname = "5D";  break;

            case  4: blockname = "6A";  break;
            case  5: blockname = "6B";  break;
            case  6: blockname = "6C";  break;
            case  7: blockname = "6D";  break;

            case  8: blockname = "7A";  break;
            case  9: blockname = "7B";  break;
            case 10: blockname = "7C";  break;
            case 11: blockname = "7D";  break;

            case 12: blockname = "8A";  break;
            case 13: blockname = "8B";  break;
            case 14: blockname = "8C";  break;
            case 15: blockname = "8D";  break;

            case 16: blockname = "9A";  break;
            case 17: blockname = "9B";  break;
            case 18: blockname = "9C";  break;
            case 19: blockname = "9D";  break;

            case 20: blockname = "10A";  break;
            case 21: blockname = "10N";  break;
            case 22: blockname = "10B";  break;
            case 23: blockname = "10C";  break;
            case 24: blockname = "10D";  break;

            case 25: blockname = "11A";  break;
            case 26: blockname = "11N";  break;
            case 27: blockname = "11B";  break;
            case 28: blockname = "11C";  break;
            case 29: blockname = "11D";  break;

            case 30: blockname = "12A";  break;
            case 31: blockname = "12N";  break;
            case 32: blockname = "12B";  break;
            case 33: blockname = "12C";  break;
            case 34: blockname = "12D";  break;
            
            case 35: blockname = "13A";  break;
            case 36: blockname = "13B";  break;
            case 37: blockname = "13C";  break;
            case 38: blockname = "13D";  break;
            case 39: blockname = "13E";  break;
            case 40: blockname = "13F";  break;
            
            default: blockname = "";
        }
        return blockname;
    }
    
    int wchar_t2char(wchar_t *inbuf, char *outbuf) {
        /* This method converts a wchar_t* (wide char string)         *
         * returned by several functions of the KeyStoneCOMM.h        *
         * library, into a C++ string (std::string).                  *
         * It uses the GNU library libiconv for this purpose:         *
         * https://www.gnu.org/software/libiconv/                     */
        int res;
        iconv_t conversion_descriptor;
        
        char *fromcode = (char*)"WCHAR_T";
        char *tocode = (char*)"UTF-8";

        conversion_descriptor = iconv_open(tocode, fromcode);
        if (conversion_descriptor == (iconv_t)-1) {
            /* something went wrong: */
            if (errno == EINVAL) {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  wchar_t2string: "
                              << "conversion from \""
                              << fromcode << "\" to \""
                              << tocode << "\" not available."
                              << std::endl;
                }
                res = -2;
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  wchar_t2string: "
                              << "iconv_open(\"" << tocode << "\", \""
                              << fromcode << "\"); failed."
                              << std::endl;
                }
                res = -3;
            }
            /* Terminate an empty output string. */
            *outbuf = '\0';        
        } else {
            /* iconv_open(...) passed: Do the conversion: */
            char *inbufcast = (char*)inbuf;
            char *outbufptr = outbuf;
            size_t inbytesleft = KEYSTONE_BUFFER_SIZE;
            size_t outbytesleft = KEYSTONE_BUFFER_SIZE;    
//            size_t nconv;            
//            nconv = iconv(cd, &inbufcast, &inbytesleft,
//                          &outbufptr, &outbytesleft);
            iconv(conversion_descriptor,
                  &inbufcast, &inbytesleft,
                  &outbufptr, &outbytesleft);
            res = iconv_close(conversion_descriptor);
            if (res) {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  wchar_t2string: "
                              << "iconv_close(); failed."
                              << std::endl;
                }
            }
        }
        return res;
    }
    
    
    int OpenSerial() {
        int res;
        if (m_serialopen) {
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: OpenSerial not executed because "
                          << m_serialname << " is already open."
                          << std::endl;
            }
        } else { // m_serialopen==false
            if (m_verbosity >= VERBOSITY_DETAIL) {
                std::cout << "opening " << m_serialname << "..."
                          << std::endl;
            }
            m_serialopen = ::OpenRadioPort((char*)m_serialname.data(),
                                           true);
            res = m_serialopen ? RES_PASS : RES_ERR_OPEN;
            if (res >= RES_PASS) {
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  OpenSerial: "
                              << m_serialname << " opened."
                              << std::endl;
                }
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  OpenSerial: "
                              << m_serialname << " opening failed."
                              << std::endl;
                }
            }
        }
        return res;
    }
    int CloseSerial() {
        int res;
        if (m_serialopen) {
            res = ::CloseRadioPort();
            if (res) {
                m_serialopen = false;
                res = RES_PASS;
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  CloseSerial: "
                              << m_serialname << " closed."
                              << std::endl;
                }
            } else {
                res = RES_ERR_CLOSE;
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  CloseSerial: "
                              << m_serialname << " closing failed."
                              << std::endl;
                }
            }            
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: CloseSerial not executed because "
                          << m_serialname << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int GetPlayMode(char *mode) { // 0==DAB, 1==FM
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            m_playmode = ::GetPlayMode();
            *mode = m_playmode;
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetPlayMode=="
                          << (int)*mode
                          << " (" << (*mode ? "FM" : "DAB") << ")."
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetPlayMode not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    int SetPlayMode(char mode) { // 0==DAB, 1==FM
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            m_playmode = mode;
            if (m_verbosity >= VERBOSITY_FUNCT) {
                std::cout << "*TODO: "
                          << "switch play mode (FM/DAB) when playing..."
                          << std::endl;
            }
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  SetPlayMode=="
                          << (int)mode
                          << " (" << (mode ? "FM" : "DAB") << ")."
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: SetRadioMode not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int DoScan(void) {
        char radiostatus;
        char freq;
        long totalprogram;
        char oldfreq = -1;
        long oldtotalprogram = -1;
        int res;
        
        if (m_serialopen) {
            if (m_playmode) { // FM mode
                res = RES_ERR_TODO;
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  TODO: "
                              << "DoScan FM not implemented yet."
                              << std::endl;
                }
                // TODO!
            } else { // DAB mode
                if (m_verbosity >= VERBOSITY_DETAIL) {
                    std::cout << "Searching for DAB stations...";
                    // clear buffer to write the text immediately:
                    std::cout.flush();
                }
                if (::DABAutoSearch(0, DAB_MUXBLOCKS - 1) == true) {
                    radiostatus = 1;
                    while (radiostatus == 1) {
                        freq = ::GetFrequency();
                        totalprogram = ::GetTotalProgram();
                        if (oldfreq != freq ||
                                oldtotalprogram != totalprogram) {
                            if (m_verbosity >= VERBOSITY_DETAIL) {
                                std::cout << "\nScanning index "
                                          << (int)freq
                                          << " (DAB multiplex block \""
                                          << DABBlockName(freq)
                                          << "\"),"
                                          << " found " << totalprogram
                                          << " programs";
                                // clear buffer to write the text
                                // immediately:
                                std::cout.flush();
                            }
                            oldfreq = freq;
                            oldtotalprogram = totalprogram;
                        }
                        else {
                            if (m_verbosity >= VERBOSITY_PROGRESS) {
                                std::cout << ".";
                                // clear buffer to write the dots
                                // immediately:
                                std::cout.flush();
                            }
                        }
                        radiostatus = ::GetPlayStatus();
                    }
                    if (m_verbosity >= VERBOSITY_DETAIL) {
                        std::cout << std::endl;
                    }
                    res = RES_PASS;
                    totalprogram = ::GetTotalProgram();
                    if (m_verbosity >= VERBOSITY_MSG) {
                        std::cout << "*MSG:  DoScan==" << totalprogram
                                  << " programs found totally."
                                  << std::endl;
                    }
                } else {
                    res = RES_ERR_FAIL;
                    // DABAutoSearch failed
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  "
                                  << "DoScan.DABAutoSearch failed."
                                  << std::endl;
                    }
                }
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: DoScan not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int DABProgramList(void) {
        long totalprogram;
        long i;
        int res;
        
        unsigned char ServiceComponentID;
        uint32 ServiceID;
        uint16 EnsembleID;
        
        if (m_serialopen) {
            totalprogram = ::GetTotalProgram();
            res = totalprogram > 0 ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                for(i = 0; i < totalprogram; i++) {
                    if(::GetProgramName(m_playmode, i, 1, wbuf)) {
                        wchar_t2char(wbuf, buf);
                        if (m_verbosity >= VERBOSITY_DETAIL) {
                            std::cout << "list index=";
                            std::cout << std::setbase(10)
                                      << std::setw(3)
                                      << std::setfill(' ')
                                      << i << ", ";
                            std::cout << "NAME=\"" << buf << "\"";
                        }
                        
                        if(::GetProgramInfo(i,
                                            &ServiceComponentID,
                                            &ServiceID,
                                            &EnsembleID)) {
                            if (m_verbosity >= VERBOSITY_DETAIL) {
                                std::cout << ", ServiceComponentID="
                                          << std::setbase(16)
                                          << std::setw(2)
                                          << std::setfill('0')
                                          << (int)ServiceComponentID;
                                std::cout << ", ServiceID="
                                          << std::setw(8)
                                          << ServiceID;
                                std::cout << ", EnsembleID="
                                          << std::setw(4)
                                          << EnsembleID;                                
                            }
                        }
                        else {
                            res = RES_ERR_FAIL;
                            if (m_verbosity >= VERBOSITY_ERR) {
                                std::cout << "*ERR:  DABProgramList."
                                          << "GetProgramInfo() failed "
                                          << "for index " << i;
                            }
                        }
                        
                        if(::GetEnsembleName(i, 1, wbuf)) {
                            wchar_t2char(wbuf, buf);
                            if (m_verbosity >= VERBOSITY_DETAIL) {
                                std::cout << ", EnsembleName=\""
                                          << buf << "\"";
                            }
                        }
                        else {
                            res = RES_ERR_FAIL;
                            if (m_verbosity >= VERBOSITY_ERR) {
                                std::cout << "*ERR:  DABProgramList."
                                          << "GetEnsembleInfo() failed "
                                          << "for index " << i;
                            }
                        }
                    }
                    else {
                        res = RES_ERR_FAIL;
                        if (m_verbosity >= VERBOSITY_ERR) {
                            std::cout << "*ERR:  DABProgramList."
                                      << "GetProgramName() failed "
                                      << "for index " << i;
                        }
                    }
                    /* add line feed: */
                    if (m_verbosity >= VERBOSITY_DETAIL) {
                        std::cout << std::setbase(10)
                                  << std::setw(0)
                                  << std::endl; // line feed
                    }
                }
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  DABProgramList=="
                              << totalprogram
                              << " programs found totally"
                              << (res == RES_PASS ? "" : "(with errors)")
                              << "." << std::endl;
                }
            } else { // GetTotalProgram() failed:
                if (totalprogram) { // totalprogram < 0
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  DABProgramList."
                                  << "GetTotalProgram failed with "
                                  << "exitcode=="
                                  << totalprogram << "." << std::endl;
                    }
                } else { // totalprogram == 0
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  DABProgramList==0 "
                                  << "programs found totally."
                                  << std::endl;
                    }
                }
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: DABProgramList not executed "
                          << "because " << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int GetVolume(char *volume) {
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *volume = ::GetVolume();
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetVolume=="
                          << (int)*volume
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetVolume not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    int SetVolume(char volume) {
        int res;
        if (m_serialopen) {
            if (volume >= '0' && volume <= '9') { // char to byte
                volume -= '0'; // change chars '0'..'9' into byte values
            }
            if (volume <= 16) { // SetVolume(...)
                res = ::SetVolume(volume) ? RES_PASS : RES_ERR_FAIL;
                if (res == RES_PASS) {
                    if (m_verbosity >= VERBOSITY_MSG) {
                        std::cout << "*MSG:  SetVolume=="
                                  << (int)volume
                                  << std::endl;
                    }
                } else { // ::SetVolume(...) failed
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  SetVolume(" << volume
                                  << ") failed."
                                  << std::endl;
                    }
                }
            } else if (volume == '+') { // VolumePlus()
                res = RES_ERR_TODO;
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  TODO: implementation of "
                              << "VolumePlus()..."
                              << std::endl;
                }
            } else if (volume == '-') { // VolumeMinus()
                res = RES_ERR_TODO;
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  TODO: implementation of "
                              << "VolumeMinus()..."
                              << std::endl;
                }
            } else { // wrong value for volume:
                res = RES_WARN_NOTRUN;
                if (m_verbosity >= VERBOSITY_WARN) {
                    std::cout << "*WARN: SetVolume not executed due to "
                              << "wrong value " << (int)volume
                              << std::endl;
                }
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: SetVolume not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int GetStereo(char *stereo) {
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *stereo = ::GetStereo();
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetStereo=="
                          << (int)*stereo
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetStereo not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }       
    int GetStereoMode(char *mode) {
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *mode = ::GetStereoMode();
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetStereoMode=="
                          << (int)*mode
                          << " (" << (*mode ? "stereo" : "mono") << ")."
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetStereoMode not executed "
                         << "because " << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    int SetStereoMode(char mode) {
        int res;
        if (m_serialopen) {
            res = ::SetStereoMode(mode) ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  SetStereoMode=="
                              << (int)mode
                              << " (" << (mode ? "stereo" : "mono")
                              << ")."
                              << std::endl;
                }
            } else { // ::SetVolume(...) failed
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  SetStereoMode(" << mode
                              << ") failed."
                              << std::endl;
                }
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: SetStereoMode not executed "
                          << "because " << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    int PlayStream(unsigned long channel) {
        int res;
        long totalprogram;
        if (m_serialopen) {
            m_programtext = ""; // delete buffered program text!
            
            if (m_playmode) { // FM mode
                res = channel >= 87000 && 
                      channel <= 108000 ? RES_PASS : RES_ERR_FAIL;
                if (res != RES_PASS) {
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  FM frequency "
                                  << (float)channel / 1000.0 
                                  << " is'nt within 87.0MHz and 108.0MHz."
                                  << std::endl;
                    }
                }
            } else { // DAB mode
                totalprogram = ::GetTotalProgram();
                res = channel >= 0 && 
                      (long)channel < totalprogram ? RES_PASS : RES_ERR_FAIL;
                if (res != RES_PASS) {
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  DAB program " << channel 
                                  << " is beyond 0 and "
                                  << totalprogram - 1 << "."
                                  << std::endl;
                    }
                }
            }
            
            /* play radio stream: either FM or DAB */
            if (res == RES_PASS) { // start radio stream
                res = ::PlayStream(m_playmode, channel) ? RES_PASS : RES_ERR_FAIL;
                if (res == RES_PASS) {
                    if (m_verbosity >= VERBOSITY_MSG) {
                        std::cout << "*MSG:  "
                                  << (m_playmode ? "FM" : "DAB")
                                  << " radio stream started playing."
                                  << std::endl;
                    }
                    if (m_playmode) { // FM
                        
                    }
                    else { // DAB mode
                    }
                }
                else { // an error occurred
                    if (m_verbosity >= VERBOSITY_ERR) {
                        std::cout << "*ERR:  PlayStream(" << m_playmode
                                  << ", " << channel <<") failed."
                                  << std::endl;
                    }
                }
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: PlayStream not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    int StopStream() {
        int res;
        if (m_serialopen) {
            m_programtext = ""; // delete buffered program text!
            res = ::StopStream() ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  "
                              << (m_playmode ? "FM" : "DAB")
                              << " radio stream stopped."
                              << std::endl;
                }
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  StopStream failed."
                              << std::endl;
                }
            }            
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: StopStream not executed because "
                          << m_serialname << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    
    /* obtain several information on the current radio stream */
    int GetTotalProgram(long *count) { // returns number of all DAB(?) programs
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *count = ::GetTotalProgram(); 
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetTotalProgram=="
                          << *count
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetTotalProgram not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetPlayIndex(long *idx) { // TODO: short info!
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *idx = ::GetPlayIndex(); 
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetPlayIndex=="
                          << *idx
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetPlayIndex not executed because "
                          << m_serialname << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetPlayStatus(char* status) { // status: 0=playing 1=scanning 2=? 3=stopped
        int res;
        std::string statustext;
        
        if (m_serialopen) {
            res = RES_PASS;
            *status = ::GetPlayStatus(); 
            if (m_verbosity >= VERBOSITY_MSG) {
                if (*status == 0) {
                    statustext = "playing stream";
                } else if (*status == 1) {
                    statustext = "scanning";
                } else if (*status == 2) { // should be a defined status
                    statustext = "?";
                } else if (*status == 3) {
                    statustext = "stopped stream";
                } else { 
                    statustext = "unknown play status";
                }
                std::cout << "*MSG:  GetPlayStatus=="
                          << (int)*status
                          << " (" << statustext << ")"
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetPlayStatus not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetSignalStrength(char* strength, int *bitError) { // TODO: short info!
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *strength = ::GetSignalStrength(bitError); 
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetSignalStrength=="
                          << (int)*strength
                          << ", bitError=="
                          << *bitError
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetSignalStrength not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetDataRate(int *datarate) { // TODO: short info!
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *datarate = ::GetDataRate(); 
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetDataRate=="
                          << *datarate
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetDataRate not executed because "
                          << m_serialname << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetSamplingRate(int *samplingrate) { // TODO: short info!
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *samplingrate = ::GetSamplingRate(); 
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  GetSamplingRate = "
                          << *samplingrate
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetSamplingRate not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetProgramName(long dabindex, std::string *programname) { // returns the name of the indexed DAB program
        int res;
        if (m_serialopen) {
            res = ::GetProgramName(m_playmode, dabindex, 1, wbuf) ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                wchar_t2char(wbuf, buf);
                *programname = std::string(buf); // create a copy from buf!
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  GetProgramName(" << dabindex
                              << ")==\"" << *programname << "\""
                              << std::endl;
                }
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  GetProgramName(" << dabindex
                              << ") failed."
                              << std::endl;
                }
            }            
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetProgramName not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetProgramText(std::string *programtext) { // returns additional information
        int res;
        int verbosity_level;
        std::string verbosity_label;
        
        if (m_serialopen) {
            if (0 == ::GetProgramText(wbuf)) { // data received
                wchar_t2char(wbuf, buf);
                m_programtext = std::string(buf); // create a copy from buf!
                res = RES_PASS;
                verbosity_level = VERBOSITY_MSG;
                verbosity_label = "*MSG:  ";
            } else { // return old data
                res = RES_WARN_OLDTEXT;
                verbosity_level = VERBOSITY_WARN;
                verbosity_label = "*WARN: ";
            }
            *programtext = m_programtext;
            if (m_verbosity >= verbosity_level) {
                std::cout << verbosity_label
                          << "GetProgramText==\""
                          << *programtext << "\""
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetProgramText not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetProgramInfo(long dabindex,
                       unsigned char *serviceComponentID,
                       uint32 *serviceID,
                       uint16 *ensembleID) { // TODO: short info!
        int res;
        if (m_serialopen) {
            res = ::GetProgramInfo(dabindex,
                                   serviceComponentID, 
                                   serviceID,
                                   ensembleID) ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  GetProgramInfo("
                              << dabindex << "): "
                              << "serviceComponentID=="
//                              << std::setbase(16)
//                              << std::setw(2)
//                              << std::setfill('0')
                              << (int)*serviceComponentID
                              << ", ServiceID=="
//                              << std::setw(8)
                              << *serviceID
                              << ", EnsembleID=="
//                              << std::setw(4)
                              << *ensembleID
                              << std::endl;
                }
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  GetProgramInfo failed."
                              << std::endl;
                }
            }            
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: StopStream not executed because "
                          << m_serialname << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetEnsembleName(long dabindex,
                        char namemode,
                        std::string *ensemblename) { // returns the multiplex block name of the indexed DAB program
        int res;
        if (m_serialopen) {
            res = ::GetEnsembleName(dabindex, namemode, wbuf) ? RES_PASS : RES_ERR_FAIL;
            if (res == RES_PASS) {
                wchar_t2char(wbuf, buf);
                *ensemblename = std::string(buf); // create a copy from buf!
                if (m_verbosity >= VERBOSITY_MSG) {
                    std::cout << "*MSG:  GetEnsembleName(" << dabindex
                              << ", " << (int)namemode
                              << ")=\"" << *ensemblename << "\""
                              << std::endl;
                }
            } else {
                if (m_verbosity >= VERBOSITY_ERR) {
                    std::cout << "*ERR:  GetEnsembleName(" << dabindex
                              << ", " << (int)namemode
                              << ") failed."
                              << std::endl;
                }
            }            
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetEnsembleName not executed "
                          << "because " << m_serialname
                          << " is already closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetFrequency(char *freq) { // TODO: check if it works correctly?
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            *freq = ::GetFrequency();
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  Getfrequency=="
                          << (int)*freq
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: GetFrequency not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }

    /* get slideshow images -- TODO: not implemented yet! */
    int MotReset() { // reset/initialize MOT mode (slideshow)
        int res;
        if (m_serialopen) {
            res = RES_PASS;
            ::MotReset(MOT_HEADER_MODE);
            if (m_verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  MotReset==MOT_HEADER_MODE"
                          << std::endl;
            }
        } else { // m_serialopen==false
            res = RES_WARN_NOTRUN;
            if (m_verbosity >= VERBOSITY_WARN) {
                std::cout << "*WARN: MotReset not executed because "
                          << m_serialname << " is closed."
                          << std::endl;
            }
        }
        return res;
    }
    int GetMotSlideshowImage(std::string *image) {
        int res = RES_ERR_TODO;
        if (m_verbosity >= VERBOSITY_ERR) {
            std::cout << "*ERR:  GetMotSlideshowImage "
                      << "not implemented yet!"
                      << std::endl;
        }
        
//        if (m_serialopen) {
////std::cout << "MotQuery==";
//            if (::MotQuery()) {
////std::cout << "true\n";
//                ::GetImage(wbuf);
//                wchar_t2char(wbuf, buf);
////std::cout << "slideshow==\"" << buf << "\"" << std::endl;
//                res = RES_PASS;
//            } else { // ::MotQuery() failed (result == false)
//std::cout << ".";
//std::cout.flush();
//                res = RES_ERR_FAIL;
//                if (m_verbosity >= VERBOSITY_ERR) {
//                    std::cout << "*ERR:  "
//                              << "GetMotSlideshowImage.MotQuery==0 "
//                              << "(false)"
//                              << std::endl;
//                }
//            }
//        } else { // m_serialopen==false
//            res = RES_WARN_NOTRUN;
//            if (m_verbosity >= VERBOSITY_WARN) {
//                std::cout << "*WARN: GetMotSlideshowImage not executed "
//                          << "because " << m_serialname << " is closed."
//                          << std::endl;
//            }
//        }
        return res;
    }
    
    
private:
    int           m_verbosity;
    bool          m_serialopen;
    std::string   m_serialname;
    char          m_playmode;   // 0==DAB, 1==FM
    
    std::string   m_programtext;
    
    wchar_t wbuf[KEYSTONE_BUFFER_SIZE];
    char buf[KEYSTONE_BUFFER_SIZE];
};
        

std::vector<std::string> split(const std::string &s,
                               char separator,
                               bool addempty) {
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(separator, pos)) != std::string::npos) {
        std::string substring(s.substr(prev_pos, pos - prev_pos));
        if (addempty || substring.length()) {
            output.push_back(substring);
        }
        prev_pos = ++pos;
    }
    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
    return output;
}

int main(int argc, char *argv[]) {
    // create and start a separate thread to read from stdin:
    stdinthr_buf = "";
    std::thread stdinthread(stdinthr_readparallel);
    std::string stdinline;
    std::vector<std::string> param;
    
    int verbosity = VERBOSITY_DEBUG;
    
    size_t param_errpos;
    unsigned long param_ulong;
    long          param_long;
    int           param_int;
    uint32        param_uint32;
    uint16        param_uint16;
    unsigned char param_uchar;
    char          param_char;
    std::string   param_string;
    
    
    KeyStone dabradio(verbosity);
    long dabindex;
    int res;
    
    
    param.push_back("help");
    while (param[0] != "exit" && param[0] != "quit") {
        if (stdinthr_peek()) { // is a command available?
            stdinline = stdinthr_readline();
            stdinline.erase(stdinline.length() - 1); // remove "\n"
            param = split(stdinline, ' ', false);    // split parameters
        }
        
        /* stdin command parser */
        res = RES_WARN_NONE;
        if (param[0] == "" || 
            param[0].substr(0, 1) == "#") { // empty command or comment
                // do nothing:
                param[0] = "";
        } else if (param[0] == "help") {
            //res = RES_PASS;
            if (param.size() == 1 ||
                (param.size() > 1 && param[1] == "help")) {
                std::cout << argv[0] << " -- help\n"
                          << "  help                   show this help screen" << "\n"
                          << "  help <command>         show detailed help on the given command" << "\n"
                          << "  open                   open serial connection" << "\n"
                          << "  close                  close serial connection" << "\n"
                          << "  get <property>         get detailed information with \"help get\"" << "\n"
                          << "  set <property> <value> get detailed information with \"help set\"" << "\n"
                          << "  scan                   scan all receivable programs and stores them" << "\n"
                          << "  list                   print a list of all stored programs" << "\n"
                          << "  playstream <channel>   start playing the program <channel>" << "\n"
                          << "  stopstream             stop playing the current program" << "\n"
                          << "  #<comment>             a comment line which does nothing" << "\n"
                          << "  ver                    display the program version (v" << VERSION << ")\n" 
                          << "  sleep <ms>             delay time in milliseconds" << "\n"
                          << "  exit || quit           exit/quit this application" << "\n"
                          << "\n"
                          << "enter these commands for getting started:\n"
                          << "-----------------------------------------\n"
                          << "  open\n"
                          << "  set volume 9\n"
                          << "  set stereo 1\n"
                          << "  # scan     # only if necessary\n"
                          << "  list\n"
                          << "  playstream 4\n"
                          << "  close\n"
                          << "  quit\n";
            } else if (param[1] == "open") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  open the connection to the MonkeyBoard\n";
            } else if (param[1] == "close") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  close the connection to the MonkeyBoard\n";
            } else if (param[1] == "get") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  get the current value of the given property.\n"
                          << "\n"
                          << "Properties:\n"
                          << "  get playmode           playmode: 0=DAB, 1=FM" << "\n"
                          << "  get volume             volume: 0..16" << "\n"
                          << "  get stereo             stereo mode: 0=mono, 1=stereo" << "\n"
                          << "  get totalprogram       total number of stored programs" << "\n"
                          << "  get playindex          index of currently playing program: 0..totalprogram-1" << "\n"
                          << "  get playstatus         playstatus: 0=playing, 1=scanning, 2=?, 3=stopped" << "\n"
                          << "  get signalstrength     0%..100% (a value below 20% isn't sufficient)" << "\n"
                          << "  get datarate           data rate in kbit/s" << "\n"
                          << "  get samplingrate       sampling rate in kHz" << "\n"
                          << "  get programname <cha>  name of the given channel" << "\n"
                          << "  get programtext        additional text sent by the radio station" << "\n"
                          << "  get programinfo <cha>  serviceComponentID, ServiceID, EnsembleID" << "\n"
                          << "  get ensemblename <cha> name of the DAB multiplex block" << "\n";
            } else if (param[1] == "set") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  set the value of the given property.\n"
                          << "\n"
                          << "Properties:\n"
                          << "  set playmode           playmode: 0=DAB, 1=FM" << "\n"
                          << "  set volume             volume: 0..16" << "\n"
                          << "  set stereo             stereo mode: 0=mono, 1=stereo" << "\n";
            } else if (param[1] == "scan") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  scan all DAB multiplex blocks for receivable programs\n"
                          << "  and store them in the internal memory of the MonkeyBoard\n";
            } else if (param[1] == "list") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  list all programs stored in the internal memory of the MonkeyBoard\n";
            } else if (param[1] == "playstream") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  start playback of the program defined by the given channel\n";
            } else if (param[1] == "stopstream") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  stop playback of the currently playing program\n";
            } else if (param[1] == "ver") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  print the program version to stdout.\n"
                          << "  The verbosity level must be higher or equal to "
                          << VERBOSITY_MSG << ".\n";
            } else if (param[1] == "sleep") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  perform a delay of the given value in milliseconds\n"
                          << "  this may be helpful in command scripts\n";
            } else if (param[1] == "exit" || param[1] == "quit") {
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  leave this application\n";
            } else { // unknown help command
                std::cout << argv[0] << " -- help " << param[1] << "\n"
                          << "  unknown command \"" << param[1] << "\"\n";
            }
            std::cout << std::endl;
            param[0] = "";
        } else if (param[0] == "open") {
            res = dabradio.OpenSerial();
            param[0] = "";
        } else if (param[0] == "close") {
            res = dabradio.CloseSerial();
            param[0] = "";
        } else if (param[0] == "get") {
            if (param.size() >= 2) {
                if (param[1] == "playmode") {
                    res = dabradio.GetPlayMode(&param_char);
                } else if (param[1] == "volume") {
                    res = dabradio.GetVolume(&param_char);
                } else if (param[1] == "stereo") {
                    res = dabradio.GetStereoMode(&param_char);
                } else if (param[1] == "totalprogram") {
                    res = dabradio.GetTotalProgram(&param_long);
                } else if (param[1] == "playindex") {
                    res = dabradio.GetPlayStatus(&param_char);
                    if (res == RES_PASS && param_char == 0) {
                        res = dabradio.GetPlayIndex(&param_long);
                    } else {
                        param_long = -1;
                        if (verbosity >= VERBOSITY_ERR) {
                            std::cout << "*ERR:  GetPlayIndex=="
                                      << param_long
                                      << std::endl;
                        }
                    }
                } else if (param[1] == "playstatus") {
                    res = dabradio.GetPlayStatus(&param_char);
                } else if (param[1] == "signalstrength") {
                    res = dabradio.GetSignalStrength(&param_char,
                                                     &param_int);
                } else if (param[1] == "datarate") {
                    res = dabradio.GetDataRate(&param_int);
                } else if (param[1] == "samplingrate") {
                    res = dabradio.GetSamplingRate(&param_int);
                } else if (param[1] == "programname") {
                    if (param.size() >= 3) {
                        try {
                            dabindex = std::stol(param[2],
                                                 &param_errpos);
                        }
                        catch (...) {
                            res = RES_ERR_SYNTAX;
                        }
                        if (param_errpos < param[2].length()) {
                            // don't accept partial conversion:
                            res = RES_ERR_SYNTAX;
                        }
                    } else { // no dabindex given
                        // take the current playing index
                        dabindex = -1;
                        res = dabradio.GetPlayStatus(&param_char);
                        if (res == RES_PASS && param_char == 0) {
                            res = dabradio.GetPlayIndex(&param_long);
                            if (res == RES_PASS) {
                                dabindex = param_long;
                            }
                        }
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.GetProgramName(dabindex,
                                                      &param_string);
                    }
                } else if (param[1] == "programtext") {
                    res = dabradio.GetProgramText(&param_string);
                } else if (param[1] == "programinfo") {
                    if (param.size() >= 3) {
                        try {
                            dabindex = std::stol(param[2],
                                                 &param_errpos);
                        }
                        catch (...) {
                            res = RES_ERR_SYNTAX;
                        }
                        if (param_errpos < param[2].length()) {
                            // don't accept partial conversion:
                            res = RES_ERR_SYNTAX;
                        }
                    } else { // no dabindex given
                        // take the current playing index
                        dabindex = -1;
                        res = dabradio.GetPlayStatus(&param_char);
                        if (res == RES_PASS && param_char == 0) {
                            res = dabradio.GetPlayIndex(&param_long);
                            if (res == RES_PASS) {
                                dabindex = param_long;
                            }
                        }
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.GetProgramInfo(dabindex,
                                                      &param_uchar,
                                                      &param_uint32,
                                                      &param_uint16);
                    }
                } else if (param[1] == "ensemblename") {
                    if (param.size() >= 3) {
                        try {
                            dabindex = std::stol(param[2],
                                                 &param_errpos);
                        }
                        catch (...) {
                            res = RES_ERR_SYNTAX;
                        }
                        if (param_errpos < param[2].length()) {
                            // don't accept partial conversion:
                            res = RES_ERR_SYNTAX;
                        }
                    } else { // no dabindex given
                        // take the current playing index
                        dabindex = -1;
                        res = dabradio.GetPlayStatus(&param_char);
                        if (res == RES_PASS && param_char == 0) {
                            res = dabradio.GetPlayIndex(&param_long);
                            if (res == RES_PASS) {
                                dabindex = param_long;
                            }
                        }
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.GetEnsembleName(dabindex,
                                                       0,
                                                       &param_string);
                    }
                } else if (param[1] == "frequency") {
                    res = dabradio.GetFrequency(&param_char);
                } else { // unknown get property
                    res = RES_ERR_SYNTAX;
                }
            } else { // missing parameter
                res = RES_ERR_SYNTAX;
            }
            param[0] = "";
        } else if (param[0] == "set") {
            if (param.size() >= 3) {
                if (param[1] == "playmode") {
                    if (param[2] == "dab") {
                        param_char = 0;
                    } else if (param[2] == "fm") {
                        param_char = 1;
                    } else {
                        try {
                            param_char = (char)std::stoi(param[2],
                                                         &param_errpos);
                        }
                        catch (...) {
                            res = RES_ERR_SYNTAX;
                        }
                        if (param_errpos < param[2].length()) {
                            // don't accept partial conversion:
                            res = RES_ERR_SYNTAX;
                        }
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.SetPlayMode(param_char);
                    }    
                } else if (param[1] == "volume") {
                    if (param[2] == "+") {
                        param_char = '+';
                    } else if (param[2] == "-") {
                        param_char = '-';
                    } else {
                        try {
                            param_char = (char)std::stoi(param[2],
                                                         &param_errpos);
                        }
                        catch (...) {
                            res = RES_ERR_SYNTAX;
                        }
                        if (param_errpos < param[2].length()) {
                            // don't accept partial conversion:
                            res = RES_ERR_SYNTAX;
                        }
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.SetVolume(param_char);
                    }    
                } else if (param[1] == "stereo") {
                    try {
                        param_char = (char)std::stoi(param[2],
                                                     &param_errpos);
                    }
                    catch (...) {
                        res = RES_ERR_SYNTAX;
                    }
                    if (param_errpos < param[2].length()) {
                        // don't accept partial conversion:
                        res = RES_ERR_SYNTAX;
                    }
                    if (res != RES_ERR_SYNTAX) {
                        res = dabradio.SetStereoMode(param_char);
                    }    
                } else { // unknown set property
                    res = RES_ERR_SYNTAX;
                }
            } else { // missing parameter
                res = RES_ERR_SYNTAX;
            }
            param[0] = "";
        } else if (param[0] == "scan") {
            res = dabradio.DoScan();
            param[0] = "";
        } else if (param[0] == "list") {
            res = dabradio.DABProgramList();
            param[0] = "";
        } else if (param[0] == "playstream") {
            if (param.size() >= 2) {
                try {
                    param_ulong = std::stoul(param[1],
                                             &param_errpos);
                }
                catch (...) {
                    res = RES_ERR_SYNTAX;
                }
                if (param_errpos < param[1].length()) {
                    // don't accept partial conversion:
                    res = RES_ERR_SYNTAX;
                }
                if (res != RES_ERR_SYNTAX) {
                    res = dabradio.PlayStream(param_ulong);
                }    
            } else { // missing parameter
                res = RES_ERR_SYNTAX;
            }
            param[0] = "";
        } else if (param[0] == "stopstream") {
            res = dabradio.StopStream();
            param[0] = "";
        } else if (param[0] == "motreset") {
            res = dabradio.MotReset();
            param[0] = "";
        } else if (param[0] == "motimage") {
            res = dabradio.GetMotSlideshowImage(&param_string);
            param[0] = "";
        } else if (param[0] == "ver") {
            if (verbosity >= VERBOSITY_MSG) {
                std::cout << "*MSG:  " << argv[0]
                          << ": version " << VERSION
                          << std::endl;
            }
            res = RES_PASS;
            param[0] = "";            
        } else if (param[0] == "sleep") {
            if (param.size() >= 2) {
                try {
                    param_ulong = std::stoul(param[1], &param_errpos);
                }
                catch (...) {
                    res = RES_ERR_SYNTAX;
                }
                if (param_errpos < param[1].length()) {
                    // don't accept partial conversion:
                    res = RES_ERR_SYNTAX;
                }
                if (res != RES_ERR_SYNTAX) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(param_ulong));
                    res = RES_PASS;
                }    
            } else { // missing parameter
                res = RES_ERR_SYNTAX;
            }
            param[0] = "";
        } else if (param[0] == "exit" || param[0] == "quit") {
            res = RES_PASS;
        } else { // unknown command
            param[0] = "";
            res = RES_ERR_SYNTAX;
        }

        /* return command result */
        if (res == RES_ERR_SYNTAX) {
            if (verbosity >= VERBOSITY_ERR) {
                std::cout << "*ERR:  syntax error \" "
                          << stdinline << "\""
                          << std::endl;
            }
        }
        if (res != RES_WARN_NONE) {
            if (verbosity >= VERBOSITY_RES) {
                std::cout << "*RES:  " << res
                          << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // print this line anyway and independent to the verbosity level
    // for signaling the termination of dabd to piped processes!
    std::cout << "*MSG:  Press <ENTER> to terminate " << argv[0]
              << std::endl;    
    stdinthr_reading = false;
    stdinthread.join();
    return 0;
}
