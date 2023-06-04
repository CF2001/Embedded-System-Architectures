set(srcs "measure_dist.c"
	     "../components/wifi/wifi_station.c"
		 )
         
idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")