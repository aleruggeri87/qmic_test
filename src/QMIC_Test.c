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
#include <io.h>       //< for _setmode()
#include <fcntl.h>    //< for _setmode() params
#include <windows.h>  //< for moving cursor on console

// Support functions and macro definitions ---------------------------------------------------------
void clear_last_N_chars(int n_chars);
void draw_map(uint32_t *frame, uint8_t first_line);

#define CHECK_ERR_EXIT(x, y) {if(QMIC_HelpPrintErrorCode(x, y, NULL)){return -1;}}
#define CHECK_ERR_ESCAPE(x, y) {if(QMIC_HelpPrintErrorCode(x, y, NULL)){goto escape;}}

// User defined settings ---------------------------------------------------------------------------
#define SHOW_LIVE             1 //< 0: save data to file; 1: show live intensity image
#if SHOW_LIVE
#define LIVE_TIME             100 // live image integration time (ms)
#else
#define N_EVENTS              512*10000 //< how many events to wait; must be a multiple of 256!
#define N_REPETITIONS         5 //< how many times repeat the acquisiton of N_EVENTS
#define DECODE_DATA           1 //< set to 1 to activate data decoding
#define SAVE_CAMERA_DATA      0 //< set to 1 to save camera data to file
#define SAVE_DECODED_DATA     1 //< set to 1 to save decoded data to file
#endif
#define DEACTIVATE_BAD_PIXELS 1 //< set to 1 to deactivate the specified "bad" pixels on-chip
#define BAD_PIX_LEN          17 //< number of bad pixels in the list below
uint16_t bad_pix_list[BAD_PIX_LEN] = {6, 34, 53, 66, 70, 104, 196, 219, 249, 268, 303, 343, 351,
	                                  415, 421, 458, 561}; //< those values are for QMIC01 camera

// MAIN function -----------------------------------------------------------------------------------
int main() {
	// some variables...
	QMIC_H q = NULL;
	QMIC_Status stat;
	QMIC_AnalogAcq analog_acq;
	float sw_ver, fw_ver;
#if SHOW_LIVE
	uint32_t *image;
#else
	uint32_t FLhist[256];
	uint32_t aval_events;
	uint32_t *data_buf;
#endif

#if SAVE_CAMERA_DATA
	FILE *camera_data_file;
#endif

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
#if SHOW_LIVE
	image = (uint32_t*)calloc(QMIC_NPIXELS, sizeof(uint32_t));
	if(image == NULL) {
		printf("(ERROR) QMIC_Test.c: image calloc error.\n");
		goto escape;
	}
#else
	data_buf = (uint32_t*)calloc(N_EVENTS, sizeof(uint32_t));
	if(data_buf == NULL) {
		printf("(ERROR) QMIC_Test.c: data_buf calloc error.\n");
		goto escape;
	}
#endif
#if DECODE_DATA
	ts = (uint64_t*)calloc(N_EVENTS, sizeof(uint64_t));
	if(ts == NULL) {
		printf("(ERROR) QMIC_Test.c: ts calloc error.\n");
		goto escape;
	}
	addr = (uint16_t*)calloc(N_EVENTS, sizeof(uint16_t));
	if(addr == NULL) {
		printf("(ERROR) QMIC_Test.c: addr calloc error.\n");
		goto escape;
	}
#endif

	// === Open output files ===
#if SAVE_CAMERA_DATA
	camera_data_file = fopen("data_out.dat", "wb");
	if(camera_data_file == NULL) {
		printf("(ERROR) QMIC_Test.c: data_out.dat fopen error.\n");
		goto escape;
	}
#endif
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

	stat = QMIC_GetVersion(q, &sw_ver, &fw_ver, NULL, NULL); //< get sw & fw versions
	CHECK_ERR_EXIT(stat, "QMIC_GetVersion");
	printf("sw. ver: %4.2f - fw. ver: %4.2f\n", sw_ver, fw_ver);

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
	stat = QMIC_FlushData(q); //< discard all the data on the camera memory (if any)
	CHECK_ERR_ESCAPE(stat, "QMIC_FlushData");

#if SHOW_LIVE
	// no need to start/stop the acquisition, the function QMIC_GetIntensityImage() will do this.
	while(TRUE) {
		memset(image, 0, QMIC_NPIXELS * sizeof(uint32_t)); //< prepare memory buffer

		stat = QMIC_GetIntensityImage(q, image, LIVE_TIME/1000.0); //< get live image, given the
		CHECK_ERR_ESCAPE(stat, "QMIC_GetIntensityImage");          //  specified integration time.

		draw_map(image, 5); //< draw the image, at the specified line of the console

		printf("Live Data (press 'q' to abort)\n");
		if(_kbhit() && _getch() == 'q') {
			printf("\n");
			goto escape;
		}

		stat = QMIC_GetAnalogAcq(q, &analog_acq); //< get some data from the telemetry sensors
		CHECK_ERR_EXIT(stat, "QMIC_GetAnalogAcq");
		printf("Sensor Temperature: %.1f*C\n", analog_acq.Tcarrier);
	}
#else
	printf("Acquiring Data (press 'q' to abort)\n");

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

#if SAVE_CAMERA_DATA
		clear_last_N_chars(last_chars);
		last_chars = printf("saving data ");
		fwrite(data_buf, sizeof(uint32_t), N_EVENTS, camera_data_file); //< save camera data to file
#endif

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

	stat = QMIC_GetFrameLenHistogram(q, FLhist, NULL);   //< get distribution of frame length of the
	CHECK_ERR_ESCAPE(stat, "QMIC_GetFrameLenHistogram"); //  last 100 ms acquisition time

	stat = QMIC_HelpPrintFrameLenStats(FLhist, NULL); //< display the frame length distribution stats
	CHECK_ERR_ESCAPE(stat, "QMIC_HelpPrintFrameLenStats");
#endif

escape: //< jump to here on error after successful initialization. This allow to properly turn-off
	    //  and deallocate the QMIC object
	stat = QMIC_Stop(q); //< stop acquisition; no additional events will be put in the camera memory
	CHECK_ERR_ESCAPE(stat, "QMIC_Stop");


	// === Close and exit ===
	stat = QMIC_Destr(&q); //< destruct QMIC object
	CHECK_ERR_EXIT(stat, "QMIC_Destr");
	q = NULL;

	// === Deallocate dynamic memory ===
#if SHOW_LIVE
	free(image);
	image = NULL;
#else
	free(data_buf);
	data_buf = NULL;
#endif
#if SAVE_CAMERA_DATA
	fclose(camera_data_file);
#endif
#if SAVE_DECODED_DATA && DECODE_DATA
	fclose(decoded_ts_file);
	fclose(decoded_addr_file);
#endif
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

// draw_map draws a 24x24 intensity map on the console, starting from the specified line.
// it uses unicode block elements to code different intensity levels.
void draw_map(uint32_t *frame, uint8_t first_line) {
	wchar_t intensityCh[5] = { L' ', L'\x2591', L'\x2592', L'\x2593', L'\x2588' };
	wchar_t map[2*(24*25) + 1] = { L' ' };
	int k, j, i;

	// rewind cursor position
	COORD pos;
	pos.X = 0;
	pos.Y = (short)first_line;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);

	// build character map
	for(k = 0; k < 24; k++) {
		for(j = 0; j < 24; j++) {
			uint32_t val = frame[k * 24 + j];
			if(val == 0) {
				i = 0;
			} else if(val > 0 && val <= 10) {
				i = 1;
			} else if(val > 10 && val <= 100) {
				i = 2;
			} else if(val > 100 && val <= 1000) {
				i = 3;
			} else {
				i = 4;
			}
			map[2*(k * 25 + j)+0] = intensityCh[i];
			map[2*(k * 25 + j)+1] = intensityCh[i];
		}
		map[2*(k * 25 + 24)+0] = L'\r';
		map[2*(k * 25 + 24)+1] = L'\n';
	}

	// print unicode data
	_setmode(_fileno(stdout), _O_U16TEXT);
	wprintf(map);
	_setmode(_fileno(stdout), _O_TEXT);
}
