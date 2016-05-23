list(APPEND SMDATA_SEXTETSTREAM_LIB_SRC
  "SextetStream/IO/StdCFileLineReader.cpp"
)
list(APPEND SMDATA_SEXTETSTREAM_LIB_HPP
  "SextetStream/IO/LineReader.h"
  "SextetStream/IO/StdCFileLineReader.h"
)

source_group("SextetStream Support Library" FILES ${SMDATA_SEXTETSTREAM_LIB_SRC} ${SMDATA_SEXTETSTREAM_LIB_HPP})
