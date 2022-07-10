#pragma once
#include <cstdio>

#include "LIWMemory.h"

void test() {
	// Init mem
	liw_minit_def();
	liw_minit_static();
	liw_minit_frame();
	liw_minit_dframe();

	// Init mem for thread
	liw_minit_def_thd();
	liw_minit_static_thd();
	liw_minit_frame_thd();
	liw_minit_dframe_thd();


	liw_hdl_type h0 = liw_new<LIWMem_Static, int>(2);
	liw_mset<LIWMem_Static, int>(h0, 12);
	int v0 = liw_mget<LIWMem_Static, int>(h0);
	printf("%d\n", v0);
	liw_delete<LIWMem_Static, int>(h0);

	liw_hdl_type h1 = liw_new<LIWMem_Default, int>(2);
	liw_mset<LIWMem_Default, int>(h1, 15);
	int v1 = liw_mget<LIWMem_Default, int>(h1);
	printf("%d\n", v1);
	liw_delete<LIWMem_Default, int>(h1);

	liw_hdl_type h2 = liw_new<LIWMem_Frame, int>(2);
	liw_mset<LIWMem_Frame, int>(h2, 15);
	int v2 = liw_mget<LIWMem_Frame, int>(h2);
	printf("%d\n", v2);
	liw_delete<LIWMem_Frame, int>(h2);

	liw_hdl_type h3 = liw_new<LIWMem_DFrame, int>(2);
	liw_mset<LIWMem_DFrame, int>(h3, 15);
	int v3 = liw_mget<LIWMem_DFrame, int>(h3);
	printf("%d\n", v3);
	liw_delete<LIWMem_DFrame, int>(h3);


	// Cleanup mem for thread
	liw_mclnup_dframe_thd();
	liw_mclnup_frame_thd();
	liw_mclnup_static_thd();
	liw_mclnup_def_thd();

	// Cleanup mem
	liw_mclnup_dframe();
	liw_mclnup_frame();
	liw_mclnup_static();
	liw_mclnup_def();
}