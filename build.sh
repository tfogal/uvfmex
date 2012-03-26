#!/bin/sh

function error {
  echo >&2 "$@"
  exit 1
}
# check for some commands we'll need
command -v mex &> /dev/null || error "'mex' must be in your path."
command -v svn &> /dev/null || error "'svn' must be in your path."

# grab or update the tuvok sources.
if ! test -d tuvok ; then
  svn co \
    --non-interactive \
    --trust-server-cert \
    https://gforge.sci.utah.edu/svn/Tuvok tuvok
else
  if test -d tuvok/.svn ; then
    echo "Updating tuvok..."
    (cd tuvok && svn update)
  fi
fi

# now build tuvok -- without Qt.
CXXF="-IIO/3rdParty/boost"
CXXF="${CXXF} -I3rdParty/boost"
CXXF="${CXXF} -IIO/3rdParty"
CXXF="${CXXF} -I3rdParty"
CXXF="${CXXF} -O0 -g"
CXXF="${CXXF} -fPIC"
CXXF="${CXXF} -DTUVOK_NO_QT"
(cd tuvok && \
  qmake QMAKE_CXXFLAGS="${CXXF}" \
    QMAKE_LDFLAGS="${LDF}" \
    -recursive Tuvok.pro) || \
  error "tuvok configuration failed."
(cd tuvok && nice make -j2) || error "could not build tuvok."

make clean
make
