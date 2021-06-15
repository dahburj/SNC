set -e
qmake
make -j$(nproc)
sudo make install
python3 setup.py build
sudo python3 setup.py install

