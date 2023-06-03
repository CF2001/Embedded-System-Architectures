set(srcs "measure_dist.c"
	     "wifi_station.c"
		 )
         
idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

# idf_component_register(SRCS "temp_sensor_TC74.c" "spi_25LC040A_eeprom.c" "dashboard.c" "measure_dist.c" "wifi_station.c"
#                     INCLUDE_DIRS ".")
