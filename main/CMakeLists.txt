set(srcs "measure_dist.c"
	    )
         
idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

# idf_component_register(SRCS "measure_dist.c" "wifi_station.c"
#                     INCLUDE_DIRS ".")
