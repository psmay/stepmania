# To determine what to put in this file, run the following from src:
#   #!/bin/sh
#   echo 'list(APPEND SMDATA_SEXTETSTREAM_LIB_SRC'
#   find SextetStream -name '*.cpp' -exec echo '  "{}"' \;
#   echo ')'
#   echo 'list(APPEND SMDATA_SEXTETSTREAM_LIB_HPP'
#   find SextetStream -name '*.h' -exec echo '  "{}"' \;
#   echo ')'
#   echo 'source_group("SextetStream Support Library" FILES ${SMDATA_SEXTETSTREAM_LIB_SRC} ${SMDATA_SEXTETSTREAM_LIB_HPP}) 

list(APPEND SMDATA_SEXTETSTREAM_LIB_SRC
  "SextetStream/Data.cpp"
  "SextetStream/IO/PacketReaderEventGenerator.cpp"
  "SextetStream/IO/StdCFilePacketReader.cpp"
)
list(APPEND SMDATA_SEXTETSTREAM_LIB_HPP
  "SextetStream/Data.h"
  "SextetStream/IO/PacketReaderEventGenerator.h"
  "SextetStream/IO/PacketReader.h"
  "SextetStream/IO/StdCFilePacketReader.h"
)
source_group("SextetStream Support Library" FILES ${SMDATA_SEXTETSTREAM_LIB_SRC} ${SMDATA_SEXTETSTREAM_LIB_HPP})

