# 빌드방법

## 우분투
```bash
#의존성 설치
python3 -m venv .chesstack
source ./.chesstack/bin/activate

sudo apt update && sudo apt install python3-dev pybind11-dev
pip3 install pygame pybind11

mkdir build && cd build
cmake ..
make -j4
```

