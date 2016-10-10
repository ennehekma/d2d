#!/bin/bash          

# read -n6 -r -p "Insert 'pruner' to inspect inut file and execute d2d in pruner mode: " key


# if [ "$key" = 'pruner' ]; then
#     vi ../../data/config/pruner/catalog_pruner_heo.json
# 	./../../build/bin/d2d ../../data/config/pruner/catalog_pruner_heo.json
# fi

# read -n7 -r -p "Insert 'lambert' to inspect inut file and execute d2d in lambert_scanner mode..." key

# if [ "$key" = 'lambert' ]; then
# 	vi ../../data/config/lambert_scanner/lambert_scanner_heo.json 
# 	./../../build/bin/d2d ../../data/config/lambert_scanner/lambert_scanner_heo.json 
# fi


# read -n4 -r -p "Insert 'sgp4' to inspect inut file and execute d2d in lambert_scanner mode..." key

# if [ "$key" = 'sgp4' ]; then
# 	vi ../../data/config/lambert_scanner/lambert_scanner_heo.json 
# 	./../../build/bin/d2d ../../data/config/lambert_scanner/lambert_scanner_heo.json 
# fi

# plot pork chop
# python ../../python/plot_porkchop.py ../../data/HEO/python/plot_porkchop_heo.json

# plot pork chop slice
python ../../python/plot_porkchop_slice.py ../../data/SSO/python/plot_porkchop_slice_heo.json

# echo "lambert_scanner complete, please inspect sgp4_scanner input file."
# read -rsp $'Press any key to continue...\n' -n1 key
# # vi ../../data/config/pruner/catalog_pruner_heo.json
# # ./../../build/bin/d2d ../../data/config/lambert_scanner/lambert_scanner_heo.json 


# echo "spg4_scanner complete, plots will now be created."
# read -rsp $'Press any key to continue...\n' -n1 key
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_a_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_a_dep.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_aop_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_aop_dep.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_e_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_e_dep.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_i_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_i_dep.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_raan_dep_bw.json
# python ../../../python/plot_lambert_scan_maps.py heo_lambert_scanner_map_order_raan_dep.json



# echo "Please inspect pruner input file."
# read -rsp $'Press any key to continue...\n' -n1 key

# echo "Pruning complete, please inspect lambert_scanner input file."
# read -rsp $'Press any key to continue...\n' -n1 key
