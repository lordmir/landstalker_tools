@echo off
release\map3d\map3d -f -d %1 -g fg0.csv -b bg0.csv -m hm0.csv
release\map3d\map3d -f -c map.cmp -g fg0.csv -b bg0.csv -m hm0.csv
release\map3d\map3d -f -d map.cmp -g fg1.csv -b bg1.csv -m hm1.csv
fc bg0.csv bg1.csv
fc fg0.csv fg1.csv
fc hm0.csv hm1.csv
dir %1 map.cmp