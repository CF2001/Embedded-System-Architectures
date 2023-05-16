set(srcs "measure_dist.c"
	     "wifi_station.c")
         
idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

# idf_component_register(SRCS "dashboard.c" "measure_dist.c" "wifi_station.c"
#                     INCLUDE_DIRS ".")
