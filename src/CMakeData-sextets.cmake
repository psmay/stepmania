# To determine what to put in this file, run the following from src:
#
# sed '/^##\t/!d ; s/^##\t//g' < CMakeData-sextets.cmake | bash
#
# The above command runs the following script:
#
##	#!/bin/bash
##	
##	WINDOWS_TEST=('(' -path '*/Windows/*.*' ')')
##	POSIX_TEST=('(' -path '*/Posix/*.*' ')')
##	PLATFORM_INDEPENDENT_TEST=('(' -not '(' ${WINDOWS_TEST[@]} -o ${POSIX_TEST[@]} ')' ')')
##	
##	echo 'list(APPEND SMDATA_SEXTETS_SRC'
##	find Sextets '(' -name '*.cpp' -a ${PLATFORM_INDEPENDENT_TEST[@]} ')' -exec echo '  "{}"' \; | sort
##	echo ')'
##	echo
##	echo 'list(APPEND SMDATA_SEXTETS_HPP'
##	find Sextets '(' -name '*.h' -a ${PLATFORM_INDEPENDENT_TEST[@]} ')' -exec echo '  "{}"' \; | sort
##	echo ')'
##	echo
##	echo 'if(WIN32)'
##	echo '  list(APPEND SMDATA_SEXTETS_SRC'
##	find Sextets '(' -name '*.cpp' -a ${WINDOWS_TEST[@]} ')' -exec echo '    "{}"' \; | sort
##	echo '  )'
##	echo '  list(APPEND SMDATA_SEXTETS_HPP'
##	find Sextets '(' -name '*.h' -a ${WINDOWS_TEST[@]} ')' -exec echo '    "{}"' \; | sort
##	echo '  )'
##	echo 'elseif(LINUX OR APPLE)'
##	echo '  list(APPEND SMDATA_SEXTETS_SRC'
##	find Sextets '(' -name '*.cpp' -a ${POSIX_TEST[@]} ')' -exec echo '    "{}"' \; | sort
##	echo '  )'
##	echo '  list(APPEND SMDATA_SEXTETS_HPP'
##	find Sextets '(' -name '*.h' -a ${POSIX_TEST[@]} ')' -exec echo '    "{}"' \; | sort
##	echo '  )'
##	echo 'endif()'
##	echo
##	echo 'source_group("Sextets Support Library" FILES ${SMDATA_SEXTETS_SRC} ${SMDATA_SEXTETS_HPP})'

list(APPEND SMDATA_SEXTETS_SRC
  "Sextets/Data.cpp"
  "Sextets/IO/NoopPacketWriter.cpp"
  "Sextets/IO/PacketReaderEventGenerator.cpp"
  "Sextets/IO/StdCFilePacketReader.cpp"
  "Sextets/IO/StdCFilePacketWriter.cpp"
  "Sextets/PacketBuffer.cpp"
  "Sextets/Packet.cpp"
)

list(APPEND SMDATA_SEXTETS_HPP
  "Sextets/Data.h"
  "Sextets/IO/NoopPacketWriter.h"
  "Sextets/IO/PacketReaderEventGenerator.h"
  "Sextets/IO/PacketReader.h"
  "Sextets/IO/PacketWriter.h"
  "Sextets/IO/PlatformFifo.h"
  "Sextets/IO/StdCFilePacketReader.h"
  "Sextets/IO/StdCFilePacketWriter.h"
  "Sextets/PacketBuffer.h"
  "Sextets/Packet.h"
  "Sextets/Platform.h"
)

if(WIN32)
  list(APPEND SMDATA_SEXTETS_SRC
    "Sextets/IO/Windows/_BasicOverlappedNamedPipe.cpp"
    "Sextets/IO/Windows/NamedFifoPacketReader.cpp"
    "Sextets/IO/Windows/NamedFifoPacketWriter.cpp"
  )
  list(APPEND SMDATA_SEXTETS_HPP
    "Sextets/IO/Windows/_BasicOverlappedNamedPipe.h"
    "Sextets/IO/Windows/NamedFifoPacketReader.h"
    "Sextets/IO/Windows/NamedFifoPacketWriter.h"
  )
elseif(LINUX OR APPLE)
  list(APPEND SMDATA_SEXTETS_SRC
    "Sextets/IO/Posix/NamedFifoPacketReader.cpp"
    "Sextets/IO/Posix/NamedFifoPacketWriter.cpp"
  )
  list(APPEND SMDATA_SEXTETS_HPP
    "Sextets/IO/Posix/NamedFifoPacketReader.h"
    "Sextets/IO/Posix/NamedFifoPacketWriter.h"
  )
endif()

source_group("Sextets Support Library" FILES ${SMDATA_SEXTETS_SRC} ${SMDATA_SEXTETS_HPP})
