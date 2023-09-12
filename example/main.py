import wave
import os
from lc3 import LC3

def Ftest_wav_to_lc3(lc3: LC3):
    # test wave encode
    comp_res = lc3.Fwav_to_lc3('original.wav', 'comp.lc3')
    if(False == comp_res):
        exit()
    print('comp ok')

def Ftest_lc3_to_wav(lc3: LC3):
    # test wave decode
    decomp_res = lc3.Flc3_to_wav('comp.lc3', 'decomp.wav')
    if(False == decomp_res):
        exit()
    print('decomp ok')

def Stest_wav_to_lc3(lc3: LC3) -> int:
    # test wave stream 
    wr = wave.Wave_read('original.wav')
    num = 0
    while(True):
        stream = wr.readframes(8192)
        if(0 == len(stream)):
            break

        w2l = lc3.Swav_to_lc3(stream)
        if(False == w2l):
            print('wave to lc3 err')
            exit()

        out = open(f'out/{num}.lc3', 'wb')
        out.write(w2l)
        out.close()
        num += 1

    print('wave stream to lc3 stream ok')
    return num

def Stest_lc3_wav(lc3: LC3, num: int):
    # test lc3 stream
    for i in range(num):
        f_name = f'out/{i}.lc3'
        f = open(f_name, 'rb')
        siz = os.path.getsize(f_name)
        stream = f.read(siz)
        res = lc3.Slc3_to_wav(stream)
        if(False == res):
            exit()
        out = open(f'out/{i}.wav', 'wb')
        out.write(res)
    
    print('lc3 stream to wave stream ok')


def main():
    lc3 = LC3()
    lc3.numChannels = 1

    Ftest_wav_to_lc3(lc3)
    Ftest_lc3_to_wav(lc3)
    print('files ok')

    cnt = Stest_wav_to_lc3(lc3)
    Stest_lc3_wav(lc3, cnt)

   
    





    


if ('__main__' == __name__):
    main()
