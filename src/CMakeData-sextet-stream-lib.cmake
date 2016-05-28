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
  "SextetStream/IO/RageFilePacketWriter.cpp"
  "SextetStream/IO/StdCFilePacketReader.cpp"
  "SextetStream/IO/PacketReaderEventGenerator.cpp"
  "SextetStream/IO/NoopPacketWriter.cpp"
  "SextetStream/Data/Packet.cpp"
)
list(APPEND SMDATA_SEXTETSTREAM_LIB_HPP
  "SextetStream/IO/PacketWriter.h"
  "SextetStream/IO/RageFilePacketWriter.h"
  "SextetStream/IO/PacketReaderEventGenerator.h"
  "SextetStream/IO/NoopPacketWriter.h"
  "SextetStream/IO/PacketReader.h"
  "SextetStream/IO/StdCFilePacketReader.h"
  "SextetStream/Data.h"
  "SextetStream/Data/Packet.h"
)
source_group("SextetStream Support Library" FILES ${SMDATA_SEXTETSTREAM_LIB_SRC} ${SMDATA_SEXTETSTREAM_LIB_HPP})

