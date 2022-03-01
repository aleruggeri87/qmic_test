/***************************************************************************************************
 * QMIC Project
 * QMIC_Test.c
 * Simple program to demonstrate the use of the QMIC SDK.
 * 
 * 2022 - Alessandro Ruggeri - Micro Photon Devices s.r.l.
 **************************************************************************************************/

#include "QMIC_SDK.h" //< QMIC header file
#include <stdlib.h>   //< for dynamic memory allocation
#include <conio.h>    //< for _kbhit & _getch()

// Support functions and macro definitions ---------------------------------------------------------
void clear_last_N_chars(int n_chars);

#define CHECK_ERR_EXIT(x, y) {if(QMIC_HelpPrintErrorCode(x, y, NULL)){return -1;}}
#define CHECK_ERR_ESCAPE(x, y) {if(QMIC_HelpPrintErrorCode(x, y, NULL)){goto escape;}}

// User defined settings ---------------------------------------------------------------------------
#define N_EVENTS              512*10000 //< how many events to wait; must be a multiple of 256!
#define N_REPETITIONS         5 //< how many times repeat the acquisiton of N_EVENTS
#define DECODE_DATA           1 //< set to 1 to activate data decoding
#define SAVE_DECODED_DATA     1 //< set to 1 to save decoded data to file
#define DEACTIVATE_BAD_PIXELS 1 //< set to 1 to deactivate the specified "bad" pixels on-chip
#define BAD_PIX_LEN          17 //< number of bad pixels in the list below
uint16_t bad_pix_list[BAD_PIX_LEN] = {6, 34, 53, 66, 70, 104, 196, 219, 249, 268, 303, 343, 351,
	                                  415, 421, 458, 561}; //< those values are for QMIC01 camera

// MAIN function -----------------------------------------------------------------------------------
int main() {
	// some variables...
	QMIC_H q = NULL;
	QMIC_Status stat;
	uint32_t aval_events;
	QMIC_AnalogAcq analog_acq;

	uint32_t *data_buf;
	FILE *camera_data_file;

#if DECODE_DATA
	uint64_t *ts;
	uint16_t *addr;
	uint64_t last_ts = 0;
#endif
#if SAVE_DECODED_DATA && DECODE_DATA
	FILE *decoded_ts_file;
	FILE *decoded_addr_file;
#endif

	// === Banner ===
	printf("====================================================================\n");
	printf("   QMIC Example program                                             \n");
	printf("====================================================================\n");

	// === Allocate dynamic memory buffers ===
	data_buf = (uint32_t*)calloc(N_EVENTS, sizeof(uint32_t));
	if(data_buf == NULL) {
		printf("(ERROR) QMIC_Test.c: data_buf calloc error.\n");
		goto escape;
	}
#if DECODE_DATA
	ts = (uint64_t*)calloc(N_EVENTS, sizeof(uint64_t));
	if(data_buf == NULL) {
		printf("(ERROR) QMIC_Test.c: ts calloc error.\n");
		goto escape;
	}
	addr = (uint16_t*)calloc(N_EVENTS, sizeof(uint16_t));
	if(data_buf == NULL) {
		printf("(ERROR) QMIC_Test.c: addr calloc error.\n");
		goto escape;
	}
#endif

	// === Open output files ===
	camera_data_file = fopen("data_out.dat", "wb");
	if(camera_data_file == NULL) {
		printf("(ERROR) QMIC_Test.c: data_out.dat fopen error.\n");
		goto escape;
	}
#if SAVE_DECODED_DATA && DECODE_DATA
	decoded_ts_file = fopen("decoded_ts_out.dat", "wb");
	if(decoded_ts_file == NULL) {
		printf("(ERROR) QMIC_Test.c: decoded_ts_out.dat fopen error.\n");
		goto escape;
	}
	decoded_addr_file = fopen("decoded_addr_out.dat", "wb");
	if(decoded_ts_file == NULL) {
		printf("(ERROR) QMIC_Test.c: decoded_addr_out.dat fopen error.\n");
		goto escape;
	}
#endif

	// === Initial configuration ===
	printf("Configuring Camera\n");

	stat = QMIC_Constr(&q, ""); //< open the first available camera
	CHECK_ERR_EXIT(stat, "QMIC_Constr");

	stat = QMIC_GetAnalogAcq(q, &analog_acq); //< get some data from the telemetry sensors
	CHECK_ERR_EXIT(stat, "QMIC_GetAnalogAcq");
	printf("Sensor Temperature: %.1f*C\n", analog_acq.Tcarrier);

	// === Load Default Configuration and Bad Pixels Map ===
	stat = QMIC_SetDefaultSettings(q); //< load suggested settings for the test
	CHECK_ERR_ESCAPE(stat, "QMIC_SetDefaultSettings");

#if DEACTIVATE_BAD_PIXELS
	stat = QMIC_SetBadPixels(q, bad_pix_list, BAD_PIX_LEN); //< turn off pixels
#else
	stat = QMIC_SetBadPixels(q, NULL, 0); //< keep all pixels on
#endif
	CHECK_ERR_ESCAPE(stat, "QMIC_SetBadPixels");


	// === Acquisition ===
	printf("Acquiring Data (press 'q' to abort)\n");

	stat = QMIC_FlushData(q); //< discard all the data on the camera memory (if any)
	CHECK_ERR_ESCAPE(stat, "QMIC_FlushData");

	stat = QMIC_Start(q); //< start the acquisition; events will accumulate in the camera memory
	CHECK_ERR_ESCAPE(stat, "QMIC_Start");

	for(int r = 0; r < N_REPETITIONS; r++) {
		printf("% 3d. Wait for %d events: ", r, N_EVENTS);
		int last_chars = 0;
		do {
			stat = QMIC_GetNDataAvailable(q, &aval_events);   //< query how many events are
			CHECK_ERR_ESCAPE(stat, "QMIC_GetNDataAvailable"); //  available in the camera memory

			// print progress and exit if 'q' is pressed
			clear_last_N_chars(last_chars);
			last_chars = printf("%6.1f%%", 100 * ((float)aval_events) / (N_EVENTS));
			if(_kbhit() && _getch()=='q') {
				printf("\n");
				goto escape;
			}
		} while(aval_events < N_EVENTS);

		clear_last_N_chars(last_chars);
		last_chars = printf("getting data");
		stat = QMIC_GetData(q, data_buf, N_EVENTS); //< download N_EVENTS from camera to the PC
		CHECK_ERR_ESCAPE(stat, "QMIC_GetData");

		clear_last_N_chars(last_chars);
		last_chars = printf("saving data ");
		fwrite(data_buf, sizeof(uint32_t), N_EVENTS, camera_data_file); //< save camera data to file

#if DECODE_DATA
		clear_last_N_chars(last_chars);
		last_chars = printf("processing data");
		stat = QMIC_HelpDecodeData64(data_buf, N_EVENTS, ts, addr, last_ts);
		CHECK_ERR_ESCAPE(stat, "QMIC_HelpDecodeData64");
		last_ts = ts[N_EVENTS - 1]; //< keep last timestamp for the next decoding
#endif
#if SAVE_DECODED_DATA && DECODE_DATA
		clear_last_N_chars(last_chars);
		last_chars = printf("saving decoded data");
		fwrite(ts, sizeof(uint64_t), N_EVENTS, decoded_ts_file);
		fwrite(addr, sizeof(uint16_t), N_EVENTS, decoded_addr_file);
#endif

		clear_last_N_chars(last_chars);
		last_chars = printf("done.              \n");
	}

escape: //< jump to here on error after successful initialization. This allow to properly turn-off
	    //  and deallocate the QMIC object
	stat = QMIC_Stop(q); //< stop acquisition; no additional events will be put in the camera memory
	CHECK_ERR_ESCAPE(stat, "QMIC_Stop");


	// === Close and exit ===
	stat = QMIC_Destr(&q); //< destruct QMIC object
	CHECK_ERR_EXIT(stat, "QMIC_Destr");
	q = NULL;

	// === Deallocate dynamic memory ===
	free(data_buf);
	data_buf = NULL;
	fclose(camera_data_file);
#if DECODE_DATA
	free(ts);
	ts = NULL;
	free(addr);
	addr = NULL;
#endif

	printf("Press ENTER to exit\n");
	while(getchar() != '\n');

	return 0;
}

// Support functions -------------------------------------------------------------------------------
void clear_last_N_chars(int n_chars) {
	for(int k = 0; k < n_chars; k++) {
		printf("\b");
	}
}
