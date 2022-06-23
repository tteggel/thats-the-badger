# That's The Badger

My badger.

## Build and deploy
```shell
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DPICO_BOARD=pimoroni_badger2040 -GNinja
ninja thats-the-badger
cp ./thats-the-badger/thats-the-badger.uf2 <root of pico>
```
From my WSL:
```shell
 ninja thats-the-badger && sudo mount -t drvfs D: /mnt/d && cp thats-the-badger/thats-the-badger.uf2 /mnt/d/
```

## Acknowledgements
* Avinal Kumar for the boilerplate https://github.com/avinal/badger2040-boilerplate/
* Michael Bell for the Badger Set https://github.com/MichaelBell/badger-set
