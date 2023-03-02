/***************************************************************************************************
 * QMIC Project
 * QMIC_SDK.h
 * Header file for the QMIC SDK
 * 
 * 2022 - Alessandro Ruggeri - Micro Photon Devices s.r.l.
 **************************************************************************************************/

#pragma once
#include <stdint.h> //< for basic int types
#include <stdio.h>  //< for FILE type

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_SDK_)
#define DLL_PUBLIC __declspec(dllexport)
#else
#define DLL_PUBLIC __declspec(dllimport)
#endif

#ifndef QBOOL
	typedef int QBOOL;
#endif
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#define QMIC_NPIXELS 576

	/** Type definitions **************************************************************************/
	typedef struct QMIC_s_H *QMIC_H; //< QMIC handle

	typedef enum { //< error type returned by most SDK functions
		// general
		OK = 0,
		ERR_NULL_PTR = -1,
		ERR_INVALID_PTR = -2,
		ERR_LOW_MEMORY = -3,
		// FPGA related
		ERR_INVALID_FPGA = -10,
		ERR_INVALID_BITFILE = -11,
		ERR_PIPE_ERROR = -12,
		ERR_PIPE_TIMEOUT = -13,
		ERR_WIRE = -14,
		ERR_FIFO_FULL = -15,
		// C code related
		ERR_GET_DATA_TIMEOUT = -50,
		ERR_PIX_EN_LOOPBACK = -51,
		ERR_PIX_EN_BUSY = -52,
		ERR_OUT_OF_RANGE_L = -53,
		ERR_OUT_OF_RANGE_H = -54,
		ERR_EMPTY_HIST = -55,
		ERR_INVALID_LEN = -56
	} QMIC_Status;

	typedef struct { //< type containing results from camera telemetry sensors
		double Tcarrier;
		double Tpower;
		double Vcc;
		double Vspad;
		double V12V;
		double V1V8;
		double Ispad;
		double I12V;
		double I1V8;
	} QMIC_AnalogAcq;

	typedef struct { //< type for advanced camera settings
		QBOOL empty_frames_compression;
		QBOOL enable_raw_mode;
		uint8_t pos_read;
		uint8_t pos_gate1;
		uint8_t pos_gate;
		uint8_t gate_len;
		uint16_t readout_time;
		QBOOL wait_gate_end;
		QBOOL unwrap_frame_len_hist;
	} QMIC_adv_settings;


	/** SDK Constructor and destructor ************************************************************/
	/** QMIC Constructor.
	 * Allocates a memory block to contain data for the QMIC camera and opens the communication.
	 * /param qmic       pointer to QMIC handle.
	 * /param Device_ID  string to select a specific device, if multiple cameras are connected to
	 *                   the same PC. Use "" if only one camera is connected, or to select the first
	 *                   available device.                                                        */
	DLL_PUBLIC QMIC_Status QMIC_Constr(QMIC_H *qmic, char *Device_ID);

	/** QMIC Destructor.
	 * Deallocates the memory used for the QMIC camera and closes the communication.
	 * /param qmic  pointer to QMIC handle.                                                       */
	DLL_PUBLIC QMIC_Status QMIC_Destr(QMIC_H *qmic);


	/** Set functions ******************************************************************************
	 * Functions in this section set parameters to the camera. They must be called only when the
	 * acquisition is not running, i.e. before calling the QMIC_Start() function, or after having
	 * called the QMIC_Stop() function.
	 * ********************************************************************************************/

	/** Load the camera with a set of predefined settings.
	 * /param qmic       QMIC handle.                                                             */
	DLL_PUBLIC QMIC_Status QMIC_SetDefaultSettings(QMIC_H qmic);

	/** Select which pixels are enabled to detect photons.
	 * Call this function or alternatively QMIC_SetBadPixels()
	 * /param qmic       QMIC handle.
	 * /param pix_state  fixed length array of active (true) or off (false) pixels.               */
	DLL_PUBLIC QMIC_Status QMIC_SetActivePixels(QMIC_H qmic, QBOOL pix_state[QMIC_NPIXELS]);

	/** Select which pixels are not enabled to detect photons.
	 * Call this function or alternatively QMIC_SetActivePixels()
	 * /param qmic            QMIC handle.
	 * /param bad_pixel_list  variable length array, listing the pixel addresses that must be turned
	 *                        off. First pixel is address number 0.
	 * /param length          length of the bad_pixel_list array.                                 */
	DLL_PUBLIC QMIC_Status QMIC_SetBadPixels(QMIC_H qmic, uint16_t *bad_pixel_list, uint16_t length);

	/** Set advanced/debug settings.
	 * /param qmic      QMIC handle.
	 * /param adv_sett  advanced settings parameters.                                             */
	DLL_PUBLIC QMIC_Status QMIC_SetAdvancedSettings(QMIC_H qmic, QMIC_adv_settings as);

	/** Set Sync output SMA delay.
	 * /param qmic   QMIC handle.
	 * /param delay  desired delay (4 ns per step).                                               */
	DLL_PUBLIC QMIC_Status QMIC_SetSyncOutDelay(QMIC_H qmic, uint8_t delay);


	/** Get functions ******************************************************************************
	 * Functions in this section get parameters from the camera. They can be called even when the
	 * acquisition is running, without interfering with the main data download.
	 * ********************************************************************************************/

	/** Get count rate of the standalone pixel located next to the main SPAD array.
	 * The user must preallocate a 1 x 32bit memory space for the cr parameter.
	 * /param qmic  QMIC handle.
	 * /param cr    pointer to the count rate value (counts per second).                          */
	DLL_PUBLIC QMIC_Status QMIC_GetStandalonePixelCR(QMIC_H qmic, uint32_t *cr);

	/** Get telemetry acquisition data.
	 * The user must preallocate a 1 x sizeof(QMIC_AnalogAcq) memory space for the analog_acq input
	 * parameter.
	 * /param qmic        QMIC handle.
	 * /param analog_acq  pointer to analog acquisition results structure.                        */
	DLL_PUBLIC QMIC_Status QMIC_GetAnalogAcq(QMIC_H qmic, QMIC_AnalogAcq *analog_acq);

	/** Get the histogram of the distribution of the frame durations.
	 * The user must preallocate a 256 x 32bit memory space for the histogram parameter.
	 * Use the QMIC_HelpPrintFrameLenStats() function to easily display this data.
	 * /param qmic      QMIC handle.
	 * /param hist      pointer to the histogram data.
	 * /param new_hist  return (true) if the histogram has been update since last function call.  */
	DLL_PUBLIC QMIC_Status QMIC_GetFrameLenHistogram(QMIC_H qmic, uint32_t *hist, QBOOL *new_hist);

	/** Get the current set Advanced camera parameters
	 * The user must preallocate a 1 x sizeof(QMIC_adv_settings) memory space for the as parameter.
	 * This function is useful to retrieve the current settings, modify only one parameter and than
	 * call QMIC_SetAdvancedSettings to update the values.
	 * /param qmic  QMIC handle.
	 * /param as    advanced settings output struct.                                              */
	DLL_PUBLIC QMIC_Status QMIC_GetAdvancedSettings(QMIC_H qmic, QMIC_adv_settings *as);

	/** Get Software and Firmware versions.
	 * /param qmic      QMIC handle.
	 * /param sw_ver    pointer to software version.
	 * /param fpga_ver  pointer to fpga version.
	 * /param sw_git    pointer to software git hash.
	 * /param fpga_git  pointer to fpga git hash.                                                 */
	DLL_PUBLIC QMIC_Status QMIC_GetVersion(QMIC_H qmic, float *sw_ver, float *fpga_ver,
	                                       uint64_t *sw_git, uint64_t *fpga_git);


	/** Acquisition functions *********************************************************************/

	/** Start the acquisition.
	 * The camera will start to put data on the camera memory.
	 * /param qmic  QMIC handle.                                                                  */
	DLL_PUBLIC QMIC_Status QMIC_Start(QMIC_H qmic);

	/** Stop the acquisition.
	 * This function must be called to stop a running acquisition.
	 * /param qmic  QMIC handle.                                                                  */
	DLL_PUBLIC QMIC_Status QMIC_Stop(QMIC_H qmic);

	/** Get how many data words are available for download.
	 * Call this function to know how much data request on subsequent calls to QMIC_GetData().
	 * /param qmic  QMIC handle.
	 * /param len   pointer to the number of 32-bits words. It will always return numbers multiple
	 *              of 256, as requested by QMIC_GetData().                                       */
	DLL_PUBLIC QMIC_Status QMIC_GetNDataAvailable(QMIC_H qmic, uint32_t *len);

	/** Download data to the PC memory.
	 * The user must preallocate a len * sizeof(uint32_t) memory space for the data parameter.
	 * For every click of a SPAD detector the camera produces an output event, coded in a 32-bit
	 * word. This function downloads this data from the camera. A specified number of words will be
	 * copied to the memory area specified by the user. The format of this data is optimized for USB
	 * transfer and for disk storage. It cannot be used directly. Call QMIC_HelpDecodeData64() to
	 * decode each event in an usable data format.
	 * To avoid overflowing on-camera memory buffer (up to 32 Mwords), call this function as
	 * frequent as possible. The user can:
	 * i)  call QMIC_GetNDataAvailable() until the desired number of words are available and then
	 *     call QMIC_GetData() to download them.
	 * ii) call directly QMIC_GetData() with the desired number of words. Note that this function
	 *     has an internal timeout (10s default), and if the desired amount of data is not available
	 *     by this time, the function will return an error.
	 * /param qmic  QMIC handle.
	 * /param data  pointer to user-allocated memory space (at least len * sizeof(uint32_t) bytes).
	 *              The function will write this area with downloaded camera data.
	 * /param len   tells the function how many data words to download. The value MUST be a multiple
	 *              of 256.                                                                       */
	DLL_PUBLIC QMIC_Status QMIC_GetData(QMIC_H qmic, uint32_t *data, uint32_t len);

	/** Download data and return an intensity image.
	* The user must preallocate a QMIC_NPIXELS * sizeof(uint32_t) memory space for the image.
	* This function will:
	* i)   start the acquisition; 
	* ii)  download data from the camera;
	* iii) parse the received data;
	* iv)  stop the acquisition when the specified exposure time is elapsed;
	* v)   build an intensity image, using data exactly from the specified exposure time;
	* vi)  sum the intensity image to the user-provided input buffer.
	* /param qmic      QMIC handle.
	* /param image     pointer to user-allocated memory space (at least len * QMIC_NPIXELS(uint32_t)
	*                  bytes). The function will write the intensity image in this buffer. If the
	*                  input buffer is not empty, new data will be summed up to the previous one.
	* /param exp_time  exposure time for the intensity image. Time is expressed in seconds.       */
	DLL_PUBLIC QMIC_Status QMIC_GetIntensityImage(QMIC_H qmic, uint32_t *image, double exp_time);

	/** Flush all the data from FPGA RAM.
	 * Call this function only when the acquisition is not running.
	 * /param qmic  QMIC handle.                                                                  */
	DLL_PUBLIC QMIC_Status QMIC_FlushData(QMIC_H qmic);


	/** Helper functions ***************************************************************************
	 * Functions in this section are utility ones. They do not require the QMIC object to be
	 * initialized, i.e. they do not require an active camera. Useful for post-processing data.
	 * ********************************************************************************************/

	/** Decode each camera data event to pixel numbers and timestamps (64-bit version).
	 * The user must preallocate a len * sizeof(int64_t) memory space for timestamps parameter.
	 * The user must preallocate a len * sizeof(uint16_t) memory space for pixel_number parameter.
	 * /param data            pointer to the input camera data.
	 * /param len             length of the data (in words).
	 * /param timestamps      output timestamp of each event. Base unit is 2 ns. Will overflow after
	 *                        about 600 years.
	 * /param pixel_number    address of the clicked pixel that produced the event
	 * /param base_timestamp  input value that will offset all the resulting timestamps. Last
	 *                        timestamp from previous function call can be used to produce always
	 *                        increasing values                                                   */
	DLL_PUBLIC QMIC_Status QMIC_HelpDecodeData64(uint32_t *data, uint32_t len, int64_t *timestamps,
	                                             uint16_t *pixel_number, int64_t base_timestamp);

	/** Decode each camera data event to pixel numbers and timestamps (32-bit version).
	* The user must preallocate a len * sizeof(int32_t) memory space for timestamps parameter.
	* The user must preallocate a len * sizeof(uint16_t) memory space for pixel_number parameter.
	* /param data            pointer to the input camera data.
	* /param len             length of the data (in words).
	* /param timestamps      output timestamp of each event. Base unit is 2 ns. Will overflow after
	*                        about 4 seconds.
	* /param pixel_number    address of the clicked pixel that produced the event
	* /param base_timestamp  input value that will offset all the resulting timestamps. Last
	*                        timestamp from previous function call can be used to produce always
	*                        increasing values                                                   */
	DLL_PUBLIC QMIC_Status QMIC_HelpDecodeData32(uint32_t *data, uint32_t len, int32_t *timestamps,
		                                         uint16_t *pixel_number, int32_t base_timestamp);

	/** Decode raw camera data events to pixel numbers and timestamps (64-bit version).
	 * The user must preallocate a len * sizeof(int64_t) memory space for timestamps parameter.
	 * The user must preallocate a len * sizeof(uint16_t) memory space for pixel_number parameter.
	 * The correct size of these output arrays (always < len) is returned by the len_out parameter.
	 * /param data            pointer to the input raw camera data.
	 * /param len             length of the data (in words).
	 * /param timestamps      output timestamp of each event. Base unit is 2 ns. Please note that:
	 *                        - this array is NOT sorted as in QMIC_HelpDecodeData64().
	 *                        - these values does not represent the correct absolute time-of-arrival
	 *                          of the detected events.
	 *                        - if the upper bits are identical between two timestamps, last 8-bits
	 *                          can be compared to determine which one has been detected first.
	 *                        - if two timestamps are identical, it is guaranteed that the two are
	 *                          a coincidence event.
	 * /param pixel_number    address of the clicked pixel that produced the event.
	 * /param base_timestamp  input value that will offset all the resulting timestamps. Last
	 *                        timestamp from previous function call can be used to produce always
	 *                        increasing values.
	 * /param len_out         length of timestamps and pixel_number arrays.                       */
	DLL_PUBLIC QMIC_Status QMIC_HelpDecodeRawData64(uint32_t *data, uint32_t len, 
		                                            int64_t *timestamps, uint16_t *pixel_number, 
		                                            int64_t base_timestamp, uint32_t *len_out);

	/** Get the actual camera frame rate.
	* /param histogram   input histogram array, as returned by QMIC_GetFrameLenHistogram
	* /param frame_rate  pointer to a float value, which will contains the actual rate in fps.    */
	DLL_PUBLIC QMIC_Status QMIC_HelpActualFrameRate(uint32_t *histogram, float* frame_rate);

	/** Print statistics about the length of the camera frames.
	 * /param histogram   input histogram array, as returned by QMIC_GetFrameLenHistogram
	 * /param string_out  string where the function will write the text output. Preallocate at least
	 *                    1024 bytes. Alternatively, set to NULL and the function will write to the
	 *                    standard output.                                                        */
	DLL_PUBLIC QMIC_Status QMIC_HelpPrintFrameLenStats(uint32_t *histogram, char* string_out);

	/** Print an extensive description of a QMIC error.
	 * If the status is equal to OK, no string will be produced. Otherwise an error description will
	 * be redirected to the selected output stream.
	 * /param status      status to be parsed.
	 * /param fncName     optional string that will appears in the error message. Useful to track
	 *                    the function that originated the error. Set to NULL to skip.
	 * /param stream_out  output stream where to redirect the produced string. Set to NULL to use
	 *                    the standard output.                                                    */
	DLL_PUBLIC QBOOL QMIC_HelpPrintErrorCode(QMIC_Status status, char* fncName, FILE* stream_out);



	// === /!\ Debug only /!\ ===
	DLL_PUBLIC QMIC_Status QMIC_TurnOn(QMIC_H qmic);
	DLL_PUBLIC QMIC_Status QMIC_TurnOff(QMIC_H qmic);
	DLL_PUBLIC QMIC_Status QMIC_InternalTests(QMIC_H qmic);
	DLL_PUBLIC QMIC_Status QMIC_SetTDCCodeLimits(QMIC_H qmic, uint8_t high, uint8_t low);
	DLL_PUBLIC QMIC_Status QMIC_SetSPADvoltage(QMIC_H qmic, double voltage);
	DLL_PUBLIC QMIC_Status QMIC_SetDCMPhase(QMIC_H qmic, int16_t phase);
	DLL_PUBLIC QMIC_Status QMIC_GetWire6(QMIC_H qmic, uint32_t *w6, uint32_t *w7);

#ifdef __cplusplus
}
#endif