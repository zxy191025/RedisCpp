#/* 
# * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> 
# * All rights reserved.
# * Date: 2025/06/13
# * Description: build project using this shell
# */

sudo mkdir build
cd build
sudo cmake ../
sudo make -j8
sudo make install
