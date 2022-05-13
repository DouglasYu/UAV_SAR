import sys
import soundfile
from scipy.fftpack import fft, fftshift, ifft2
from scipy.signal import hilbert
import numpy as np
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt
import datetime
import os
from scipy.io.wavfile import read as wavread

def wKA_FMCW(wavpath, v, start_offset, end_offset, out_img_path):
    '''
    - Modified from Kaiqi Chen's simulation code and code from 21class senior design SAR group.
    - https://github.com/qig2/SAR-Payload
    Base on the wav file path and drone's velocity, generate the SAR image path and export to the specified destination.
    :param wavpath: path for wav audio file
    :param v: drone's velocity, should be constant
    :param start_offset: signal component that need to be clipped
    :param end_offset: signal component that need to be clipped
    :param out_img_path: directory of export path of the generated image
    :return: None
    '''

    now = datetime.datetime.now()
    fileName = "sar_image_{}-{}-{}-{}-{}-{}".format(now.year, now.month, now.day, now.hour, now.minute, now.second)
    export_path = os.path.join(out_img_path, fileName+'.png')
    fig_title = wavpath.strip().split('/')[-1]
    # print(now)

    path = wavpath
    [Aux_data, FS] = soundfile.read(path)
    # [FS, Aux_data] = wavread(path)
    V = float(v)
    Tp = 1/48.5/2 # pulse time - should be half of the period certainly used

    # manually clipped the signal that not stable in velocity - constant movement
    numpoints = Aux_data.shape[0]
    start_offset = int(start_offset * numpoints)
    end_offset = int(end_offset * numpoints)

    trig = Aux_data[start_offset:numpoints - end_offset, 1]
    s = Aux_data[start_offset:numpoints - end_offset, 0]  # %-1*S(:,1)

    # flip the audio signal => image follows the convention, drone start from the x axis
    trig = np.flip(trig, axis=0)
    s = np.flip(s, axis=0)

    nummax = min(s.shape[0], trig.shape[0])

    # constant used
    pi = 3.14159265
    c = 3E8  # (m/s) speed of light
    N = int(Tp * FS)  # %# of samples per pulse
    fstart = 2373.66E6  # %(Hz) LFM start frequency 2260
    fstop = 2486.40E6  # %(Hz) LFM stop frequency   2590%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%!!!!!!!!!!!!!!!!!!!!!!!!!

    # data preprocessing
    pzero = (trig > 0.1) # as marker of each impulse
    first_period = 1
    sif = None
    k = 1
    while (k< nummax-N+1):
        if k%100000 == 0: # as the procedure visualizer
            print("Processing [{}/{}] -- {}% ".format(int(k), nummax-N+1, 1.*k/(nummax-N+1)*100))
        if (pzero[int(k)]==1) and (pzero[int(k-1)]==0):
            if(first_period): # Generally, the first square wave period is incomplete, so it is discarded
                k=k+1
                first_period=0
            else:
                SIF = s[int(k):int(k+N-1)]
                # hilbert transform - obtain the complex component from the real value signal
                k = k + N / 4
                tmp_arr = hilbert(SIF)

                if(sif is None):
                    sif = tmp_arr
                else:
                    sif = np.row_stack((sif,tmp_arr))
        else:
            k=k+1

    # load IQ converted data
    sif_mean = np.mean(sif, 0)
    for ii in range(sif.shape[0]):
        sif[ii, :] = sif[ii, :] - sif_mean

    # radar parameters
    fc = (fstop - fstart) / 2 + fstart
    B = fstop - fstart # bandwidth
    Rs = 0
    delta_x = (Tp * 2) * V
    L = delta_x * (sif.shape[0]) # aperture length
    Kr = np.linspace(((4 * pi / c) * (fc - B / 2)), ((4 * pi / c) * (fc + B / 2)), (sif.shape[1]))

    # add window for better handling the periodicity requirements, avoid spectrum leak
    N = sif.shape[1]
    i_tmp = np.linspace(1, N + 1, N)
    H = 0.5 + 0.5 * np.cos(2 * pi * (i_tmp - N / 2) / N)

    for ii in range(sif.shape[0]):
        sif[ii,:] = sif[ii,:]*H

    zpad = sif.shape[0]

    # wKA algorithm part
    S = fftshift(fft(sif, None, 0), 0)
    Kx = np.linspace((-pi / delta_x), (pi / delta_x), (S.shape[0]))

    # match filter
    aa_max = 10
    aa_min = 1e3
    phi_mf = np.zeros((S.shape[0], S.shape[1]))
    for ii in range(S.shape[1]):
        for jj in range(S.shape[0]):
            aa = np.sqrt((Kr[ii]) ** 2 - (Kx[jj]) ** 2)
            if aa > aa_max:
                aa_max = aa
            if aa < aa_min:
                aa_min = aa
            phi_mf[jj, ii] = Rs * np.sqrt((Kr[ii]) ** 2 - (Kx[jj]) ** 2)
    smf = np.exp(1j * phi_mf)
    S_mf = S * smf

    kstart = aa_min  #
    kstop = aa_max

    Ky_even = np.linspace(kstart, kstop, 1024)
    S_st = np.zeros((zpad, len(Ky_even)), dtype=complex)
    Ky = np.zeros((zpad, len(Kr)))

    for ii in range(zpad):
        Ky[ii,:] = np.sqrt(Kr**2-Kx[ii]**2)
        tmp_func = interp1d(Ky[ii,:], S_mf[ii,:], bounds_error=False, fill_value=1E-30)
        S_st[ii,:] = tmp_func(Ky_even)

    # ifft - img reconstruct
    v = ifft2(S_st, (S_st.shape[0] * 4, S_st.shape[1] * 4))
    bw = 3E8 * (kstop - kstart) / (4 * pi)
    max_range = (3E8 * S_st.shape[1] / (2 * bw)) * 1 / .3048
    S_image = v
    S_image = np.fliplr(np.rot90(S_image))
    cr1 = -L / 2 / 0.3048  #
    cr2 = L / 2 / 0.3048  # %80; %(ft)
    dr1 = 1  # %1 + Rs/.3048; %(ft) down range
    dr2 = 500 + Rs / .3048  #
    # %data truncation
    dr_index1 = int(round((dr1 / max_range) * (S_image.shape[0])))  #
    dr_index2 = int(round((dr2 / max_range) * (S_image.shape[0])))  #

    cr_index1 = int(round(((cr1 + zpad * delta_x / (2 * .3048)) / (zpad * delta_x / .3048)) * (S_image.shape[1])))  # %!!!!!!!!!!!!!!!!!!!!!!
    cr_index2 = int(round(((cr2 + zpad * delta_x / (2 * .3048)) / (zpad * delta_x / .3048)) * (S_image.shape[1])))

    trunc_image = S_image[dr_index1:dr_index2, cr_index1:cr_index2]  # % 2493*3841
    downrange = np.linspace(-1 * dr1, -1 * dr2, trunc_image.shape[0]) + Rs  # %/.3048
    crossrange = np.linspace(cr1, cr2, trunc_image.shape[1])

    for ii in range(trunc_image.shape[1]):  #= 1:size(trunc_image,2):
        trunc_image[:,ii] = (trunc_image[:,ii].T)*(abs(downrange*.3048))**(3/2)

    trunc_image = 20 * np.log10(abs(trunc_image))  # %added to scale to d^3/2

    plt.figure(dpi=300)
    plt.pcolormesh(crossrange, downrange, trunc_image, vmin=trunc_image.max() - 50, vmax=trunc_image.max())
    plt.title(fig_title)
    plt.gca().invert_yaxis()
    plt.colorbar()
    plt.ylabel('Downrange')
    plt.xlabel('Crossrange')
    # plt.show()

    plt.savefig(export_path)
    print("Figure saved in \n{}\n successfully".format(export_path))

    # offset image

    # test = np.empty(shape=trunc_image.shape)
    # trunc_image = np.flip(trunc_image, axis=0)
    # for i in range(trunc_image.shape[0]):
    #     offset = -16.163 * np.exp(1.402 * (i / 1000 - 3)) - 75.795 # curve-fit parameters
    #     test[i, :] = trunc_image[i, :] - np.ones(trunc_image.shape[1]) * offset
    # plt.figure()
    # plt.pcolormesh(crossrange, downrange, test, cmap='gray', vmin=test.max() - 25, vmax=test.max())
    # plt.colorbar()
    # plt.title("offset image")
    # plt.tight_layout()
    # plt.ylabel('Downrange')
    # plt.xlabel('Crossrange')
    # plt.show()




if __name__=='__main__':
    # (wav_path, velocity, start_offset, end_offset, img_path) = [
    #     './wav/sample.wav',
    #
    #     '2', '0', '0', './images']
    # wKA_FMCW(wav_path, velocity, start_offset, end_offset, img_path)
    try:
        wav_path = sys.argv[1]
        velocity = float(sys.argv[2])
        start_offset = float(sys.argv[3])
        end_offset = float(sys.argv[4])
        img_path = sys.argv[5]
        wKA_FMCW(wav_path, velocity, start_offset, end_offset, img_path)
    except Exception as e:
        print("Input argvs are : {}".format(sys.argv))
        print("Please check args: 'python main.py drone_velocity output_img_path start_offset end_offset'")
        print(e)


