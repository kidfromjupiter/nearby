# Google Nearby Sharing & Connections for Linux (Unofficial)

<img width="982" height="760" alt="image" src="https://github.com/user-attachments/assets/9533ea09-81d9-4162-b90f-e0bd8b714d1d" />



## Demo


https://github.com/user-attachments/assets/048afa1e-40a4-4351-a859-c81b642fc6e3

## What
This repo consists of the entirety of google's open source nearby library. Currently, it is seperated into 3 sections. 
- Sharing
- Connections
- Presence

    
Linux specific implementation and compatibility shims are provided for building **Sharing** and **Connections**. 
Nearby presence may or may not build. I haven't tested it.

## Why
This repo could've been a PR on the official repo. All I've done is implement the platform abstraction layer google has provided for a linux specific environment.
It's not like I haven't made a PR towards the official repo. I got tired of begging the official nearby maintainers for a PR review. So here we are. 


Moreover, all I've wanted was seamless file sharing between my android devices and my linux workstation + laptop. With this repo and the example application provided here,
it accomplishes that goal perfectly. This repo wasn't created out of any altruistic goals or out of the goodness of my heart. I had a problem. I solved it. Simple as that. 


## How

### How to install
This repo provides prebuilt binaries of the Quick Share application. The only officially supported distro is Fedora 43 for now. The newest ubuntu images *should* work fine
although that needs to be tested. I want to support more distros so if you encounter issues installing on your distro, please let me know. 


#### Prerequisites

- `sdbus-cpp >= 2.0`
- `bluez >= 5.85`

To install the prerequisites, run this command

```bash
sudo dnf install -y \
  bluez bluez-libs bluez-libs-devel \
  sdbus-cpp sdbus-cpp-devel
```
---

To install the Quick Share application, 

1. Go to [releases](https://github.com/kidfromjupiter/nearby/releases)
2. Download the latest nearby-file-share-linux-*.tar.gz
3. `mkdir -p nearby && tar -xf nearby-file-share-linux-*.tar.gz -C nearby`
4. `cd nearby`
5. `chmod +x install_nearby_file_share.sh`
6. `./install_nearby_file_share.sh`

*coming soon*
### How to build
*coming soon*
### How to contribute
*coming soon*

## Special thanks

https://github.com/proatgram and https://github.com/vibhavp

*they were the original authors of the linux platform support [PR](https://github.com/google/nearby/pull/2098) that I have based much of this codebase upon. I am standing on the shoulders
of giants*
