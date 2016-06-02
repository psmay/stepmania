# To determine what to put in this file, run the following from src:
#   #!/bin/sh
#   echo 'list(APPEND SMDATA_SEXTETS_SRC'
#   find Sextets -name '*.cpp' -exec echo '  "{}"' \; | sort
#   echo ')'
#   echo 'list(APPEND SMDATA_SEXTETS_HPP'
#   find Sextets -name '*.h' -exec echo '  "{}"' \; | sort
#   echo ')'
#   echo 'source_group("Sextets Support Library" FILES ${SMDATA_SEXTETS_SRC} ${SMDATA_SEXTETS_HPP})'

list(APPEND SMDATA_SEXTETS_SRC
  "Sextets/Data.cpp"
  "Sextets/IO/NoopPacketWriter.cpp"
  "Sextets/IO/PacketReaderEventGenerator.cpp"
  "Sextets/IO/RageFilePacketWriter.cpp"
  "Sextets/IO/StdCFilePacketReader.cpp"
  "Sextets/Packet.cpp"
  "Sextets/PacketBuffer.cpp"
)
list(APPEND SMDATA_SEXTETS_HPP
  "Sextets/Data.h"
  "Sextets/IO/NoopPacketWriter.h"
  "Sextets/IO/PacketReaderEventGenerator.h"
  "Sextets/IO/PacketReader.h"
  "Sextets/IO/PacketWriter.h"
  "Sextets/IO/RageFilePacketWriter.h"
  "Sextets/IO/StdCFilePacketReader.h"
  "Sextets/Packet.h"
  "Sextets/PacketBuffer.h"
)
source_group("Sextets Support Library" FILES ${SMDATA_SEXTETS_SRC} ${SMDATA_SEXTETS_HPP})
