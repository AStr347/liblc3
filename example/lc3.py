from ctypes import *

ILC3_OK = 0
ILC3_BAD_ARG = -1
ILC3_BAD_INOUT = -2


class ilc3_coder(Structure):
    _fields_ = [
        ("bitrate", c_uint32),
        ("samplesiz", c_uint32),
        ("srate_hz", c_uint16),
        ("frame_us", c_float),
        ("nch", c_uint8),
    ]



class LC3:
    __so: CDLL
    __coder: ilc3_coder

    def __coder_init(self, bitrate, samplesiz, srate_hz, nch):
        self.__coder = ilc3_coder()
        p_coder = pointer(self.__coder)
        self.__so.lc3_coder_init(p_coder,
                                 bitrate,
                                 samplesiz,
                                 srate_hz,
                                 nch,
                                 c_float(10000.0))

    def __init__(self) -> None:
        # load shared lib
        so = cdll.LoadLibrary("liblc3.so")
        self.__so = so
        
        # init lc3 coder params
        self.__coder_init(16000, 16, 48000, 2)

    def Fwav_to_lc3(self, src_path: str, dst_path: str) -> bool:
        '''
        file wav convert to file lc3
        '''
        src_cstr = src_path.encode('utf-8') + b'\0'
        dst_cstr = dst_path.encode('utf-8') + b'\0'
        p_coder = pointer(self.__coder)
        res = self.__so.file_wav_to_lc3(p_coder, src_cstr, dst_cstr)
        return (ILC3_OK == res)

    def Flc3_to_wav(self, src_path: str, dst_path: str) -> bool:
        '''
        file lc3 convert to file wave
        '''
        src_cstr = src_path.encode('utf-8') + b'\0'
        dst_cstr = dst_path.encode('utf-8') + b'\0'
        p_coder = pointer(self.__coder)
        res = self.__so.file_lc3_to_wav(p_coder, src_cstr, dst_cstr)
        return (ILC3_OK == res)

    def Swav_to_lc3(self, instream: bytearray) -> bool | bytearray:
        '''
        stream wav convert to stream lc3
        '''
        in_siz = len(instream)
        # do output space eqaul size
        # because can't caluculate compressed size
        outstream = (c_char * in_siz)(0)

        p_coder = pointer(self.__coder)
        encres = self.__so.stream_to_lc3(p_coder,
                                         instream,
                                         in_siz,
                                         outstream,
                                         in_siz,
                                         False)
        if(ILC3_OK > encres):
            return False
        
        # read needed size of any type buffer
        res = (c_char * encres)(0)
        for i in range(encres):
            res[i] = outstream[i]

        return bytearray(res)

    def Slc3_to_wav(self, instream: bytearray) -> bool | bytearray:
        '''
        stream lc3 convert to stream wav
        '''
        in_siz = len(instream)
        out_siz = in_siz * 50
        # do output space size mul on 50
        # because can't caluculate decompressed size
        outstream = (c_char * out_siz)(0)

        p_coder = pointer(self.__coder)
        decres = self.__so.lc3_to_stream(p_coder,
                                         instream,
                                         in_siz,
                                         outstream,
                                         out_siz)
        
        if(ILC3_OK > decres):
            return False

        # read needed size of any type buffer
        res = (c_char * decres)(0)
        for i in range(decres):
            res[i] = outstream[i]

        return bytearray(res)

    @property
    def sampleRate(self) -> c_uint16:
        return self.__coder.srate_hz

    @sampleRate.setter
    def sampleRate(self, srate: c_uint16):
        coder = self.__coder
        self.__coder_init(coder.bitrate, coder.samplesiz, srate, coder.nch)

    @property
    def bitRate(self) -> c_uint32:
        return self.__coder.bitrate

    @bitRate.setter
    def bitRate(self, bitrate: c_uint32):
        coder = self.__coder
        self.__coder_init(bitrate, coder.samplesiz, coder.srate_hz, coder.nch)

    @property
    def sampleSiz(self) -> c_uint32:
        return self.__coder.samplesiz

    @sampleSiz.setter
    def sampleSiz(self, samplesiz: c_uint32):
        coder = self.__coder
        self.__coder_init(coder.bitrate, samplesiz, coder.srate_hz, coder.nch)

    @property
    def numChannels(self) -> c_uint8:
        return self.__coder.nch

    @numChannels.setter
    def numChannels(self, nch: c_uint8):
        coder = self.__coder
        self.__coder_init(coder.bitrate, coder.samplesiz, coder.srate_hz, nch)
        


