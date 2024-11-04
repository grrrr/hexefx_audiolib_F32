/* filter_ir_cabsim.h
 *
 * Piotr Zapart 01.2024 www.hexefx.com
 *  - Combined into a stereo speaker simulator with included set of IRs.
 *  - Added stereo enhancer for double tracking emulation
 * 
 * based on:
 *               A u d i o FilterConvolutionUP
 * Uniformly-Partitioned Convolution Filter for Teeny 4.0
 * Written by Brian Millier November 2019
 * adapted from routines written for Teensy 4.0 by Frank DD4WH 
 * that were based upon code/literature by Warren Pratt 
 *
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>."
 */
#ifndef _FILTER_IR_CABSIM_H
#define _FILTER_IR_CABSIM_H

#include <Arduino.h>
#include <Audio.h>
#include "AudioStream.h"
#include "AudioStream_F32.h"
#include "arm_math.h"
#include "filter_ir_cabsim_irs.h"
#include "basic_allpass.h"
#include "basic_delay.h"
#include "basic_shelvFilter.h"
#include "basic_DSPutils.h"
#include <arm_const_structs.h>


#define IR_BUFFER_SIZE  128
#define IR_NFORMAX      (2048 / IR_BUFFER_SIZE)
#define IR_FFT_LENGTH   (2 * IR_BUFFER_SIZE)
#define IR_N_B          (1)
#define IR_MAX_REG_NUM  11       // max number of registered IRs


class AudioFilterIRCabsim_F32 : public AudioStream_F32
{
public:
    AudioFilterIRCabsim_F32();
    virtual void update(void);
    void ir_register(const float32_t *irPtr, uint8_t position);
    void ir_load(uint8_t idx);
    uint8_t ir_get(void) {return ir_idx;} 
    float32_t ir_get_len_ms(void)
    {
		return ir_length_ms;
    }
	void doubler_set(bool s)
	{
		__disable_irq();
		doubleTrack = s;
		if (doubleTrack) 
		{
			delay.reset();
		}
		__enable_irq();
	}	
	bool doubler_tgl()
	{
		__disable_irq();
		doubleTrack = doubleTrack ? false : true;
		if (doubleTrack) 
		{
			delay.reset();
		}
		__enable_irq();
		return doubleTrack;
	}
	bool doubler_get() {return doubleTrack;}
	bool init_done() {return initialized;}
private:
    audio_block_f32_t *inputQueueArray_f32[2];
	uint16_t block_size = AUDIO_BLOCK_SAMPLES;
    float32_t audio_gain = 0.3f;
    int idx_t = 0;
    int16_t *sp_L;
    int16_t *sp_R;
    const uint32_t FFT_L = IR_FFT_LENGTH;
    uint8_t first_block = 1;
    uint8_t ir_loaded = 0;  
    uint8_t ir_idx = 0xFF;
    uint32_t nfor = 0;
    const uint32_t nforMax = IR_NFORMAX;

    int buffidx = 0;
    int k = 0;
	float32_t fmask[IR_NFORMAX][IR_FFT_LENGTH * 2];
	float32_t ac2[512];
	float32_t accum[IR_FFT_LENGTH * 2];
	float32_t fftin[IR_FFT_LENGTH * 2];
	float32_t* last_sample_buffer_R;
	float32_t* last_sample_buffer_L;
	float32_t* maskgen;
	float32_t* fftout;

	float32_t* ptr_fftout;
	float32_t* ptr_fmask;
	float32_t* ptr1;
	float32_t* ptr2;
	int k512;
	int j512;

	const arm_cfft_instance_f32 *S = &arm_cfft_sR_f32_len256;
	const arm_cfft_instance_f32 *iS = &arm_cfft_sR_f32_len256;
	
    uint32_t N_BLOCKS = IR_N_B;

	static const uint32_t delay_l = AUDIO_SAMPLE_RATE * 0.01277f; 	//15ms delay
	AudioBasicDelay delay;

	float32_t ir_length_ms = 0.0f;
	// default IR table, use NULL for bypass
    const float32_t *irPtrTable[IR_MAX_REG_NUM] = 
    {
        ir_1_guitar, ir_2_guitar, ir_3_guitar, ir_4_guitar, ir_10_guitar, ir_11_guitar, ir_6_guitar, ir_7_bass,  ir_8_bass, ir_9_bass, NULL
    };
    void init_partitioned_filter_masks(const float32_t *irPtr);
	bool initialized = false;
	
	// stereo doubler
	static constexpr float32_t doubler_gainL = 0.55f;
	static constexpr float32_t doubler_gainR = 0.65f;
	bool doubleTrack = false;
	static const uint8_t nfir = 30;	// fir taps
	arm_fir_instance_f32 FIR_preL, FIR_preR, FIR_postL, FIR_postR;
	float32_t FIRstate[4][AUDIO_BLOCK_SAMPLES + nfir];
	float32_t FIRk_preL[30] = {
		 0.000894872763f,  0.00020902598f,  0.000285242248f,  0.000503875781f,  0.00207542209f,  0.0013392308f, 
		-0.00476867426f,  -0.0112718018f,  -0.00560652791f,   0.0158470348f,    0.0319586769f,   0.0108086104f, 
		-0.0470990688f,   -0.0834295526f,  -0.0208595414f,    0.154734746f,     0.35352844f,     0.441179603f, 
		 0.35352844f,      0.154734746f,   -0.0208595414f,   -0.0834295526f,   -0.0470990688f,   0.0108086104f, 
		 0.0319586769f,    0.0158470348f,  -0.00560652791f,  -0.0112718018f,   -0.00476867426f,  0.0013392308f };
	float32_t FIRk_preR[30] = {
		 0.00020902598f,   0.000285242248f, 0.000503875781f,  0.00207542209f,   0.0013392308f,  -0.00476867426f, 
		-0.0112718018f,   -0.00560652791f,  0.0158470348f,    0.0319586769f,    0.0108086104f,  -0.0470990688f,
		-0.0834295526f,   -0.0208595414f,   0.154734746f,     0.35352844f,      0.441179603f,    0.35352844f, 
		 0.154734746f,    -0.0208595414f,  -0.0834295526f,   -0.0470990688f,    0.0108086104f,   0.0319586769f, 
		 0.0158470348f,   -0.00560652791f, -0.0112718018f,   -0.00476867426f,   0.0013392308f,   0.00207542209f };
	float32_t FIRk_postL[30] = {
		 0.000285242248f,  0.000503875781f, 0.00207542209f,   0.0013392308f,   -0.00476867426f, -0.0112718018f,
		-0.00560652791f,   0.0158470348f,   0.0319586769f,    0.0108086104f,   -0.0470990688f,  -0.0834295526f,
		-0.0208595414f,    0.154734746f,    0.35352844f,      0.441179603f,     0.35352844f,     0.154734746f,
		-0.0208595414f,   -0.0834295526f,  -0.0470990688f,    0.0108086104f,    0.0319586769f,   0.0158470348f,
		-0.00560652791f,  -0.0112718018f,  -0.00476867426f,   0.0013392308f,    0.00207542209f,  0.000503875781f };
	float32_t FIRk_postR[30] = {
		 0.000503875781f,  0.00207542209f,  0.0013392308f,   -0.00476867426f,  -0.0112718018f,  -0.00560652791f, 
		 0.0158470348f,    0.0319586769f,   0.0108086104f,   -0.0470990688f,   -0.0834295526f,  -0.0208595414f, 
		 0.154734746f,     0.35352844f,     0.441179603f,     0.35352844f,      0.154734746f,   -0.0208595414f,
		-0.0834295526f,   -0.0470990688f,   0.0108086104f,    0.0319586769f,    0.0158470348f,  -0.00560652791f, 
		-0.0112718018f,   -0.00476867426f,  0.0013392308f,    0.00207542209f,   0.000503875781f, 0.0f };

}; 


#endif // _FILTER_IR_CONVOLVER_H
