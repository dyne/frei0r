from dyne/devuan:beowulf
maintainer jaromil "https://github.com/jaromil"

run echo "deb-src http://deb.devuan.org/merged beowulf main" > /etc/apt/sources.list
run echo "deb http://deb.devuan.org/merged beowulf main"    >> /etc/apt/sources.list
run apt-get -qq update
run apt-get build-dep -yy frei0r-plugins
run apt-get -yy install cmake clang
copy . .

