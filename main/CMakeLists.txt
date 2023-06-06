set(srcs "measure_dist.c"
	     "../components/wifi/wifi_station.c"
		 "../components/TC74/temp_sensor_TC74"
		 )
         
idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")