
Upgrading Boost
'''''''''''''''

# boost
rm -rf vendor/boost
mkdir vendor/boost
export BOOST_VERSION=1.72.0
export _BOOST_VERSION=1_72_0
wget "https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${_BOOST_VERSION}.tar.gz"
tar -xvf "boost_${_BOOST_VERSION}.tar.gz"
cd "boost_${_BOOST_VERSION}"
./bootstrap.sh
cd tools/bcp
../../b2
cd ../..
# chmod +x bin.v2/tools/bcp not sure why had
bin.v2/tools/bcp/gcc-9.2.0/release/link-static/bcp LICENSE_1_0.txt boost/format.hpp boost/range/combine.hpp ../vendor/boost
cd ..
rm -rf boost*




poetry install
