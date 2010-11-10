#!/bin/bash
DATENOW=`date +"%Y%m%d"`
mkdir -p package/apps
mkdir -p package/gxgeo/roms
mkdir -p package/gxgeo/save
mkdir -p package/gxgeo/conf
cd package/gxgeo
touch roms/empty
touch save/empty
cp -r ../../conf/gngeorc ./conf/gngeorc
tar xf ../../resources/romrc.tar.bz2
cd ..
cp ../$(basename `cd .. && pwd`).dol apps/boot.dol
cp ../resources/icon.png apps/icon.png
echo -e "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n\
  <app version=\"$DATENOW\">\n\
    <name>GxGeo</name>\n\
    <coder>blizzo</coder>\n\
    <version>$DATENOW</version>\n\
    <release_date>$DATENOW</release_date>\n\
    <short_description>Neo-Geo Emulator</short_description>\n\
    <long_description>A Neo-Geo Emulator based on GnGeo</long_description>\n\
  </app>" > apps/meta.xml
tar zvcf "../gxgeo-$DATENOW.tar.gz" * ../READ.MII 2> /dev/null
cd ..
rm -r package

